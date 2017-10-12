#include "calc.h"

HashTable pools;
HashTable vars;
HashTable funcs;
HashTable files;
HashTable frees;
HashTable topFrees;
func_symbol_t *topSyms = NULL;
int isSyntaxData = 1;
int exitCode = 0;

linenostack_t linenostack[1024]={{0,"TOP",NULL}};
int linenostacktop = 0;

char *types[] = { "NULL", "int", "long", "float", "double", "str", "array" };

#define CALC_CONV(dst,src,val) \
	do { \
		switch((src)->type) { \
			case INT_T: CALC_CONV_ival2##val(dst, src);break; \
			case LONG_T: CALC_CONV_lval2##val(dst, src);break; \
			case FLOAT_T: CALC_CONV_fval2##val(dst, src);break; \
			case DOUBLE_T: CALC_CONV_dval2##val(dst, src);break; \
			case STR_T: { \
				char *__p = (src)->str; \
				type_enum_t __type = (dst)->type; \
				CALC_CONV_str2##val(dst, src); \
				if(((dst) != (src) && __type == STR_T) || ((dst) == (src) && (dst)->type != STR_T)) { \
					free(__p); \
				} \
				break; \
			} \
			default: \
				if((dst) != (src)) { \
					if((dst)->type == STR_T) { \
						free((dst)->str); \
					} \
					if((dst)->type != NULL_T) { \
						memset((dst), 0, sizeof(exp_val_t)); \
					} \
				} \
		} \
	} while(0)

#define CALC_CONV_ival2ival(dst, src) (dst)->ival = (int)(src)->ival;(dst)->type = INT_T
#define CALC_CONV_ival2lval(dst, src) (dst)->lval = (long int)(src)->ival;(dst)->type = LONG_T
#define CALC_CONV_ival2fval(dst, src) (dst)->fval = (float)(src)->ival;(dst)->type = FLOAT_T
#define CALC_CONV_ival2dval(dst, src) (dst)->dval = (double)(src)->ival;(dst)->type = DOUBLE_T
#define CALC_CONV_ival2str(dst, src) (dst)->str = (char*)malloc(12);memset((dst)->str, 0, 12);\
	(dst)->strlen = snprintf((dst)->str, 11, "%d", (src)->ival);(dst)->type = STR_T

#define CALC_CONV_lval2ival(dst, src) (dst)->ival = (int)(src)->lval;(dst)->type = INT_T
#define CALC_CONV_lval2lval(dst, src) (dst)->lval = (long int)(src)->lval;(dst)->type = LONG_T
#define CALC_CONV_lval2fval(dst, src) (dst)->fval = (float)(src)->lval;(dst)->type = FLOAT_T
#define CALC_CONV_lval2dval(dst, src) (dst)->dval = (double)(src)->lval;(dst)->type = DOUBLE_T
#define CALC_CONV_lval2str(dst, src) (dst)->str = (char*)malloc(22);memset((dst)->str, 0, 22);\
	(dst)->strlen = snprintf((dst)->str, 21, "%ld", (src)->lval);(dst)->type = STR_T

#define CALC_CONV_fval2ival(dst, src) (dst)->ival = (int)(src)->fval;(dst)->type = INT_T
#define CALC_CONV_fval2lval(dst, src) (dst)->lval = (long int)(src)->fval;(dst)->type = LONG_T
#define CALC_CONV_fval2fval(dst, src) (dst)->fval = (float)(src)->fval;(dst)->type = FLOAT_T
#define CALC_CONV_fval2dval(dst, src) (dst)->dval = (double)(src)->fval;(dst)->type = DOUBLE_T
#define CALC_CONV_fval2str(dst, src) (dst)->str = (char*)malloc(23);memset((dst)->str, 0, 23);\
	(dst)->strlen = snprintf((dst)->str, 22, "%f", (src)->fval);(dst)->type = STR_T

#define CALC_CONV_dval2ival(dst, src) (dst)->ival = (int)(src)->dval;(dst)->type = INT_T
#define CALC_CONV_dval2lval(dst, src) (dst)->lval = (long int)(src)->dval;(dst)->type = LONG_T
#define CALC_CONV_dval2fval(dst, src) (dst)->fval = (float)(src)->dval;(dst)->type = FLOAT_T
#define CALC_CONV_dval2dval(dst, src) (dst)->dval = (double)(src)->dval;(dst)->type = DOUBLE_T
#define CALC_CONV_dval2str(dst, src) (dst)->str = (char*)malloc(33);memset((dst)->str, 0, 33);\
	(dst)->strlen = snprintf((dst)->str, 32, "%lf", (src)->dval);(dst)->type = STR_T

#define CALC_CONV_str2ival(dst, src) (dst)->ival = 0;sscanf(__p, "%d", &(dst)->ival);(dst)->type = INT_T
#define CALC_CONV_str2lval(dst, src) (dst)->lval = 0;sscanf(__p, "%ld", &(dst)->lval);(dst)->type = LONG_T
#define CALC_CONV_str2fval(dst, src) (dst)->fval = 0.0;sscanf(__p, "%f", &(dst)->fval);(dst)->type = FLOAT_T
#define CALC_CONV_str2dval(dst, src) (dst)->dval = 0.0;sscanf(__p, "%lf", &(dst)->dval);(dst)->type = DOUBLE_T
#define CALC_CONV_str2str(dst, src) if((dst) != (src)) { \
		(dst)->str = strndup((src)->str, (src)->strlen); \
		(dst)->strlen = (src)->strlen; \
		(dst)->type = STR_T; \
	}

#define CALC_CONV_op_INIT() exp_val_t val1 = {NULL_T}, val2 = {NULL_T}
#define CALC_CONV_op(op1, op2, t, val) \
	do { \
		if((op1)->type !=t) { \
			CALC_CONV(&val1, op1, val); \
			op1 = &val1; \
		} \
		if((op2)->type != t) { \
			CALC_CONV(&val2, op2, val); \
			op2 = &val2; \
		} \
	} while(0)

void calc_conv_str(exp_val_t *dst, exp_val_t *src) {
	CALC_CONV(dst, src, str);
}

// dst = op1 + op2
void calc_add(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV_op_INIT();
	
	switch (max(op1->type, op2->type)) {
		case INT_T: {
			CALC_CONV_op(op1, op2, INT_T, ival);
			dst->ival = op1->ival + op2->ival;
			dst->type = INT_T;
			break;
		}
		case LONG_T: {
			CALC_CONV_op(op1, op2, LONG_T, lval);
			dst->lval = op1->lval + op2->lval;
			dst->type = LONG_T;
			break;
		}
		case FLOAT_T: {
			CALC_CONV_op(op1, op2, FLOAT_T, fval);
			dst->fval = op1->fval + op2->fval;
			dst->type = FLOAT_T;
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_op(op1, op2, DOUBLE_T, dval);
			dst->dval = op1->dval + op2->dval;
			dst->type = DOUBLE_T;
			break;
		}
		case STR_T: {
			CALC_CONV_op(op1, op2, STR_T, str);
			dst->type = STR_T;
			dst->strlen = op1->strlen + op2->strlen;
			dst->str = (char*) malloc(dst->strlen+1);
			memcpy(dst->str, op1->str, op1->strlen);
			memcpy(dst->str+op1->strlen, op2->str, op2->strlen);
			*(dst->str + dst->strlen) = '\0';
			if(val1.type == STR_T) {
				free(val1.str);
			}
			if(val2.type == STR_T) {
				free(val2.str);
			}
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// dst = op1 - op2
void calc_sub(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV_op_INIT();
	
	if(op1->type == STR_T) {
		exp_val_t val1 = {INT_T, 0};
		str2val(&val1, op1->str);
		op1 = &val1;
	}
	if(op2->type == STR_T) {
		exp_val_t val2 = {INT_T, 0};
		str2val(&val2, op2->str);
		op2 = &val2;
	}
	switch (max(op1->type, op2->type)) {
		case INT_T: {
			CALC_CONV_op(op1, op2, INT_T, ival);
			dst->ival = op1->ival - op2->ival;
			dst->type = INT_T;
			break;
		}
		case LONG_T: {
			CALC_CONV_op(op1, op2, LONG_T, lval);
			dst->lval = op1->lval - op2->lval;
			dst->type = LONG_T;
			break;
		}
		case FLOAT_T: {
			CALC_CONV_op(op1, op2, FLOAT_T, fval);
			dst->fval = op1->fval - op2->fval;
			dst->type = FLOAT_T;
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_op(op1, op2, DOUBLE_T, dval);
			dst->dval = op1->dval - op2->dval;
			dst->type = DOUBLE_T;
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// dst = op1 * op2
void calc_mul(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV_op_INIT();
	
	if(op1->type == STR_T) {
		exp_val_t val1 = {INT_T, 0};
		str2val(&val1, op1->str);
		op1 = &val1;
	}
	if(op2->type == STR_T) {
		exp_val_t val2 = {INT_T, 0};
		str2val(&val2, op2->str);
		op2 = &val2;
	}
	switch (max(op1->type, op2->type)) {
		case INT_T: {
			CALC_CONV_op(op1, op2, INT_T, ival);
			dst->ival = op1->ival * op2->ival;
			dst->type = INT_T;
			break;
		}
		case LONG_T: {
			CALC_CONV_op(op1, op2, LONG_T, lval);
			dst->lval = op1->lval * op2->lval;
			dst->type = LONG_T;
			break;
		}
		case FLOAT_T: {
			CALC_CONV_op(op1, op2, FLOAT_T, fval);
			dst->fval = op1->fval * op2->fval;
			dst->type = FLOAT_T;
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_op(op1, op2, DOUBLE_T, dval);
			dst->dval = op1->dval * op2->dval;
			dst->type = DOUBLE_T;
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// dst = op1 / op2
void calc_div(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV_op_INIT();
	CALC_CONV_op(op1, op2, DOUBLE_T, dval);

	dst->type = DOUBLE_T;
	if(op2->dval) {
		dst->dval = op1->dval / op2->dval;
	} else {
		dst->dval = 0.0;
		yyerror("op1 / op2, op2==0!!!\n");
	}
}

// dst = op1 % op2
void calc_mod(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV_op_INIT();
	CALC_CONV_op(op1, op2, LONG_T, lval);

	dst->type = LONG_T;
	if(op2->lval) {
		dst->lval = op1->lval % op2->lval;
	} else {
		dst->lval = 0;
		yyerror("op1 %% op2, op2==0!!!\n");
	}
}

#define CALC_ABS_DEF(dst,src,type,val) case type: dst->val=(src->val > 0 ? src->val : -src->val);break

// dst = |src|
void calc_abs(exp_val_t *dst, exp_val_t *src) {
	if(src->type == STR_T) {
		exp_val_t val1 = {INT_T, 0};
		str2val(&val1, src->str);
		src = &val1;
	}
	dst->type = src->type;
	switch (src->type) {
		CALC_ABS_DEF(dst, src,INT_T,ival);
		CALC_ABS_DEF(dst,src,LONG_T,lval);
		CALC_ABS_DEF(dst,src,FLOAT_T,fval);
		CALC_ABS_DEF(dst,src,DOUBLE_T,dval);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

#define CALC_MINUS_DEF(dst,src,type,val) case type: dst->val=-src->val;break

// dst = -src
void calc_minus(exp_val_t *dst, exp_val_t *src) {
	if(src->type == STR_T) {
		exp_val_t val1 = {INT_T, 0};
		str2val(&val1, src->str);
		src = &val1;
	}
	dst->type = src->type;
	switch (src->type) {
		CALC_MINUS_DEF(dst, src, INT_T, ival);
		CALC_MINUS_DEF(dst, src, LONG_T, lval);
		CALC_MINUS_DEF(dst, src, FLOAT_T, fval);
		CALC_MINUS_DEF(dst, src, DOUBLE_T, dval);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// dst = pow(op1, op2)
void calc_pow(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV_op_INIT();
	CALC_CONV_op(op1, op2, DOUBLE_T, dval);

	dst->type = DOUBLE_T;
	dst->dval = pow(op1->dval, op2->dval);
}

#define CALC_SQRT_DEF(dst,src,type,val) case type: dst->dval=sqrt(src->val);break

// dst = sqrt(src)
void calc_sqrt(exp_val_t *dst, exp_val_t *src) {
	if(src->type == STR_T) {
		exp_val_t val1 = {INT_T, 0};
		str2val(&val1, src->str);
		src = &val1;
	}
	dst->type = DOUBLE_T;
	switch (src->type) {
		CALC_SQRT_DEF(dst, src, INT_T, ival);
		CALC_SQRT_DEF(dst,src,LONG_T,lval);
		CALC_SQRT_DEF(dst,src,FLOAT_T,fval);
		CALC_SQRT_DEF(dst,src,DOUBLE_T,dval);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

void calc_array_echo(exp_val_t *ptr, call_args_t *args, int deep) {
	register int i;
#ifdef DEBUG
	for(i = 0; i<deep; i++) {
		printf(" ");
	}
#endif
	printf("[");
#ifdef DEBUG
	if(args->next) {
		printf("\n");
	}
#endif
	for (i = 0; i < args->val.ival; i++) {
		if(args->next) {
			calc_array_echo(&ptr->array[i], args->next, deep+4);
			if(i+1 < args->val.ival) {
				printf(",");
			}
		#ifdef DEBUG
			printf("\n");
		#endif
		} else {
			if(i) {
				printf(", ");
			}
			calc_echo(&ptr->array[i]);
		}
	}
#ifdef DEBUG
	if(args->next) {
		for(i = 0; i<deep; i++) {
			printf(" ");
		}
	}
#endif
	printf("]");
}

#ifdef DEBUG
#define CALC_ECHO_DEF(src,type,val,str) case type: printf("(%s) (" str ")", types[type-NULL_T],src->val);break
#else
#define CALC_ECHO_DEF(src,type,val,str) case type: printf(str,src->val);break
#endif

void calc_echo(exp_val_t *src) {
	switch (src->type) {
		CALC_ECHO_DEF(src, NULL_T, str, "%s");
		CALC_ECHO_DEF(src, INT_T, ival, "%d");
		CALC_ECHO_DEF(src, LONG_T, lval, "%ld");
		CALC_ECHO_DEF(src, FLOAT_T, fval, "%.16f");
		CALC_ECHO_DEF(src, DOUBLE_T, dval, "%.19lf");
		CALC_ECHO_DEF(src, STR_T, str, "%s");
		case ARRAY_T:
			calc_array_echo(src, src->arrayArgs, 0);
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
#undef CALC_ECHO_DEF

#define CALC_SPRINTF(src,type,val,str) case type: len = sprintf(s,str,src->val);smart_string_appendl(buf, s, len);break
// printf(src)
void calc_sprintf(smart_string *buf, exp_val_t *src) {
	char s[32] = "";
	int len;

	switch (src->type) {
		CALC_SPRINTF(src, INT_T, ival, "%d");
		CALC_SPRINTF(src, LONG_T, lval, "%ld");
		CALC_SPRINTF(src, FLOAT_T, fval, "%.16f");
		CALC_SPRINTF(src, DOUBLE_T, dval, "%.19lf");
		case STR_T: smart_string_appendc(buf, '"');smart_string_appendl(buf, src->str, src->strlen);smart_string_appendc(buf, '"');break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
#undef CALC_SPRINTF

int calc_clear_or_list_vars(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key) {
	int result = va_arg(args, int);
	char *key = strndup(hash_key->arKey, hash_key->nKeyLength);

	printf("\x1b[34m");
	if (result == ZEND_HASH_APPLY_KEEP) {
		printf("    (%6s) %s = ", types[val->type - NULL_T], key);
	} else {
		printf("    remove variable %s, type is %s, value is ", key, types[val->type - NULL_T]);
	}
	free(key);
	calc_echo(val);
	printf("\n");
	printf("\x1b[0m");
	return result;
}

void calc_func_print(func_def_f *def) {
	smart_string buf = {NULL, 0, 0};

	smart_string_appends(&buf, def->name);
	smart_string_appendc(&buf, '(');

	def->argc = 0;
	def->minArgc = 0;
	unsigned errArgc = 0;
	if (def->args) {
		register func_args_t *args = def->args;
		while (args) {
			if(def->argc) {
				smart_string_appends(&buf, ", ");
			}
			smart_string_appends(&buf, args->name);
			if (args->val.type != VAR_T) {
				smart_string_appendc(&buf, '=');
				calc_sprintf(&buf, &args->val);
			}
			if (!errArgc && def->argc != def->minArgc && args->val.type == VAR_T) {
				errArgc = def->argc;
			}
			def->argc++;
			if (args->val.type == VAR_T) {
				def->minArgc++;
			}
			args = args->next;
		}
	}
	smart_string_appendc(&buf, ')');
	smart_string_0(&buf);

	def->names = buf.c;

	dprintf("%s", def->names);
	dprintf(" argc = %d, minArgc = %d", def->argc, def->minArgc);
	
	if (errArgc) {
		INTERRUPT(__LINE__, "The user function %s the first %d argument not default value", def->names, errArgc + 1);
	}
}

void calc_func_def(func_def_f *def) {
	dprintf("define function ");
	calc_func_print(def);
	dprintf("\n");

	def->filename = curFileName;
	def->lineno = linenofunc;
	if(def->names && zend_hash_add(&funcs, def->name, strlen(def->name), def, 0, NULL) == FAILURE) {
		INTERRUPT(__LINE__, "The user function \"%s\" already exists.\n", def->names);
	}
}

void calc_free_args(call_args_t *args) {
	register call_args_t *tmpArgs;

	while (args) {
		calc_free_expr(&args->val);
		tmpArgs = args;
		args = args->next;
		free(tmpArgs);
	}
}

void calc_free_func(func_def_f *def) {
	free(def->names);

	//zend_hash_destroy(&def->frees);
}

void calc_run_copy(exp_val_t *ret, exp_val_t *expr) {
	memcpy(ret, expr, sizeof(exp_val_t));
}

void calc_run_strdup(exp_val_t *ret, exp_val_t *expr) {
	ret->type = STR_T;
	ret->str = strndup(expr->str, expr->strlen);
	ret->strlen = expr->strlen;
	ret->run = calc_run_strdup;
}

void calc_run_variable(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t *ptr = NULL;
	
	zend_hash_find(&vars, expr->str, strlen(expr->str), (void**)&ptr);
	if (ptr) {
		if(ptr->type < ARRAY_T || (linenostack[linenostacktop].syms->type == RET_STMT_T && linenostack[linenostacktop].syms->expr->type == VAR_T)) {
			memcpy(ret, ptr, sizeof(exp_val_t));
		}
		if(ptr->type == STR_T) {
			ret->str = strndup(ptr->str, ptr->strlen);
		}
		if(ret->type == ARRAY_T) {
			ptr->isref = 1;
		}
	} else {
		ret->type = INT_T;
		ret->ival = 0;
	}
}

void calc_run_add(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	calc_add(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_sub(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	calc_sub(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_mul(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	calc_mul(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_div(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	calc_div(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_mod(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	calc_mod(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_pow(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	calc_pow(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_abs(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T};
	
	expr->left->run(&left, expr->left);
	
	calc_abs(ret, &left);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
}

void calc_run_minus(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T};
	
	expr->left->run(&left, expr->left);
	
	calc_minus(ret, &left);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
}

void calc_run_iif(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t cond = {NULL_T};

	expr->cond->run(&cond, expr->cond);
	CALC_CONV((&cond), (&cond), dval);

	if (cond.dval) {
		expr->left->run(ret, expr->left);
	} else {
		expr->right->run(ret, expr->right);
	}
	
	calc_free_expr(&cond);
}

void calc_run_gt(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	CALC_CONV(&left, &left, dval);
	CALC_CONV(&right, &right, dval);
	
	ret->type = INT_T;
	ret->ival = left.dval > right.dval;
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_lt(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	CALC_CONV(&left, &left, dval);
	CALC_CONV(&right, &right, dval);
	
	ret->type = INT_T;
	ret->ival = left.dval < right.dval;
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_ge(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	CALC_CONV(&left, &left, dval);
	CALC_CONV(&right, &right, dval);
	
	ret->type = INT_T;
	ret->ival = left.dval >= right.dval;
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_le(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	CALC_CONV(&left, &left, dval);
	CALC_CONV(&right, &right, dval);
	
	ret->type = INT_T;
	ret->ival = left.dval <= right.dval;
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_eq(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	CALC_CONV(&left, &left, dval);
	CALC_CONV(&right, &right, dval);
	
	ret->type = INT_T;
	ret->ival = left.dval == right.dval;
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_ne(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->left->run(&left, expr->left);
	expr->right->run(&right, expr->right);
	
	CALC_CONV(&left, &left, dval);
	CALC_CONV(&right, &right, dval);
	
	ret->type = INT_T;
	ret->ival = left.dval != right.dval;
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_array(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t *ptr = NULL, *tmp, val = {NULL_T};
	register call_args_t *args = expr->callArgs;
	
	zend_hash_find(&vars, expr->callName, strlen(expr->callName), (void**)&ptr);
	if(!ptr) {
		yyerror("(warning) variable %s not exists, cannot read array or string value.\n", expr->callName);
	} else if (ptr->type == STR_T) {
		str_array_access:
		if(args->next) {
			yyerror("An string of %s dimension bounds.\n", expr->callName);
			return;
		}
		calc_run_expr(&val, &args->val);
		CALC_CONV((&val), (&val), ival);
		if(val.ival<0 || val.ival>=ptr->strlen) {
			yyerror("An string of %s index out of bounds.\n", expr->callName);
			return;
		}
		ret->type = STR_T;
		ret->str = strndup(ptr->str+val.ival, 1);
		ret->strlen = 1;
		ret->run = calc_run_strdup;
	} else if (ptr->type != ARRAY_T) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", expr->callName, types[ptr->type - NULL_T]);
	} else {
		ret->type = INT_T;
		ret->ival = 0;

		tmp = ptr;
		while (args) {
			calc_run_expr(&val, &args->val);
			CALC_CONV((&val), (&val), ival);
			val.type = INT_T;
			if (val.ival < 0) {
				val.ival = -val.ival;
			}

			if (val.ival < tmp->arrlen) {
				tmp = &tmp->array[val.ival];
				if (tmp->type == STR_T && args->next) {
					args = args->next;
					ptr = tmp;
					goto str_array_access;
				} else if (tmp->type != ARRAY_T) {
					if (args->next) {
						yyerror("An array of %s dimension bounds.\n", expr->callName);
					} else {
						calc_run_expr(ret, tmp);
					}
					return;
				}
			} else {
				yyerror("An array of %s index out of bounds.\n", expr->callName);
				return;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", expr->callName);
	}
}

void calc_run_func(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	func_def_f *def = NULL;
	exp_val_t val = {NULL_T}, *ptr;
	register call_args_t *tmpArgs = NULL;
	register func_args_t *funcArgs;
	register func_symbol_t *syms;

	if(linenostacktop+1 == sizeof(linenostack)/sizeof(linenostack_t)) {
		yyerror("stack overflow.\n");
		return;
	}
	
	zend_hash_find(&funcs, expr->callName, strlen(expr->callName), (void**)&def);
	if (def) {
		tmpArgs = expr->callArgs;
		while(tmpArgs) {
			argc++;
		
			tmpArgs = tmpArgs->next;
		}

		if (argc > def->argc || argc < def->minArgc) {
			yyerror("The custom function %s the number of parameters should be %d, at least %d, the actual %d.\n", expr->callName, def->argc, def->minArgc, argc);
		}

		HashTable tmpVars = vars;
		funcArgs = def->args;

		zend_hash_init(&vars, 2, (dtor_func_t)calc_free_vars);

		#ifndef NO_FUNC_RUN_ARGS
			smart_string buf = {NULL, 0, 0};

			smart_string_appends(&buf, def->name);
			smart_string_appendc(&buf, '(');

			argc = 0;
		#endif

		tmpArgs = expr->callArgs;
		while (funcArgs) {
			if(tmpArgs) {
				calc_run_expr(&val, &tmpArgs->val);
				#ifndef NO_FUNC_RUN_ARGS
					if(argc) {
						smart_string_appends(&buf, ", ");
					}
					calc_sprintf(&buf, &val);
				#endif
				tmpArgs = tmpArgs->next;
			} else {
				calc_run_expr(&val, &funcArgs->val);
			}
			zend_hash_update(&vars, funcArgs->name, strlen(funcArgs->name), &val, sizeof(exp_val_t), NULL);
			funcArgs = funcArgs->next;
			#ifndef NO_FUNC_RUN_ARGS
				argc++;
			#endif
		}
		#ifndef NO_FUNC_RUN_ARGS
			smart_string_appendc(&buf, ')');
			smart_string_0(&buf);
		#endif

		syms = def->syms;
		while(syms) {
			if(syms->type != GLOBAL_T) {
				syms = syms->next;
				continue;
			}

			tmpArgs = syms->args;
			while(tmpArgs) {
				ptr = NULL;

				zend_hash_find(&tmpVars, tmpArgs->val.str, strlen(tmpArgs->val.str), (void**)&ptr);
				if(ptr) {
					zend_hash_add(&tmpVars, tmpArgs->val.str, strlen(tmpArgs->val.str), ptr, sizeof(exp_val_t), NULL);
				}
				tmpArgs = tmpArgs->next;
			}

			syms = syms->next;
		}

		#ifndef NO_FUNC_RUN_ARGS
			linenostack[++linenostacktop].funcname = buf.c;
		#else
			linenostack[++linenostacktop].funcname = def->names;
		#endif
		linenostack[linenostacktop].lineno = def->lineno;
		linenostack[linenostacktop].filename = def->filename;

		calc_run_syms(ret, def->syms);

		#ifndef NO_FUNC_RUN_ARGS
			free(buf.c);
		#endif
		vars.pDestructor = NULL;

		syms = def->syms;
		while(syms) {
			if(syms->type != GLOBAL_T) {
				syms = syms->next;
				continue;
			}

			tmpArgs = syms->args;
			while(tmpArgs) {
				ptr = NULL;

				zend_hash_find(&vars, tmpArgs->val.str, strlen(tmpArgs->val.str), (void**)&ptr);
				if(ptr) {
					if(tmpArgs->val.type != ARRAY_T) {
						zend_hash_update(&tmpVars, tmpArgs->val.str, strlen(tmpArgs->val.str), ptr, sizeof(exp_val_t), NULL);
					}
					zend_hash_del(&vars, tmpArgs->val.str, strlen(tmpArgs->val.str));
				}
				tmpArgs = tmpArgs->next;
			}

			syms = syms->next;
		}
		vars.pDestructor = (dtor_func_t)calc_free_vars;
		zend_hash_destroy(&vars);

		vars = tmpVars;

		linenostacktop--;
	} else {
		yyerror("undefined user function for %s.\n", expr->callName);
	}
}

void calc_run_sys_sqrt(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	
	calc_sqrt(ret, &val);
	
	calc_free_expr(&val);
}

void calc_run_sys_pow(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val1 = {NULL_T}, val2 = {NULL_T};

	args->val.run(&val1, &args->val);
	args->next->val.run(&val2, &args->next->val);
	
	calc_pow(ret, &val1, &val2);
	
	calc_free_expr(&val1);
	calc_free_expr(&val2);
}

void calc_run_sys_sin(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = sin(val.dval); // 正弦
	
	calc_free_expr(&val);
}

void calc_run_sys_asin(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = asin(val.dval); // 反正弦
	
	calc_free_expr(&val);
}

void calc_run_sys_cos(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = cos(val.dval); // 余弦
	
	calc_free_expr(&val);
}

void calc_run_sys_acos(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = acos(val.dval); // 反余弦
	
	calc_free_expr(&val);
}

void calc_run_sys_tan(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = tan(val.dval); // 正切
	
	calc_free_expr(&val);
}

void calc_run_sys_atan(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = atan(val.dval); // 反正切
	
	calc_free_expr(&val);
}

void calc_run_sys_ctan(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = ctan(val.dval); // 余切
	
	calc_free_expr(&val);
}

void calc_run_sys_rad(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = val.dval * M_PI / 180.0; // 弧度 - 双精度类型
	
	calc_free_expr(&val);
}

void calc_run_sys_rand(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	ret->type = INT_T;
	ret->ival = rand();
}

void calc_run_sys_randf(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	ret->type = DOUBLE_T;
	ret->dval = (double) rand() / (double) RAND_MAX;
}

void calc_run_sys_strlen(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	exp_val_t *ptr;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};
	
	ret->type = LONG_T;
	switch(args->val.type) {
		case STR_T:
			ret->lval = (long int) args->val.strlen;
			break;

		case VAR_T: {
			ptr = NULL;

			zend_hash_find(&vars, args->val.str, strlen(args->val.str), (void**)&ptr);

			if(ptr) {
				if(ptr->type == STR_T) {
					ret->lval = (long int) ptr->strlen;
				} else {
					CALC_CONV(ptr, ptr, str);
					ret->lval = (long int) ptr->strlen;
				}
			}
			break;
		}
		default: {
			args->val.run(&val, &args->val);

			if(val.type == STR_T) {
				ret->lval = (long int) val.strlen;
			} else {
				CALC_CONV(&val, &val, str);
				ret->lval = (long int) val.strlen;
			}
			
			if(val.type == STR_T) {
				free(val.str);
			}

			break;
		}
	}
}

void calc_run_sys_microtime(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	ret->type = DOUBLE_T;
	ret->dval = microtime();
}

void calc_run_sys_srand(exp_val_t *ret, exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->callArgs;
	register call_args_t *tmpArgs = expr->callArgs;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->callArgc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->callName, expr->callArgc, argc);
		return;
	}
	
	srand((unsigned int) microtime());
}

void calc_array_init(exp_val_t *ptr, call_args_t *args) {
	register int i;

	ptr->type = ARRAY_T;
	ptr->isref = 0;
	ptr->arrlen = args->val.ival;
	ptr->array = (exp_val_t*) malloc(sizeof(exp_val_t) * args->val.ival);
	ptr->run = calc_run_copy;

	memset(ptr->array, 0, sizeof(exp_val_t) * args->val.ival);

	for (i = 0; i < args->val.ival; i++) {
		ptr->array[i].run = calc_run_copy;
		if (args->next) {
			calc_array_init(&ptr->array[i], args->next);
		}
	}
}

status_enum_t calc_run_sym_echo(exp_val_t *ret, func_symbol_t *syms) {
	register call_args_t *args = syms->args;
	exp_val_t val = {NULL_T};
	
	while (args) {
		switch (args->val.type) {
			case STR_T:
				printf("%s", args->val.str);
				break;
			case NULL_T:
				printf("null");
				break;
			default: {
				calc_run_expr(&val, &args->val);
				calc_echo(&val);
				calc_free_expr(&val);
				break;
			}
		}
		args = args->next;
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_ret(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(ret, syms->expr);
	return RET_STATUS;
}

status_enum_t calc_run_sym_if(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t cond = {NULL_T};

	calc_run_expr(&cond, syms->cond);
	CALC_CONV((&cond), (&cond), dval);

	return (cond.dval ? calc_run_syms(ret, syms->lsyms) : calc_run_syms(ret, syms->rsyms));
}

status_enum_t calc_run_sym_while(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t cond = {NULL_T};

	calc_run_expr(&cond, syms->cond);
	CALC_CONV((&cond), (&cond), dval);

	status_enum_t status;
	while (cond.dval) {
		status = calc_run_syms(ret, syms->lsyms);
		if (status != NONE_STATUS) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}

		calc_run_expr(&cond, syms->cond);
		CALC_CONV((&cond), (&cond), dval);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_do_while(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t cond = {NULL_T};
	status_enum_t status;

	do {
		status = calc_run_syms(ret, syms->lsyms);
		if (status != NONE_STATUS) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}

		calc_run_expr(&cond, syms->cond);
		CALC_CONV((&cond), (&cond), dval);
	} while (cond.dval);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_break(exp_val_t *ret, func_symbol_t *syms) {
	return BREAK_STATUS;
}

status_enum_t calc_run_sym_list(exp_val_t *ret, func_symbol_t *syms) {
	LIST_STMT("\x1b[34m--- list in func(funcname: %s, line: %d) ---\x1b[0m\n", linenostack[linenostacktop].funcname, syms->lineno, ZEND_HASH_APPLY_KEEP);

	return NONE_STATUS;
}

status_enum_t calc_run_sym_clear(exp_val_t *ret, func_symbol_t *syms) {
	LIST_STMT("\x1b[34m--- clear in func(funcname: %s, line: %d) ---\x1b[0m\n", linenostack[linenostacktop].funcname, syms->lineno, ZEND_HASH_APPLY_REMOVE);

	return NONE_STATUS;
}

status_enum_t calc_run_sym_array(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	register call_args_t *callArgs = syms->args->next;
	call_args_t *tmpArgs = (call_args_t*) malloc(sizeof(call_args_t)), *args = tmpArgs;

	while (callArgs) {
		tmpArgs->val.type = NULL_T;
		calc_run_expr(&tmpArgs->val, &callArgs->val);
		CALC_CONV((&tmpArgs->val), (&tmpArgs->val), ival);
		tmpArgs->val.type = INT_T;
		if (tmpArgs->val.ival < 0) {
			tmpArgs->val.ival = -tmpArgs->val.ival;
		}
		if(tmpArgs->val.ival == 0) {
			yyerror("When the array %s is defined, the superscript is not equal to zero.\n", syms->args->val.str);
			abort();
		}
		callArgs = callArgs->next;
		if(callArgs) {
			tmpArgs->next = (call_args_t*) malloc(sizeof(call_args_t));
			tmpArgs = tmpArgs->next;
		} else {
			tmpArgs->next = NULL;
		}
	}

	tmpArgs->next = NULL;
	calc_array_init(&val, args);
	val.arrayArgs = args;

	zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_null(exp_val_t *ret, func_symbol_t *syms) {
	return NONE_STATUS;
}

status_enum_t calc_run_sym_inc(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t *ptr = NULL;

	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		exp_val_t val={INT_T,1};
		calc_add(ptr, ptr, &val);
	} else {
		exp_val_t val;
		val.type = INT_T;
		val.ival = 1;
		val.run = calc_run_copy;
		
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
				
	return NONE_STATUS;
}

status_enum_t calc_run_sym_dec(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t *ptr = NULL;
	
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		exp_val_t val={INT_T,1};
		calc_sub(ptr, ptr, &val);
	} else {
		exp_val_t val;
		val.type = INT_T;
		val.ival = -1;
		val.run = calc_run_copy;
		
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_func(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	calc_run_expr(&val, syms->expr);
	calc_free_expr(&val);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_assign(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		if(ptr->type == STR_T) {
			free(ptr->str);
		}
		memcpy(ptr, &val, sizeof(exp_val_t));
	} else {
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_addeq(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		calc_add(ptr, ptr, &val);
		ptr->run = calc_run_copy;
		calc_free_expr(&val);
	} else {
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_subeq(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		calc_sub(ptr, ptr, &val);
		ptr->run = calc_run_copy;
		calc_free_expr(&val);
	} else {
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_muleq(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		calc_mul(ptr, ptr, &val);
		ptr->run = calc_run_copy;
		calc_free_expr(&val);
	} else {
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_diveq(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		calc_div(ptr, ptr, &val);
		ptr->run = calc_run_copy;
		calc_free_expr(&val);
	} else {
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_modeq(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
	if (ptr) {
		calc_mod(ptr, ptr, &val);
		ptr->run = calc_run_copy;
		calc_free_expr(&val);
	} else {
		zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_array_set(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->var->callName, strlen(syms->var->callName), (void**)&ptr);
	if (!ptr || ptr->type != ARRAY_T) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", syms->var->callName, types[ptr->type - NULL_T]);
	} else {
		register call_args_t *args = syms->var->callArgs;

		exp_val_t *tmp = ptr, argsVal = {NULL_T};
		while (args) {
			calc_run_expr(&argsVal, &args->val);
			CALC_CONV((&argsVal), (&argsVal), ival);
			argsVal.type = INT_T;
			if (argsVal.ival < 0) {
				argsVal.ival = -argsVal.ival;
			}

			if (argsVal.ival < tmp->arrlen) {
				tmp = &tmp->array[argsVal.ival];
				if (tmp->type != ARRAY_T) {
					if (args->next) {
						yyerror("An array of %s dimension bounds.\n", syms->var->callName);
						
						goto breakStmt;
					} else {
						if(tmp->type == NULL_T) {
							switch(syms->type) {
								case SUBEQ_STMT_T:
									calc_minus(tmp, &val);
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								case MULEQ_STMT_T:
								case DIVEQ_STMT_T:
								case MODEQ_STMT_T:
									tmp->type = INT_T;
									tmp->ival = 0;
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								default:
									memcpy(tmp, &val, sizeof(exp_val_t));
									break;
							}
						} else {
							switch(syms->type) {
								case ADDEQ_STMT_T:
									calc_add(tmp, tmp, &val);
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								case SUBEQ_STMT_T:
									calc_sub(tmp, tmp, &val);
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								case MULEQ_STMT_T:
									calc_mul(tmp, tmp, &val);
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								case DIVEQ_STMT_T:
									calc_div(tmp, tmp, &val);
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								case MODEQ_STMT_T:
									calc_mod(tmp, tmp, &val);
									tmp->run = calc_run_copy;
									calc_free_expr(&val);
									break;
								default:
									if(tmp->type == STR_T) {
										free(tmp->str);
									}
									memcpy(tmp, &val, sizeof(exp_val_t));
									break;
							}
						}
						
						return NONE_STATUS;
					}
				}
			} else {
				yyerror("An array of %s index out of bounds.\n", syms->var->callName);
				goto breakStmt;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", syms->var->callName);
	}
	
	breakStmt:
	calc_free_expr(&val);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_for(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t cond = {NULL_T};

	calc_run_syms(ret, syms->lsyms);
	
	calc_run_expr(&cond, syms->cond);
	CALC_CONV((&cond), (&cond), dval);

	status_enum_t status;
	while (cond.dval) {
		status = calc_run_syms(ret, syms->forSyms);
		if (status != NONE_STATUS) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}
		
		calc_run_syms(ret, syms->rsyms);

		calc_run_expr(&cond, syms->cond);
		CALC_CONV((&cond), (&cond), dval);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_switch(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t cond = {NULL_T};
	register status_enum_t status;
	
	calc_run_expr(&cond, syms->cond);
	CALC_CONV((&cond), (&cond), str);
	
	if(cond.type != STR_T) {
		return NONE_STATUS;
	}

	zend_hash_find(syms->ht, cond.str, cond.strlen, (void**)&syms->rsyms);
	calc_free_expr(&cond);
	
	return syms->rsyms && calc_run_syms(ret, syms->rsyms) == RET_STATUS ? RET_STATUS : NONE_STATUS;
}

status_enum_t calc_run_syms(exp_val_t *ret, func_symbol_t *syms) {
	register status_enum_t status;

	if(linenostacktop+1 == sizeof(linenostack)/sizeof(linenostack_t)) {
		yyerror("stack overflow.\n");
		return;
	}

	linenostack[++linenostacktop].funcname = NULL;

	while (syms) {
		linenostack[linenostacktop].syms = syms;
		linenostack[linenostacktop].lineno = syms->lineno;
		linenostack[linenostacktop].filename = syms->filename;
		
		status = syms->run(ret, syms);
		if(status != NONE_STATUS) {
			return status;
		}

		syms = syms->next;
	}

	return NONE_STATUS;
}

void calc_array_free(exp_val_t *ptr, call_args_t *args) {
	register int i;
	for (i = 0; i < args->val.ival; i++) {
		if(ptr->array[i].type == ARRAY_T && args->next) {
			calc_array_free(&ptr->array[i], args->next);
		} else {
			calc_free_expr(&ptr->array[i]);
		}
	}
	free(ptr->array);
}

void calc_free_vars(exp_val_t *expr) {
	switch (expr->type) {
		case ARRAY_T: {
			dprintf("--- FreeVars: ARRAY_T(%d) ---\n", (int)expr->isref);
			if(!expr->isref) {
				calc_array_free(expr, expr->arrayArgs);
				calc_free_args(expr->arrayArgs);
			}
			break;
		}
		case STR_T: {
			dprintf("--- FreeVars: STR_T ---\n");
			free(expr->str);
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

#define YYPARSE() \
	do { \
		frees.pDestructor = NULL; \
		while((yret=yyparse()) && isSyntaxData) { \
			if(yywrap()) { \
				break; \
			} \
		} \
		if(isSyntaxData) { \
			memset(&expr, 0, sizeof(exp_val_t)); \
			calc_run_syms(&expr, topSyms); \
			calc_free_expr(&expr); \
			zend_hash_clean(&topFrees); \
			topSyms = NULL; \
			if(isolate) { \
				zend_hash_clean(&vars); \
				zend_hash_clean(&funcs); \
			} \
		} \
		while(yret && !yywrap()) {} \
		zend_hash_clean(&files); \
		frees.pDestructor = (dtor_func_t)free_frees; \
	} while(0)

void append_pool(void *ptr, dtor_func_t run) {
	pool_t p = {ptr,run};

	zend_hash_next_index_insert(&pools, &p, sizeof(pool_t), NULL);
}

void zend_hash_destroy_ptr(HashTable *ht) {
	zend_hash_destroy(ht);
	free(ht);
}
void free_pools(pool_t *p) {
	p->run(p->ptr);
}

int main(int argc, char **argv) {
	zend_hash_init(&files, 2, NULL);
	zend_hash_init(&frees, 20, NULL);
	zend_hash_init(&topFrees, 20, (dtor_func_t)free_frees);
	zend_hash_init(&vars, 2, (dtor_func_t)calc_free_vars);
	zend_hash_init(&funcs, 2, (dtor_func_t)calc_free_func);
	zend_hash_init(&pools, 2, (dtor_func_t)free_pools);

	#define USAGE() printf("Usage: %s { -v | -h | - | files.. }\n" \
		"\n" \
		"  author           zhang bao cai\n" \
		"  email            talent518@yeah.net\n" \
		"  git URL          https://github.com/talent518/calc.git\n" \
		"\n" \
		"  options:\n" \
		"    -              from stdin input source code.\n" \
		"    -v             Version number.\n" \
		"    -h             This help.\n" \
		"    -i             Isolate run.\n" \
		"    -I             Not isolate run.\n" \
		"    files...       from file input source code for multiple.\n" \
		, argv[0])
	if(argc > 1) {
		register int i,yret;
		register int isolate = 1;
		exp_val_t expr;
		for(i = 1; i<argc; i++) {
			if(argv[i][0] == '-') {
				if(argv[i][1] && !argv[i][2]) {
					switch(argv[i][1]) {
						case 'v':
							printf("v1.1\n");
							break;
						case 'h':
							USAGE();
							break;
						case 'i':
							isolate = 1;
							break;
						case 'I':
							isolate = 0;
							break;
						case 's':
							isSyntaxData = 1;
							break;
						case 'S':
							isSyntaxData = 0;
							break;
						EMPTY_SWITCH_DEFAULT_CASE()
					}
				} else {
					yyrestart(stdin);
					YYPARSE();
					break;
				}
			} else {
				FILE *fp = fopen(argv[i], "r");
				if(fp) {
					dprintf("==========================\n");
					dprintf("BEGIN INPUT: %s\n", argv[i]);
					dprintf("--------------------------\n");
					curFileName = argv[i];
					yylineno = 1;
					zend_hash_add(&files, argv[i], strlen(argv[i]), NULL, 0, NULL);
					yyrestart(fp);
					YYPARSE();
					fclose(fp);
				} else {
					yyerror("File \"%s\" not found!\n", argv[i]);
				}
			}
		}
	} else {
		USAGE();
	}
	
	yylex_destroy();

	zend_hash_destroy(&topFrees);
	zend_hash_destroy(&files);
	zend_hash_destroy(&frees);
	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);
	zend_hash_destroy(&pools);

	return exitCode;
}

void yyerror(const char *s, ...) {
	fprintf(stderr, "\x1b[31m");
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "Then error: in file \"%s\" on line %d: \n", curFileName, yylineno);

	register int i, n, len, lineno;
	char buf[256];
	char linenoformat[8];
	FILE *fp;
	for(i=linenostacktop; i>0; i--) {
		if(linenostack[i].funcname) {
			fprintf(stderr, "%s(%d): %s\n", linenostack[i].filename, linenostack[i].lineno, linenostack[i].funcname);
		} else {
			fprintf(stderr, "%s(%d): \n", linenostack[i].filename, linenostack[i].lineno);
		}

		fp = fopen(linenostack[i].filename, "r");
		if(!fp) {
			continue;
		}
		n = 0;
		buf[0] = '\0';
		lineno = max(linenostack[i].lineno-2, 1);
		while(n<lineno && fgets(buf, sizeof(buf), fp)) {
			len = strnlen(buf, sizeof(buf));
			n++;
			while(n<lineno && !(buf[len-1] == '\r' || buf[len-1] == '\n') && fgets(buf, sizeof(buf), fp)) {
				len = strnlen(buf, sizeof(buf));
			}
		}
		if(n!=lineno) {
			goto endecholine;
		}

		lineno = linenostack[i].lineno+2;
		sprintf(linenoformat, "%%%dd: ", (int)(log10((double)lineno)+1.0));
		fprintf(stderr, "\x1b[37m");
	echoline:
		if(n == linenostack[i].lineno) {
			fprintf(stderr, "\x1b[32m");
			fprintf(stderr, linenoformat, n);
			fwrite(buf, 1, len, stderr);
			while(n<lineno && !(buf[len-1] == '\r' || buf[len-1] == '\n') && fgets(buf, sizeof(buf), fp)) {
				len = strnlen(buf, sizeof(buf));
				fwrite(buf, 1, len, stderr);
			}
			fprintf(stderr, "\x1b[37m");
		} else {
			fprintf(stderr, linenoformat, n);
			fwrite(buf, 1, len, stderr);
			while(n<lineno && !(buf[len-1] == '\r' || buf[len-1] == '\n') && fgets(buf, sizeof(buf), fp)) {
				len = strnlen(buf, sizeof(buf));
				fwrite(buf, 1, len, stderr);
			}
		}

		if(n<lineno && fgets(buf, sizeof(buf), fp)) {
			len = strnlen(buf, sizeof(buf));
			n++;
			goto echoline;
		} else if(n<lineno && (buf[len-1] == '\r' || buf[len-1] == '\n')) {
			len = 1;
			buf[0] = '\n';
			n++;
			lineno=n;
			goto echoline;
		}
	
	endecholine:
		fprintf(stderr, "\x1b[31m");

		fclose(fp);
	}
	fprintf(stderr, "-----------------------------------\n");

	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "\x1b[0m");
}

int yywrap() {
	dprintf("--------------------------\n");
	dprintf("END INPUT: %s\n", curFileName);
	dprintf("==========================\n");

	if(includeDeep>0) {
		zend_hash_del(&files, curFileName, strlen(curFileName));

		includeDeep--;

		/*if(EXPECTED(isSyntaxData)) {
			free(curFileName);
		}*/

		fclose(yyin);
		curFileName = tailWrapStack->filename;
		yylineno = tailWrapStack->lineno;

		wrap_stack_t *ptr = tailWrapStack;

		tailWrapStack = tailWrapStack->prev;

		free(ptr);

		yypop_buffer_state();

		return 0;
	} else {
		return 1;
	}
}