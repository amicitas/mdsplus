

#include "testutils.h"

// TODO: remove config.h from header .. here needed for EXPORT
#include "config.h"

////////////////////////////////////////////////////////////////////////////////
//  Dummy Test Funtions  ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern "C" {
 
EXPORT int get_ghostbusters_phone() { return 5552368; }

EXPORT int test_nextint(int *x) { return ++(*x); }
EXPORT int test_addint(int *a, int *b) { return (*a)+(*b); }

}
