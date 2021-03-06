#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <complex.h>
#include "calc.h"
#include "parser.h"
#include "scanner.h"

HashTable pools;
HashTable vars;
HashTable funcs;
HashTable files;
HashTable frees;
HashTable results;
HashTable consts;
HashTable runfiles;
func_symbol_t *topSyms = NULL;
int isSyntaxData = 1;
int isolate = 1;
int exitCode = 0;
char *errmsg = NULL;
int errmsglen = 0;

linenostack_t linenostack[1024]={{0,"TOP",NULL}};
int linenostacktop = 0;

char *types[] = { "NULL", "int", "long", "float", "double", "str", "array", "map", "ptr" };

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
			src->fval = src->lval;
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
void calc_conv_to_hashtable(exp_val_t *src) {
	if(src->type != MAP_T) {
		DNEW1(map, map_t);
		map->gc = 0;
		zend_hash_init(&map->ht, 2, (dtor_func_t)calc_free_vars);
		
		DNEW1(ptr, exp_val_t);
		memcpy(ptr, src, sizeof(exp_val_t));
		zend_hash_next_index_insert(&map->ht, ptr, 0, NULL);
		
		src->type = MAP_T;
		src->map = map;
	}
}

void calc_array_sprintf(smart_string *buf, array_t *arr, unsigned int n, unsigned int ii, int dim, unsigned int deep) {
	register int i, hasNext = dim+1<arr->dims;
	register unsigned int dims = arr->args[arr->dims-1-dim], nii;

#ifdef HAVE_EXPR_TYPE
	for(i = 0; i<deep*4; i++) {
		smart_string_appendc(buf, ' ');
	}
	smart_string_appends(buf, "array(");
	smart_string_append_unsigned(buf, dims);
	smart_string_appends(buf, ") ");
#endif
	smart_string_appendc(buf, '[');
#ifdef HAVE_EXPR_TYPE
	if(hasNext) {
		smart_string_appendc(buf, '\n');
	}
#endif

	for (i = 0; i < dims; i++) {
		nii = ii+i*n;
		if(hasNext) {
			calc_array_sprintf(buf, arr, n*dims, nii, dim+1, deep+1);
			if(i+1 < dims) {
				smart_string_appendc(buf, ',');
			}
		#ifdef HAVE_EXPR_TYPE
			smart_string_appendc(buf, '\n');
		#endif
		} else {
			if(i) {
				smart_string_appends(buf, ", ");
			}
			calc_sprintf_ex(buf, &arr->array[nii], deep+1);
		}
	}
#ifdef HAVE_EXPR_TYPE
	if(hasNext) {
		for(i = 0; i<deep*4; i++) {
			smart_string_appendc(buf, ' ');
		}
	}
#endif
	smart_string_appendc(buf, ']');
}

int calc_map_apply_sprintf(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key) {
	smart_string *buf = va_arg(args, smart_string*);
	unsigned int deep = va_arg(args, unsigned int);
	register int i;

#ifdef HAVE_EXPR_TYPE
	for(i = 0; i<deep*4; i++) {
		smart_string_appendc(buf, ' ');
	}
#endif
	if(hash_key->nKeyLength > 0) {
		smart_string_appendc(buf, '"');
		smart_string_appendl(buf, hash_key->arKey, hash_key->nKeyLength);
		smart_string_appendc(buf, '"');
	} else {
		smart_string_append_unsigned(buf, hash_key->h);
	}
	
	smart_string_appendc(buf, ':');
	calc_sprintf_ex(buf, val, deep+1);
	smart_string_appendc(buf, ',');
	return ZEND_HASH_APPLY_KEEP;
}

void calc_map_sprintf(smart_string *buf, HashTable *ht, unsigned int deep) {
	register int i, hasNext = zend_hash_num_elements(ht);
#ifdef HAVE_EXPR_TYPE
	if(hasNext) {
		for(i = 0; i<deep*4; i++) {
			smart_string_appendc(buf, ' ');
		}
	}
#endif
	smart_string_appendc(buf, '{');
	if(hasNext) {
		zend_hash_apply_with_arguments(ht, (apply_func_args_t)calc_map_apply_sprintf, 2, buf, deep+1);
		buf->len--;
	}
#ifdef HAVE_EXPR_TYPE
	if(hasNext) {
		for(i = 0; i<deep*4; i++) {
			smart_string_appendc(buf, ' ');
		}
	}
#endif
	smart_string_appendc(buf, '}');
}

void calc_sprintf_ex(smart_string *buf, exp_val_t *src, unsigned int deep) {
	char s[36] = "";
	int len;

	switch (src->type) {
		case NULL_T:
			smart_string_appends(buf, "NULL");
			break;
		case INT_T:
		#ifdef HAVE_EXPR_TYPE
			smart_string_appends(buf, "int(");
		#endif
			len = sprintf(s, "%d", src->ival);
			smart_string_appendl(buf, s, len);
		#ifdef HAVE_EXPR_TYPE
			smart_string_appendc(buf, ')');
		#endif
			break;
		case LONG_T:
		#ifdef HAVE_EXPR_TYPE
			smart_string_appends(buf, "long(");
		#endif
			len = sprintf(s, "%ld", src->lval);
			smart_string_appendl(buf, s, len);
		#ifdef HAVE_EXPR_TYPE
			smart_string_appendc(buf, ')');
		#endif
			break;
		case FLOAT_T:
		#ifdef HAVE_EXPR_TYPE
			smart_string_appends(buf, "float(");
		#endif
			len = sprintf(s, "%g", src->fval);
			if(strchr(s, '.') == NULL) {
				strcat(s, ".0");
				len += 2;
			}
			smart_string_appendl(buf, s, len);
		#ifdef HAVE_EXPR_TYPE
			smart_string_appendc(buf, ')');
		#endif
			break;
		case DOUBLE_T:
		#ifdef HAVE_EXPR_TYPE
			smart_string_appends(buf, "double(");
		#endif
			len = sprintf(s, "%g", src->dval);
			if(strchr(s, '.') == NULL) {
				strcat(s, ".0");
				len += 2;
			}
			smart_string_appendl(buf, s, len);
		#ifdef HAVE_EXPR_TYPE
			smart_string_appendc(buf, ')');
		#endif
			break;
		case STR_T:
		#ifdef HAVE_EXPR_TYPE
			smart_string_appends(buf, "str(");
			smart_string_append_unsigned(buf, src->str->n);
			smart_string_appends(buf, ") \"");
		#endif
			smart_string_appendl(buf, src->str->c, src->str->n);
		#ifdef HAVE_EXPR_TYPE
			smart_string_appendc(buf, '"');
		#endif
			break;
		case ARRAY_T: {
			calc_array_sprintf(buf, src->arr, 1, 0, 0, deep+1);
			break;
		}
		case MAP_T: {
			calc_map_sprintf(buf, &src->map->ht, deep+1);
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

int calc_list_funcs(func_def_f *def) {
	printf("\x1b[34m  %6s function %s\x1b[0m\n", def->run?"system":"user", def->names);

	return ZEND_HASH_APPLY_KEEP;
}

int calc_clear_or_list_vars(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key) {
	int result = va_arg(args, int);
	char *key = strndup(hash_key->arKey, hash_key->nKeyLength);

	printf("\x1b[34m  (%6s) %s = ", types[val->type], key);
	free(key);

	if(val->type == ARRAY_T) {
		printf("array(%d, [%d", val->arr->arrlen, val->arr->args[0]);
		int i;
		for(i=1; i<val->arr->dims; i++) {
			printf(", %d", val->arr->args[i]);
		}
		printf("]) ");
	} else if(val->type == STR_T) {
		printf("str(%d) \"", val->str->n);
	}
	calc_echo(val);
	if(val->type == STR_T) {
		printf("\"");
	}
	printf("\n");
	printf("\x1b[0m");
	return result;
}

void calc_func_def(func_def_f *def) {
	def->run = NULL;
	if(zend_hash_quick_add(&funcs, def->name->c, def->name->n, def->name->h, def, 0, NULL) == FAILURE) {
		ABORT(EXIT_CODE_FUNC_EXISTS, "The user function %s() already exists.\n", def->name->c);
		return;
	}

	def->argc = 0;
	def->minArgc = 0;
	
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
	dprintf("\n");

	def->filename = curFileName;
	def->lineno = linenofunc;
	def->varlen = 0;

	register func_symbol_t *syms = def->syms;
	register call_args_t *tmpArgs = NULL;

	while(syms) {
		if(syms->type != GLOBAL_STMT_T) {
			syms = syms->next;
			continue;
		}

		tmpArgs = syms->args;
		while(tmpArgs) {
			def->varlen++;

			tmpArgs = tmpArgs->next;
		}

		syms = syms->next;
	}

	register unsigned int i = 0;
	if(def->varlen) {
		CNEW(def->vars, var_t, def->varlen);
		syms = def->syms;

		while(syms) {
			if(syms->type != GLOBAL_STMT_T) {
				syms = syms->next;
				continue;
			}

			tmpArgs = syms->args;
			while(tmpArgs) {
				def->vars[i++] = *tmpArgs->val.var;

				tmpArgs = tmpArgs->next;
			}

			syms = syms->next;
		}
	} else {
		def->vars = NULL;
	}

	linenofunc = 0;
	linenofuncname = NULL;

	if (errArgc) {
		ABORT(EXIT_CODE_FUNC_ERR_ARG, "The user function %s the first %d argument not default value", def->names, errArgc + 1);
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
	if(def->run) return;
	
	free(def->names);

	if(def->vars) free(def->vars);

	//zend_hash_destroy(&def->frees);
}

void calc_run_variable(exp_val_t *expr) {
	exp_val_t *ptr = NULL;
	
	zend_hash_quick_find(&vars, expr->var->c, expr->var->n, expr->var->h, (void**)&ptr);
	if (EXPECTED(ptr!=NULL)) {
		memcpy_ref_expr(expr->result, ptr);
	} else {
		calc_free_expr(expr->result);
		CNEW01(ptr, exp_val_t);
		zend_hash_quick_update(&vars, expr->var->c, expr->var->n, expr->var->h, ptr, 0, NULL);
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
	
	type_enum_t type = max(expr->defExp->left->result->type, expr->defExp->right->result->type);

	CALC_CONV_op(expr->defExp->left->result, expr->defExp->right->result, double);

	calc_free_expr(expr->result);

	if(type>LONG_T) {
		expr->result->type = DOUBLE_T;
		expr->result->dval = pow(expr->defExp->left->result->dval, expr->defExp->right->result->dval);
	} else {
		expr->result->type = LONG_T;
		expr->result->lval = (long int) pow(expr->defExp->left->result->dval, expr->defExp->right->result->dval);
	}
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
	exp_val_t *ptr = NULL, *tmpPtr;
	register call_args_t *args = expr->call->args;
	register int isref = 1;
	
	calc_free_expr(expr->result);

	zend_hash_quick_find(&vars, expr->call->name->c, expr->call->name->n, expr->call->name->h, (void**)&ptr);
	
array_access:
	if(UNEXPECTED(!ptr)) {
		yyerror("(warning) variable %s not exists, cannot read array or string value.\n", expr->call->name->c);
	} else if (UNEXPECTED(ptr->type == STR_T)) {
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
	} else if (UNEXPECTED(ptr->type == ARRAY_T)) {
		unsigned int i = 0, n = 1, ii = 0;
		while (args) {
			calc_run_expr(&args->val);
			calc_conv_to_int(args->val.result);
			if (UNEXPECTED(args->val.result->ival < 0)) {
				args->val.result->ival = -args->val.result->ival;
			}

			if (UNEXPECTED(args->val.result->ival >= ptr->arr->args[i])) {
				yyerror("An array of %s index out of bounds.\n", expr->call->name->c);
				return;
			}
			ii += args->val.result->ival*n;
			n *= ptr->arr->args[i];
			if(UNEXPECTED(++i  == ptr->arr->dims)) {
				ptr = &ptr->arr->array[ii];
				if (UNEXPECTED(args->next!=NULL)) {
					args = args->next;
					goto array_access;
				} else {
					memcpy_ref_expr(expr->result, ptr);
				}
				return;
			}

			args = args->next;
		}

		yyerror("Array %s dimension deficiency.\n", expr->call->name->c);
	} else if (UNEXPECTED(ptr->type == MAP_T)) {
		tmpPtr = NULL;
		
		calc_run_expr(&args->val);
		calc_conv_to_str(args->val.result);
		
		zend_symtable_find(&ptr->map->ht, args->val.result->str->c, args->val.result->str->n, (void**)&tmpPtr);
		
		if(tmpPtr) {
			if(args->next) {
				ptr = tmpPtr;
				args = args->next;
				goto array_access;
			} else {
				memcpy_ref_expr(expr->result, tmpPtr);
			}
		}
	} else {
		yyerror("(warning) variable %s not is a array or map(hash table), type is %s.\n", expr->call->name->c, types[ptr->type]);
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
			return;
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

		for(argc=0; argc<def->varlen; argc++) {
			ptr = NULL;

			zend_hash_quick_find(&tmpVars, def->vars[argc].c, def->vars[argc].n, def->vars[argc].h, (void**)&ptr);
			if(ptr) {
				CNEW01(p, exp_val_t);
				memcpy_ref_expr(p, ptr);
				if(zend_hash_quick_exists(&vars, def->vars[argc].c, def->vars[argc].n, def->vars[argc].h)) {
					yyerror("global variable \"%s\" and function %s parameter conflict.\n", def->vars[argc].c,
					#ifndef NO_FUNC_RUN_ARGS
						buf.c
					#else
						def->names
					#endif
						);
				} else {
					zend_hash_quick_update(&vars, def->vars[argc].c, def->vars[argc].n, def->vars[argc].h, p, 0, NULL);
				}
			}
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

		for(argc=0; argc<def->varlen; argc++) {
			ptr = NULL;

			zend_hash_quick_find(&vars, def->vars[argc].c, def->vars[argc].n, def->vars[argc].h, (void**)&ptr);
			if(ptr) {
				CNEW01(p, exp_val_t);
				memcpy_ref_expr(p, ptr);
				zend_hash_quick_update(&tmpVars, def->vars[argc].c, def->vars[argc].n, def->vars[argc].h, p, 0, NULL);
			}
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

	return ZEND_HASH_APPLY_KEEP;
}

extern int isRunSyms;

typedef struct {
	string_t *str;
	HashTable *ht;
} runfile_before_t;
void calc_run_sys_runfile_before(runfile_before_t *rb) {
	zend_hash_copy(&files, rb->ht, NULL, 1, 0);
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

	runfile_before_t rb = {
		args->val.result->str,
		expr->call->ht
	};

	func_symbol_t *_topSyms = topSyms;
	int _isolate = isolate;
	int _isRunSyms = isRunSyms;
	isolate = 0;
	isRunSyms = 0;
	topSyms = NULL;
	int ret = calc_runfile(expr->result, rb.str->c, (top_syms_run_before_func_t)calc_run_sys_runfile_before, &rb);
	topSyms = _topSyms;
	isolate = _isolate;
	isRunSyms = _isRunSyms;
	
	zend_hash_apply_with_arguments(rb.ht, (apply_func_args_t)apply_delete, 1, &files);
	
	if(ret && expr->result->type == NULL_T) {
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
	
	type_enum_t type = max(args->val.result->type, args->next->val.result->type);

	CALC_CONV_op(args->val.result, args->next->val.result, double);

	if(type>LONG_T) {
		expr->result->type = DOUBLE_T;
		expr->result->dval = pow(args->val.result->dval, args->next->val.result->dval);
	} else {
		expr->result->type = LONG_T;
		expr->result->lval = (long int) pow(args->val.result->dval, args->next->val.result->dval);
	}
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

	expr->type = NULL_T;
	expr->run = NULL;
	
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
		
		expr->type = STR_T;
		expr->str = str;
		expr->run = NULL;
		expr->result->type = STR_T;
		expr->result->str = str;
	}
}

status_enum_t calc_run_sym_echo(exp_val_t *ret, func_symbol_t *syms) {
	register call_args_t *args = syms->args;
	smart_string buf = {NULL,0,0};
	while (args) {
		calc_run_expr(&args->val);
		calc_sprintf(&buf, args->val.result);
		args = args->next;
	}
	if(buf.c) {
		fwrite(buf.c, 1, buf.len, stdout);
		free(buf.c);
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

#define LIST_STMT(info, funcname, lineno, t) printf(info, funcname, lineno);zend_hash_apply_with_arguments(&vars, (apply_func_args_t)calc_clear_or_list_vars, 1, t)

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
		syms->val->defExp->left->run = NULL; \
		memcpy_ref_expr(syms->val->defExp->left->result, ptr); \
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
	exp_val_t *ptr = NULL, *tmpPtr;
	register call_args_t *args = syms->var->call->args;

	zend_hash_quick_find(&vars, syms->var->call->name->c, syms->var->call->name->n, syms->var->call->name->h, (void**)&ptr);

	if (UNEXPECTED(!ptr)) {
		CNEW1(ptr, exp_val_t);
		CNEW1(ptr->map, map_t);
		ptr->type = MAP_T;
		ptr->map->gc = 0;
		zend_hash_init(&ptr->map->ht, 2, (dtor_func_t)calc_free_vars);
		zend_hash_quick_update(&vars, syms->var->call->name->c, syms->var->call->name->n, syms->var->call->name->h, ptr, 0, NULL);
		goto array_assign_map;
	}
array_assign:
	if(ptr->type == ARRAY_T) {
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
	} else if(ptr->type == MAP_T) {
	array_assign_map:
		tmpPtr = NULL;
		
		calc_run_expr(&args->val);
		calc_conv_to_str(args->val.result);
		
	#define ARRAY_ASSIGN_MAP \
		zend_hash_index_find(&ptr->map->ht, idx, (void**)&tmpPtr); \
		if(!tmpPtr) { \
			CNEW01(tmpPtr, exp_val_t); \
			zend_hash_index_update(&ptr->map->ht, idx, tmpPtr, 0, NULL); \
		} \
		ptr = tmpPtr; \
		if(args->next) { \
			args = args->next; \
			goto array_assign; \
		} else { \
			VAR_ACC(); \
			return NONE_STATUS; \
		}
		
		do {
			ulong idx;
			ZEND_HANDLE_NUMERIC_EX(args->val.result->str->c, args->val.result->str->n+1, idx, ARRAY_ASSIGN_MAP);

			idx = zend_hash_func(args->val.result->str->c, args->val.result->str->n);
			zend_hash_quick_find(&ptr->map->ht, args->val.result->str->c, args->val.result->str->n, idx, (void**)&tmpPtr);
			if(!tmpPtr) {
				CNEW01(tmpPtr, exp_val_t);
				zend_hash_quick_update(&ptr->map->ht, args->val.result->str->c, args->val.result->str->n, idx, tmpPtr, 0, NULL);
			}
			ptr = tmpPtr;
			if(args->next) {
				args = args->next;
				goto array_assign;
			} else {
				VAR_ACC();
			}
		} while(0);
	#undef ARRAY_ASSIGN_MAP
	} else {
		calc_conv_to_hashtable(ptr);
		goto array_assign_map;
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
			}
			break;
		}
		case MAP_T: {
			dprintf("--- FreeVars: MAP_T(%u) ---\n", expr->map->gc);
			if(!(expr->map->gc--)) {
				zend_hash_destroy(&expr->map->ht);
				free(expr->map);
			}
			break;
		}
		case PTR_T: {
			dprintf("--- FreeVars: PTR_T(%u) ---\n", expr->arr->gc);
			if(!(expr->ptr->gc--)) {
				expr->ptr->dtor(expr->ptr);
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

	expr->type = NULL_T;
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
	FILE *fp = stdin;
	char filepath[1024] = "";
	
	if(strcmp(filename, "-")) {
		if(realpath(filename, filepath)) {
			char *cwd = getcwd(NULL, 0);
			unsigned int cwdlen = strlen(cwd);
			if(cwdlen>0 && !strncmp(filepath, cwd, cwdlen) && filepath[cwdlen]=='/') {
				filepath[cwdlen-1] = '.';
				filename = strdup(filepath+cwdlen-1);
			} else {
				filename = strdup(filepath);
			}
			free(cwd);
			
			zend_hash_next_index_insert(&frees, filename, 0, NULL);
			
			fp = fopen(filename, "r");
		} else {
			yyerror("File \"%s\" not found!\n", filepath);
			return 1;
		}
	}

	var_t var = {filename, strlen(filename)};
	var.h = zend_hash_func(var.c, var.n);

	if(zend_hash_quick_add(&files, var.c, var.n, var.h, NULL, 0, NULL)!=SUCCESS) {
		if(fp != stdin) {
			fclose(fp);
		}
		yyerror("File \"%s\" already included!\n", filename);
		return 1;
	}
	int ret = 0;

	topSyms = NULL;
	if(isSyntaxData && zend_hash_quick_find(&runfiles, var.c, var.n, var.h, (void**)&topSyms) == SUCCESS) {
		memset(expr, 0, sizeof(exp_val_t));
		if(before) {
			before(ptr);
		}
		calc_run_syms(expr, topSyms);
		topSyms = NULL;
	} else {
		dprintf("==========================\n");
		dprintf("BEGIN INPUT: %s\n", filename);
		dprintf("--------------------------\n");
		curFileName = filename;
		yylineno = 1;

		yyrestart(fp);
		
		frees.pDestructor = NULL;

		int ret = yyparse();
		
		if(!ret && isSyntaxData) {
			memset(expr, 0, sizeof(exp_val_t));
			if(before) {
				before(ptr);
			}
			zend_hash_quick_add(&runfiles, var.c, var.n, var.h, topSyms, 0, NULL);
			calc_run_syms(expr, topSyms);
			topSyms = NULL;
			if(isolate) {
				zend_hash_clean(&vars);
				zend_hash_apply(&funcs, (apply_func_t)apply_funcs);
			}
		}
		while(ret && !yywrap()) {}
		frees.pDestructor = (dtor_func_t)free_frees;
	}

	zend_hash_quick_del(&files, var.c, var.n, var.h);
	if(fp != stdin) {
		fclose(fp);
	}
	
	return ret;
}

void yyerror(const char *s, ...) {
	fprintf(stderr, "\x1b[31m");
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "Then error: in file \"%s\" on line %d: \n", curFileName, yylineno);

	register int i, n, len, lineno;
	char buf[256];
	char linenoformat[8];
	FILE *fp;
	char *cwd = getcwd(NULL, 0);
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
	free(cwd);
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
void ext_consts();

void calc_init() {
	zend_hash_init(&files, 2, NULL);
	zend_hash_init(&frees, 20, NULL);
	zend_hash_init(&vars, 20, (dtor_func_t)calc_free_vars);
	zend_hash_init(&funcs, 20, (dtor_func_t)calc_free_func);
	zend_hash_init(&pools, 2, (dtor_func_t)free_pools);
	zend_hash_init(&results, 20, (dtor_func_t)calc_free_vars);
	zend_hash_init(&consts, 20, NULL);
	zend_hash_init(&runfiles, 2, NULL);
	
	ext_consts();
	ext_funcs();
}

void calc_destroy() {
	yylex_destroy();

	zend_hash_destroy(&files);
	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);
	zend_hash_destroy(&pools);
	zend_hash_destroy(&results);
	zend_hash_destroy(&frees);
	zend_hash_destroy(&consts);
	zend_hash_destroy(&runfiles);
	
	if(errmsg) {
		free(errmsg);
	}
}
