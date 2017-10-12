/* simplest version of calculator */
%{

#include "calc.h"

#define NEW_FREES(dst, type) dst=NEW1(type);zend_hash_next_index_insert(&frees, dst, 0, NULL)
#define MEMDUP(dst,src,type) NEW_FREES(dst, type);memcpy(dst,src,sizeof(type))
#define CALL_ARGS(args,v) NEW_FREES(args, call_args_t);args->val=v;args->tail=args;args->next=NULL
#define FUNC_ARGS(args,v) NEW_FREES(args, func_args_t);args->val=v;args->tail=args;args->next=NULL
#define APPEND(args,val) args->tail->next=val;args->tail=val
#define STMT(o,t,k,v) NEW_FREES(o.syms, func_symbol_t);o.syms->type=t;o.syms->k=v;o.syms->lineno=yylineno;o.syms->filename=curFileName;o.syms->tail=o.syms;o.syms->next=NULL

#define FUNC_MOVE_FREES(def) //zend_hash_init(&def->frees, zend_hash_num_elements(&frees), (dtor_func_t)free_frees);zend_hash_apply_with_argument(&frees, (apply_func_arg_t)zend_hash_apply_append_frees, &def->frees)

#define VAR_RUN(t) (t == VAR_T ? calc_run_sym_variable_assign : calc_run_sym_array_assign)

#define VAL_EXPR_LT(dst, op1, t) dst.type=t;NEW_FREES(dst.defExp, exp_def_t);MEMDUP(dst.defExp->left,&op1,exp_val_t)
#define VAL_EXPR_LN(dst, op1, t, n) dst.type=t;MEMDUP(dst.ref,&op1,exp_val_t);dst.run = calc_run_##n
#define VAL_EXPR_LR(dst, op1, op2, t) VAL_EXPR_LT(dst, op1, t);MEMDUP(dst.defExp->right,&op2,exp_val_t)
#define VAL_EXPR(dst, op1, op2, t, n) VAL_EXPR_LR(dst, op1, op2, t);dst.run = calc_run_##n
#define MEMDUP_EXPR(dst, src, op1, op2, t, n) VAL_EXPR(src, op1, op2, t, n);MEMDUP(dst.syms->val,&src,exp_val_t)

#define EXPR(n,t,k,v) exp_val_t n;n.type=t;n.k=v;n.run=calc_run_copy

void yypush_buffer_state ( void* );
void* yy_create_buffer ( FILE *file, int size );

wrap_stack_t *curWrapStack = NULL;
wrap_stack_t *tailWrapStack = NULL;
unsigned int includeDeep = 0;
char *curFileName = "-";

int zend_hash_apply_append_frees(void*, HashTable*);
%}

/* declare tokens */
%token EOL
%token CONST_PI NUMBER CONST_RAND_MAX
%token LIST CLEAR
%token CALL FUNC
%token VARIABLE STR
%token ECHO_T GLOBAL_T RET IF ELSE WHILE DO BREAK ARRAY FOR
%token INC DEC
%token ADDEQ SUBEQ MULEQ DIVEQ MODEQ POWEQ
%token SWITCH CASE DEFAULT
%token INCLUDE

%nonassoc UMINUS
%nonassoc IFX
%nonassoc CASEX

%left '?' ':'
%left LOGIC
%left '+' '-'
%left '*' '/' '%' '^'

%start calclist

%%

/************************ 入口语法 ************************/
calclist:
 | calclist FUNC VARIABLE '(' ')' '{' '}' { if(EXPECTED(isSyntaxData)) { NEW_FREES($$.def, func_def_f);$$.def->name = $3.var;$$.def->args=NULL;$$.def->syms=NULL;FUNC_MOVE_FREES($$.def);calc_func_def($$.def); } }
 | calclist FUNC VARIABLE '(' ')' '{' funcStmtList '}' { if(EXPECTED(isSyntaxData)) { NEW_FREES($$.def, func_def_f);$$.def->name = $3.var;$$.def->args=NULL;$$.def->syms=$7.syms;FUNC_MOVE_FREES($$.def);calc_func_def($$.def); } }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' '}' { if(EXPECTED(isSyntaxData)) { NEW_FREES($$.def, func_def_f);$$.def->name = $3.var;$$.def->args=$5.defArgs;$$.def->syms=NULL;FUNC_MOVE_FREES($$.def);calc_func_def($$.def); } }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' funcStmtList '}' { if(EXPECTED(isSyntaxData)) { NEW_FREES($$.def, func_def_f);$$.def->name = $3.var;$$.def->args=$5.defArgs;$$.def->syms=$8.syms;FUNC_MOVE_FREES($$.def);calc_func_def($$.def); } }
 | calclist stmtList { if(EXPECTED(isSyntaxData)) { if(topSyms) {APPEND(topSyms,$2.syms);} else { topSyms = $2.syms; } /*zend_hash_apply_with_argument(&frees, (apply_func_arg_t)zend_hash_apply_append_frees, &topFrees);*/ } }
 | calclist INCLUDE STR ';' {
	FILE *fp = fopen($3.str->c, "r");
	if(fp) {
		if (zend_hash_add(&files, $3.str->c, $3.str->n, NULL, 0, NULL) == SUCCESS) {
			wrap_stack_t *_wrap_stack = (wrap_stack_t*)malloc(sizeof(wrap_stack_t));
			_wrap_stack->prev = tailWrapStack;
			_wrap_stack->fp = yyin;
			_wrap_stack->filename = curFileName;
			_wrap_stack->lineno = yylineno;
			curFileName = $3.str->c;
			yylineno = 1;
			tailWrapStack = _wrap_stack;
			includeDeep++;
			dprintf("--------------------------\n");
			dprintf("END INPUT: %s\n", $3.str);
			dprintf("==========================\n");
			yypush_buffer_state(yy_create_buffer(fp, 16384));
		} else {
			yyerror("File \"%s\" already included!\n", $3.str->c);
			fclose(fp);/*
			if(EXPECTED(isSyntaxData)) {
				free($3.str->c);
				free($3.str);
			}*/
		}
	} else {
		yyerror("File \"%s\" not found!\n", $3.str->c);/*
		if(EXPECTED(isSyntaxData)) {
			free($3.str->c);
			free($3.str);
		}*/
	}/*
	if(EXPECTED(isSyntaxData)) {
		zend_hash_clean(&frees);
	}*/
}
;

/************************ 函数参数语法 ************************/
funcArgList:  funcArg
 | funcArgList ',' funcArg { if(EXPECTED(isSyntaxData)) { APPEND($$.defArgs,$3.defArgs); } }
;
funcArg: VARIABLE '=' number { if(EXPECTED(isSyntaxData)) { FUNC_ARGS($$.defArgs,$3);$$.defArgs->name = $1.var; } }
 | VARIABLE '=' STR { if(EXPECTED(isSyntaxData)) { FUNC_ARGS($$.defArgs,$3);$$.defArgs->name = $1.var; } }
 | VARIABLE { if(EXPECTED(isSyntaxData)) { FUNC_ARGS($$.defArgs,$1);$$.defArgs->name = $1.var; } }
;
/************************ 函数语句语法 ************************/
funcStmtList: funcStmt
 | funcStmtList funcStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms); } }
;
funcStmt: stmt
 | GLOBAL_T varArgList ';' { if(EXPECTED(isSyntaxData)) { STMT($$,GLOBAL_T,args,$2.callArgs);$$.syms->run = calc_run_sym_null; } }
;
switchStmtList: switchStmt
 | switchStmtList switchStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms);$$.syms->tail=$2.syms->tail; } }
;
switchStmt: nullCaseStmt %prec CASEX
 | caseStmt breakStmtList { if(EXPECTED(isSyntaxData)) { $$.syms=$1.syms;$$.syms->next=$2.syms;$$.syms->tail = $2.syms->tail; } }
;

caseStmt: nullCaseStmt
 | DEFAULT ':' { if(EXPECTED(isSyntaxData)) { STMT($$,DEFAULT_STMT_T,cond,NULL);$$.syms->run = calc_run_sym_null; } }
;
nullCaseStmt: CASE const ':' { if(EXPECTED(isSyntaxData)) { STMT($$,CASE_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$2,exp_val_t);if($$.syms->cond->type != STR_T) {CALC_CONV($$.syms->cond, $$.syms->cond, str);zend_hash_next_index_insert(&frees, $$.syms->cond->str, 0, NULL);zend_hash_next_index_insert(&frees, $$.syms->cond->str->c, 0, NULL);}$$.syms->run = calc_run_sym_null; } }
;
breakStmtList: breakStmt
 | breakStmtList breakStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms); } }
;
breakStmt: stmt
 | BREAK ';' { if(EXPECTED(isSyntaxData)) { STMT($$,BREAK_STMT_T,args,NULL);$$.syms->run = calc_run_sym_break; } }
;
stmtList: stmt
 | stmtList stmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms); } }
;
stmt: forStmt ';'
 | RET stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,RET_STMT_T,expr,NULL);MEMDUP($$.syms->expr,&$2,exp_val_t);$$.syms->run = calc_run_sym_ret; } }
 | IF '(' stmtExpr ')' stmt %prec IFX { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$5.syms;$$.syms->rsyms=NULL;$$.syms->run = calc_run_sym_if; } }
 | IF '(' stmtExpr ')' stmt ELSE stmt { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$5.syms;$$.syms->rsyms=$7.syms;$$.syms->run = calc_run_sym_if; } }
 | IF '(' stmtExpr ')' stmt ELSE '{' stmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$5.syms;$$.syms->rsyms=$8.syms;$$.syms->run = calc_run_sym_if; } }
 | IF '(' stmtExpr ')' '{' stmtList '}' %prec IFX { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=NULL;$$.syms->run = calc_run_sym_if; } }
 | IF '(' stmtExpr ')' '{' stmtList '}' ELSE stmt { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=$9.syms;$$.syms->run = calc_run_sym_if; } }
 | IF '(' stmtExpr ')' '{' stmtList '}' ELSE '{' stmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=$10.syms;$$.syms->run = calc_run_sym_if; } }
 | WHILE '(' stmtExpr ')' '{' breakStmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,WHILE_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=NULL;$$.syms->run = calc_run_sym_while; } }
 | DO '{' breakStmtList '}' WHILE '(' stmtExpr ')' ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DO_WHILE_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$7,exp_val_t);$$.syms->lsyms=$3.syms;$$.syms->rsyms=NULL;$$.syms->run = calc_run_sym_do_while; } }
 | FOR '(' forStmtList ';' forStmtExpr ';' forStmtList ')' '{' breakStmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,FOR_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$5,exp_val_t);$$.syms->lsyms=$3.syms;$$.syms->rsyms=$7.syms;$$.syms->forSyms=$10.syms;$$.syms->run = calc_run_sym_for; } }
 | ARRAY VARIABLE arrayArgList ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ARRAY_STMT_T,args,$3.callArgs);MEMDUP($$.syms->var,&$2,exp_val_t);$$.syms->run = calc_run_sym_array; } }
 | SWITCH '(' stmtExpr ')' '{' switchStmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,SWITCH_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=NULL;func_symbol_t *syms = $6.syms;$$.syms->ht = (HashTable*)malloc(sizeof(HashTable));zend_hash_init($$.syms->ht, 2, NULL);while(syms){if(syms->type == CASE_STMT_T) zend_hash_add($$.syms->ht, syms->cond->str->c, syms->cond->str->n, syms->next, 0, NULL);if(syms->type == DEFAULT_STMT_T) $$.syms->rsyms = syms->next;syms = syms->next;}append_pool($$.syms->ht, (dtor_func_t)zend_hash_destroy_ptr);$$.syms->run = calc_run_sym_switch; } }
 | ';' { if(EXPECTED(isSyntaxData)) { STMT($$,NULL_STMT_T,args,NULL);$$.syms->run = calc_run_sym_null; } } // 空语句
;
forStmtList: forStmt
 | /* 空语句 */ { if(EXPECTED(isSyntaxData)) { STMT($$,NULL_STMT_T,args,NULL);$$.syms->run = calc_run_sym_null; } }
 | forStmtList ',' forStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$3.syms); } }
;
forStmt: LIST { if(EXPECTED(isSyntaxData)) { STMT($$,LIST_STMT_T,args,NULL);$$.syms->run = calc_run_sym_list; } }
 | CLEAR { if(EXPECTED(isSyntaxData)) { STMT($$,CLEAR_STMT_T,args,NULL);$$.syms->run = calc_run_sym_clear; } }
 | ECHO_T stmtExprArgList { if(EXPECTED(isSyntaxData)) { STMT($$,ECHO_STMT_T,args,$2.callArgs);$$.syms->run = calc_run_sym_echo; } }
 | varExpr '=' stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr INC { if(EXPECTED(isSyntaxData)) { EXPR(val,INT_T,ival,1);STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, val, ADD_T, add);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr DEC { if(EXPECTED(isSyntaxData)) { EXPR(val,INT_T,ival,1);STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, val, SUB_T, sub);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr ADDEQ stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, $3, ADD_T, add);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr SUBEQ stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, $3, SUB_T, sub);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr MULEQ stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, $3, MUL_T, mul);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr DIVEQ stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, $3, DIV_T, div);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr MODEQ stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, $3, MOD_T, mod);$$.syms->run = VAR_RUN($1.type); } }
 | varExpr POWEQ stmtExpr { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP_EXPR($$, $2, $1, $3, POW_T, pow);$$.syms->run = VAR_RUN($1.type); } }
 | callExpr { if(EXPECTED(isSyntaxData)) { STMT($$,FUNC_STMT_T,args,NULL);MEMDUP($$.syms->expr,&$1,exp_val_t);$$.syms->run = calc_run_sym_func; } }
;
forStmtExpr: stmtExpr
 | /* 空语句 */ { $$.type = NULL_T; }
;
varArgList: varArg
 | varArgList ',' varArg { if(EXPECTED(isSyntaxData)) { APPEND($$.callArgs,$3.callArgs); } }
;
varArg: VARIABLE { if(EXPECTED(isSyntaxData)) { CALL_ARGS($$.callArgs,$1); } }
;
arrayArgList: arrayArg
 | arrayArgList arrayArg { if(EXPECTED(isSyntaxData)) { APPEND($$.callArgs,$2.callArgs); } }
;
arrayArg: '[' stmtExpr ']' { if(EXPECTED(isSyntaxData)) { CALL_ARGS($$.callArgs,$2); } }
;
stmtExpr: const
 | varExpr
 | callExpr
 | '|' stmtExpr '|' { if(EXPECTED(isSyntaxData)) { VAL_EXPR_LN($$, $2, ABS_T, abs); } }
 | '(' stmtExpr ')' { if(EXPECTED(isSyntaxData)) { $$ = $2; } } // 括号
 | '-' stmtExpr %prec UMINUS { if(EXPECTED(isSyntaxData)) { VAL_EXPR_LN($$, $2, MINUS_T, minus); } }
 | stmtExpr '+' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $1, $3, ADD_T, add); } }
 | stmtExpr '-' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $1, $3, SUB_T, sub); } }
 | stmtExpr '*' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $1, $3, MUL_T, mul); } }
 | stmtExpr '/' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $1, $3, DIV_T, div); } }
 | stmtExpr '%' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $1, $3, MOD_T, mod); } }
 | stmtExpr '^' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $1, $3, POW_T, pow); } }
 | stmtExpr LOGIC stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR_LR($$, $1, $3, $2.type);$$.run = $2.run; } }
 | stmtExpr '?' stmtExpr ':' stmtExpr { if(EXPECTED(isSyntaxData)) { VAL_EXPR($$, $3, $5, IF_T, iif);MEMDUP($$.defExp->cond,&$1,exp_val_t); } }
;
varExpr: VARIABLE
 | VARIABLE arrayArgList { if(EXPECTED(isSyntaxData)) { $$.type=ARRAY_T;NEW_FREES($$.call, func_call_t);$$.call->name=$1.var;$$.call->args=$2.callArgs;$$.run = calc_run_array; } }
;
callExpr: CALL '(' ')' { if(EXPECTED(isSyntaxData)) { $$=$1;$$.call->args=NULL;$$.run = $1.run; } } // 系统函数
 | CALL '(' stmtExprArgList ')' { if(EXPECTED(isSyntaxData)) { $$=$1;$$.call->args=$3.callArgs;$$.run = $1.run; } } // 系统函数
 | VARIABLE '(' ')' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_T;NEW_FREES($$.call, func_call_t);$$.call->type=USER_F;$$.call->name=$1.var;$$.call->args=NULL;$$.run = calc_run_func; } } // 用户自定义函数
 | VARIABLE '(' stmtExprArgList ')' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_T;NEW_FREES($$.call, func_call_t);$$.call->type=USER_F;$$.call->name=$1.var;$$.call->args=$3.callArgs;$$.run = calc_run_func; } } // 用户自定义函数
;
stmtExprArgList: stmtExprArg
 | stmtExprArgList ',' stmtExprArg { if(EXPECTED(isSyntaxData)) { APPEND($$.callArgs,$3.callArgs); } }
;
stmtExprArg: stmtExpr { if(EXPECTED(isSyntaxData)) { CALL_ARGS($$.callArgs,$1); } }
;
const: number
 | STR
;
number: NUMBER
 | CONST_RAND_MAX
 | CONST_PI { if(EXPECTED(isSyntaxData)) {$$.type=DOUBLE_T;$$.dval=atof("3.141592653589793238462643383279502884197169");$$.run = calc_run_copy; } } // PI
;

%%

int zend_hash_apply_append_frees(void *ptr, HashTable *ht) {
	zend_hash_next_index_insert(ht, ptr, 0, NULL);
	return ZEND_HASH_APPLY_REMOVE;
}
