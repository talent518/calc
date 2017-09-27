#include "calc.h"

HashTable vars;
HashTable funcs;

typedef struct _linenostack {
	unsigned int lineno;
	char *funcname;
} linenostack_t;

static linenostack_t linenostack[1024]={0};
static int linenostacktop = -1;

char *types[] = { "int", "long", "float", "double" };

// dst = (double)(src)
#define CALC_CONV(dst,src,val) \
	switch(src->type) { \
		case INT_T: dst->val=src->ival;break; \
		case LONG_T: dst->val=src->lval;break; \
		case FLOAT_T: dst->val=src->fval;break; \
		case DOUBLE_T: dst->val=src->dval;break; \
	}

// dst = op1 + op2
void calc_add(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	switch (op1->type) {
		case INT_T:
			switch (op2->type) {
				case INT_T:
					dst->type = INT_T;
					dst->lval = op1->ival + op2->ival;
					break;
				case LONG_T:
					dst->type = LONG_T;
					dst->lval = op1->ival + op2->lval;
					break;
				case FLOAT_T:
					dst->type = FLOAT_T;
					dst->fval = op1->ival + op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->ival + op2->dval;
					break;
			}
			break;
		case LONG_T:
			switch (op2->type) {
				case INT_T:
					dst->type = LONG_T;
					dst->lval = op1->lval + op2->ival;
					break;
				case LONG_T:
					dst->type = LONG_T;
					dst->lval = op1->lval + op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->lval + op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->lval + op2->dval;
					break;
			}
			break;
		case FLOAT_T:
			switch (op2->type) {
				case INT_T:
					dst->type = FLOAT_T;
					dst->dval = op1->fval + op2->ival;
					break;
				case LONG_T:
					dst->type = DOUBLE_T;
					dst->fval = op1->fval + op2->lval;
					break;
				case FLOAT_T:
					dst->type = FLOAT_T;
					dst->dval = op1->fval + op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->fval + op2->dval;
					break;
			}
			break;
		case DOUBLE_T:
			switch (op2->type) {
				case INT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval + op2->ival;
					break;
				case LONG_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval + op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval + op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval + op2->dval;
					break;
			}
			break;
	}
}

// dst = op1 - op2
void calc_sub(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	switch (op1->type) {
		case INT_T:
			switch (op2->type) {
				case INT_T:
					dst->type = INT_T;
					dst->ival = op1->ival - op2->ival;
					break;
				case LONG_T:
					dst->type = LONG_T;
					dst->lval = op1->ival - op2->lval;
					break;
				case FLOAT_T:
					dst->type = FLOAT_T;
					dst->dval = op1->ival - op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->ival - op2->dval;
					break;
			}
			break;
		case LONG_T:
			switch (op2->type) {
				case INT_T:
					dst->type = LONG_T;
					dst->lval = op1->lval - op2->ival;
					break;
				case LONG_T:
					dst->type = LONG_T;
					dst->lval = op1->lval - op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->lval - op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->lval - op2->dval;
					break;
			}
			break;
		case FLOAT_T:
			switch (op2->type) {
				case INT_T:
					dst->type = FLOAT_T;
					dst->dval = op1->fval - op2->ival;
					break;
				case LONG_T:
					dst->type = DOUBLE_T;
					dst->fval = op1->fval - op2->lval;
					break;
				case FLOAT_T:
					dst->type = FLOAT_T;
					dst->fval = op1->fval - op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->fval - op2->dval;
					break;
			}
			break;
		case DOUBLE_T:
			switch (op2->type) {
				case INT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval - op2->ival;
					break;
				case LONG_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval - op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval - op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval - op2->dval;
					break;
			}
			break;
	}
}

// dst = op1 * op2
void calc_mul(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	switch (op1->type) {
		case INT_T:
			switch (op2->type) {
				case INT_T:
					dst->type = INT_T;
					dst->ival = op1->ival * op2->ival;
					break;
				case LONG_T:
					dst->type = LONG_T;
					dst->lval = op1->ival * op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->ival * (double) op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->ival * (double) op2->dval;
					break;
			}
			break;
		case LONG_T:
			switch (op2->type) {
				case INT_T:
					dst->type = LONG_T;
					dst->lval = op1->lval * op2->ival;
					break;
				case LONG_T:
					dst->type = LONG_T;
					dst->lval = op1->lval * op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->lval * (double) op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->lval * op2->dval;
					break;
			}
			break;
		case FLOAT_T:
			switch (op2->type) {
				case INT_T:
					dst->type = FLOAT_T;
					dst->fval = (double) op1->fval * (double) op2->ival;
					break;
				case LONG_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->fval * (double) op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->fval * (double) op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = (double) op1->fval * op2->dval;
					break;
			}
			break;
		case DOUBLE_T:
			switch (op2->type) {
				case INT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval * (double) op2->ival;
					break;
				case LONG_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval * (double) op2->lval;
					break;
				case FLOAT_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval * (double) op2->fval;
					break;
				case DOUBLE_T:
					dst->type = DOUBLE_T;
					dst->dval = op1->dval * op2->dval;
					break;
			}
			break;
	}
}

// dst = op1 / op2
void calc_div(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV(op1, op1, dval);
	CALC_CONV(op2, op2, dval);
	op1->type = DOUBLE_T;
	op2->type = DOUBLE_T;

	dst->type = DOUBLE_T;
	dst->dval = op1->dval / op2->dval;
}

// dst = op1 % op2
void calc_mod(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV(op1, op1, lval);
	CALC_CONV(op2, op2, lval);
	op1->type = LONG_T;
	op2->type = LONG_T;

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
	dst->type = src->type;
	switch (src->type) {
		CALC_MINUS_DEF(dst, src, INT_T, ival);
		CALC_MINUS_DEF(dst,src,LONG_T,lval);
		CALC_MINUS_DEF(dst,src,FLOAT_T,fval);
		CALC_MINUS_DEF(dst,src,DOUBLE_T,dval);
	}
}

// dst = pow(op1, op2)
void calc_pow(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2) {
	CALC_CONV(op1, op1, dval);
	CALC_CONV(op2, op2, dval);
	op1->type = DOUBLE_T;
	op2->type = DOUBLE_T;

	dst->type = DOUBLE_T;
	dst->dval = pow(op1->dval, op2->dval);
}

#define CALC_SQRT_DEF(dst,src,type,val) case type: dst->dval=sqrt(src->val);break

// dst = sqrt(src)
void calc_sqrt(exp_val_t *dst, exp_val_t *src) {
	dst->type = DOUBLE_T;
	switch (src->type) {
		CALC_SQRT_DEF(dst, src, INT_T, ival);
		CALC_SQRT_DEF(dst,src,LONG_T,lval);
		CALC_SQRT_DEF(dst,src,FLOAT_T,fval);
		CALC_SQRT_DEF(dst,src,DOUBLE_T,dval);
	}
}

#ifdef DEBUG
#define CALC_ECHO_DEF(src,type,val,str) case type: printf("(%s) (" str ")", types[type-INT_T],src->val);break
#else
#define CALC_ECHO_DEF(src,type,val,str) case type: printf(str,src->val);break
#endif
// printf(src)
void calc_echo(exp_val_t *src) {
	switch (src->type) {
		CALC_ECHO_DEF(src, INT_T, ival, "%d");
		CALC_ECHO_DEF(src,LONG_T,lval,"%ld");
		CALC_ECHO_DEF(src,FLOAT_T,fval,"%.16f");
		CALC_ECHO_DEF(src,DOUBLE_T,dval,"%.19lf");
	}
}

#define CALC_SPRINTF(buf,src,type,val,str) case type: sprintf(buf,str,src->val);break
// printf(src)
void calc_sprintf(char *buf, exp_val_t *src) {
	switch (src->type) {
		CALC_SPRINTF(buf, src, INT_T, ival, "%d");
		CALC_SPRINTF(buf,src,LONG_T,lval,"%ld");
		CALC_SPRINTF(buf,src,FLOAT_T,fval,"%.16f");
		CALC_SPRINTF(buf,src,DOUBLE_T,dval,"%.19lf");
	}
}

void calc_print(char *p) {
	for (; *p; p++) {
		if (*p == '\\') {
			p++;
			switch (*p) {
				case '\0':
					return;
				case '\\':
					putchar(*p);
					break;
				case 'a':
					putchar('\a');
					break;
				case 'b':
					putchar('\b');
					break;
				case 'f':
					putchar('\f');
					break;
				case 'n':
					putchar('\n');
					break;
				case 'r':
					putchar('\r');
					break;
				case 't':
					putchar('\t');
					break;
				case 'v':
					putchar('\v');
					break;
				default:
					putchar(*p);
					break;
			}
		} else {
			putchar(*p);
		}
	}
}

int calc_clear_or_list_vars(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key) {
	int result = va_arg(args, int);
	char *key = strndup(hash_key->arKey, hash_key->nKeyLength);
	
	if (result == ZEND_HASH_APPLY_KEEP) {
		printf("    (%6s) %s = ", types[val->type - INT_T], key);
	} else {
		printf("    remove variable %s, type is %s, value is ", key, types[val->type - INT_T]);
	}
	free(key);
	calc_echo(val);
	printf("\n");
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
		yyerror("The user function %s the first %d argument not default value", names, errArgc + 1);
		exit(0);
	}
}

void calc_func_def(func_def_f *def) {
	dprintf("define function ");
	calc_func_print(def);
	dprintf("\n");

	zend_hash_update(&funcs, def->name, strlen(def->name), def, sizeof(func_def_f), NULL);
}

void calc_free_args(call_args_t *args) {
	call_args_t *tmpArgs;

	while (args) {
		switch (args->val.type) {
		case VAR_T:
			free(args->val.str);
			break;
		}
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
		}

		tmpSyms = syms;
		syms = syms->next;
		free(tmpSyms);
	}
}

void calc_free_func(func_def_f *def) {
	free(def->name);

	func_args_t *args = def->args, *tmpArgs;

	while (args) {
		tmpArgs = args;
		args = args->next;
		//free(tmpArgs->name);
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
		case VAR_T: {
			exp_val_t *ptr = NULL;
			zend_hash_find(&vars, expr->str, strlen(expr->str), (void**)&ptr);
			if (ptr) {
				memcpy(ret, ptr, sizeof(exp_val_t));
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
			exp_val_t left, right;
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
			break;
		}
		case ABS_T:
		case MINUS_T: {
			exp_val_t val;
			calc_run_expr(&val, expr->left);
			if (expr->type == ABS_T) {
				calc_abs(ret, &val);
			} else {
				calc_minus(ret, &val);
			}
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
			exp_val_t left, right;

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
			break;
		}
		case IF_T: {
			exp_val_t cond;

			calc_run_expr(&cond, expr->cond);
			CALC_CONV((&cond), (&cond), dval);

			if (cond.dval) {
				calc_run_expr(ret, expr->left);
			} else {
				calc_run_expr(ret, expr->right);
			}
			break;
		}
		case ARRAY_T: {
			exp_val_t *ptr = NULL;
			zend_hash_find(&vars, expr->call.name, strlen(expr->call.name), (void**)&ptr);
			if (ptr->type != ARRAY_T) {
				yyerror("(warning) variable %s not is a array, type is %s", expr->call.name, types[ptr->type - INT_T]);
			} else {
				ret->type = INT_T;
				ret->ival = 0;
				call_args_t *args = expr->call.args;

				exp_val_t *tmp = ptr, val;
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
								memcpy(ret, tmp, sizeof(exp_val_t));
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
						calc_print(args->val.str);
						break;
					case NULL_T:
						printf("null");
						break;
					default: {
						exp_val_t val;
						calc_run_expr(&val, &args->val);
						calc_echo(&val);
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
				exp_val_t cond;

				calc_run_expr(&cond, syms->cond);
				CALC_CONV((&cond), (&cond), dval);

				status_enum_t status = (cond.dval ? calc_run_syms(ret, syms->lsyms) : calc_run_syms(ret, syms->rsyms));
				if (status != NONE_STATUS) {
					return status;
				}
				break;
			}
			case WHILE_STMT_T: {
				exp_val_t cond;

				calc_run_expr(&cond, syms->cond);
				CALC_CONV((&cond), (&cond), dval);

				status_enum_t status;
				while (cond.dval) {
					status = calc_run_syms(ret, syms->lsyms);
					if (status != NONE_STATUS) {
						return status;
					}

					calc_run_expr(&cond, syms->cond);
					CALC_CONV((&cond), (&cond), dval);
				}
				break;
			}
			case DO_WHILE_STMT_T: {
				exp_val_t cond;
				status_enum_t status;

				do {
					status = calc_run_syms(ret, syms->lsyms);
					if (status == RET_STATUS) {
						return status;
					} else if (status == BREAK_STATUS) {
						break;
					}

					calc_run_expr(&cond, syms->cond);
					CALC_CONV((&cond), (&cond), dval);
				} while (cond.dval);
				break;
			}
			case BREAK_STMT_T: {
				return BREAK_STATUS;
			}
			case ARRAY_STMT_T: {
				exp_val_t val;
				call_args_t *callArgs = syms->args->next;
				call_args_t *tmpArgs = (call_args_t*) malloc(sizeof(call_args_t)), *args = tmpArgs;

				while (callArgs) {
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
				exp_val_t val;
				
				calc_run_expr(&val, syms->val);
				
				switch (syms->var->type) {
					case VAR_T: {
						exp_val_t *ptr = NULL;
						zend_hash_find(&vars, syms->args->val.str, strlen(syms->args->val.str), (void**)&ptr);
						if (ptr) {
							switch(syms->type) {
								case ADDEQ_STMT_T:
									calc_add(ptr, ptr, &val);
									break;
								case SUBEQ_STMT_T:
									calc_sub(ptr, ptr, &val);
									break;
								case MULEQ_STMT_T:
									calc_mul(ptr, ptr, &val);
									break;
								case DIVEQ_STMT_T:
									calc_div(ptr, ptr, &val);
									break;
								case MODEQ_STMT_T:
									calc_mod(ptr, ptr, &val);
									break;
								default:
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
							yyerror("(warning) variable %s not is a array, type is %s", syms->var->call.name, types[ptr->type - INT_T]);
						} else {
							call_args_t *args = syms->var->call.args;

							exp_val_t *tmp = ptr, argsVal;
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
														break;
													case SUBEQ_STMT_T:
														calc_sub(tmp, tmp, &val);
														break;
													case MULEQ_STMT_T:
														calc_mul(tmp, tmp, &val);
														break;
													case DIVEQ_STMT_T:
														calc_div(tmp, tmp, &val);
														break;
													case MODEQ_STMT_T:
														calc_mod(tmp, tmp, &val);
														break;
													default:
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
		}

		nextStmt: syms = syms->next;
	}

	return NONE_STATUS;
}

void call_func_run(exp_val_t *ret, func_def_f *def, call_args_t *args) {
	HashTable tmpVars = vars;
	
	zend_hash_init(&vars, 2, (dtor_func_t)call_free_vars);

	call_args_t *tmpArgs = args;
	func_args_t *funcArgs = def->args;
	exp_val_t val;
	while (funcArgs) {
		if(tmpArgs) {
			calc_run_expr(&val, &tmpArgs->val);
			tmpArgs = tmpArgs->next;
		} else {
			val = funcArgs->val;
		}
		zend_hash_update(&vars, funcArgs->name, strlen(funcArgs->name), &val, sizeof(exp_val_t), NULL);
		funcArgs = funcArgs->next;
	}

	calc_run_syms(ret, def->syms);

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
				exit(0);
			}

			dprintf("call user function for ");
			calc_func_print(def);
			dprintf("\n");

			linenostack[++linenostacktop].funcname = name;
			call_func_run(ret, def, args);
			linenostacktop--;
		} else {
			dprintf("undefined user function for %s\n", name);
		}
		break;
	}
	default:
		dprintf("undefined system function for %s\n", name);
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

void call_free_expr(exp_val_t *expr) {
	switch (expr->type) {
		case VAR_T: {
			dprintf("--- Free: VAR_T --- ");
			free(expr->str);
			break;
		}
		case ADD_T:
		case SUB_T:
		case MUL_T:
		case DIV_T:
		case MOD_T:
		case POW_T: {
			dprintf("--- Free: ADD_T/.../POW_T --- ");
			call_free_expr(expr->left);
			call_free_expr(expr->right);

			free(expr->left);
			free(expr->right);
			break;
		}
		case ABS_T:
		case MINUS_T: {
			dprintf("--- Free: ABS_T/MINUS_T --- ");
			call_free_expr(expr->left);
			
			free(expr->left);
			break;
		}
		case FUNC_T: {
			dprintf("--- Free: FUNC_T --- ");
			calc_free_args(expr->call.args);
			break;
		}
		case LOGIC_GT_T:
		case LOGIC_LT_T:
		case LOGIC_GE_T:
		case LOGIC_LE_T:
		case LOGIC_EQ_T:
		case LOGIC_NE_T: {
			dprintf("--- Free: LOGIC_??_T --- ");
			call_free_expr(expr->left);
			call_free_expr(expr->right);
			
			free(expr->left);
			free(expr->right);
			break;
		}
		case IF_T: {
			dprintf("--- Free: IF_T --- ");
			call_free_expr(expr->cond);
			call_free_expr(expr->left);
			call_free_expr(expr->right);

			free(expr->cond);
			free(expr->left);
			free(expr->right);
			break;
		}
		case ARRAY_T: {
			dprintf("--- Free: ARRAY_T --- ");
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
			call_free_expr(&ptr->array[i]);
		}
	}
	free(ptr->array);
}

void call_free_vars(exp_val_t *expr) {
	switch (expr->type) {
		case ARRAY_T: {
			dprintf("--- Free: ARRAY_T(%08x) ---\n", expr->arrayArgs);
			calc_array_free(expr, expr->arrayArgs);
			calc_free_args(expr->arrayArgs);
			break;
		}
		default: {
			dprintf("--- Free: %s ---\n", types[expr->type - INT_T]);
		}
	}
	//free(expr);
}

int main(int argc, char **argv) {
	zend_hash_init(&vars, 2, (dtor_func_t)call_free_vars);
	zend_hash_init(&funcs, 2, (dtor_func_t)calc_free_func);

	yyparse();

	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);

	return 0;
}

void yyerror(char *s, ...) {
	fprintf(stderr, "===================================\n");
	fprintf(stderr, "There is an error in line %d: \n", yylineno);

	int i;
	for(i=0; i<=linenostacktop; i++) {
		fprintf(stderr, "Line %d in user function %s\n", linenostack[i].lineno, linenostack[i].funcname);
	}

	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	
	fprintf(stderr, "\n");
	fprintf(stderr, "===================================\n");
}

