#ifndef EXT_H
#define EXT_H

#include "calc.h"

#define FUNC_DEF(n) extern func_def_f calc_def_##n

#define FUNC(n,names) \
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
		__FILE__ /* filename */ \
	}; \
	void calc_run_sys_##n(exp_val_t *expr)

#endif