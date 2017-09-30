/* simplest version of calculator */
%{

#include "calc.h"

#define ASSIGNMENT_STATEMENT(var,expr) zend_hash_update(&vars, var->str, strlen(var->str), expr, sizeof(exp_val_t), NULL);

#define MEMDUP(dst,src,type) dst=(type*)malloc(sizeof(type));memcpy(dst,src,sizeof(type));zend_hash_next_index_insert(&frees, dst, 0, NULL)
#define CALL_ARGS(args,v) args=malloc(sizeof(call_args_t));args->val=v;args->tail=args;args->next=NULL;zend_hash_next_index_insert(&frees, args, 0, NULL)
#define FUNC_ARGS(args,v) args=malloc(sizeof(func_args_t));args->val=v;args->tail=args;args->next=NULL;zend_hash_next_index_insert(&frees, args, 0, NULL)
#define APPEND(args,val) args->tail->next=val;args->tail=val
#define STMT(o,t,k,v) o.def.syms=malloc(sizeof(func_symbol_t));o.def.syms->type=t;o.def.syms->k=v;o.def.syms->lineno=yylineno;o.def.syms->tail=o.def.syms;o.def.syms->next=NULL;zend_hash_next_index_insert(&frees, o.def.syms, 0, NULL)

void yypush_buffer_state ( void* );
void* yy_create_buffer ( FILE *file, int size );

wrap_stack_t *curWrapStack = NULL;
wrap_stack_t *tailWrapStack = NULL;
unsigned int includeDeep = 0;
char *curFileName = "-";
%}

/* declare tokens */
%token EOL
%token CONST_PI NUMBER CONST_RAND_MAX
%token LIST CLEAR SRAND
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
 | calclist FUNC VARIABLE '(' ')' '{' '}' { if(EXPECTED(isSyntaxData)) { $$.def.name = $3.str;$$.def.args=NULL;$$.def.syms=NULL;calc_func_def(&($$.def));zend_hash_clean(&frees); zend_hash_clean(&frees); } }
 | calclist FUNC VARIABLE '(' ')' '{' funcStmtList '}' { if(EXPECTED(isSyntaxData)) { $$.def.name = $3.str;$$.def.args=NULL;$$.def.syms=$7.def.syms;calc_func_def(&($$.def)); zend_hash_clean(&frees); } }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' '}' { if(EXPECTED(isSyntaxData)) { $$.def.name = $3.str;$$.def.args=$5.def.args;$$.def.syms=NULL;calc_func_def(&($$.def)); zend_hash_clean(&frees); } }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' funcStmtList '}' { if(EXPECTED(isSyntaxData)) { $$.def.name = $3.str;$$.def.args=$5.def.args;$$.def.syms=$8.def.syms;calc_func_def(&($$.def)); zend_hash_clean(&frees); } }
 | calclist stmtList { if(EXPECTED(isSyntaxData)) { if(topSyms) {APPEND(topSyms,$2.def.syms); } else { topSyms = $2.def.syms; } zend_hash_clean(&frees); } }
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
 | funcArgList ',' funcArg { if(EXPECTED(isSyntaxData)) { APPEND($$.def.args,$3.def.args); } }
;
funcArg: VARIABLE '=' number { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_DEF_T;FUNC_ARGS($$.def.args,$3);$$.def.args->name = $1.str; } }
 | VARIABLE { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_DEF_T;$1.type=NULL_T;FUNC_ARGS($$.def.args,$1);$$.def.args->name = $1.str; } }
;
/************************ 函数语句语法 ************************/
funcStmtList: funcStmt
 | funcStmtList funcStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.def.syms,$2.def.syms); } }
;
funcStmt: stmt
 | GLOBAL_T varArgList ';' { if(EXPECTED(isSyntaxData)) { STMT($$,GLOBAL_T,args,$2.call.args); } }
;
breakStmtList: breakStmt
 | breakStmtList breakStmt { if(EXPECTED(isSyntaxData)) { APPEND($$.def.syms,$2.def.syms); } }
;
breakStmt: stmt
 | BREAK ';' { if(EXPECTED(isSyntaxData)) { STMT($$,BREAK_STMT_T,args,NULL); } }
;
stmtList: stmt
 | stmtList stmt { if(EXPECTED(isSyntaxData)) { APPEND($$.def.syms,$2.def.syms); } }
;
stmt: ECHO_T echoArgList ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ECHO_STMT_T,args,$2.call.args); } }
 | RET stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,RET_STMT_T,expr,NULL);MEMDUP($$.def.syms->expr,&$2,exp_val_t); } }
 | VARIABLE '=' stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t); }  }
 | IF '(' stmtExpr ')' '{' stmtList '}' %prec IFX { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$3,exp_val_t);$$.def.syms->lsyms=$6.def.syms;$$.def.syms->rsyms=NULL; } }
 | IF '(' stmtExpr ')' '{' stmtList '}' ELSE '{' stmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$3,exp_val_t);$$.def.syms->lsyms=$6.def.syms;$$.def.syms->rsyms=$10.def.syms; } }
 | WHILE '(' stmtExpr ')' '{' breakStmtList '}' { if(EXPECTED(isSyntaxData)) { STMT($$,WHILE_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$3,exp_val_t);$$.def.syms->lsyms=$6.def.syms;$$.def.syms->rsyms=NULL; } }
 | DO '{' breakStmtList '}' WHILE '(' stmtExpr ')' ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DO_WHILE_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$7,exp_val_t);$$.def.syms->lsyms=$3.def.syms;$$.def.syms->rsyms=NULL; } }
 | LIST ';' { if(EXPECTED(isSyntaxData)) { STMT($$,LIST_STMT_T,args,NULL); } }
 | CLEAR ';' { if(EXPECTED(isSyntaxData)) { STMT($$,CLEAR_STMT_T,args,NULL); } }
 | ARRAY VARIABLE arrayArgList ';' { if(EXPECTED(isSyntaxData)) { CALL_ARGS($1.call.args,$2);$1.call.args->next=$3.call.args;STMT($$,ARRAY_STMT_T,args,$1.call.args); } }
 | VARIABLE arrayArgList '=' stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; } }
 | VARIABLE INC ';' { if(EXPECTED(isSyntaxData)) { STMT($$,INC_STMT_T,val,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t); } }
 | VARIABLE DEC ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DEC_STMT_T,val,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t); } }
 | VARIABLE ADDEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ADDEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t); } }
 | VARIABLE SUBEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,SUBEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t); } }
 | VARIABLE MULEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MULEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t); } }
 | VARIABLE DIVEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DIVEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t); } }
 | VARIABLE MODEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MODEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t); } }
 | VARIABLE arrayArgList ADDEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,ADDEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; } }
 | VARIABLE arrayArgList SUBEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,SUBEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; } }
 | VARIABLE arrayArgList MULEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MULEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; } }
 | VARIABLE arrayArgList DIVEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,DIVEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; } }
 | VARIABLE arrayArgList MODEQ stmtExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,MODEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; } }
 | callExpr ';' { if(EXPECTED(isSyntaxData)) { STMT($$,FUNC_STMT_T,args,NULL);MEMDUP($$.def.syms->expr,&$1,exp_val_t); } }
 | SRAND '(' ')' ';' { if(EXPECTED(isSyntaxData)) { STMT($$,SRAND_STMT_T,args,NULL); } } // 空语句
 | ';' { if(EXPECTED(isSyntaxData)) { STMT($$,NULL_STMT_T,args,NULL); } } // 空语句
;
varArgList: varArg
 | varArgList ',' varArg { if(EXPECTED(isSyntaxData)) { APPEND($$.call.args,$3.call.args); } }
;
varArg: VARIABLE { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); } }
;
arrayArgList: arrayArg
 | arrayArgList arrayArg { if(EXPECTED(isSyntaxData)) { APPEND($$.call.args,$2.call.args); } }
;
arrayArg: '[' stmtExpr ']' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$2); } }
;
echoArgList: echoArg
 | echoArgList ',' echoArg { if(EXPECTED(isSyntaxData)) { APPEND($$.call.args,$3.call.args); } }
;
echoArg: stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); } }
 | STR { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); } }
;
stmtExpr: VARIABLE
 | number
 | '-' stmtExpr %prec UMINUS { if(EXPECTED(isSyntaxData)) { $$.type=MINUS_T;MEMDUP($$.left,&$2,exp_val_t);$$.right=NULL; } }
 | stmtExpr '+' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=ADD_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr '-' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=SUB_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr '*' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=MUL_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr '/' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=DIV_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr '%' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=MOD_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr '^' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=POW_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr LOGIC stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=$2.type;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); } }
 | stmtExpr '?' stmtExpr ':' stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=IF_T;MEMDUP($$.cond,&$1,exp_val_t);MEMDUP($$.left,&$3,exp_val_t);MEMDUP($$.right,&$5,exp_val_t); } }
 | '|' stmtExpr '|' { if(EXPECTED(isSyntaxData)) { $$.type=ABS_T;MEMDUP($$.left,&$2,exp_val_t);$$.right=NULL; } }
 | '(' stmtExpr ')'   { if(EXPECTED(isSyntaxData)) { $$ = $2; } } // 括号
 | VARIABLE arrayArgList { if(EXPECTED(isSyntaxData)) { $$.type=ARRAY_T;$$.call.name=$1.str;$$.call.args=$2.call.args; } } // 用户自定义函数
 | CALL '(' ')' { if(EXPECTED(isSyntaxData)) { $$=$1;$$.call.args=NULL; } } // 系统函数
 | CALL '(' stmtExprArgList ')' { if(EXPECTED(isSyntaxData)) { $$=$1;$$.call.args=$3.call.args; } } // 系统函数
 | callExpr
;
callExpr: VARIABLE '(' ')' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_T;$$.call.type=USER_F;$$.call.name=$1.str;$$.call.args=NULL; } } // 用户自定义函数
 | VARIABLE '(' stmtExprArgList ')' { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_T;$$.call.type=USER_F;$$.call.name=$1.str;$$.call.args=$3.call.args; } } // 用户自定义函数
;
stmtExprArgList: stmtExprArg
 | stmtExprArgList ',' stmtExprArg { if(EXPECTED(isSyntaxData)) { APPEND($$.call.args,$3.call.args); } }
;
stmtExprArg: stmtExpr { if(EXPECTED(isSyntaxData)) { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); } }
;
number: NUMBER
 | CONST_RAND_MAX
 | CONST_PI { if(EXPECTED(isSyntaxData)) {$$.type=DOUBLE_T;$$.dval=atof("3.141592653589793238462643383279502884197169"); } } // PI
;

%%

