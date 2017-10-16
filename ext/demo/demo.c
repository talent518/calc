#include "demo.h"

CONST_INT(DEMO_INT,1);
CONST_LONG(DEMO_LONG,2);
CONST_FLOAT(DEMO_FLOAT,3);
CONST_DOUBLE(DEMO_DOUBLE,4);
CONST_STR_EX(DEMO_STR_EX,"demo test const.");
CONST_STR(DEMO_STR,dstr);

FUNC(demo, "demo()", "return: DEMO_STR_EX") {
	memcpy_ref_expr(retval, &CONST(DEMO_STR_EX));
}
