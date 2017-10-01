#include "calc.h"

HashTable vars;
HashTable funcs;
HashTable files;
HashTable frees;
func_symbol_t *topSyms = NULL;
int isSyntaxData = 1;
int exitCode = 0;

typedef struct _linenostack {
	unsigned int lineno;
	char *funcname;
} linenostack_t;

static linenostack_t linenostack[1024]={{0,"TOP"}};
static int linenostacktop = 0;

char *types[] = { "NULL", "int", "long", "float", "double", "str", "array" };

#define CALC_CONV(dst,src,val) \
	switch((src)->type) { \
		case INT_T: CALC_CONV_ival2##val(dst, src);break; \
		case LONG_T: CALC_CONV_lval2##val(dst, src);break; \
		case FLOAT_T: CALC_CONV_fval2##val(dst, src);break; \
		case DOUBLE_T: CALC_CONV_dval2##val(dst, src);break; \
		case STR_T: { \
			char *__p = (src)->str; \
			type_enum_t __type = (dst)->type; \
			CALC_CONV_str2##val(dst, src); \
			if((dst) != (src) && __type == STR_T) { \
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
	}

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
	if(op1->type !=t) { \
		CALC_CONV(&val1, op1, val); \
		op1 = &val1; \
	} \
	if(op2->type != t) { \
		CALC_CONV(&val2, op2, val); \
		op2 = &val2; \
	}

void str2val(exp_val_t *val, char *str) {
	int n=strlen(str);
	switch(str[n-1]) {
		case 'I':
		case 'i':
			val->type=INT_T;
			val->ival = atol(str);
			break;
		case 'L':
		case 'l':
			val->type=LONG_T;
			val->lval = atol(str);
			break;
		case 'F':
		case 'f':
			val->type=FLOAT_T;
			val->fval = atof(str);
			break;
		case 'D':
		case 'd':
			val->type=DOUBLE_T;
			val->dval = atof(str);
			break;
		case 'R':
		case 'r':
			val->type=DOUBLE_T;
			val->dval = atof(str) * M_PI / 180.0;
			break;
		default:
			if(strchr(str,'.')) {
				double d = atof(str);
				if(FLT_MIN <= d && d <= FLT_MAX) {
					val->type = FLOAT_T;
					val->fval = d;
				} else {
					val->type = DOUBLE_T;
					val->dval = d;
				}
			} else {
				long int i = atoi(str);
				if(INT_MIN <= i && i <= INT_MAX) {
					val->type = INT_T;
					val->ival = i;
				} else {
					val->type = LONG_T;
					val->lval = i;
				}
			}
			break;
	}
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
		yyerror("op1 / op2, op2==0!!!");
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
		yyerror("op1 % op2, op2==0!!!");
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
	}
}

void calc_array_echo(exp_val_t *ptr, call_args_t *args, int deep) {
	register int i;
	for(i = 0; i<deep; i++) {
		printf(" ");
	}
	printf("[");
	if(args->next) {
		printf("\n");
	}
	for (i = 0; i < args->val.ival; i++) {
		if(args->next) {
			calc_array_echo(&ptr->array[i], args->next, deep+4);
			if(i+1 == args->val.ival) {
				printf("\n");
			} else {
				printf(",\n");
			}
		} else {
			if(i) {
				printf(", ");
			}
			calc_echo(&ptr->array[i]);
		}
	}
	if(args->next) {
		for(i = 0; i<deep; i++) {
			printf(" ");
		}
	}
	printf("]");
}

#ifdef DEBUG
#define CALC_ECHO_DEF(src,type,val,str) case type: printf("(%s) (" str ")", types[type-NULL_T],src->val);break
#else
#define CALC_ECHO_DEF(src,type,val,str) case type: printf(str,src->val);break
#endif

void calc_echo(exp_val_t *src) {
	switch (src->type) {
		CALC_ECHO_DEF(src, NULL_T, ival, "(null)");
		CALC_ECHO_DEF(src, INT_T, ival, "%d");
		CALC_ECHO_DEF(src, LONG_T, lval, "%ld");
		CALC_ECHO_DEF(src, FLOAT_T, fval, "%.16f");
		CALC_ECHO_DEF(src, DOUBLE_T, dval, "%.19lf");
		CALC_ECHO_DEF(src, STR_T, str, "%s");
		case ARRAY_T:
			calc_array_echo(src, src->arrayArgs, 0);
			break;
	}
}
#undef CALC_ECHO_DEF

#define CALC_SPRINTF(buf,src,type,val,str) case type: sprintf(buf,str,src->val);break
// printf(src)
void calc_sprintf(char *buf, exp_val_t *src) {
	switch (src->type) {
		CALC_SPRINTF(buf, src, INT_T, ival, "%d");
		CALC_SPRINTF(buf,src,LONG_T,lval,"%ld");
		CALC_SPRINTF(buf,src,FLOAT_T,fval,"%.16f");
		CALC_SPRINTF(buf,src,DOUBLE_T,dval,"%.19lf");
		CALC_SPRINTF(buf,src,STR_T,str,"\"%s\"");
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
	char names[1024] = { "" };

	strcat(names, def->name);
	strcat(names, "(");

	def->argc = 0;
	def->minArgc = 0;
	unsigned errArgc = 0;
	if (def->args) {
		strcat(names, def->args->name);
		if (def->args->val.type != NULL_T) {
			strcat(names, "=");
			calc_sprintf(names + strlen(names), &def->args->val);
		}
		func_args_t *args = def->args->next;
		def->argc++;
		if (def->args->val.type == NULL_T) {
			def->minArgc++;
		}
		while (args) {
			strcat(names, ", ");
			strcat(names, args->name);
			if (args->val.type != NULL_T) {
				strcat(names, "=");
				calc_sprintf(names + strlen(names), &args->val);
			}
			if (def->argc == def->minArgc) {
				if (args->val.type == NULL_T) {
					def->minArgc++;
				}
			} else if (!errArgc && args->val.type == NULL_T) {
				errArgc = def->argc;
			}
			def->argc++;
			args = args->next;
		}
	}
	strcat(names, ")");
	dprintf("%s", names);
	dprintf(" argc = %d, minArgc = %d", def->argc, def->minArgc);
	
	if (errArgc) {
		INTERRUPT(__LINE__, "The user function %s the first %d argument not default value", names, errArgc + 1);

		def->names = NULL;
		return;
	}
	
	def->names = strdup(names);
}

void calc_func_def(func_def_f *def) {
	dprintf("define function ");
	calc_func_print(def);
	dprintf("\n");

	if(def->names && zend_hash_add(&funcs, def->name, strlen(def->name), def, sizeof(func_def_f), NULL) == FAILURE) {
		INTERRUPT(__LINE__, "The user function \"%s\" already exists.\n", def->names);
	}
}

void calc_free_args(call_args_t *args) {
	call_args_t *tmpArgs;

	while (args) {
		calc_free_expr(&args->val);
		tmpArgs = args;
		args = args->next;
		free(tmpArgs);
	}
}

void calc_free_syms(func_symbol_t *syms) {
	func_symbol_t *tmpSyms;

	while (syms) {
		switch (syms->type) {
			case ECHO_STMT_T: {
				calc_free_args(syms->args);
				break;
			}
			case RET_STMT_T: {
				calc_free_expr(syms->expr);
				free(syms->expr);
				break;
			}
			case IF_STMT_T: {
				calc_free_expr(syms->cond);
				free(syms->cond);
				calc_free_syms(syms->lsyms);
				calc_free_syms(syms->rsyms);
				break;
			}
			case WHILE_STMT_T: {
				calc_free_expr(syms->cond);
				free(syms->cond);
				calc_free_syms(syms->lsyms);
				break;
			}
			case DO_WHILE_STMT_T: {
				calc_free_expr(syms->cond);
				free(syms->cond);
				calc_free_syms(syms->lsyms);
				break;
			}
			case BREAK_STMT_T: {
				// none EXPR
				break;
			}
			case ARRAY_STMT_T: {
				free(syms->args->val.str);
				calc_free_args(syms->args->next);
				free(syms->args);
				break;
			}
			case GLOBAL_T: {
				calc_free_args(syms->args);
				break;
			}
			case INC_STMT_T: {
				free(syms->args->val.str);
				free(syms->args);
				break;
			}
			case DEC_STMT_T: {
				free(syms->args->val.str);
				free(syms->args);
				break;
			}
			case ASSIGN_STMT_T:
			case ADDEQ_STMT_T:
			case SUBEQ_STMT_T:
			case MULEQ_STMT_T:
			case DIVEQ_STMT_T:
			case MODEQ_STMT_T: {
				calc_free_expr(syms->val);
				
				switch (syms->var->type) {
					case VAR_T: {
						free(syms->args->val.str);
						break;
					}
					case ARRAY_T: {
						free(syms->var->call.name);
						calc_free_args(syms->var->call.args);
						break;
					}
				}
				free(syms->var);
				free(syms->val);
				break;
			}
			case FUNC_STMT_T: {
				calc_free_expr(syms->expr);
				free(syms->expr);
				break;
			}
		}

nextStmt:
		tmpSyms = syms;
		syms = syms->next;
		free(tmpSyms);
	}
}

void calc_free_func(func_def_f *def) {
	free(def->name);
	free(def->names);

	func_args_t *args = def->args, *tmpArgs;

	while (args) {
		tmpArgs = args;
		calc_free_expr(&args->val);
		args = args->next;
		free(tmpArgs->name);
		free(tmpArgs);
	}

	calc_free_syms(def->syms);
}

void calc_run_expr(exp_val_t *ret, exp_val_t *expr) {
	memset(ret, 0, sizeof(exp_val_t));

	switch (expr->type) {
		case INT_T:
		case LONG_T:
		case FLOAT_T:
		case DOUBLE_T:
			memcpy(ret, expr, sizeof(exp_val_t));
			break;
		case STR_T:
			CALC_CONV(ret, expr, str);
			break;
		case VAR_T: {
			exp_val_t *ptr = NULL;
			zend_hash_find(&vars, expr->str, strlen(expr->str), (void**)&ptr);
			if (ptr) {
				if(ptr->type < ARRAY_T) {
					memcpy(ret, ptr, sizeof(exp_val_t));
				}
				if(ptr->type == STR_T) {
					ret->str = strndup(ptr->str, ptr->strlen);
				}
			} else {
				ret->type = INT_T;
				ret->ival = 0;
			}
			break;
		}
		case ADD_T:
		case SUB_T:
		case MUL_T:
		case DIV_T:
		case MOD_T:
		case POW_T: {
			exp_val_t left = {NULL_T}, right = {NULL_T};
			calc_run_expr(&left, expr->left);
			calc_run_expr(&right, expr->right);
			switch (expr->type) {
			case ADD_T:
				calc_add(ret, &left, &right);
				break;
			case SUB_T:
				calc_sub(ret, &left, &right);
				break;
			case MUL_T:
				calc_mul(ret, &left, &right);
				break;
			case DIV_T:
				calc_div(ret, &left, &right);
				break;
			case MOD_T:
				calc_mod(ret, &left, &right);
				break;
			case POW_T:
				calc_pow(ret, &left, &right);
				break;
			}
			calc_free_expr(&left);
			calc_free_expr(&right);
			break;
		}
		case ABS_T:
		case MINUS_T: {
			exp_val_t val = {NULL_T};
			calc_run_expr(&val, expr->left);
			if (expr->type == ABS_T) {
				calc_abs(ret, &val);
			} else {
				calc_minus(ret, &val);
			}
			calc_free_expr(&val);
			break;
		}
		case FUNC_T: {
			call_args_t *args = NULL, *tmpArgs = NULL, *callArgs = expr->call.args;
			unsigned argc = 0;

			while (callArgs) {
				if (tmpArgs) {
					tmpArgs->next = (call_args_t*) malloc(sizeof(call_args_t));
					tmpArgs = tmpArgs->next;
				} else {
					tmpArgs = (call_args_t*) malloc(sizeof(call_args_t));
					args = tmpArgs;
				}

				tmpArgs->val.type = NULL_T;
				calc_run_expr(&tmpArgs->val, &callArgs->val);

				tmpArgs->next = NULL;

				callArgs = callArgs->next;
				argc++;
			}

			calc_call(ret, expr->call.type, expr->call.name, argc, args);

			calc_free_args(args);
			break;
		}
		case LOGIC_GT_T:
		case LOGIC_LT_T:
		case LOGIC_GE_T:
		case LOGIC_LE_T:
		case LOGIC_EQ_T:
		case LOGIC_NE_T: {
			exp_val_t left = {NULL_T}, right = {NULL_T};

			calc_run_expr(&left, expr->left);
			calc_run_expr(&right, expr->right);

			CALC_CONV((&left), (&left), dval);
			CALC_CONV((&right), (&right), dval);

			ret->type = INT_T;
			switch (expr->type) {
			case LOGIC_GT_T:
				ret->ival = left.dval > right.dval;
				break;
			case LOGIC_LT_T:
				ret->ival = left.dval < right.dval;
				break;
			case LOGIC_GE_T:
				ret->ival = left.dval >= right.dval;
				break;
			case LOGIC_LE_T:
				ret->ival = left.dval <= right.dval;
				break;
			case LOGIC_EQ_T:
				ret->ival = left.dval == right.dval;
				break;
			case LOGIC_NE_T:
				ret->ival = left.dval != right.dval;
				break;
			}
			calc_free_expr(&left);
			calc_free_expr(&right);
			break;
		}
		case IF_T: {
			exp_val_t cond = {NULL_T};

			calc_run_expr(&cond, expr->cond);
			CALC_CONV((&cond), (&cond), dval);

			if (cond.dval) {
				calc_run_expr(ret, expr->left);
			} else {
				calc_run_expr(ret, expr->right);
			}
			calc_free_expr(&cond);
			break;
		}
		case ARRAY_T: {
			exp_val_t *ptr = NULL;
			zend_hash_find(&vars, expr->call.name, strlen(expr->call.name), (void**)&ptr);
			if (ptr->type != ARRAY_T) {
				yyerror("(warning) variable %s not is a array, type is %s", expr->call.name, types[ptr->type - NULL_T]);
			} else {
				ret->type = INT_T;
				ret->ival = 0;
				call_args_t *args = expr->call.args;

				exp_val_t *tmp = ptr, val = {NULL_T};
				while (args) {
					calc_run_expr(&val, &args->val);
					CALC_CONV((&val), (&val), ival);
					val.type = INT_T;
					if (val.ival < 0) {
						val.ival = -val.ival;
					}

					if (val.ival < tmp->arrlen) {
						tmp = &tmp->array[val.ival];
						if (tmp->type != ARRAY_T) {
							if (args->next) {
								yyerror("An array of %s dimension bounds", expr->call.name);
							} else {
								calc_run_expr(ret, tmp);
							}
							return;
						}
					} else {
						yyerror("An array of %s index out of bounds", expr->call.name);
						tmp = NULL;
						return;
					}

					args = args->next;
				}

				yyerror("Array %s dimension deficiency", expr->call.name);
			}
			break;
		}
	}
}

void calc_array_init(exp_val_t *ptr, call_args_t *args) {
	ptr->type = ARRAY_T;
	ptr->arrlen = args->val.ival;
	ptr->array = (exp_val_t*) malloc(sizeof(exp_val_t) * args->val.ival);

	memset(ptr->array, 0, sizeof(exp_val_t) * args->val.ival);

	if (args->next) {
		int i;
		for (i = 0; i < args->val.ival; i++) {
			calc_array_init(&ptr->array[i], args->next);
		}
	}
}

status_enum_t calc_run_syms(exp_val_t *ret, func_symbol_t *syms) {
	while (syms) {
		linenostack[linenostacktop].lineno = syms->lineno;
		switch (syms->type) {
			case ECHO_STMT_T: {
				call_args_t *args = syms->args;
				while (args) {
					switch (args->val.type) {
					case STR_T:
						printf("%s", args->val.str);
						break;
					case NULL_T:
						printf("null");
						break;
					default: {
						exp_val_t val = {NULL_T};
						calc_run_expr(&val, &args->val);
						calc_echo(&val);
						calc_free_expr(&val);
						break;
					}
					}
					args = args->next;
				}
				break;
			}
			case RET_STMT_T: {
				calc_run_expr(ret, syms->expr);
				return RET_STATUS;
			}
			case IF_STMT_T: {
				exp_val_t cond = {NULL_T};

				calc_run_expr(&cond, syms->cond);
				CALC_CONV((&cond), (&cond), dval);

				status_enum_t status = (cond.dval ? calc_run_syms(ret, syms->lsyms) : calc_run_syms(ret, syms->rsyms));
				calc_free_expr(&cond);
				if (status != NONE_STATUS) {
					return status;
				}
				break;
			}
			case WHILE_STMT_T: {
				exp_val_t cond = {NULL_T};

				calc_run_expr(&cond, syms->cond);
				CALC_CONV((&cond), (&cond), dval);

				status_enum_t status;
				while (cond.dval) {
					status = calc_run_syms(ret, syms->lsyms);
					if (status != NONE_STATUS) {
						calc_free_expr(&cond);
						return status;
					}

					calc_run_expr(&cond, syms->cond);
					CALC_CONV((&cond), (&cond), dval);
				}
				calc_free_expr(&cond);
				break;
			}
			case DO_WHILE_STMT_T: {
				exp_val_t cond = {NULL_T};
				status_enum_t status;

				do {
					status = calc_run_syms(ret, syms->lsyms);
					if (status == RET_STATUS) {
						calc_free_expr(&cond);
						return status;
					} else if (status == BREAK_STATUS) {
						break;
					}

					calc_run_expr(&cond, syms->cond);
					CALC_CONV((&cond), (&cond), dval);
				} while (cond.dval);
				calc_free_expr(&cond);
				break;
			}
			case BREAK_STMT_T: {
				return BREAK_STATUS;
			}
			case LIST_STMT_T: {
				LIST_STMT("\x1b[34m--- list in func(funcname: %s, line: %d) ---\x1b[0m\n", linenostack[linenostacktop].funcname, syms->lineno, ZEND_HASH_APPLY_KEEP);
				break;
			}
			case CLEAR_STMT_T: {
				LIST_STMT("\x1b[34m--- clear in func(funcname: %s, line: %d) ---\x1b[0m\n", linenostack[linenostacktop].funcname, syms->lineno, ZEND_HASH_APPLY_REMOVE);
				break;
			}
			case ARRAY_STMT_T: {
				exp_val_t val = {NULL_T};
				call_args_t *callArgs = syms->args->next;
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
						yyerror("When the array %s is defined, the superscript is not equal to zero.", syms->args->val.str);
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
				break;
			}
			case GLOBAL_T: {
				// 只在函数执行完成后做复制对应tmpVars中的变量到全局vars中)
				break;
			}
			case INC_STMT_T: {
				exp_val_t *ptr = NULL;
				zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
				if (ptr) {
					exp_val_t val={INT_T,1};
					calc_add(ptr, ptr, &val);
				} else {
					exp_val_t val;
					val.type = INT_T;
					val.ival = 1;
					
					zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
				}
				break;
			}
			case DEC_STMT_T: {
				exp_val_t *ptr = NULL;
				zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
				if (ptr) {
					exp_val_t val={INT_T,1};
					calc_sub(ptr, ptr, &val);
				} else {
					exp_val_t val;
					val.type = INT_T;
					val.ival = -1;
					
					zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
				}
				break;
			}
			case ASSIGN_STMT_T:
			case ADDEQ_STMT_T:
			case SUBEQ_STMT_T:
			case MULEQ_STMT_T:
			case DIVEQ_STMT_T:
			case MODEQ_STMT_T: {
				exp_val_t val = {NULL_T};
				
				calc_run_expr(&val, syms->val);
				
				switch (syms->var->type) {
					case VAR_T: {
						exp_val_t *ptr = NULL;
						zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
						if (ptr) {
							switch(syms->type) {
								case ADDEQ_STMT_T:
									calc_add(ptr, ptr, &val);
									calc_free_expr(&val);
									break;
								case SUBEQ_STMT_T:
									calc_sub(ptr, ptr, &val);
									calc_free_expr(&val);
									break;
								case MULEQ_STMT_T:
									calc_mul(ptr, ptr, &val);
									calc_free_expr(&val);
									break;
								case DIVEQ_STMT_T:
									calc_div(ptr, ptr, &val);
									calc_free_expr(&val);
									break;
								case MODEQ_STMT_T:
									calc_mod(ptr, ptr, &val);
									calc_free_expr(&val);
									break;
								default:
									if(ptr->type == STR_T) {
										free(ptr->str);
									}
									memcpy(ptr, &val, sizeof(exp_val_t));
									break;
							}
						} else {
							zend_hash_update(&vars, syms->args->val.str, strlen(syms->args->val.str), &val, sizeof(exp_val_t), NULL);
						}
						break;
					}
					case ARRAY_T: {
						exp_val_t *ptr = NULL;
						zend_hash_find(&vars, syms->var->call.name, strlen(syms->var->call.name), (void**)&ptr);
						if (ptr->type != ARRAY_T) {
							yyerror("(warning) variable %s not is a array, type is %s", syms->var->call.name, types[ptr->type - NULL_T]);
						} else {
							call_args_t *args = syms->var->call.args;

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
											yyerror("An array of %s dimension bounds", syms->var->call.name);
										} else {
											if(tmp->type == NULL_T) {
												switch(syms->type) {
													case SUBEQ_STMT_T:
														calc_minus(tmp, &val);
														calc_free_expr(&val);
														break;
													case MULEQ_STMT_T:
													case DIVEQ_STMT_T:
													case MODEQ_STMT_T:
														tmp->type = INT_T;
														tmp->ival = 0;
														break;
													default:
														memcpy(tmp, &val, sizeof(exp_val_t));
														break;
												}
											} else {
												switch(syms->type) {
													case ADDEQ_STMT_T:
														calc_add(tmp, tmp, &val);
														calc_free_expr(&val);
														break;
													case SUBEQ_STMT_T:
														calc_sub(tmp, tmp, &val);
														calc_free_expr(&val);
														break;
													case MULEQ_STMT_T:
														calc_mul(tmp, tmp, &val);
														calc_free_expr(&val);
														break;
													case DIVEQ_STMT_T:
														calc_div(tmp, tmp, &val);
														calc_free_expr(&val);
														break;
													case MODEQ_STMT_T:
														calc_mod(tmp, tmp, &val);
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
										}
										goto nextStmt;
									}
								} else {
									yyerror("An array of %s index out of bounds", syms->var->call.name);
									tmp = NULL;
									goto nextStmt;
								}

								args = args->next;
							}

							yyerror("Array %s dimension deficiency", syms->var->call.name);
						}
					}
				}
				break;
			}
			case FUNC_STMT_T: {
				exp_val_t val = {NULL_T};
				calc_run_expr(&val, syms->expr);
				calc_free_expr(&val);
				break;
			}
			case SRAND_STMT_T: {
				seed_rand();
				break;
			}
		}

		nextStmt: syms = syms->next;
	}

	return NONE_STATUS;
}

void call_func_run(exp_val_t *ret, func_def_f *def, call_args_t *args) {
	HashTable tmpVars = vars;
	call_args_t *tmpArgs = args;
	func_args_t *funcArgs = def->args;
	func_symbol_t *syms;
	call_args_t *_args;
	exp_val_t val = {NULL_T};
	
	zend_hash_init(&vars, 2, (dtor_func_t)call_free_vars);

	while (funcArgs) {
		if(tmpArgs) {
			calc_run_expr(&val, &tmpArgs->val);
			tmpArgs = tmpArgs->next;
		} else {
			calc_run_expr(&val, &funcArgs->val);
		}
		zend_hash_update(&vars, funcArgs->name, strlen(funcArgs->name), &val, sizeof(exp_val_t), NULL);
		funcArgs = funcArgs->next;
	}

	syms = def->syms;
	while(syms) {
		if(syms->type != GLOBAL_T) {
			syms = syms->next;
			continue;
		}
		
		_args = syms->args;
		while(_args) {
			exp_val_t *ptr = NULL;
			
			zend_hash_find(&tmpVars, _args->val.str, strlen(_args->val.str), (void**)&ptr);
			if(ptr) {
				zend_hash_add(&tmpVars, _args->val.str, strlen(_args->val.str), ptr, sizeof(exp_val_t), NULL);
			}
			_args = _args->next;
		}
		
		syms = syms->next;
	}

	calc_run_syms(ret, def->syms);
	
	vars.pDestructor = NULL;
	
	syms = def->syms;
	while(syms) {
		if(syms->type != GLOBAL_T) {
			syms = syms->next;
			continue;
		}
		
		_args = syms->args;
		while(_args) {
			exp_val_t *ptr = NULL;
			
			zend_hash_find(&vars, _args->val.str, strlen(_args->val.str), (void**)&ptr);
			if(ptr) {
				if(_args->val.type != ARRAY_T) {
					zend_hash_update(&tmpVars, _args->val.str, strlen(_args->val.str), ptr, sizeof(exp_val_t), NULL);
				}
				zend_hash_del(&vars, _args->val.str, strlen(_args->val.str));
			}
			_args = _args->next;
		}
		
		syms = syms->next;
	}
	vars.pDestructor = (dtor_func_t)call_free_vars;
	zend_hash_destroy(&vars);

	vars = tmpVars;
}

void calc_call(exp_val_t *ret, call_enum_f ftype, char *name, unsigned argc, call_args_t *args) {
	call_args_t *tmpArgs = args;
	unsigned _argc = 0;
	while (tmpArgs) {
		_argc++;
		tmpArgs = tmpArgs->next;
	}

	if (ftype != USER_F && _argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d. %s", name, _argc);
		exit(0);
	}

	switch (ftype) {
	case SQRT_F:
		calc_sqrt(ret, &(args->val)); // 开方
		break;
	case POW_F:
		calc_pow(ret, &(args->val), &(args->next->val)); // 乘方
		break;
	case SIN_F:
	case ASIN_F:
	case COS_F:
	case ACOS_F:
	case TAN_F:
	case ATAN_F:
	case CTAN_F:
	case RAD_F:
		CALC_CONV((ret), (&(args->val)), dval)
		;
		ret->type = DOUBLE_T;
		switch (ftype) {
		case SIN_F:
			ret->dval = sin(ret->dval); // 正弦
			break;
		case ASIN_F:
			ret->dval = asin(ret->dval); // 反正弦
			break;
		case COS_F:
			ret->dval = cos(ret->dval); // 余弦
			break;
		case ACOS_F:
			ret->dval = acos(ret->dval); // 反余弦
			break;
		case TAN_F:
			ret->dval = tan(ret->dval); // 正切
			break;
		case ATAN_F:
			ret->dval = atan(ret->dval); // 反正切
			break;
		case CTAN_F:
			ret->dval = ctan(ret->dval); // 余切
			break;
		case RAD_F:
			ret->dval = ret->dval * M_PI / 180.0; // 弧度 - 双精度类型
			break;
		}
		break;
	case RAND_F:
		ret->type = INT_T;
		ret->ival = rand();
		break;
	case RANDF_F:
		ret->type = DOUBLE_T;
		ret->dval = (double) rand() / (double) RAND_MAX;
		break;
	case USER_F: {
		func_def_f *def = NULL;
		zend_hash_find(&funcs, name, strlen(name), (void**)&def);
		if (def) {
			if (_argc > def->argc || _argc < def->minArgc) {
				//printf("minArgc=%d, _argc=%d, argc=%d\n", def->minArgc, _argc, def->argc);
				yyerror("The custom function %s the number of parameters should be %d, at least %d, the actual %d. %s", name, def->argc, def->minArgc, _argc);
			}

			dprintf("call user function for %s\n", def->names);

			linenostack[++linenostacktop].funcname = name;
			call_func_run(ret, def, args);
			linenostacktop--;
		} else {
			yyerror("undefined user function for %s\n", name);
		}
		break;
	}
	default:
		yyerror("undefined system function for %s\n", name);
		break;
	}
}

// 产生随即种子
void seed_rand() {
	struct timeval tp = { 0 };

	if (gettimeofday(&tp, NULL)) {
		srand((unsigned int) time((time_t*)NULL));
		return;
	}

	srand((unsigned int) (tp.tv_sec + tp.tv_usec));
}

void calc_free_expr(exp_val_t *expr) {
	switch (expr->type) {
		case STR_T:
		case VAR_T: {
			dprintf("--- FreeExpr: VAR_T/STR_T ---\n");
			free(expr->str);
			break;
		}
		case ADD_T:
		case SUB_T:
		case MUL_T:
		case DIV_T:
		case MOD_T:
		case POW_T: {
			dprintf("--- FreeExpr: ADD_T/.../POW_T ---\n");
			calc_free_expr(expr->left);
			calc_free_expr(expr->right);

			free(expr->left);
			free(expr->right);
			break;
		}
		case ABS_T:
		case MINUS_T: {
			dprintf("--- FreeExpr: ABS_T/MINUS_T ---\n");
			calc_free_expr(expr->left);
			
			free(expr->left);
			break;
		}
		case FUNC_T: {
			dprintf("--- FreeExpr: FUNC_T ---\n");
			calc_free_args(expr->call.args);
			if(expr->call.type == USER_F) {
				dprintf("--- FreeExpr: USER_F ---\n");
				free(expr->call.name);
			}
			break;
		}
		case LOGIC_GT_T:
		case LOGIC_LT_T:
		case LOGIC_GE_T:
		case LOGIC_LE_T:
		case LOGIC_EQ_T:
		case LOGIC_NE_T: {
			dprintf("--- FreeExpr: LOGIC_??_T ---\n");
			calc_free_expr(expr->left);
			calc_free_expr(expr->right);
			
			free(expr->left);
			free(expr->right);
			break;
		}
		case IF_T: {
			dprintf("--- FreeExpr: IF_T ---\n");
			calc_free_expr(expr->cond);
			calc_free_expr(expr->left);
			calc_free_expr(expr->right);

			free(expr->cond);
			free(expr->left);
			free(expr->right);
			break;
		}
		case ARRAY_T: {
			dprintf("--- FreeExpr: ARRAY_T ---\n");
			calc_free_args(expr->call.args);
			free(expr->call.name);
			break;
		}
	}
}

void calc_array_free(exp_val_t *ptr, call_args_t *args) {
	int i;
	for (i = 0; i < args->val.ival; i++) {
		if(ptr->array[i].type == ARRAY_T && args->next) {
			calc_array_free(&ptr->array[i], args->next);
		} else {
			calc_free_expr(&ptr->array[i]);
		}
	}
	free(ptr->array);
}

void call_free_vars(exp_val_t *expr) {
	switch (expr->type) {
		case ARRAY_T: {
			dprintf("--- FreeVars: ARRAY_T ---\n");
			calc_array_free(expr, expr->arrayArgs);
			calc_free_args(expr->arrayArgs);
			break;
		}
		case STR_T: {
			dprintf("--- FreeVars: STR_T ---\n");
			free(expr->str);
			break;
		}
		default: {
			dprintf("--- FreeVars: %s ---\n", types[expr->type - NULL_T]);
		}
	}
}

#ifdef DEBUG
static void free_frees(char *s) {
	dprintf("FREES: %s\n", s);
	free(s);
}
#else
	#define free_frees free
#endif

#define YYPARSE() while(yyparse() && isSyntaxData) { \
		frees.pDestructor = (dtor_func_t)free_frees; \
		if(yywrap()) { \
			break; \
		} else { \
			zend_hash_clean(&frees); \
			frees.pDestructor = NULL; \
		} \
	} \
	if(isSyntaxData) { \
		memset(&expr, 0, sizeof(exp_val_t)); \
		calc_run_syms(&expr, topSyms); \
		calc_free_expr(&expr); \
		calc_free_syms(topSyms); \
		topSyms = NULL; \
		if(isolate) { \
			zend_hash_clean(&vars); \
			zend_hash_clean(&funcs); \
		} \
	} else { \
		frees.pDestructor = (dtor_func_t)free_frees; \
	} \
	while(!yywrap()) {} \
	zend_hash_clean(&files); \
	zend_hash_clean(&frees); \
	frees.pDestructor = NULL

int main(int argc, char **argv) {
	zend_hash_init(&files, 2, NULL);
	zend_hash_init(&frees, 20, NULL);
	zend_hash_init(&vars, 2, (dtor_func_t)call_free_vars);
	zend_hash_init(&funcs, 2, (dtor_func_t)calc_free_func);

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
		int i;
		int isolate = 1;
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

	zend_hash_destroy(&files);
	zend_hash_destroy(&frees);
	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);

	return exitCode;
}

void yyerror(const char *s, ...) {
	fprintf(stderr, "\x1b[31m");
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "Then error: in file \"%s\" on line %d: \n", curFileName, yylineno);

	int i;
	for(i=0; i<=linenostacktop; i++) {
		fprintf(stderr, "Line %d in user function %s()\n", linenostack[i].lineno, linenostack[i].funcname);
	}
	fprintf(stderr, "-----------------------------------\n");

	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "\x1b[0m");
}

