/*      Tdi1Compile.C
        The interface to compiler.

        Ken Klare, LANL CTR-7   (c)1989,1990
*/

extern unsigned short OpcCompile;
#define DEF_FREEXD

#include <stdlib.h>
#include <string.h>
#include <mdsdescrip.h>
#include "tdirefzone.h"
#include "tdirefstandard.h"
#include <libroutines.h>
#include <tdishr_messages.h>
#include "tdithreadsafe.h"
#include <mdsshr.h>
#include <STATICdef.h>

extern void LockMdsShrMutex(pthread_mutex_t *, int *);
extern void UnlockMdsShrMutex(pthread_mutex_t *);

extern int TdiEvaluate();
extern int TdiYacc();
extern int TdiIntrinsic();
extern void TdiYyReset();
/*-------------------------------------------------------
        Interface to compiler/parser.
                expression = COMPILE(string, [arg1,...])
        YACC converts TdiYacc.Y to TdiYacc.C to parse TDI statements. Also YACC subroutines.
        LEX converts TdiLex.X to TdiLex.C for lexical analysis. Also LEX subroutines.

        Limitations:
        For recursion must pass LEX:
                zone status bol cur end
                tdiyylval tdiyyval
                tdiyytext[YYLMAX] tdiyyleng tdiyymorfg tdiyytchar tdiyyin? tdiyyout?
        For recursion must pass YACC also:
                tdiyydebug? tdiyyv[YYMAXDEPTH] tdiyychar tdiyynerrs tdiyyerrflag
        Thus no recursion because the tdiyy's are built into LEX and YACC.
        IMMEDIATE (`) must never call COMPILE. NEED to prevent this.
*/
STATIC_THREADSAFE int yacc_mutex_initialized = 0;
STATIC_THREADSAFE pthread_mutex_t yacc_mutex;
int Tdi1Compile(int opcode __attribute__ ((unused)), int narg, struct descriptor *list[],
		struct descriptor_xd *out_ptr)
{
  INIT_STATUS;
  struct descriptor *text_ptr;
  LockMdsShrMutex(&yacc_mutex, &yacc_mutex_initialized);
  GET_TDITHREADSTATIC_P;
  if (TdiThreadStatic_p->compiler_recursing == 1) {
    fprintf(stderr, "Error: Recursive calls to TDI Compile is not supported");
    return TdiRECURSIVE;
  }
  EMPTYXD(tmp);
  FREEXD_ON_EXIT(&tmp);
  status = TdiEvaluate(list[0], &tmp MDS_END_ARG);
  text_ptr = tmp.pointer;
  if (STATUS_OK && text_ptr->dtype != DTYPE_T)
    status = TdiINVDTYDSC;
  if STATUS_OK {
    if (text_ptr->length > 0) {
      if (TdiThreadStatic_p->compiler_recursing == 1) {
	fprintf(stderr, "Error: Recursive calls to TDI Compile is not supported\n");
	return TdiRECURSIVE;
      }
      TdiThreadStatic_p->compiler_recursing = 1;
      if (!TdiRefZone.l_zone)
	status = LibCreateVmZone(&TdiRefZone.l_zone);
      /****************************************
      In case we bomb out, probably not needed.
      ****************************************/
      TdiRefZone.l_status = TdiBOMB;
      if (TdiRefZone.a_begin) free(TdiRefZone.a_begin);
      TdiRefZone.a_begin = TdiRefZone.a_cur = memcpy(malloc(text_ptr->length), text_ptr->pointer, text_ptr->length);
      TdiRefZone.a_end = TdiRefZone.a_cur + text_ptr->length;
      TdiRefZone.l_ok = 0;
      TdiRefZone.l_narg = narg - 1;
      TdiRefZone.l_iarg = 0;
      TdiRefZone.a_list = &list[0];
      if STATUS_OK {
	TdiYyReset();
	if (IS_NOT_OK(TdiYacc()) && IS_OK(TdiRefZone.l_status))
	  status = TdiSYNTAX;
	else
	  status = TdiRefZone.l_status;
      }
      /************************
      Move from temporary zone.
      ************************/
      if STATUS_OK {
	if (TdiRefZone.a_result == 0)
	  MdsFree1Dx(out_ptr, NULL);
	else
	  status = MdsCopyDxXd((struct descriptor *)TdiRefZone.a_result, out_ptr);
      }
      LibResetVmZone(&TdiRefZone.l_zone);
      TdiThreadStatic_p->compiler_recursing = 0;
    }
  }
  FREEXD_NOW(&tmp);
  if STATUS_NOT_OK MdsFree1Dx(out_ptr, NULL);
  UnlockMdsShrMutex(&yacc_mutex);
  return status;
}

/*-------------------------------------------------------
  Compile and evaluate an expression.
      result = EXECUTE(string, [arg1,...])
*/
int Tdi1Execute(int opcode __attribute__ ((unused)), int narg, struct descriptor *list[], struct descriptor_xd *out_ptr)
{
  INIT_STATUS;
  FREEXD_ON_EXIT(out_ptr);
  EMPTYXD(tmp);
  FREEXD_ON_EXIT(&tmp);
  status = TdiIntrinsic(OpcCompile, narg, list, &tmp);
  if STATUS_OK
    status = TdiEvaluate(tmp.pointer, out_ptr MDS_END_ARG);
  FREEXD_NOW(&tmp);
  if STATUS_NOT_OK MdsFree1Dx(out_ptr, NULL);
  FREE_CANCEL(out_ptr);
  return status;
}
