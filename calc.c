#include "calc.h"

HashTable pools;
HashTable vars;
HashTable funcs;
HashTable files;
HashTable frees;
HashTable topFrees;
HashTable results;
func_symbol_t *topSyms = NULL;
int isSyntaxData = 1;
int exitCode = 0;

linenostack_t linenostack[1024]={{0,"TOP",NULL}};
int linenostacktop = 0;

char *types[] = { "NULL", "int", "long", "float", "double", "str", "array" };

void calc_conv_to_int(exp_val_t *src) {
	switch(src->type) {
		case INT_T: {
			return;
		}
		case LONG_T: {
			src->ival = src->lval;
			break;
		}
		case FLOAT_T: {
			src->ival = src->fval;
			break;
		}
		case DOUBLE_T: {
			src->ival = src->dval;
			break;
		}
		case STR_T: {
			string_t *str = src->str;
			
			src->ival = 0;
			sscanf(str->c, "%d", &src->ival);
			
			free_str(str);
			break;
		}
		default: {
			calc_free_expr(src);
			memset(src, 0, sizeof(exp_val_t));
			src->result = src;
			src->run = NULL;
			break;
		}
	}
	
	src->type = INT_T;
}

void calc_conv_to_long(exp_val_t *src) {
	switch(src->type) {
		case INT_T: {
			src->lval = src->ival;
			break;
		}
		case LONG_T: {
			return;
		}
		case FLOAT_T: {
			src->lval = src->fval;
			break;
		}
		case DOUBLE_T: {
			src->lval = src->dval;
			break;
		}
		case STR_T: {
			string_t *str = src->str;
			
			src->lval = 0;
			sscanf(str->c, "%ld", &src->lval);
			
			free_str(str);
			break;
		}
		default: {
			calc_free_expr(src);
			memset(src, 0, sizeof(exp_val_t));
			src->result = src;
			src->run = NULL;
			break;
		}
	}
	
	src->type = LONG_T;
	src->result = src;
	src->run = NULL;
}
void calc_conv_to_float(exp_val_t *src) {
	switch(src->type) {
		case INT_T: {
			src->fval = src->ival;
			break;
		}
		case LONG_T: {
			src->fval = src->fval;
			break;
		}
		case FLOAT_T: {
			return;
		}
		case DOUBLE_T: {
			src->fval = src->dval;
			break;
		}
		case STR_T: {
			string_t *str = src->str;
			
			src->fval = 0;
			sscanf(str->c, "%f", &src->fval);
			
			free_str(str);
			break;
		}
		default: {
			calc_free_expr(src);
			memset(src, 0, sizeof(exp_val_t));
			src->result = src;
			src->run = NULL;
			break;
		}
	}
	
	src->type = FLOAT_T;
	src->result = src;
	src->run = NULL;
}
void calc_conv_to_double(exp_val_t *src) {
	switch(src->type) {
		case INT_T: {
			src->dval = src->ival;
			break;
		}
		case LONG_T: {
			src->dval = src->lval;
			break;
		}
		case FLOAT_T: {
			src->dval = src->fval;
			break;
		}
		case DOUBLE_T: {
			return;
		}
		case STR_T: {
			string_t *str = src->str;
			
			src->dval = 0;
			sscanf(str->c, "%lf", &src->dval);
			
			free_str(str);
			break;
		}
		default: {
			calc_free_expr(src);
			memset(src, 0, sizeof(exp_val_t));
			src->result = src;
			src->run = NULL;
			break;
		}
	}
	
	src->type = DOUBLE_T;
	src->result = src;
	src->run = NULL;
}

#define CALC_CONV_num2str(fmt, size, key) CNEW0(str->c, char, size); str->n = snprintf(str->c, size, fmt, src->key)

void calc_conv_to_str(exp_val_t *src) {
	DNEW01(str, string_t);
	
	switch(src->type) {
		case INT_T: {
			CALC_CONV_num2str("%d", 12, ival);
			break;
		}
		case LONG_T: {
			CALC_CONV_num2str("%ld", 22, lval);
			break;
		}
		case FLOAT_T: {
			CALC_CONV_num2str("%f", 23, fval);
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_num2str("%lf", 33, dval);
			break;
		}
		case STR_T: {
			free(str);
			return;
		}
		default: {
			calc_free_expr(src);
			CNEW0(str->c, char, 1);
			*(str->c) = '\0';
			break;
		}
	}
	
	src->type = STR_T;
	src->str = str;
	src->result = src;
	src->run = NULL;
}
void calc_conv_to_array(exp_val_t *src) {
	if(src->type != ARRAY_T) {
		DNEW01(arr, array_t);
		CNEW1(arr->array, exp_val_t);
		memcpy(arr->array, src, sizeof(exp_val_t));
		arr->array->result = arr->array;
		arr->arrlen = 1;
		arr->dims = 1;
		arr->args[0] = 1;
		
		src->type = ARRAY_T;
		src->arr = arr;
	}
}

void calc_array_echo(array_t *arr, unsigned int n, unsigned int ii, int dim) {
	register int i, hasNext = dim+1<arr->dims;
#ifdef DEBUG
	for(i = 0; i<dim*4; i++) {
		printf(" ");
	}
#endif
	printf("[");
#ifdef DEBUG
	if(hasNext) {
		printf("\n");
	}
#endif

	register unsigned int dims = arr->args[arr->dims-1-dim], nii;
	for (i = 0; i < dims; i++) {
		nii = ii+i*n;
		if(hasNext) {
			calc_array_echo(arr, n*dims, nii, dim+1);
			if(i+1 < dims) {
				printf(",");
			}
		#ifdef DEBUG
			printf("\n");
		#endif
		} else {
			if(i) {
				printf(", ");
			}
			calc_echo(&arr->array[nii]);
		}
	}
#ifdef DEBUG
	if(hasNext) {
		for(i = 0; i<dim*4; i++) {
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
		case NULL_T: printf("NULL");break;
		CALC_ECHO_DEF(src, INT_T, ival, "%d");
		CALC_ECHO_DEF(src, LONG_T, lval, "%ld");
		CALC_ECHO_DEF(src, FLOAT_T, fval, "%.16f");
		CALC_ECHO_DEF(src, DOUBLE_T, dval, "%.19lf");
		CALC_ECHO_DEF(src, STR_T, str->c, "%s");
		case ARRAY_T: {
			calc_array_echo(src->arr, 1, 0, 0);
			break;
		}
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
		case STR_T: smart_string_appendc(buf, '"');smart_string_appendl(buf, src->str->c, src->str->n);smart_string_appendc(buf, '"');break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
#undef CALC_SPRINTF

int calc_clear_or_list_vars(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key) {
	int result = va_arg(args, int);
	char *key = strndup(hash_key->arKey, hash_key->nKeyLength);

	printf("\x1b[34m");
	if (result == ZEND_HASH_APPLY_KEEP) {
		printf("    (%6s) %s = ", types[val->type], key);
	} else {
		printf("    remove variable %s, type is %s, value is ", key, types[val->type]);
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

void calc_run_variable(exp_val_t *expr) {
	exp_val_t *ptr = NULL;
	
	zend_hash_find(&vars, expr->var, strlen(expr->var), (void**)&ptr);
	if (ptr) {
		memcpy(expr->result, ptr, sizeof(exp_val_t));
		if(ptr->type == STR_T) {
			expr->result->str->gc++;
		}
		if(expr->result->type == ARRAY_T) {
			expr->result->arr->gc++;
		}
	} else {
		expr->result->type = INT_T;
		expr->result->ival = 0;
	}
}

// left + right
void calc_run_add(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);

	switch (max(expr->defExp->left->result->type, expr->defExp->right->result->type)) {
		case INT_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, int);
			expr->result->ival = expr->defExp->left->result->ival + expr->defExp->right->result->ival;
			expr->result->type = INT_T;
			break;
		}
		case LONG_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, long);
			expr->result->lval = expr->defExp->left->result->lval + expr->defExp->right->result->lval;
			expr->result->type = LONG_T;
			break;
		}
		case FLOAT_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, float);
			expr->result->fval = expr->defExp->left->result->fval + expr->defExp->right->result->fval;
			expr->result->type = FLOAT_T;
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
			expr->result->dval = expr->defExp->left->result->dval + expr->defExp->right->result->dval;
			expr->result->type = DOUBLE_T;
			break;
		}
		case STR_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, str);
			
			CNEW01(expr->result->str, string_t);
			expr->result->str->n = expr->defExp->right->result->str->n + expr->defExp->right->result->str->n;
			expr->result->str->c = (char*) malloc(expr->result->str->n + 1);
			memcpy(expr->result->str->c, expr->defExp->left->result->str->c, expr->defExp->left->result->str->n);
			memcpy(expr->result->str->c + expr->defExp->left->result->str->n, expr->defExp->right->result->str->c, expr->defExp->right->result->str->n);
			*(expr->result->str->c + expr->result->str->n) = '\0';

			expr->result->type = STR_T;
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// left - right
void calc_run_sub(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	str2num(expr->defExp->left->result);
	str2num(expr->defExp->right->result);
	
	switch (max(expr->defExp->left->result->type, expr->defExp->right->result->type)) {
		case INT_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, int);
			expr->result->ival = expr->defExp->left->result->ival - expr->defExp->right->result->ival;
			expr->result->type = INT_T;
			break;
		}
		case LONG_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, long);
			expr->result->lval = expr->defExp->left->result->lval - expr->defExp->right->result->lval;
			expr->result->type = LONG_T;
			break;
		}
		case FLOAT_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, float);
			expr->result->fval = expr->defExp->left->result->fval - expr->defExp->right->result->fval;
			expr->result->type = FLOAT_T;
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
			expr->result->dval = expr->defExp->left->result->dval - expr->defExp->right->result->dval;
			expr->result->type = DOUBLE_T;
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// left * right
void calc_run_mul(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	str2num(expr->defExp->left->result);
	str2num(expr->defExp->right->result);
	
	switch (max(expr->defExp->left->result->type, expr->defExp->right->result->type)) {
		case INT_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, int);
			expr->result->ival = expr->defExp->left->result->ival * expr->defExp->right->result->ival;
			expr->result->type = INT_T;
			break;
		}
		case LONG_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, long);
			expr->result->lval = expr->defExp->left->result->lval * expr->defExp->right->result->lval;
			expr->result->type = LONG_T;
			break;
		}
		case FLOAT_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, float);
			expr->result->fval = expr->defExp->left->result->fval * expr->defExp->right->result->fval;
			expr->result->type = FLOAT_T;
			break;
		}
		case DOUBLE_T: {
			CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
			expr->result->dval = expr->defExp->left->result->dval * expr->defExp->right->result->dval;
			expr->result->type = DOUBLE_T;
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// left / right
void calc_run_div(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	expr->result->type = DOUBLE_T;
	if(expr->defExp->right->result->dval) {
		expr->result->dval = expr->defExp->left->result->dval / expr->defExp->right->result->dval;
	} else {
		expr->result->dval = 0.0;
		yyerror("op1 / op2, op2==0!!!\n");
	}
}

// left % right
void calc_run_mod(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, long);

	expr->result->type = LONG_T;
	if(expr->defExp->right->result->lval) {
		expr->result->lval = expr->defExp->left->result->lval % expr->defExp->right->result->lval;
	} else {
		expr->result->lval = 0;
		yyerror("op1 %% op2, op2==0!!!\n");
	}
}

// left ^ right
void calc_run_pow(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	expr->result->type = DOUBLE_T;
	expr->result->dval = pow(expr->defExp->left->result->dval, expr->defExp->right->result->dval);
}

// |ref|
void calc_run_abs(exp_val_t *expr) {
	calc_run_expr(expr->ref);
	
	str2num(expr->ref->result);

#define CALC_ABS_DEF(t,k) case t: expr->result->k=(expr->ref->result->k > 0 ? expr->ref->result->k : -expr->ref->result->k);expr->result->type = t;break
	switch (expr->ref->result->type) {
		CALC_ABS_DEF(INT_T, ival);
		CALC_ABS_DEF(LONG_T, lval);
		CALC_ABS_DEF(FLOAT_T, fval);
		CALC_ABS_DEF(DOUBLE_T, dval);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
#undef CALC_ABS_DEF
}

// -ref
void calc_run_minus(exp_val_t *expr) {
	calc_run_expr(expr->ref);

#define CALC_MINUS_DEF(t,k) case t: expr->result->k=-expr->ref->result->k;expr->result->type = t;break
	switch (expr->ref->result->type) {
		CALC_MINUS_DEF(INT_T, ival);
		CALC_MINUS_DEF(LONG_T, lval);
		CALC_MINUS_DEF(FLOAT_T, fval);
		CALC_MINUS_DEF(DOUBLE_T, dval);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
#undef CALC_MINUS_DEF
}

void calc_run_iif(exp_val_t *expr) {
	calc_run_expr(expr->defExp->cond);
	calc_conv_to_double(expr->defExp->cond->result);

	if (expr->defExp->cond->result->dval) {
		calc_run_expr(expr->defExp->left);
		memcpy_ref_expr(expr->result, expr->defExp->left->result);
	} else {
		calc_run_expr(expr->defExp->right);
		memcpy_ref_expr(expr->result, expr->defExp->right->result);
	}
}

void calc_run_gt(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval > expr->defExp->right->result->dval;
}

void calc_run_lt(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval < expr->defExp->right->result->dval;
}

void calc_run_ge(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval >= expr->defExp->right->result->dval;
}

void calc_run_le(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval <= expr->defExp->right->result->dval;
}

void calc_run_eq(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval == expr->defExp->right->result->dval;
}

void calc_run_ne(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval != expr->defExp->right->result->dval;
}

void calc_run_array(exp_val_t *expr) {
	exp_val_t *ptr = NULL, *tmp;
	register call_args_t *args = expr->call->args;
	
	zend_hash_find(&vars, expr->call->name, strlen(expr->call->name), (void**)&ptr);
	if(!ptr) {
		yyerror("(warning) variable %s not exists, cannot read array or string value.\n", expr->call->name);
	} else if (ptr->type == STR_T) {
		str_array_access:
		if(args->next) {
			yyerror("An string of %s dimension bounds.\n", expr->call->name);
			return;
		}
		calc_run_expr(&args->val);
		calc_conv_to_int(args->val.result);
		if(args->val.result->ival<0 || args->val.result->ival>=ptr->str->n) {
			yyerror("An string of %s index out of bounds.\n", expr->call->name);
			return;
		}
		expr->result->type = STR_T;
		CNEW01(expr->result->str, string_t);
		expr->result->str->c = strndup(ptr->str->c+args->val.result->ival, 1);
		expr->result->str->n = 1;
	} else if (ptr->type != ARRAY_T) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", expr->call->name, types[ptr->type - NULL_T]);
	} else {
		expr->result->type = INT_T;
		expr->result->ival = 0;

		exp_val_t val = {NULL_T};
		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&args->val);
			calc_conv_to_int(args->val.result);
			if (args->val.result->ival < 0) {
				args->val.result->ival = -args->val.result->ival;
			}

			if (args->val.result->ival >= ptr->arr->args[i]) {
				yyerror("An array of %s index out of bounds.\n", expr->call->name);
				return;
			}
			ii += args->val.result->ival*n;
			n *= ptr->arr->args[i];
			if(++i  == ptr->arr->dims) {
				ptr = &ptr->arr->array[ii];
				if (args->next) {
					if(ptr->type == STR_T) goto str_array_access;
					yyerror("An array of %s dimension bounds.\n", expr->call->name);
				} else {
					calc_free_expr(ptr);
					memcpy_ref_expr(expr->result, ptr);
				}
				return;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", expr->call->name);
	}
}

void calc_run_func(exp_val_t *expr) {
	register unsigned int argc = 0;
	func_def_f *def = NULL;
	exp_val_t *ptr;
	register call_args_t *tmpArgs = NULL;
	register func_args_t *funcArgs;
	register func_symbol_t *syms;

	if(linenostacktop+1 == sizeof(linenostack)/sizeof(linenostack_t)) {
		yyerror("stack overflow.\n");
		return;
	}
	
	zend_hash_find(&funcs, expr->call->name, strlen(expr->call->name), (void**)&def);
	if (def) {
		tmpArgs = expr->call->args;
		while(tmpArgs) {
			argc++;
		
			tmpArgs = tmpArgs->next;
		}

		if (argc > def->argc || argc < def->minArgc) {
			yyerror("The custom function %s the number of parameters should be %d, at least %d, the actual %d.\n", expr->call->name, def->argc, def->minArgc, argc);
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

		tmpArgs = expr->call->args;
		exp_val_t *p;
		while (funcArgs) {
			CNEW1(p, exp_val_t);
			
			if(tmpArgs) {
				calc_run_expr(&tmpArgs->val);
				#ifndef NO_FUNC_RUN_ARGS
					if(argc) {
						smart_string_appends(&buf, ", ");
					}
					calc_sprintf(&buf, tmpArgs->val.result);
				#endif
				memcpy_ref_expr(p, tmpArgs->val.result);
				tmpArgs = tmpArgs->next;
			} else {
				calc_run_expr(&funcArgs->val);
				memcpy_ref_expr(p, funcArgs->val.result);
			}
			zend_hash_update(&vars, funcArgs->name, strlen(funcArgs->name), p, 0, NULL);
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

				zend_hash_find(&tmpVars, tmpArgs->val.var, strlen(tmpArgs->val.var), (void**)&ptr);
				if(ptr) {
					CNEW1(p, exp_val_t);
					memcpy_ref_expr(p, ptr);
					ptr = NULL;
					zend_hash_update(&vars, tmpArgs->val.var, strlen(tmpArgs->val.var), p, 0, (void**)&ptr);
					if(ptr) {
						yyerror("global variable \"%s\" and function parameter conflict.\n", tmpArgs->val.var);
					}
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

		calc_run_syms(expr->result, def->syms);

		#ifndef NO_FUNC_RUN_ARGS
			free(buf.c);
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

				zend_hash_find(&vars, tmpArgs->val.var, strlen(tmpArgs->val.var), (void**)&ptr);
				if(ptr) {
					CNEW1(p, exp_val_t);
					memcpy_ref_expr(p, ptr);
					zend_hash_update(&tmpVars, tmpArgs->val.var, strlen(tmpArgs->val.var), p, 0, NULL);
				}
				tmpArgs = tmpArgs->next;
			}

			syms = syms->next;
		}
		zend_hash_destroy(&vars);

		vars = tmpVars;

		linenostacktop--;
	} else {
		yyerror("undefined user function for %s.\n", expr->call->name);
	}
}

void calc_run_sys_sqrt(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	
	str2num(args->val.result);

#define CALC_SQRT_DEF(t,k) case t: expr->result->dval=sqrt(args->val.result->k);expr->result->type = DOUBLE_T;break
	switch (args->val.result->type) {
		CALC_SQRT_DEF(INT_T, ival);
		CALC_SQRT_DEF(LONG_T, lval);
		CALC_SQRT_DEF(FLOAT_T, fval);
		CALC_SQRT_DEF(DOUBLE_T, dval);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
#undef CALC_SQRT_DEF
}

void calc_run_sys_pow(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}

	calc_run_expr(&args->val);
	calc_run_expr(&args->next->val);
	
	CALC_CONV_op(args->val.result, args->next->val.result, double);

	expr->result->type = DOUBLE_T;
	expr->result->dval = pow(args->val.result->dval, args->next->val.result->dval);
}

void calc_run_sys_sin(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = sin(args->val.result->dval); // 正弦
}

void calc_run_sys_asin(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = asin(args->val.result->dval); // 反正弦
}

void calc_run_sys_cos(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = cos(val.dval); // 余弦
}

void calc_run_sys_acos(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = acos(args->val.result->dval); // 反余弦
}

void calc_run_sys_tan(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = tan(args->val.result->dval); // 正切
}

void calc_run_sys_atan(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = atan(args->val.result->dval); // 反正切
}

void calc_run_sys_ctan(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	exp_val_t val = {NULL_T};

	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = ctan(args->val.result->dval); // 余切
}

void calc_run_sys_rad(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = args->val.result->dval * M_PI / 180.0; // 弧度 - 双精度类型
}

void calc_run_sys_rand(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	expr->result->type = INT_T;
	expr->result->ival = rand();
}

void calc_run_sys_randf(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	expr->result->type = DOUBLE_T;
	expr->result->dval = (double) rand() / (double) RAND_MAX;
}

void calc_run_sys_strlen(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;
	exp_val_t *ptr;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	expr->result->type = LONG_T;
	switch(args->val.type) {
		case STR_T:
			expr->result->lval = (long int) args->val.str->n;
			break;

		case VAR_T: {
			ptr = NULL;

			zend_hash_find(&vars, args->val.var, strlen(args->val.var), (void**)&ptr);

			if(ptr) {
				if(ptr->type == STR_T) {
					expr->result->lval = (long int) ptr->str->n;
				} else {
					calc_conv_to_str(ptr);
					expr->result->lval = (long int) ptr->str->n;
				}
			}
			break;
		}
		default: {
			calc_run_expr(&args->val);

			if(args->val.result->type == STR_T) {
				expr->result->lval = (long int) args->val.result->str->n;
			}

			break;
		}
	}
}

void calc_run_sys_microtime(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	expr->result->type = DOUBLE_T;
	expr->result->dval = microtime();
}

void calc_run_sys_srand(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function %s the number of parameters should be %d, the actual %d.\n", expr->call->name, expr->call->argc, argc);
		return;
	}
	
	srand((unsigned int) microtime());
}

status_enum_t calc_run_sym_echo(exp_val_t *ret, func_symbol_t *syms) {
	register call_args_t *args = syms->args;
	
	while (args) {
		switch (args->val.type) {
			case STR_T:
				printf("%s", args->val.str->c);
				break;
			case NULL_T:
				printf("null");
				break;
			default: {
				calc_run_expr(&args->val);
				calc_echo(args->val.result);
				break;
			}
		}
		args = args->next;
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_ret(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(syms->expr);
	
	memcpy_ref_expr(ret, syms->expr);
	
	return RET_STATUS;
}

status_enum_t calc_run_sym_if(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(syms->cond);
	calc_conv_to_double(syms->cond->result);

	return (syms->cond->result->dval ? calc_run_syms(ret, syms->lsyms) : calc_run_syms(ret, syms->rsyms));
}

status_enum_t calc_run_sym_while(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(syms->cond);
	calc_conv_to_double(syms->cond->result);

	status_enum_t status;
	while (syms->cond->result->dval) {
		status = calc_run_syms(ret, syms->lsyms);
		if (status != NONE_STATUS) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}

		calc_run_expr(syms->cond);
		calc_conv_to_double(syms->cond->result);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_do_while(exp_val_t *ret, func_symbol_t *syms) {
	status_enum_t status;

	do {
		status = calc_run_syms(ret, syms->lsyms);
		if (status != NONE_STATUS) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}

		calc_run_expr(syms->cond);
		calc_conv_to_double(syms->cond->result);
	} while (syms->cond->result->dval);
	
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
	register call_args_t *args = syms->args;
	DNEW01(arr, array_t);

	arr->arrlen = 1;
	while (args) {
		calc_run_expr(&args->val);
		calc_conv_to_int(args->val.result);

		if (args->val.result->ival < 0) {
			args->val.result->ival = -args->val.result->ival;
		}
		if(args->val.result->ival == 0) {
			yyerror("When the array %s is defined, the superscript is not equal to zero.\n", syms->var->var);
			return NONE_STATUS;
		}
		arr->arrlen *= args->val.result->ival;
		arr->args[arr->dims] = args->val.result->ival;
		arr->dims++;
		args = args->next;
	}

	CNEW0(arr->array, exp_val_t, arr->arrlen);

	DNEW1(ptr, exp_val_t);
	ptr->type = ARRAY_T;
	ptr->arr = arr;
	ptr->result = ptr;
	ptr->run = NULL;
	
	zend_hash_update(&vars, syms->var->var, strlen(syms->var->var), ptr, 0, NULL);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_null(exp_val_t *ret, func_symbol_t *syms) {
	return NONE_STATUS;
}

status_enum_t calc_run_sym_func(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(syms->expr);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_variable_assign(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t *ptr = NULL;

	calc_run_expr(syms->val);
	zend_hash_find(&vars, syms->var->var, strlen(syms->var->var), (void**)&ptr);
	if (ptr) {
		calc_free_expr(ptr);
		memcpy_ref_expr(ptr, syms->val->result);
	} else {
		DNEW1(p, exp_val_t);
		memcpy_ref_expr(p, syms->val->result);

		zend_hash_update(&vars, syms->var->var, strlen(syms->var->var), p, 0, NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_array_assign(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t *ptr = NULL;

	calc_run_expr(syms->val);
	zend_hash_find(&vars, syms->var->call->name, strlen(syms->var->call->name), (void**)&ptr);
	if (!ptr || ptr->type != ARRAY_T) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", syms->var->call->name, types[ptr->type - NULL_T]);
	} else {
		register call_args_t *args = syms->var->call->args;

		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&args->val);
			calc_conv_to_int(args->val.result);
			if (args->val.result->ival < 0) {
				args->val.result->ival = -args->val.result->ival;
			}

			if (args->val.result->ival >= ptr->arr->args[i]) {
				yyerror("An array of %s index out of bounds.\n", syms->var->call->name);
				return NONE_STATUS;
			}
			ii += args->val.result->ival*n;
			n *= ptr->arr->args[i];
			if(++i == ptr->arr->dims) {
				ptr = &ptr->arr->array[ii];
				if (args->next) {
					yyerror("An array of %s dimension bounds.\n", syms->var->call->name);
				} else {
					calc_free_expr(ptr);
					memcpy_ref_expr(ptr, args->val.result);
				}
				
				return NONE_STATUS;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", syms->var->call->name);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_for(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_syms(ret, syms->lsyms);
	
	calc_run_expr(syms->cond);
	calc_conv_to_double(syms->cond->result);

	status_enum_t status;
	while (syms->cond->result->dval) {
		status = calc_run_syms(ret, syms->forSyms);
		if (status != NONE_STATUS) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}
		
		calc_run_syms(ret, syms->rsyms);

		calc_run_expr(syms->cond);
		calc_conv_to_double(syms->cond->result);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_switch(exp_val_t *ret, func_symbol_t *syms) {
	register status_enum_t status;
	
	calc_run_expr(syms->cond);
	calc_conv_to_str(syms->cond->result);

	zend_hash_find(syms->ht, syms->cond->result->str->c, syms->cond->result->str->n, (void**)&syms->rsyms);
	
	return syms->rsyms && calc_run_syms(ret, syms->rsyms) == RET_STATUS ? RET_STATUS : NONE_STATUS;
}

status_enum_t calc_run_syms(exp_val_t *ret, func_symbol_t *syms) {
	register status_enum_t status;

	if(linenostacktop+1 == sizeof(linenostack)/sizeof(linenostack_t)) {
		yyerror("stack overflow.\n");
		return NONE_STATUS;
	}

	linenostack[++linenostacktop].funcname = NULL;

	while (syms) {
		linenostack[linenostacktop].syms = syms;
		linenostack[linenostacktop].lineno = syms->lineno;
		linenostack[linenostacktop].filename = syms->filename;
		
		status = syms->run(ret, syms);
		if(status != NONE_STATUS) {
			linenostacktop--;
			return status;
		}

		syms = syms->next;
	}
	linenostacktop--;

	return NONE_STATUS;
}

void calc_free_expr(exp_val_t *expr) {
	switch (expr->type) {
		case ARRAY_T: {
			dprintf("--- FreeVars: ARRAY_T(%u) ---\n", expr->arr->gc);
			if(!(expr->arr->gc--)) {
				register unsigned int i;
				for (i = 0; i < expr->arr->arrlen; i++) {
					calc_free_expr(&expr->arr->array[i]);
				}
				free(expr->arr->array);
				free(expr->arr);
			}
			break;
		}
		case STR_T: {
			dprintf("--- FreeVars: STR_T(%u) ---\n", expr->str->gc);
			free_str(expr->str);
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
void calc_free_vars(exp_val_t *expr) {
	calc_free_expr(expr);
	free(expr);
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
	zend_hash_init(&vars, 20, (dtor_func_t)calc_free_vars);
	zend_hash_init(&funcs, 20, (dtor_func_t)calc_free_func);
	zend_hash_init(&pools, 2, (dtor_func_t)free_pools);
	zend_hash_init(&results, 20, (dtor_func_t)calc_free_vars);

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
	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);
	zend_hash_destroy(&pools);
	zend_hash_destroy(&results);
	zend_hash_destroy(&frees);

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
