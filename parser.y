/* simplest version of calculator */
%{

#include "calc.h"

#define MEMALLOC(dst, type) dst=(type*)malloc(sizeof(type));zend_hash_next_index_insert(&frees, dst, 0, NULL)
#define MEMDUP(dst,src,type) MEMALLOC(dst, type);memcpy(dst,src,sizeof(type))
#define CALL_ARGS(args,v) MEMALLOC(args, call_args_t);args->val=v;args->tail=args;args->next=NULL
#define FUNC_ARGS(args,v) MEMALLOC(args, func_args_t);args->val=v;args->tail=args;args->next=NULL
#define APPEND(args,val) args->tail->next=val;args->tail=val
#define STMT(o,t,k,v) MEMALLOC(o.syms, func_symbol_t);o.syms->type=t;o.syms->k=v;o.syms->lineno=yylineno;o.syms->tail=o.syms;o.syms->next=NULL

#define FUNC_MOVE_FREES(ht) zend_hash_init(ht, zend_hash_num_elements(&frees), (dtor_func_t)free_frees);zend_hash_apply_with_argument(&frees, (apply_func_arg_t)zend_hash_apply_append_frees, ht)

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
%token ECHO_T GLOBAL_T RET IF ELSE WHILE DO BREAK ARRAY
%token INC DEC
%token ADDEQ SUBEQ MULEQ DIVEQ MODEQ
%token INCLUDE

%nonassoc UMINUS
%nonassoc IFX

%left '?' ':'
%left LOGIC
%left '+' '-'
%left '*' '/' '%' '^'

%start calclist

%%

/************************ 入口语法 ************************/
calclist:
 | calclist FUNC VARIABLE '(' ')' '{' '}' { if(EXPECTED(isSyntaxData)) { MEMALLOC($$.def, func_def_f);$$.def->name = $3.str;$$.def->args=NULL;$$.def->syms=NULL;FUNC_MOVE_FREES(&$$.def->frees);calc_func_def($$.def); } }
 | calclist FUNC VARIABLE '(' ')' '{' funcStmtList '}' { if(EXPECTED(isSyntaxData)) { MEMALLOC($$.def, func_def_f);$$.def->name = $3.str;$$.def->args=NULL;$$.def->syms=$7.syms;FUNC_MOVE_FREES(&$$.def->frees);calc_func_def($$.def); } }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' '}' { if(EXPECTED(isSyntaxData)) { MEMALLOC($$.def, func_def_f);$$.def->name = $3.str;$$.def->args=$5.defArgs;$$.def->syms=NULL;FUNC_MOVE_FREES(&$$.def->frees);calc_func_def($$.def); } }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' funcStmtList '}' { if(EXPECTED(isSyntaxData)) { MEMALLOC($$.def, func_def_f);$$.def->name = $3.str;$$.def->args=$5.defArgs;$$.def->syms=$8.syms;FUNC_MOVE_FREES(&$$.def->frees);calc_func_def($$.def); } }
 | calclist stmtList { if(EXPECTED(isSyntaxData)) { if(topSyms) {APPEND(topSyms,$2.syms);} else { topSyms = $2.syms; } zend_hash_apply_with_argument(&frees, (apply_func_arg_t)zend_hash_apply_append_frees, &topFrees); } }
 | calclist INCLUDE STR ';' {
	FILE *fp = fopen($3.str, "r");
	if(fp) {
		if (zend_hash_add(&files, $3.str, strlen($3.str), NULL, 0, NULL) == SUCCESS) {
			wrap_stack_t *_wrap_stack = (wrap_stack_t*)malloc(sizeof(wrap_stack_t));
			_wrap_stack->prev = tailWrapStack;
			_wrap_stack->fp = yyin;
			_wrap_stack->filename = curFileName;
			_wrap_stack->lineno = yylineno;
			curFileName = $3.str;
			yylineno = 1;
			tailWrapStack = _wrap_stack;
			includeDeep++;
			dprintf("--------------------------\n");
			dprintf("END INPUT: %s\n", $3.str);
			dprintf("==========================\n");
			yypush_buffer_state(yy_create_buffer(fp, 16384));
		} else {
			yyerror("File \"%s\" already included!\n", $3.str);
			fclose(fp);
			if(EXPECTED(isSyntaxData)) {
				free($3.str);
			}
		}
	} else {
		yyerror("File \"%s\" not found!\n", $3.str);
		if(EXPECTED(isSyntaxData)) {
			free($3.str);
		}
	}
	if(EXPECTED(isSyntaxData)) {
		zend_hash_clean(&frees);
	}
}
;

/************************ 函数参数语法 ************************/
funcArgList:  funcArg
 | funcArgList ',' funcArg { if(EXPECTED(isSyntaxData)) { APPEND($$.defArgs,$3.defArgs); } }
;
funcArg: VARIABLE '=' number { if(EXPECTED(isSyntaxData)) { FUNC_ARGS($$.defArgs,$3);$$.defArgs->name = $1.str; } }
 | VARIABLE '=' STR { if(EXPECTED(isSyntaxData)) { FUNC_ARGS($$.defArgs,$3);$$.defArgs->name = $1.str; } }
 | VARIABLE { if(EXPECTED(isSyntaxData)) { FUNC_ARGS($$.defArgs,$1);$$.defArgs->name = $1.str; } }
;
/************************ 函数语句语法 ************************/
funcStmtList: funcStmt
 | funcStmtList funcStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms); } }
;
funcStmt: stmt
 | GLOBAL_T varArgList ';' { if(EXPECTED(isSyntaxData)) { STMT($$,GLOBAL_T,args,$2.callArgs); } }
;
breakStmtList: breakStmt
 | breakStmtList breakStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms); } }
;
breakStmt: stmt
 | BREAK ';' { if(EXPECTED(isSyntaxData)) { STMT($$,BREAK_STMT_T,args,NULL); } }
;
stmtList: stmt
 | stmtList stmt { if(EXPECTED(isSyntaxData)) { APPEND($$.syms,$2.syms); } }
;
stmt: ECHO_T stmtExprArgList ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ECHO_STMT_T,args,$2.callArgs); } }
 | RET stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,RET_STMT_T,expr,NULL);MEMDUP($$.syms->expr,&$2,exp_val_t); } }
 | VARIABLE '=' stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t); }  }
 | IF '(' stmtExpr ')' '{' stmtList '}' %prec IFX { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=NULL; } }
 | IF '(' stmtExpr ')' '{' stmtList '}' ELSE '{' stmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=$10.syms; } }
 | WHILE '(' stmtExpr ')' '{' breakStmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,WHILE_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$3,exp_val_t);$$.syms->lsyms=$6.syms;$$.syms->rsyms=NULL; } }
 | DO '{' breakStmtList '}' WHILE '(' stmtExpr ')' ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DO_WHILE_STMT_T,cond,NULL);MEMDUP($$.syms->cond,&$7,exp_val_t);$$.syms->lsyms=$3.syms;$$.syms->rsyms=NULL; } }
 | LIST ';' { if(EXPECTED(isSyntaxData)) { STMT($$,LIST_STMT_T,args,NULL); } }
 | CLEAR ';' { if(EXPECTED(isSyntaxData)) { STMT($$,CLEAR_STMT_T,args,NULL); } }
 | ARRAY VARIABLE arrayArgList ';' { if(EXPECTED(isSyntaxData)) { CALL_ARGS($1.callArgs,$2);$1.callArgs->next=$3.callArgs;STMT($$,ARRAY_STMT_T,args,$1.callArgs); } }
 | VARIABLE arrayArgList '=' stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$4,exp_val_t);$$.syms->var->type=ARRAY_T;$$.syms->var->callName=$1.str;$$.syms->var->callArgs=$2.callArgs; } }
 | VARIABLE INC ';' { if(EXPECTED(isSyntaxData)) { STMT($$,INC_STMT_T,val,NULL);MEMDUP($$.syms->var,&$1,exp_val_t); } }
 | VARIABLE DEC ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DEC_STMT_T,val,NULL);MEMDUP($$.syms->var,&$1,exp_val_t); } }
 | VARIABLE ADDEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ADDEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t); } }
 | VARIABLE SUBEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,SUBEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t); } }
 | VARIABLE MULEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MULEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t); } }
 | VARIABLE DIVEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DIVEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t); } }
 | VARIABLE MODEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MODEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$3,exp_val_t); } }
 | VARIABLE arrayArgList ADDEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ADDEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$4,exp_val_t);$$.syms->var->type=ARRAY_T;$$.syms->var->callName=$1.str;$$.syms->var->callArgs=$2.callArgs; } }
 | VARIABLE arrayArgList SUBEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,SUBEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$4,exp_val_t);$$.syms->var->type=ARRAY_T;$$.syms->var->callName=$1.str;$$.syms->var->callArgs=$2.callArgs; } }
 | VARIABLE arrayArgList MULEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MULEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$4,exp_val_t);$$.syms->var->type=ARRAY_T;$$.syms->var->callName=$1.str;$$.syms->var->callArgs=$2.callArgs; } }
 | VARIABLE arrayArgList DIVEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DIVEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$4,exp_val_t);$$.syms->var->type=ARRAY_T;$$.syms->var->callName=$1.str;$$.syms->var->callArgs=$2.callArgs; } }
 | VARIABLE arrayArgList MODEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MODEQ_STMT_T,var,NULL);MEMDUP($$.syms->var,&$1,exp_val_t);MEMDUP($$.syms->val,&$4,exp_val_t);$$.syms->var->type=ARRAY_T;$$.syms->var->callName=$1.str;$$.syms->var->callArgs=$2.callArgs; } }
 | callExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,FUNC_STMT_T,args,NULL);MEMDUP($$.syms->expr,&$1,exp_val_t); } }
 | ';' { if(EXPECTED(isSyntaxData)) { STMT($$,NULL_STMT_T,args,NULL); } } // 空语句
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
stmtExpr: VARIABLE
 | number
 | STR
 | '-' stmtExpr %prec UMINUS { if(EXPECTED(isSyntaxData)) { $$.type=MINUS_T;MEMDUP($$.left,&$2,exp_val_t);$$.right=NULL;$$.run = calc_run_minus; } }
 | stmtExpr '+' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=ADD_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = calc_run_add; } }
 | stmtExpr '-' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=SUB_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = calc_run_sub; } }
 | stmtExpr '*' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=MUL_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = calc_run_mul; } }
 | stmtExpr '/' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=DIV_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = calc_run_div; } }
 | stmtExpr '%' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=MOD_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = calc_run_mod; } }
 | stmtExpr '^' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=POW_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = calc_run_pow; } }
 | stmtExpr LOGIC stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=$2.type;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t);$$.run = $2.run; } }
 | stmtExpr '?' stmtExpr ':' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=IF_T;MEMDUP($$.cond,&$1,exp_val_t);MEMDUP($$.left,&$3,exp_val_t);MEMDUP($$.right,&$5,exp_val_t);$$.run = calc_run_iif; } }
 | '|' stmtExpr '|' { if(EXPECTED(isSyntaxData)) { $$.type=ABS_T;MEMDUP($$.left,&$2,exp_val_t);$$.right=NULL;$$.run = calc_run_abs; } }
 | '(' stmtExpr ')' { if(EXPECTED(isSyntaxData)) { $$ = $2; } } // 括号
 | VARIABLE arrayArgList { if(EXPECTED(isSyntaxData)) { $$.type=ARRAY_T;$$.callName=$1.str;$$.callArgs=$2.callArgs;$$.run = calc_run_array; } } // 用户自定义函数
 | callExpr
;
callExpr: CALL '(' ')' { if(EXPECTED(isSyntaxData)) { $$=$1;$$.callArgs=NULL;$$.run = $1.run; } } // 系统函数
 | CALL '(' stmtExprArgList ')' { if(EXPECTED(isSyntaxData)) { $$=$1;$$.callArgs=$3.callArgs;$$.run = $1.run; } } // 系统函数
 | VARIABLE '(' ')' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_T;$$.callType=USER_F;$$.callName=$1.str;$$.callArgs=NULL;$$.callRawArgs=0;$$.run = calc_run_func; } } // 用户自定义函数
 | VARIABLE '(' stmtExprArgList ')' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_T;$$.callType=USER_F;$$.callName=$1.str;$$.callArgs=$3.callArgs;$$.callRawArgs=0;$$.run = calc_run_func; } } // 用户自定义函数
;
stmtExprArgList: stmtExprArg
 | stmtExprArgList ',' stmtExprArg { if(EXPECTED(isSyntaxData)) { APPEND($$.callArgs,$3.callArgs); } }
;
stmtExprArg: stmtExpr { if(EXPECTED(isSyntaxData)) { CALL_ARGS($$.callArgs,$1); } }
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

