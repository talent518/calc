#ifndef EXT_H
#define EXT_H

#include "calc.h"

#define retval expr->result

#define FUNC_DEF(n) extern func_def_f calc_def_##n

#define FUNC(n,names,desc) \
	void calc_run_sys_##n(exp_val_t *expr); \
	var_t calc_name_##n = {#n,sizeof(#n)-1,0}; \
	func_def_f calc_def_##n = { \
		calc_run_sys_##n, /* isSys */ \
		&calc_name_##n, /* name */ \
		0, /* argc */ \
		0, /* minArgc */ \
		names, \
		NULL, /* args, */ \
		NULL, /* syms, */ \
		__LINE__, /* lineno */ \
		__FILE__, /* filename */ \
		desc, \
		0, /* varlen */ \
		NULL /* vars */ \
	}; \
	void calc_run_sys_##n(exp_val_t *expr)

#define CONST(n) calc_const_##n
#define CONST_DEF(n) extern exp_val_t CONST(n)

#define CONST_EX(n,t,k,v) exp_val_t CONST(n) = { \
		t, \
		{.k=v} \
	}

#define CONST_INT(n,v) CONST_EX(n,INT_T,ival,v)

#define CONST_LONG(n,v) CONST_EX(n,LONG_T,lval,v)

#define CONST_FLOAT(n,v) CONST_EX(n,FLOAT_T,fval,v)

#define CONST_DOUBLE(n,v) CONST_EX(n,DOUBLE_T,dval,v)

#define CONST_STR_EX(n,s) \
	string_t calc_const_str_##n = { \
		s, \
		sizeof(s)-1, \
		1 \
	}; \
	CONST_EX(n,STR_T,str,&calc_const_str_##n)

#define CONST_STR(n,s) CONST_STR_EX(n,#s)

#endif