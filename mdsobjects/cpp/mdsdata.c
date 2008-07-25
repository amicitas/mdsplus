#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mdsdescrip.h>
#include <mdsshr.h>
#include <mds_stdarg.h>
#include <treeshr.h>
#include <libroutines.h>
#include <cacheshr.h>

extern int TreeBeginSegment(int nid, struct descriptor *start, struct descriptor *end, 
							struct descriptor *dim, struct descriptor_a *initialData, int idx);
extern int TreePutRow(int nid, int bufsize, _int64 *timestamp, struct descriptor_a *rowdata);
extern int TdiData();
extern int TdiDecompile();
extern int TdiCompile();
extern void *createScalarData(int dtype, int length, char *ptr);
extern void *createArrayData(int dtype, int length, int nDims, int *dims, char *ptr);
extern void *createCompoundData(int dtype, int length, char *ptr, int nDescs, char **descs);
extern void *createApdData(int nData, char **dataPtr);
extern void *convertDataToDsc(void *data);
extern void convertTime(int *time, char *retTime);

void *convertToScalarDsc(int clazz, int dtype, int length, char *ptr)
{
	EMPTYXD(emptyXd);
	int status;
	struct descriptor dsc;


	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = emptyXd;
	
	dsc.class = clazz;
	dsc.dtype = dtype;
	dsc.length = length;
	dsc.pointer = ptr;
	status = MdsCopyDxXd(&dsc, xdPtr);
	if(!(status & 1))
	{
		printf("PANIC in convertToScalarDsc: MdsCopyDxXd failed\n");
		exit(0);
	}
	return xdPtr;
}

#define MAX_DIMS 32
/*static void incrementIdx(int *idxs, int nDims, int *dims)
{
	int currDim = nDims - 1;
	while(currDim >=0 && idxs[currDim] == dims[currDim] - 1)
	{
		idxs[currDim] = 0;
		currDim--;
	}
	if(currDim < 0)
	{
		printf("INTERNAL ERROR IN incrementIdx!!\n");
		exit(0);
	}
	idxs[currDim]++;
}

static int colOrderIdx(int *idxs, int nDims, int *dims)
{
	int i;
	int idx = idxs[nDims - 1];
	for(i = nDims - 2; i >= 0; i--)
		idx = idx * dims[i] + idxs[i];
	return idx;
}
static int rowOrderIdx(int *idxs, int nDims, int *dims)
{
	int i;
	int idx = idxs[0];
	for(i = 1; i < nDims; i++)
		idx = idx * dims[i] + idxs[i];
	return idx;
}



static void toColOrder(char *inArr, char *outArr, int length, int nDims, int *dims)
{
	int idxs[MAX_DIMS], i;
	int samples = 1;
	for(i = 0; i < nDims; i++)
	{
		samples *= dims[i];
		idxs[i] = 0;
	}
	memcpy(outArr, inArr, samples * length);
	return;



	for(i = 0; i < samples; i++)
	{
		memcpy(&outArr[colOrderIdx(idxs, nDims, dims)*length], &inArr[rowOrderIdx(idxs, nDims, dims)*length], length);
		if(i < samples - 1) incrementIdx(idxs, nDims, dims);
	}
}
static void toRowOrder(char *inArr, char *outArr, int length, int nDims, int *dims)
{
	int idxs[MAX_DIMS], i;
	int samples = 1;
	for(i = 0; i < nDims; i++)
	{
		samples *= dims[i];
		idxs[i] = 0;
	}
	memcpy(outArr, inArr, samples * length);
	return;
	for(i = 0; i < samples; i++)
	{
		memcpy(&outArr[rowOrderIdx(idxs, nDims, dims)*length], &inArr[colOrderIdx(idxs, nDims, dims)*length], length);
		if(i < samples - 1) incrementIdx(idxs, nDims, dims);
	}
}

*/	

void *convertToArrayDsc(int clazz, int dtype, int length, int arsize, int nDims, int *dims, void *ptr)
{
	EMPTYXD(emptyXd);
	int status, i;
	DESCRIPTOR_A(arr1Dsc, length, dtype, ptr, arsize);
	DESCRIPTOR_A_COEFF(arrNDsc, length, dtype, 0, MAX_DIMS, arsize);
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = emptyXd;

	if(nDims == 1)
	{
		arr1Dsc.class = clazz;
		status = MdsCopyDxXd((struct descriptor *)&arr1Dsc, xdPtr);
		if(!(status & 1))
		{
			printf("PANIC in convertToArrayDsc: MdsCopyDxXd failed: %s\n", MdsGetMsg(status));
			exit(0);
		}
		return (void *)xdPtr;
	}
	arrNDsc.dimct = nDims;
	for(i = 0; i < nDims; i++)
		arrNDsc.m[i] = dims[nDims - i - 1];
	arrNDsc.pointer = ptr;

	status = MdsCopyDxXd((struct descriptor *)&arrNDsc, xdPtr);
	if(!(status & 1))
	{
		printf("PANIC in convertToArrayDsc: MdsCopyDxXd failed\n");
		exit(0);
	}
	return xdPtr;
}

#define MAX_ARGS 128
void *convertToCompoundDsc(int clazz, int dtype, int length, void *ptr, int ndescs, void **descs)
{
	EMPTYXD(emptyXd);
	struct descriptor_xd *xds[MAX_ARGS];
	int status, i;
	DESCRIPTOR_R(recDsc, 0, MAX_ARGS);
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = emptyXd;
	recDsc.dtype = dtype;
	recDsc.length = length;
	recDsc.pointer = ptr;
	recDsc.ndesc = ndescs;
	for(i = 0; i < ndescs; i++)
	{
		xds[i] = 0;
		if(descs[i])
		{
			xds[i] = (struct descriptor_xd *)convertDataToDsc(descs[i]);
			recDsc.dscptrs[i] = xds[i]->pointer;
		}
		else
			recDsc.dscptrs[i] = 0;
	}
	status = MdsCopyDxXd((struct descriptor *)&recDsc, xdPtr);
	if(!(status & 1))
	{
		printf("PANIC in convertToCompoundDsc: MdsCopyDxXd failed\n");
		exit(0);
	}
	for(i = 0; i < ndescs; i++)
		if(xds[i])
			MdsFree1Dx(xds[i], 0);

	return xdPtr;
}
			
void *convertToApdDsc(int ndescs, void **descs)
{
	EMPTYXD(emptyXd);
	struct descriptor_xd **xds = (struct descriptor_xd **)malloc(ndescs * sizeof(struct descriptor_xd *));
	int status, i;
	DESCRIPTOR_APD(apdDsc, DTYPE_DSC, 0, 0);
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = emptyXd;
	apdDsc.arsize = ndescs * sizeof(struct descriptor *);
	apdDsc.pointer = malloc(ndescs * sizeof(struct descriptor *));
	for(i = 0; i < ndescs; i++)
	{
		xds[i] = 0;
		if(descs[i])
		{
			xds[i] = (struct descriptor_xd *)convertDataToDsc(descs[i]);
			((struct descriptor **)apdDsc.pointer)[i] = xds[i]->pointer;
		}
		else
			((struct descriptor **)apdDsc.pointer)[i] = 0;
	}
	status = MdsCopyDxXd((struct descriptor *)&apdDsc, xdPtr);
	if(!(status & 1))
	{
		printf("PANIC in convertToApdDsc: MdsCopyDxXd failed\n");
		exit(0);
	}
	free((char *)apdDsc.pointer);
	for(i = 0; i < ndescs; i++)
		if(xds[i])
			MdsFree1Dx(xds[i], 0);
	free((char *)xds);
	return xdPtr;
}
			
void *evaluateData(void *dscPtr)
{
	EMPTYXD(emptyXd);
	int status;

	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = emptyXd;
	status = TdiData(dscPtr, xdPtr MDS_END_ARG);
	if(!(status & 1))
		return 0;
	return (void *)xdPtr;
}


void *convertFromDsc(void *ptr)
{
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)ptr;
	struct descriptor *dscPtr;
	int i;
	
	
	
	if(xdPtr->class != CLASS_XD)
	{
		printf("PANIC in convertFromDsc: not an XD\n");
		exit(0);
	}
	dscPtr = xdPtr->pointer;
	switch(dscPtr->class) {
		case CLASS_S : return createScalarData(dscPtr->dtype, dscPtr->length, dscPtr->pointer);
		case CLASS_A : 
			{

				ARRAY_COEFF(char , 64) *arrDscPtr;
				arrDscPtr = (void *)dscPtr;
				if(arrDscPtr->dimct > 1)
				{
					return createArrayData(arrDscPtr->dtype, arrDscPtr->length, arrDscPtr->dimct, 
						(int *)&arrDscPtr->m, arrDscPtr->pointer);
				}
				else
				{
					int dims = arrDscPtr->arsize/arrDscPtr->length;
					return createArrayData(arrDscPtr->dtype, arrDscPtr->length, 1, &dims, arrDscPtr->pointer);
				}
			}
		case CLASS_R :
			{
				struct descriptor_r *dscRPtr = (struct descriptor_r *)dscPtr;
				char **descs = malloc(dscRPtr->ndesc * sizeof(char *));
				void *retData;
				for(i = 0; i < dscRPtr->ndesc; i++)
				{
					if(dscRPtr->dscptrs[i])
					{
						EMPTYXD(currXd);
						MdsCopyDxXd(dscRPtr->dscptrs[i], &currXd);
						descs[i] = convertFromDsc(&currXd);
						MdsFree1Dx(&currXd, 0);
					}
					else
						descs[i] = 0;
				}
				retData = createCompoundData(dscRPtr->dtype, dscRPtr->length, dscRPtr->pointer, dscRPtr->ndesc, descs);
				free((char *)descs);
				return retData;
			}
		case CLASS_APD :
			{
				struct descriptor_a *arrPtr = (struct descriptor_a *)dscPtr;
				char **descs = malloc(arrPtr->arsize);
				void *retData;
				int size = arrPtr->arsize/arrPtr->length;
				for(i = 0; i < size; i++)
				{
					if(((char **)arrPtr->pointer)[i])
					{
						EMPTYXD(currXd);
						MdsCopyDxXd(((struct descriptor **)arrPtr->pointer)[i], &currXd);
						descs[i] = convertFromDsc(&currXd);
						MdsFree1Dx(&currXd, 0);
					}
					else
						descs[i] = 0;
				}
				retData = createApdData(size, descs);

				free((char *)descs);
				return retData;
			}
		default: 
			printf("CONVERSION NOT YET SUPPORTED\n");
			exit(0);
	}
	return(0);
}


void freeDsc(void *dscPtr)
{
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)dscPtr;
	if(xdPtr->class != CLASS_XD)
	{
		printf("PANIC in convertFromDsc: not an XD\n");
		exit(0);
	}
	MdsFree1Dx(xdPtr, 0);
	free((char *)xdPtr);
}

char *decompileDsc(void *ptr)
{
    int status;
	EMPTYXD(xd);
	char *buf;
	struct descriptor *dscPtr = (struct descriptor *)ptr;

	status = TdiDecompile(dscPtr, &xd MDS_END_ARG);
	if(!(status & 1)) 
    {
      printf("Error decompiling expression: %s\n", MdsGetMsg(status));
      return NULL;
    }

	dscPtr = xd.pointer;
	buf = (char *)malloc(dscPtr->length + 1);
	memcpy(buf, dscPtr->pointer, dscPtr->length);
	buf[dscPtr->length] = 0;
	MdsFree1Dx(&xd, NULL);
	return buf;
}

void freeChar(void *ptr){free(ptr);}

void *compileFromExpr(char *expr)
{
	EMPTYXD(xd);
	int status;
	struct descriptor exprD = {0, DTYPE_T, CLASS_S, 0};
	void *data;

	exprD.length = strlen(expr);
	exprD.pointer = (char *)expr;
	status = TdiCompile(&exprD, &xd MDS_END_ARG);
	if(!(status & 1))
    {
       return NULL;
    }
	data = convertFromDsc(&xd);
	MdsFree1Dx(&xd, 0);
	return data;
}

void *compileFromExprWithArgs(char *expr, int nArgs, void **args)
{
	int varIdx;
	int i;
	void *arglist[MAX_ARGS];
	void *data;
	EMPTYXD(xd);
	struct descriptor exprD = {0, DTYPE_T, CLASS_S, 0};

	exprD.length = strlen(expr);
	exprD.pointer = (char *)expr;

	arglist[1] = &exprD;
	varIdx = 2;
	for(i = 0; i < nArgs; i++)
		arglist[varIdx++] = args[i];
	arglist[varIdx++] = &xd;
	arglist[varIdx++] = MdsEND_ARG;
	*(int *)&arglist[0] = varIdx-1;
	LibCallg(arglist, TdiCompile);
	data = convertFromDsc(&xd);
	MdsFree1Dx(&xd, 0);
	return data;
}


int getTreeData(int nid, void **data, int isCached)
{
	EMPTYXD(xd);
	int status;

	if(isCached)
		status = RTreeGetRecord(nid, &xd);
	else
		status = TreeGetRecord(nid, &xd);
	if(!(status & 1)) return status;

	*data = convertFromDsc(&xd);
	MdsFree1Dx(&xd, 0);
	return status;
}

int putTreeData(int nid, void *data, int isCached)
{
	struct descriptor_xd *xdPtr;
	int status;

	xdPtr = (struct descriptor_xd *)convertDataToDsc(data);
	if(isCached)
		status = RTreePutRecord(nid, (struct descriptor *)xdPtr, WRITE_BACK);
	else
		status = TreePutRecord(nid, (struct descriptor *)xdPtr, 0);
	freeDsc(xdPtr);	
	return status;
}

int deleteTreeData(int nid)
{
	EMPTYXD(xd);
	int status;

	status = TreePutRecord(nid, (struct descriptor *)&xd, 0);
	return status;
}

	
void convertTime(int *time, char *retTime)
{
	char timeStr[512];
	int retLen;
	struct descriptor time_dsc = {511, DTYPE_T, CLASS_S, timeStr};

	LibSysAscTim(&retLen, &time_dsc,time);
	timeStr[retLen] = 0;
	strcpy(retTime, timeStr);
}

int beginTreeSegment(int nid, void *dataDsc, void *startDsc, void *endDsc, void *dimDsc, int isCached)
{
	struct descriptor_xd *dataXd = (struct descriptor_xd *)dataDsc;
	struct descriptor_xd *startXd = (struct descriptor_xd *)startDsc;
	struct descriptor_xd *endXd = (struct descriptor_xd *)endDsc;
	struct descriptor_xd *dimXd = (struct descriptor_xd *)dimDsc;
	int status; 
	
	if(isCached)
	{
		status = RTreeBeginSegment(nid, startXd->pointer, endXd->pointer, dimXd->pointer, 
			(struct descriptor_a *)dataXd->pointer, -1, WRITE_BUFFER);
		if(status & 1) status = RTreePutSegment(nid, -1, (struct descriptor_a *)dataXd->pointer, WRITE_BUFFER);
	}

	else
		status = TreeBeginSegment(nid, startXd->pointer, endXd->pointer, dimXd->pointer, 
			(struct descriptor_a *)dataXd->pointer, -1);

	freeDsc(dataXd);
	freeDsc(startXd);
	freeDsc(endXd);
	freeDsc(dimXd);


	return status;
}

#define PUTROW_BUFSIZE 1000
int putTreeRow(int nid, void *dataDsc, _int64 *time, int isCached, int isLast)
{
	struct descriptor_xd *dataXd = (struct descriptor_xd *)dataDsc;
	int status;

	if(isCached)
	{
		status = RTreePutRow(nid, PUTROW_BUFSIZE, time, (struct descriptor_a *)dataXd->pointer, (isLast)?WRITE_LAST:WRITE_BUFFER);
	}
	else
		status = TreePutRow(nid, PUTROW_BUFSIZE, time, (struct descriptor_a *)dataXd->pointer);
	freeDsc(dataXd);
	return status;
}

