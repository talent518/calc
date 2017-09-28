/* simplest version of calculator */
%{

#include "calc.h"

#define ASSIGNMENT_STATEMENT(var,expr) zend_hash_update(&vars, var->str, strlen(var->str), expr, sizeof(exp_val_t), NULL);

#define MEMDUP(dst,src,type) dst=(type*)malloc(sizeof(type));memcpy(dst,src,sizeof(type))
#define CALL_ARGS(args,v) args=malloc(sizeof(call_args_t));args->val=v;args->tail=args;args->next=NULL
#define FUNC_ARGS(args,v) args=malloc(sizeof(func_args_t));args->val=v;args->tail=args;args->next=NULL
#define APPEND(args,val) args->tail->next=val;args->tail=val
#define STMT(o,t,k,v) o.def.syms=malloc(sizeof(func_symbol_t));o.def.syms->type=t;o.def.syms->k=v;o.def.syms->lineno=yylineno;o.def.syms->tail=o.def.syms;o.def.syms->next=NULL

%}

/* declare tokens */
%token EOL
%token CONST_PI NUMBER CONST_RAND_MAX
%token LIST CLEAR SRAND
%token CALL FUNC
%token VARIABLE STR
%token ECHO_T RET IF ELSE WHILE DO BREAK ARRAY
%token INC DEC
%token ADDEQ SUBEQ MULEQ DIVEQ MODEQ

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
 | calclist LIST ';' { printf("list\n");zend_hash_apply_with_arguments(&vars, (apply_func_args_t)calc_clear_or_list_vars, 1, ZEND_HASH_APPLY_KEEP); }
 | calclist CLEAR ';' { printf("clear\n");zend_hash_apply_with_arguments(&vars, (apply_func_args_t)calc_clear_or_list_vars, 1, ZEND_HASH_APPLY_REMOVE); }
 | calclist VARIABLE '=' expr ';' { ASSIGNMENT_STATEMENT((&$2),(&$4));free($2.str); }
 | calclist ECHO_T echo ';' { /* echo expr/str */ }
 | calclist FUNC VARIABLE '(' ')' '{' '}' { $$.def.name = $3.str;$$.def.args=NULL;$$.def.syms=NULL;calc_func_def(&($$.def)); }
 | calclist FUNC VARIABLE '(' ')' '{' stmtList '}' { $$.def.name = $3.str;$$.def.args=NULL;$$.def.syms=$7.def.syms;calc_func_def(&($$.def)); }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' '}' { $$.def.name = $3.str;$$.def.args=$5.def.args;$$.def.syms=NULL;calc_func_def(&($$.def)); }
 | calclist FUNC VARIABLE '(' funcArgList ')' '{' stmtList '}' { $$.def.name = $3.str;$$.def.args=$5.def.args;$$.def.syms=$8.def.syms;calc_func_def(&($$.def)); }
 | calclist SRAND ';' { seed_rand(); }
 | calclist SRAND '(' ')' ';' { seed_rand(); }
 | calclist CALL '(' ')' ';' { printf("warning: System function %s is not in this call\n", $$.call.name); } // 系统函数
 | calclist CALL '(' argList ')' ';' { printf("warning: System function %s is not in this call\n", $$.call.name); } // 系统函数
 | calclist VARIABLE '(' ')' ';' { calc_call(&$$,USER_F,$2.str,0,NULL);free($2.str); } // 用户自定义函数
 | calclist VARIABLE '(' argList ')' ';' { calc_call(&$$,USER_F,$2.str,0,$4.call.args);free($2.str);calc_free_args($4.call.args); } // 用户自定义函数
;

/************************ 函数语法 ************************/
funcArgList:  funcArg
 | funcArgList ',' funcArg { APPEND($$.def.args,$3.def.args); }
;
funcArg: VARIABLE '=' number { $$.type=FUNC_DEF_T;FUNC_ARGS($$.def.args,$3);$$.def.args->name = $1.str; }
 | VARIABLE { $$.type=FUNC_DEF_T;$1.type=NULL_T;FUNC_ARGS($$.def.args,$1);$$.def.args->name = $1.str; }
;
stmtList: stmt
 | stmtList stmt { APPEND($$.def.syms,$2.def.syms); }
;
stmt: ECHO_T echoArgList ';' { STMT($$,ECHO_STMT_T,args,$2.call.args); }
 | RET stmtExpr ';' { STMT($$,RET_STMT_T,expr,NULL);MEMDUP($$.def.syms->expr,&$2,exp_val_t); }
 | VARIABLE '=' stmtExpr ';' { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t);  }
 | IF '(' stmtExpr ')' '{' stmtList '}' %prec IFX { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$3,exp_val_t);$$.def.syms->lsyms=$6.def.syms;$$.def.syms->rsyms=NULL; }
 | IF '(' stmtExpr ')' '{' stmtList '}' ELSE '{' stmtList '}' { STMT($$,IF_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$3,exp_val_t);$$.def.syms->lsyms=$6.def.syms;$$.def.syms->rsyms=$10.def.syms; }
 | WHILE '(' stmtExpr ')' '{' stmtList '}' { STMT($$,WHILE_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$3,exp_val_t);$$.def.syms->lsyms=$6.def.syms;$$.def.syms->rsyms=NULL; }
 | DO '{' stmtList '}' WHILE '(' stmtExpr ')' ';' { STMT($$,DO_WHILE_STMT_T,cond,NULL);MEMDUP($$.def.syms->cond,&$7,exp_val_t);$$.def.syms->lsyms=$3.def.syms;$$.def.syms->rsyms=NULL; }
 | BREAK ';' { STMT($$,BREAK_STMT_T,args,NULL); }
 | ARRAY VARIABLE arrayArgList ';' { CALL_ARGS($1.call.args,$2);$1.call.args->next=$3.call.args;STMT($$,ARRAY_STMT_T,args,$1.call.args); }
 | VARIABLE arrayArgList '=' stmtExpr ';' { STMT($$,ASSIGN_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; }
 | VARIABLE INC ';' { STMT($$,INC_STMT_T,val,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t); }
 | VARIABLE DEC ';' { STMT($$,DEC_STMT_T,val,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t); }
 | VARIABLE ADDEQ stmtExpr ';' { STMT($$,ADDEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t);  }
 | VARIABLE SUBEQ stmtExpr ';' { STMT($$,SUBEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t);  }
 | VARIABLE MULEQ stmtExpr ';' { STMT($$,MULEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t);  }
 | VARIABLE DIVEQ stmtExpr ';' { STMT($$,DIVEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t);  }
 | VARIABLE MODEQ stmtExpr ';' { STMT($$,MODEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$3,exp_val_t);  }
 | VARIABLE arrayArgList ADDEQ stmtExpr ';' { STMT($$,ADDEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; }
 | VARIABLE arrayArgList SUBEQ stmtExpr ';' { STMT($$,SUBEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; }
 | VARIABLE arrayArgList MULEQ stmtExpr ';' { STMT($$,MULEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; }
 | VARIABLE arrayArgList DIVEQ stmtExpr ';' { STMT($$,DIVEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; }
 | VARIABLE arrayArgList MODEQ stmtExpr ';' { STMT($$,MODEQ_STMT_T,var,NULL);MEMDUP($$.def.syms->var,&$1,exp_val_t);MEMDUP($$.def.syms->val,&$4,exp_val_t);$$.def.syms->var->type=ARRAY_T;$$.def.syms->var->call.name=$1.str;$$.def.syms->var->call.args=$2.call.args; }
 | ';' { STMT($$,NULL_STMT_T,args,NULL); } // 空语句
;
arrayArgList: arrayArg
 | arrayArgList arrayArg { APPEND($$.call.args,$2.call.args); }
;
arrayArg: '[' stmtExpr ']' { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$2); }
;
echoArgList: echoArg
 | echoArgList ',' echoArg { APPEND($$.call.args,$3.call.args); }
;
echoArg: stmtExpr { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); }
 | STR { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); }
;
stmtExpr: VARIABLE
 | number
 | '-' stmtExpr %prec UMINUS { $$.type=MINUS_T;MEMDUP($$.left,&$2,exp_val_t);$$.right=NULL; }
 | stmtExpr '+' stmtExpr { $$.type=ADD_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr '-' stmtExpr { $$.type=SUB_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr '*' stmtExpr { $$.type=MUL_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr '/' stmtExpr { $$.type=DIV_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr '%' stmtExpr { $$.type=MOD_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr '^' stmtExpr { $$.type=POW_T;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr LOGIC stmtExpr { $$.type=$2.type;MEMDUP($$.left,&$1,exp_val_t);MEMDUP($$.right,&$3,exp_val_t); }
 | stmtExpr '?' stmtExpr ':' stmtExpr { $$.type=IF_T;MEMDUP($$.cond,&$1,exp_val_t);MEMDUP($$.left,&$3,exp_val_t);MEMDUP($$.right,&$5,exp_val_t); }
 | '|' stmtExpr '|' { $$.type=ABS_T;MEMDUP($$.left,&$2,exp_val_t);$$.right=NULL; }
 | '(' stmtExpr ')'   { $$ = $2; } // 括号
 | VARIABLE arrayArgList { $$.type=ARRAY_T;$$.call.name=$1.str;$$.call.args=$2.call.args; } // 用户自定义函数
 | CALL '(' ')' { $$=$1;$$.call.args=NULL; } // 系统函数
 | CALL '(' stmtExprArgList ')' { $$=$1;$$.call.args=$3.call.args; } // 系统函数
 | VARIABLE '(' ')' { $$.type=FUNC_T;$$.call.type=USER_F;$$.call.name=$1.str;$$.call.args=NULL; } // 用户自定义函数
 | VARIABLE '(' stmtExprArgList ')' { $$.type=FUNC_T;$$.call.type=USER_F;$$.call.name=$1.str;$$.call.args=$3.call.args; } // 用户自定义函数
;
stmtExprArgList: stmtExprArg
 | stmtExprArgList ',' stmtExprArg { APPEND($$.call.args,$3.call.args); }
;
stmtExprArg: stmtExpr { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); }
;

/************************ 顶级语法 ************************/
echo: expr { calc_echo(&$1); }
 | STR { calc_print($1.str);free($1.str); }
 | echo ',' expr { calc_echo(&$3); }
 | echo ',' STR { calc_print($3.str);free($3.str); }
;
expr: value
 | '-' expr %prec UMINUS { calc_minus(&$$, &$2); }
 | expr '+' expr { calc_add(&$$, &$1, &$3); } // 加法
 | expr '-' expr { calc_sub(&$$, &$1, &$3); } // 减法
 | expr '*' expr { calc_mul(&$$, &$1, &$3); } // 乘法
 | expr '/' expr { calc_div(&$$, &$1, &$3); } // 除法
 | expr '%' expr { calc_mod(&$$, &$1, &$3); } // 求余
 | expr '^' expr { calc_pow(&$$, &$1, &$3); } // 乘方
 | '|' expr '|' { calc_abs(&$$, &$2); } // 绝对值
 | '(' expr ')'   { $$ = $2; } // 括号
 | CALL '(' ')' { calc_call(&$$,$1.call.type,$1.call.name,$1.call.argc,NULL); } // 系统函数
 | CALL '(' argList ')' { calc_call(&$$,$1.call.type,$1.call.name,$1.call.argc,$3.call.args);calc_free_args($3.call.args); } // 系统函数
 | VARIABLE '(' ')' { calc_call(&$$,USER_F,$1.str,0,NULL);free($1.str); } // 用户自定义函数
 | VARIABLE '(' argList ')' { calc_call(&$$,USER_F,$1.str,0,$3.call.args);free($1.str);calc_free_args($3.call.args); } // 用户自定义函数
;
argList: arg
 | argList ',' arg { call_args_t *args=$$.call.args;while(args->next){args=args->next;}args->next=$3.call.args; }
;
arg: expr { $$.type=FUNC_CALL_T;CALL_ARGS($$.call.args,$1); }
;
value: number
 | VARIABLE { exp_val_t *ptr=NULL;zend_hash_find(&vars, $1.str, strlen($1.str), (void**)&ptr);free($1.str);if(ptr) {memcpy(&$$, ptr, sizeof(exp_val_t));} else {memset(&$$, 0, sizeof(exp_val_t));} free($1.str); }
;
number: NUMBER
 | CONST_RAND_MAX
 | CONST_PI { $$.type=DOUBLE_T;$$.dval=atof("3.141592653589793238462643383279502884197169"); } // PI
;

%%

