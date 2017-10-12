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
			dst->str = NEW1(string_t);
			dst->str->gc = 0;
			dst->str->n = op1->str->n + op2->str->n;
			dst->str->c = (char*) malloc(dst->str->n+1);
			memcpy(dst->str->c, op1->str, op1->str->n);
			memcpy(dst->str->c+op1->str->n, op2->str->c, op2->str->n);
			*(dst->str->c + dst->str->n) = '\0';
			if(val1.type == STR_T) {
				free_str(val1.str);
			}
			if(val2.type == STR_T) {
				free_str(val2.str);
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
		str2val(&val1, op1->str->c);
		op1 = &val1;
	}
	if(op2->type == STR_T) {
		exp_val_t val2 = {INT_T, 0};
		str2val(&val2, op2->str->c);
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
		str2val(&val1, op1->str->c);
		op1 = &val1;
	}
	if(op2->type == STR_T) {
		exp_val_t val2 = {INT_T, 0};
		str2val(&val2, op2->str->c);
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
		str2val(&val1, src->str->c);
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
		str2val(&val1, src->str->c);
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
		str2val(&val1, src->str->c);
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
	for (i = 0; i < arr->args[dim]; i++) {
		if(hasNext) {
			calc_array_echo(arr, n*arr->args[dim], ii+i*n, dim+1);
			if(i+1 < arr->args[dim]) {
				printf(",");
			}
		#ifdef DEBUG
			printf("\n");
		#endif
		} else {
			if(i) {
				printf(", ");
			}
			calc_echo(&arr->array[ii+i*n]);
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
		CALC_ECHO_DEF(src, NULL_T, str, "%s");
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
	expr->str->gc++;
	ret->type = STR_T;
	ret->str = expr->str;
	ret->run = calc_run_strdup;
}

void calc_run_variable(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t *ptr = NULL;
	
	zend_hash_find(&vars, expr->var, strlen(expr->var), (void**)&ptr);
	if (ptr) {
		memcpy(ret, ptr, sizeof(exp_val_t));
		if(ptr->type == STR_T) {
			ret->str->gc++;
		}
		if(ret->type == ARRAY_T) {
			ret->arr->gc++;
		}
	} else {
		ret->type = INT_T;
		ret->ival = 0;
	}
}

void calc_run_add(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
	calc_add(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_sub(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
	calc_sub(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_mul(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
	calc_mul(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_div(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
	calc_div(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_mod(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
	calc_mod(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_pow(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
	calc_pow(ret, &left, &right);
	ret->run = calc_run_copy;
	
	calc_free_expr(&left);
	calc_free_expr(&right);
}

void calc_run_abs(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t ref = {NULL_T};
	
	expr->ref->run(&ref, expr->ref);
	
	calc_abs(ret, &ref);
	ret->run = calc_run_copy;
	
	calc_free_expr(&ref);
}

void calc_run_minus(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t ref = {NULL_T};
	
	expr->ref->run(&ref, expr->ref);
	
	calc_minus(ret, &ref);
	ret->run = calc_run_copy;
	
	calc_free_expr(&ref);
}

void calc_run_iif(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t cond = {NULL_T};

	expr->defExp->cond->run(&cond, expr->defExp->cond);
	CALC_CONV((&cond), (&cond), dval);

	if (cond.dval) {
		expr->defExp->left->run(ret, expr->defExp->left);
	} else {
		expr->defExp->right->run(ret, expr->defExp->right);
	}
	
	calc_free_expr(&cond);
}

void calc_run_gt(exp_val_t *ret, exp_val_t *expr) {
	exp_val_t left = {NULL_T}, right = {NULL_T};
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
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
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
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
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
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
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
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
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
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
	
	expr->defExp->left->run(&left, expr->defExp->left);
	expr->defExp->right->run(&right, expr->defExp->right);
	
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
		calc_run_expr(&val, &args->val);
		CALC_CONV((&val), (&val), ival);
		if(val.ival<0 || val.ival>=ptr->str->n) {
			yyerror("An string of %s index out of bounds.\n", expr->call->name);
			return;
		}
		ret->type = STR_T;
		CNEW01(ret->str, string_t);
		ret->str->c = strndup(ptr->str->c+val.ival, 1);
		ret->str->n = 1;
		ret->run = calc_run_strdup;
	} else if (ptr->type != ARRAY_T) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", expr->call->name, types[ptr->type - NULL_T]);
	} else {
		ret->type = INT_T;
		ret->ival = 0;

		exp_val_t val = {NULL_T};
		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&val, &args->val);
			CALC_CONV((&val), (&val), ival);
			val.type = INT_T;
			if (val.ival < 0) {
				val.ival = -val.ival;
			}

			if (val.ival >= ptr->arr->args[i]) {
				yyerror("An array of %s index out of bounds.\n", expr->call->name);
				return;
			}
			ii += val.ival*n;
			n *= ptr->arr->args[i];
			if(++i  == ptr->arr->dims) {
				ptr = &ptr->arr->array[ii];
				if (args->next) {
					if(ptr->type == STR_T) goto str_array_access;
					yyerror("An array of %s dimension bounds.\n", expr->call->name);
				} else {
					calc_run_expr(ret, ptr);
				}
				return;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", expr->call->name);
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
		while (funcArgs) {
			if(tmpArgs) {
				val.type = NULL_T;
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

				zend_hash_find(&tmpVars, tmpArgs->val.var, strlen(tmpArgs->val.var), (void**)&ptr);
				if(ptr) {
					zend_hash_add(&tmpVars, tmpArgs->val.var, strlen(tmpArgs->val.var), ptr, sizeof(exp_val_t), NULL);
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

				zend_hash_find(&vars, tmpArgs->val.var, strlen(tmpArgs->val.var), (void**)&ptr);
				if(ptr) {
					zend_hash_update(&tmpVars, tmpArgs->val.var, strlen(tmpArgs->val.var), ptr, sizeof(exp_val_t), NULL);
					zend_hash_del(&vars, tmpArgs->val.var, strlen(tmpArgs->val.var));
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
		yyerror("undefined user function for %s.\n", expr->call->name);
	}
}

void calc_run_sys_sqrt(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	
	calc_sqrt(ret, &val);
	
	calc_free_expr(&val);
}

void calc_run_sys_pow(exp_val_t *ret, exp_val_t *expr) {
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
	
	exp_val_t val1 = {NULL_T}, val2 = {NULL_T};

	args->val.run(&val1, &args->val);
	args->next->val.run(&val2, &args->next->val);
	
	calc_pow(ret, &val1, &val2);
	
	calc_free_expr(&val1);
	calc_free_expr(&val2);
}

void calc_run_sys_sin(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = sin(val.dval); // 正弦
	
	calc_free_expr(&val);
}

void calc_run_sys_asin(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = asin(val.dval); // 反正弦
	
	calc_free_expr(&val);
}

void calc_run_sys_cos(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = cos(val.dval); // 余弦
	
	calc_free_expr(&val);
}

void calc_run_sys_acos(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = acos(val.dval); // 反余弦
	
	calc_free_expr(&val);
}

void calc_run_sys_tan(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = tan(val.dval); // 正切
	
	calc_free_expr(&val);
}

void calc_run_sys_atan(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = atan(val.dval); // 反正切
	
	calc_free_expr(&val);
}

void calc_run_sys_ctan(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = ctan(val.dval); // 余切
	
	calc_free_expr(&val);
}

void calc_run_sys_rad(exp_val_t *ret, exp_val_t *expr) {
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

	args->val.run(&val, &args->val);
	CALC_CONV(&val, &val, dval);
	ret->type = DOUBLE_T;
	ret->dval = val.dval * M_PI / 180.0; // 弧度 - 双精度类型
	
	calc_free_expr(&val);
}

void calc_run_sys_rand(exp_val_t *ret, exp_val_t *expr) {
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
	
	ret->type = INT_T;
	ret->ival = rand();
}

void calc_run_sys_randf(exp_val_t *ret, exp_val_t *expr) {
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
	
	ret->type = DOUBLE_T;
	ret->dval = (double) rand() / (double) RAND_MAX;
}

void calc_run_sys_strlen(exp_val_t *ret, exp_val_t *expr) {
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
	
	exp_val_t val = {NULL_T};
	
	ret->type = LONG_T;
	switch(args->val.type) {
		case STR_T:
			ret->lval = (long int) args->val.str->n;
			break;

		case VAR_T: {
			ptr = NULL;

			zend_hash_find(&vars, args->val.var, strlen(args->val.var), (void**)&ptr);

			if(ptr) {
				if(ptr->type == STR_T) {
					ret->lval = (long int) ptr->str->n;
				} else {
					CALC_CONV(ptr, ptr, str);
					ret->lval = (long int) ptr->str->n;
				}
			}
			break;
		}
		default: {
			args->val.run(&val, &args->val);

			if(val.type != STR_T) {
				CALC_CONV(&val, &val, str);
			}
			
			if(val.type == STR_T) {
				ret->lval = (long int) val.str->n;
				free_str(val.str);
			}

			break;
		}
	}
}

void calc_run_sys_microtime(exp_val_t *ret, exp_val_t *expr) {
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
	
	ret->type = DOUBLE_T;
	ret->dval = microtime();
}

void calc_run_sys_srand(exp_val_t *ret, exp_val_t *expr) {
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
	exp_val_t val = {NULL_T};
	
	while (args) {
		switch (args->val.type) {
			case STR_T:
				printf("%s", args->val.str->c);
				break;
			case NULL_T:
				printf("null");
				break;
			default: {
				val.type = NULL_T;
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
	register call_args_t *args = syms->args;
	DNEW01(arr, array_t);

	while (args) {
		calc_run_expr(&val, &args->val);
		CALC_CONV((&val), (&val), ival);
		val.type = INT_T;
		if (val.ival < 0) {
			val.ival = -val.ival;
		}
		if(val.ival == 0) {
			yyerror("When the array %s is defined, the superscript is not equal to zero.\n", syms->args->val.str);
			return NONE_STATUS;
		}
		arr->arrlen *= val.ival;
		arr->args[arr->dims] = val.ival;
		arr->dims++;
		args = args->next;
	}

	CNEW0(arr->array, exp_val_t, arr->arrlen);

	register unsigned int i;
	for (i = 0; i < args->val.ival; i++) {
		arr->array[i].run = calc_run_copy;
	}

	val.type = ARRAY_T;
	val.arr = arr;
	
	zend_hash_update(&vars, syms->var->var, strlen(syms->var->var), &val, sizeof(exp_val_t), NULL);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_null(exp_val_t *ret, func_symbol_t *syms) {
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
	zend_hash_find(&vars, syms->var->var, strlen(syms->var->var), (void**)&ptr);
	if (ptr) {
		calc_free_expr(ptr);
		memcpy(ptr, &val, sizeof(exp_val_t));
	} else {
		zend_hash_update(&vars, syms->var->var, strlen(syms->var->var), &val, sizeof(exp_val_t), NULL);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_array_assign(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t val = {NULL_T};
	exp_val_t *ptr = NULL;

	calc_run_expr(&val, syms->val);
	zend_hash_find(&vars, syms->var->call->name, strlen(syms->var->call->name), (void**)&ptr);
	if (!ptr || ptr->type != ARRAY_T) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", syms->var->call->name, types[ptr->type - NULL_T]);
	} else {
		register call_args_t *args = syms->var->call->args;

		exp_val_t val = {NULL_T};
		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&val, &args->val);
			CALC_CONV((&val), (&val), ival);
			val.type = INT_T;
			if (val.ival < 0) {
				val.ival = -val.ival;
			}

			if (val.ival >= ptr->arr->args[i]) {
				yyerror("An array of %s index out of bounds.\n", syms->var->call->name);
				goto breakStmt;
			}
			ii += val.ival*n;
			n *= ptr->arr->args[i];
			if(++i  == ptr->arr->dims) {
				ptr = &ptr->arr->array[ii];
				if (args->next) {
					yyerror("An array of %s dimension bounds.\n", syms->var->call->name);
					
					goto breakStmt;
				} else {
					calc_free_expr(ptr);
					memcpy(ptr, &val, sizeof(exp_val_t));
					
					return NONE_STATUS;
				}
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", syms->var->call->name);
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

	zend_hash_find(syms->ht, cond.str->c, cond.str->n, (void**)&syms->rsyms);
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
			linenostacktop--;
			return status;
		}

		syms = syms->next;
	}
	linenostacktop--;

	return NONE_STATUS;
}

zend_always_inline void calc_array_free(array_t *arr) {
	register unsigned int i;
	for (i = 0; i < arr->arrlen; i++) {
		calc_free_expr(&arr->array[i]);
	}
	free(arr->array);
	free(arr);
}

void calc_free_vars(exp_val_t *expr) {
	switch (expr->type) {
		case ARRAY_T: {
			dprintf("--- FreeVars: ARRAY_T(%u) ---\n", expr->arr->gc);
			if(!(expr->arr->gc--)) {
				calc_array_free(expr->arr);
			}
			break;
		}
		case STR_T: {
			dprintf("--- FreeVars: STR_T(%u) ---\n", expr->str->gc);
			if(!(expr->str->gc--)) {
				free(expr->str->c);
				free(expr->str);
			}
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