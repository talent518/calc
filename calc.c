#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "calc.h"
#include "parser.h"
#include "scanner.h"

HashTable pools;
HashTable vars;
HashTable funcs;
HashTable files;
HashTable frees;
HashTable topFrees;
HashTable results;
HashTable consts;
func_symbol_t *topSyms = NULL;
int isSyntaxData = 1;
int isolate = 1;
int exitCode = 0;
char *errmsg = NULL;
int errmsglen = 0;

linenostack_t linenostack[1024]={{0,"TOP",NULL}};
int linenostacktop = 0;

char *types[] = { "NULL", "int", "long", "float", "double", "str", "array", "ptr" };

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
				long int i = atol(str);
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

void unescape(char *p) {
	char *q = p;
	while(*p) {
		if(*p == '\\') {
			p++;
			switch(*p) {
				case '\\':
				case '"':
					*(q++) = *(p++);
					break;
				case 'a':
					*(q++) = '\a';
					p++;
					break;
				case 'b':
					*(q++) = '\b';
					p++;
					break;
				case 'f':
					*(q++) = '\f';
					p++;
					break;
				case 'r':
					*(q++) = '\r';
					p++;
					break;
				case 'n':
					*(q++) = '\n';
					p++;
					break;
				case 't':
					*(q++) = '\t';
					p++;
					break;
				case 'v':
					*(q++) = '\v';
					p++;
					break;
				case 'x':
				case 'X': {
					int a[2] = {-1, -1};
					char *t;
					for(t = p+1; *t>0 && t<=p+2; t++) {
						if(*t>='0' && *t<='9') {
							a[t-p-1] = *t - '0';
						} else if(*t>='a' && *t<='f') {
							a[t-p-1] = *t - 'a' + 10;
						} else if(*t>='A' && *t<='F') {
							a[t-p-1] = *t - 'A' + 10;
						} else {
							break;
						}
					}
					if(a[0] == -1 || a[1] == -1 || (a[0] == 0 && a[1] == 0)) {
						*(q++) = '\\';
						*(q++) =  *p;
					} else {
						*(q++) = ((a[0]<<4) | a[1]);
						p+=3;
					}
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7': {
					int a[3] = {-1, -1, -1};
					char *t;
					for(t = p; *t>0 && t<=p+2; t++) {
						if(*t>='0' && *t<='7') {
							a[t-p] = *t - '0';
						} else {
							break;
						}
					}
					if(a[0] == -1 || a[1] == -1 || a[2] == -1 || (a[0] == 0 && a[1] == 0 && a[2] == 0)) {
						*(q++) = '\\';
						*(q++) =  *p;
					} else {
						*(q++) = ((a[0]<<6) | (a[1]<<3) | a[2]);
						p+=3;
					}
					break;
				}
				default:
					*(q++) = '\\';
					*(q++) =  *p;
					break;
			}
		} else {
			*(q++) = *(p++);
		}
	}
	*q = '\0';
}

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
			break;
		}
	}
	
	src->type = LONG_T;
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
			break;
		}
	}
	
	src->type = FLOAT_T;
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
			break;
		}
	}
	
	src->type = DOUBLE_T;
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
#define CALC_ECHO_DEF(src,type,val,str) case type: printf("(%s)(" str ")", types[type],src->val);break
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
		case STR_T:
		#ifdef DEBUG
			printf("(str:%d)(\"", src->str->n);
		#endif
			fwrite(src->str->c, 1, src->str->n, stdout);
		#ifdef DEBUG
			printf("\")");
		#endif
			break;
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

void calc_func_def(func_def_f *def) {
	def->run = NULL;
	if(zend_hash_quick_add(&funcs, def->name->c, def->name->n, def->name->h, def, 0, NULL) == FAILURE) {
		ABORT(EXIT_CODE_FUNC_EXISTS, "The user function \"%s\" already exists.\n", def->names);
		return;
	}

	def->argc = 0;
	def->minArgc = 0;
	
	if(def->names != NULL) {
		unsigned errArgc = 0;
	
		dprintf("define function ");
		smart_string buf = {NULL, 0, 0};

		smart_string_appendl(&buf, def->name->c, def->name->n);
		smart_string_appendc(&buf, '(');
		if (def->args) {
			register func_args_t *args = def->args;
			while (args) {
				if(def->argc) {
					smart_string_appends(&buf, ", ");
				}
				smart_string_appendl(&buf, args->name->c, args->name->n);
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
			ABORT(EXIT_CODE_FUNC_ERR_ARG, "The user function %s the first %d argument not default value", def->names, errArgc + 1);
			return;
		}
		dprintf("\n");
	}

	def->filename = curFileName;
	def->lineno = linenofunc;

	linenofunc = 0;
	linenofuncname = NULL;
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
	if(def->run) return;
	
	free(def->names);

	//zend_hash_destroy(&def->frees);
}

void calc_run_variable(exp_val_t *expr) {
	exp_val_t *ptr = NULL;
	
	zend_hash_quick_find(&vars, expr->var->c, expr->var->n, expr->var->h, (void**)&ptr);
	if (EXPECTED(ptr!=NULL)) {
		expr->result = ptr;
	} else {
		CNEW01(expr->result, exp_val_t);
		zend_hash_quick_update(&vars, expr->var->c, expr->var->n, expr->var->h, expr->result, 0, NULL);
	}
}

// left + right
void calc_run_add(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);

	calc_free_expr(expr->result);

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
			expr->result->str->n = expr->defExp->left->result->str->n + expr->defExp->right->result->str->n;
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

	calc_free_expr(expr->result);
	
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

	calc_free_expr(expr->result);
	
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

	calc_free_expr(expr->result);

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

	calc_free_expr(expr->result);

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

	calc_free_expr(expr->result);

	expr->result->type = DOUBLE_T;
	expr->result->dval = pow(expr->defExp->left->result->dval, expr->defExp->right->result->dval);
}

// |ref|
void calc_run_abs(exp_val_t *expr) {
	calc_run_expr(expr->ref);
	
	str2num(expr->ref->result);

	calc_free_expr(expr->result);

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

	calc_free_expr(expr->result);

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

	if (EXPECTED(expr->defExp->cond->result->dval)) {
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

	calc_free_expr(expr->result);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval > expr->defExp->right->result->dval;
}

void calc_run_lt(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	calc_free_expr(expr->result);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval < expr->defExp->right->result->dval;
}

void calc_run_ge(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	calc_free_expr(expr->result);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval >= expr->defExp->right->result->dval;
}

void calc_run_le(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	calc_free_expr(expr->result);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval <= expr->defExp->right->result->dval;
}

void calc_run_eq(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	calc_free_expr(expr->result);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval == expr->defExp->right->result->dval;
}

void calc_run_ne(exp_val_t *expr) {
	calc_run_expr(expr->defExp->left);
	calc_run_expr(expr->defExp->right);
	
	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	calc_free_expr(expr->result);
	
	expr->result->type = INT_T;
	expr->result->ival = expr->defExp->left->result->dval != expr->defExp->right->result->dval;
}

void calc_run_array(exp_val_t *expr) {
	exp_val_t *ptr = NULL, *tmp;
	register call_args_t *args = expr->call->args;
	register int isref = 1;
	
	zend_hash_quick_find(&vars, expr->call->name->c, expr->call->name->n, expr->call->name->h, (void**)&ptr);
	if(UNEXPECTED(!ptr)) {
		yyerror("(warning) variable %s not exists, cannot read array or string value.\n", expr->call->name->c);
	} else if (UNEXPECTED(ptr->type == STR_T)) {
		str_array_access:
		EXPR_RESULT(expr);
		if(UNEXPECTED(args->next!=NULL)) {
			yyerror("An string of %s dimension bounds.\n", expr->call->name->c);
			return;
		}
		
		calc_run_expr(&args->val);
		calc_conv_to_int(args->val.result);
		if(UNEXPECTED(args->val.result->ival<0 || args->val.result->ival>=ptr->str->n)) {
			yyerror("An string of %s index out of bounds.\n", expr->call->name->c);
			return;
		}
		
		expr->result->type = STR_T;
		CNEW01(expr->result->str, string_t);
		expr->result->str->c = strndup(ptr->str->c+args->val.result->ival, 1);
		expr->result->str->n = 1;
	} else if (UNEXPECTED(ptr->type != ARRAY_T)) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", expr->call->name->c, types[ptr->type]);
	} else {
		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&args->val);
			calc_conv_to_int(args->val.result);
			if (UNEXPECTED(args->val.result->ival < 0)) {
				args->val.result->ival = -args->val.result->ival;
			}

			if (UNEXPECTED(args->val.result->ival >= ptr->arr->args[i])) {
				yyerror("An array of %s index out of bounds.\n", expr->call->name->c);
				EXPR_RESULT(expr);
				return;
			}
			ii += args->val.result->ival*n;
			n *= ptr->arr->args[i];
			if(UNEXPECTED(++i  == ptr->arr->dims)) {
				ptr = &ptr->arr->array[ii];
				if (UNEXPECTED(args->next!=NULL)) {
					if(ptr->type == STR_T) goto str_array_access;
					yyerror("An array of %s dimension bounds.\n", expr->call->name->c);
					EXPR_RESULT(expr);
				} else {
					expr->result = ptr;
				}
				return;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", expr->call->name->c);
		EXPR_RESULT(expr);
	}
}

void calc_run_func(exp_val_t *expr) {
	register unsigned int argc = 0;
	func_def_f *def = NULL;
	exp_val_t *ptr;
	register call_args_t *tmpArgs = NULL;
	register func_args_t *funcArgs;
	register func_symbol_t *syms;

	calc_free_expr(expr->result);

	if(linenostacktop+1 == sizeof(linenostack)/sizeof(linenostack_t)) {
		yyerror("stack overflow.\n");
		return;
	}
	
	zend_hash_quick_find(&funcs, expr->call->name->c, expr->call->name->n, expr->call->name->h, (void**)&def);
	if (def) {
		if(def->run) {
			def->run(expr);
			return;
		}
		
		tmpArgs = expr->call->args;
		while(tmpArgs) {
			argc++;
		
			tmpArgs = tmpArgs->next;
		}

		if (argc > def->argc || argc < def->minArgc) {
			yyerror("The custom function %s the number of parameters should be %d, at least %d, the actual %d.\n", expr->call->name->c, def->argc, def->minArgc, argc);
		}

		HashTable tmpVars = vars;
		funcArgs = def->args;

		zend_hash_init(&vars, 2, (dtor_func_t)calc_free_vars);

		#ifndef NO_FUNC_RUN_ARGS
			smart_string buf = {NULL, 0, 0};

			smart_string_appendl(&buf, def->name->c, def->name->n);
			smart_string_appendc(&buf, '(');

			argc = 0;
		#endif

		tmpArgs = expr->call->args;
		exp_val_t *p;
		while (funcArgs) {
			CNEW01(p, exp_val_t);
			
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
			zend_hash_quick_update(&vars, funcArgs->name->c, funcArgs->name->n, funcArgs->name->h, p, 0, NULL);
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

				zend_hash_quick_find(&tmpVars, tmpArgs->val.var->c, tmpArgs->val.var->n, tmpArgs->val.var->h, (void**)&ptr);
				if(ptr) {
					CNEW01(p, exp_val_t);
					memcpy_ref_expr(p, ptr);
					ptr = NULL;
					zend_hash_quick_update(&vars, tmpArgs->val.var->c, tmpArgs->val.var->n, tmpArgs->val.var->h, p, 0, (void**)&ptr);
					if(ptr) {
						yyerror("global variable \"%s\" and function parameter conflict.\n", tmpArgs->val.var->c);
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
			zend_hash_next_index_insert(&frees, buf.c, 0, NULL);
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

				zend_hash_quick_find(&vars, tmpArgs->val.var->c, tmpArgs->val.var->n, tmpArgs->val.var->h, (void**)&ptr);
				if(ptr) {
					CNEW01(p, exp_val_t);
					memcpy_ref_expr(p, ptr);
					zend_hash_quick_update(&tmpVars, tmpArgs->val.var->c, tmpArgs->val.var->n, tmpArgs->val.var->h, p, 0, NULL);
				}
				tmpArgs = tmpArgs->next;
			}

			syms = syms->next;
		}
		zend_hash_destroy(&vars);

		vars = tmpVars;

		linenostacktop--;
	} else {
		yyerror("undefined user function for %s.\n", expr->call->name->c);
	}
}

int apply_delete(void *ptr, int num_args, va_list args, zend_hash_key *hash_key) {
	HashTable *ht = va_arg(args, HashTable*);
	
	zend_hash_quick_del(ht, hash_key->arKey, hash_key->nKeyLength, hash_key->h);
}

void calc_run_sys_runfile_before(HashTable *ht) {
	zend_hash_copy(&files, ht, NULL, 1, 0);
}

void calc_run_sys_runfile(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function runfile(string str) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	
	if(args->val.result->type != STR_T) {
		yyerror("The system function runfile(string str) parameter not string type.\n");
	}
	
	func_symbol_t *tmpTopSyms = topSyms;
	//HashTable tmpTopFrees = topFrees;
	
	//zend_hash_init(&topFrees, 20, (dtor_func_t)free_frees);
	topSyms = NULL;
	
	int _isolate = isolate;
	isolate = 0;
	int ret = calc_runfile(expr->result, args->val.result->str->c, (top_syms_run_before_func_t)calc_run_sys_runfile_before, expr->call->ht);
	isolate = _isolate;
	
	zend_hash_apply_with_arguments(expr->call->ht, (apply_func_args_t)apply_delete, 1, &files);
	
	//zend_hash_destroy(&tmpTopFrees);
	//topFrees = tmpTopFrees;
	tmpTopSyms = topSyms;
	
	if(ret) {
		expr->result->type = STR_T;
		CNEW01(expr->result->str, string_t);
		expr->result->str->c = strndup(errmsg, errmsglen);
		expr->result->str->n = errmsglen;
	}
}

void calc_run_sys_sqrt(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function sqrt(double num) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function pow(double base, double exp) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function sin(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function asin(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function cos(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = cos(args->val.result->dval); // 余弦
}

void calc_run_sys_acos(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function acos(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = acos(args->val.result->dval); // 反余弦
}

void calc_run_sys_tan(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function tan(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = tan(args->val.result->dval); // 正切
}

void calc_run_sys_atan(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function atan(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = atan(args->val.result->dval); // 反正切
}

void calc_run_sys_ctan(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function ctan(double arg) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	calc_run_expr(&args->val);
	calc_conv_to_double(args->val.result);
	expr->result->type = DOUBLE_T;
	expr->result->dval = ctan(args->val.result->dval); // 余切
}

void calc_run_sys_rad(exp_val_t *expr) {
	register unsigned int argc = 0;
	call_args_t *args = expr->call->args;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function rad(double angle) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function rand() the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	expr->result->type = INT_T;
	expr->result->ival = rand();
}

void calc_run_sys_randf(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function randf() the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function strlen(string str) the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	expr->result->type = LONG_T;
	switch(args->val.type) {
		case STR_T:
			expr->result->lval = (long int) args->val.str->n;
			break;

		case VAR_T: {
			ptr = NULL;

			zend_hash_quick_find(&vars, args->val.var->c, args->val.var->n, args->val.var->h, (void**)&ptr);

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

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function microtime() the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	expr->result->type = DOUBLE_T;
	expr->result->dval = microtime();
}

void calc_run_sys_srand(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function srand() the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	srand((unsigned int) microtime());
}

extern int isExitStmt;
YY_BUFFER_STATE yy_current_buffer();
void calc_run_sys_passthru(exp_val_t *expr) {
	register unsigned int argc = 0;
	register call_args_t *tmpArgs = expr->call->args;

	calc_free_expr(expr->result);
	
	if(!isExitStmt || yyin==stdin) {
		return;
	}
	
	while(tmpArgs) {
		argc++;
		
		tmpArgs = tmpArgs->next;
	}

	if (expr->call->argc != argc) {
		yyerror("The system function passthru() the number of parameters should be %d, the actual %d.\n", expr->call->argc, argc);
		return;
	}
	
	struct stat buf;
	int ret = stat(curFileName, &buf);
	if(ret) {
		yyerror("The system function passthru() of stat(fd, struct stat *buf) return value is %d.\n", ret);
	} else {
		YY_BUFFER_STATE yybuf = yy_current_buffer();
		char *p = yybuf->yy_buf_pos;
		int n = yybuf->yy_n_chars - (p - yybuf->yy_ch_buf);
		int nfile = buf.st_size - ftell(yyin);
		DNEW01(str, string_t);
		CNEW(str->c, char, n+nfile+1);
		str->n = n+nfile;
		memcpy(str->c, p, n);
		
		*(str->c+str->n) = '\0';
		
		p = str->c+n;
		while(nfile>0 && (n=fread(p, 1, nfile, yyin))>0) {
			p+=n;
			nfile-=n;
		}
		
		expr->result->type = STR_T;
		expr->result->str = str;
	}
}

status_enum_t calc_run_sym_echo(exp_val_t *ret, func_symbol_t *syms) {
	register call_args_t *args = syms->args;
	
	while (args) {
		calc_run_expr(&args->val);
		calc_echo(args->val.result);
		args = args->next;
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_ret(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(syms->expr);
	
	memcpy_ref_expr(ret, syms->expr->result);
	
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
		if (UNEXPECTED(status != NONE_STATUS)) {
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
		if (UNEXPECTED(status != NONE_STATUS)) {
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

		if (UNEXPECTED(args->val.result->ival < 0)) {
			args->val.result->ival = -args->val.result->ival;
		}
		if(UNEXPECTED(args->val.result->ival == 0)) {
			yyerror("When the array %s is defined, the superscript is not equal to zero.\n", syms->var->var->c);
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
	
	zend_hash_quick_update(&vars, syms->var->var->c, syms->var->var->n, syms->var->var->h, ptr, 0, NULL);
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_null(exp_val_t *ret, func_symbol_t *syms) {
	return NONE_STATUS;
}

status_enum_t calc_run_sym_func(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_expr(syms->expr);
	
	return NONE_STATUS;
}

#define VAR_ACC() \
	if(EXPECTED(syms->type==ACC_STMT_T)) { \
		ptr->result = ptr; \
		ptr->run = NULL; \
		syms->val->defExp->left = ptr; \
		syms->val->result = ptr; \
		calc_run_expr(syms->val); \
	} else { \
		calc_run_expr(syms->val); \
		memcpy_ref_expr(ptr, syms->val->result); \
	}

status_enum_t calc_run_sym_variable_assign(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t *ptr = NULL;
	zend_hash_quick_find(&vars, syms->var->var->c, syms->var->var->n, syms->var->var->h, (void**)&ptr);
	if (UNEXPECTED(ptr==NULL)) {
		CNEW01(ptr, exp_val_t);
		ptr->type = INT_T;
		zend_hash_quick_update(&vars, syms->var->var->c, syms->var->var->n, syms->var->var->h, ptr, 0, NULL);
	}
	VAR_ACC();
	return NONE_STATUS;
}

status_enum_t calc_run_sym_array_assign(exp_val_t *ret, func_symbol_t *syms) {
	exp_val_t *ptr = NULL;

	zend_hash_quick_find(&vars, syms->var->call->name->c, syms->var->call->name->n, syms->var->call->name->h, (void**)&ptr);
	if (UNEXPECTED(!ptr || ptr->type != ARRAY_T)) {
		yyerror("(warning) variable %s not is a array, type is %s.\n", syms->var->call->name->c, types[ptr->type]);
	} else {
		register call_args_t *args = syms->var->call->args;

		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&args->val);
			calc_conv_to_int(args->val.result);
			if (UNEXPECTED(args->val.result->ival < 0)) {
				args->val.result->ival = -args->val.result->ival;
			}

			if (UNEXPECTED(args->val.result->ival >= ptr->arr->args[i])) {
				yyerror("An array of %s index out of bounds.\n", syms->var->call->name->c);
				return NONE_STATUS;
			}
			ii += args->val.result->ival*n;
			n *= ptr->arr->args[i];
			if(UNEXPECTED(++i == ptr->arr->dims)) {
				ptr = &ptr->arr->array[ii];
				if (UNEXPECTED(args->next!=NULL)) {
					yyerror("An array of %s dimension bounds.\n", syms->var->call->name->c);
				} else {
					VAR_ACC();
				}
				
				return NONE_STATUS;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", syms->var->call->name->c);
	}
	
	return NONE_STATUS;
}

status_enum_t calc_run_sym_for(exp_val_t *ret, func_symbol_t *syms) {
	calc_run_syms(ret, syms->lsyms);
	
	if(UNEXPECTED(syms->cond->type == NULL_T)) {
		syms->cond->result->dval = 1;
	} else {
		calc_run_expr(syms->cond);
		calc_conv_to_double(syms->cond->result);
	}

	status_enum_t status;
	while (syms->cond->result->dval) {
		status = calc_run_syms(ret, syms->forSyms);
		if (UNEXPECTED(status != NONE_STATUS)) {
			return status == RET_STATUS ? RET_STATUS : NONE_STATUS;
		}
		
		calc_run_syms(ret, syms->rsyms);

		if(EXPECTED(syms->cond->type != NULL_T)) {
			calc_run_expr(syms->cond);
			calc_conv_to_double(syms->cond->result);
		}
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
	if(EXPECTED(syms==NULL)) {
		return NONE_STATUS;
	}

	if(UNEXPECTED(linenostacktop+1 == sizeof(linenostack)/sizeof(linenostack_t))) {
		yyerror("stack overflow.\n");
		return NONE_STATUS;
	}

	linenostack[++linenostacktop].funcname = NULL;

	register status_enum_t status;
	while (syms) {
		linenostack[linenostacktop].syms = syms;
		linenostack[linenostacktop].lineno = syms->lineno;
		linenostack[linenostacktop].filename = syms->filename;
		
		status = syms->run(ret, syms);
		if(EXPECTED(status != NONE_STATUS)) {
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
				expr->type = NULL_T;
			}
			break;
		}
		case PTR_T: {
			dprintf("--- FreeVars: PTR_T(%u) ---\n", expr->arr->gc);
			if(!(expr->ptr->gc--)) {
				expr->ptr->dtor(expr->ptr);
				expr->type = NULL_T;
			}
			break;
		}
		case STR_T: {
			dprintf("--- FreeVars: STR_T(%u) ---\n", expr->str->gc);
			if(expr->str->gc<=0) {
				expr->type = NULL_T;
			}
			free_str(expr->str);
			break;
		}
		default: {
			expr->type = NULL_T;
			break;
		}
	}
}
void calc_free_vars(exp_val_t *expr) {
	calc_free_expr(expr);
	free(expr);
}

void append_pool(void *ptr, dtor_func_t run) {
	pool_t p = {ptr,run};

	zend_hash_next_index_insert(&pools, &p, sizeof(pool_t), NULL);
}

void zend_hash_destroy_ptr(HashTable *ht) {
	zend_hash_destroy(ht);
	free(ht);
}
int apply_funcs(func_def_f *def) {
	return def->run ? ZEND_HASH_APPLY_KEEP : ZEND_HASH_APPLY_REMOVE ;
}
int calc_runfile(exp_val_t *expr, char *filename, top_syms_run_before_func_t before, void *ptr) {
	int yret, ret = 0;
	FILE *fp = stdin;
	char filepath[1024] = "";
	
	if(strcmp(filename, "-")) {
		if(realpath(filename, filepath)) {
			filename = strdup(filepath);
			
			zend_hash_next_index_insert(&frees, filename, 0, NULL);
			
			fp = fopen(filename, "r");
		} else {
			yyerror("File \"%s\" not found!\n", filepath);
			return 1;
		}
	}

	if(zend_hash_add(&files, filename, strlen(filename), NULL, 0, NULL)!=SUCCESS) {
		if(fp != stdin) {
			fclose(fp);
		}
		yyerror("File \"%s\" already included!\n", filename);
		return 1;
	}
	
	dprintf("==========================\n");
	dprintf("BEGIN INPUT: %s\n", filename);
	dprintf("--------------------------\n");
	curFileName = filename;
	yylineno = 1;

	yyrestart(fp);
	
	frees.pDestructor = NULL;
	while((yret=yyparse()) && isSyntaxData) {
		ret |= yret;
		
		if(yywrap()) {
			break;
		}
	}
	
	if(isSyntaxData) {
		memset(expr, 0, sizeof(exp_val_t));
		if(before) {
			before(ptr);
		}
		calc_run_syms(expr, topSyms);
		zend_hash_clean(&topFrees);
		topSyms = NULL;
		if(isolate) {
			zend_hash_clean(&vars);
			zend_hash_apply(&funcs, (apply_func_t)apply_funcs);
		}
	}
	while(yret && !yywrap()) {}
	frees.pDestructor = (dtor_func_t)free_frees;
	
	if(fp != stdin) {
		fclose(fp);
	}
	
	return yret | ret;
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

		if(!strcmp(linenostack[i].filename, "-")) {
			continue;
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
	errmsglen = vfprintf(stderr, s, ap);
	va_end(ap);

	if(errmsg) {
		free(errmsg);
	}
	CNEW(errmsg, char, errmsglen+1);
	va_start(ap, s);
	vsprintf(errmsg, s, ap);
	errmsglen--;
	while(errmsg[errmsglen] == '\n' || errmsg[errmsglen] == '\n') {
		errmsg[errmsglen] = '\0';
		errmsglen--;
	}
	errmsglen++;
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

void free_pools(pool_t *p) {
	p->run(p->ptr);
}

void ext_funcs();

void calc_init() {
	zend_hash_init(&files, 2, NULL);
	zend_hash_init(&frees, 20, NULL);
	zend_hash_init(&topFrees, 20, (dtor_func_t)free_frees);
	zend_hash_init(&vars, 20, (dtor_func_t)calc_free_vars);
	zend_hash_init(&funcs, 20, (dtor_func_t)calc_free_func);
	zend_hash_init(&pools, 2, (dtor_func_t)free_pools);
	zend_hash_init(&results, 20, (dtor_func_t)calc_free_vars);
	zend_hash_init(&consts, 20, (dtor_func_t)calc_free_vars);
	
	ext_funcs();
}

void calc_destroy() {
	yylex_destroy();

	zend_hash_destroy(&topFrees);
	zend_hash_destroy(&files);
	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);
	zend_hash_destroy(&pools);
	zend_hash_destroy(&results);
	zend_hash_destroy(&frees);
	zend_hash_destroy(&consts);
	
	if(errmsg) {
		free(errmsg);
	}
}
