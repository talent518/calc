%option yywrap yylineno
%option lex-compat

%{
#include "calc.h"
#include "parser.h"

void comment_str(char *s) {
	register char *p = s;

	while(*p) {
	}
}

%}

%%

"+" |
"-" |
"*" |
"/" |
"%" |
"(" |
")" |
"{" |
"}" |
"[" |
"]" |
"|" |
"^" |
";" |
"=" |
"?" |
":" |
","                                            { return yytext[0]; }
">"                                            { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_GT_T;yylval.run = calc_run_gt;} return LOGIC; }
"<"                                            { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_LT_T;yylval.run = calc_run_lt;} return LOGIC; }
">="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_GE_T;yylval.run = calc_run_ge;} return LOGIC; }
"<="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_LE_T;yylval.run = calc_run_le;} return LOGIC; }
"=="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_EQ_T;yylval.run = calc_run_eq;} return LOGIC; }
"!="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_NE_T;yylval.run = calc_run_ne;} return LOGIC; }
"<>"                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_NE_T;yylval.run = calc_run_ne;} return LOGIC; }
"++"                                           { return INC; }
"--"                                           { return DEC; }
"+="                                           { return ADDEQ; }
"-="                                           { return SUBEQ; }
"*="                                           { return MULEQ; }
"/="                                           { return DIVEQ; }
"%="                                           { return MODEQ; }
"^="                                           { return POWEQ; }
"runfile"                                      { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RUNFILE_F;yylval.call->argc=1;CNEW1(yylval.call->ht, HashTable);zend_hash_init(yylval.call->ht, 2, NULL);zend_hash_copy(yylval.call->ht, &files, NULL, 1, 0);append_pool(yylval.call->ht, (dtor_func_t)zend_hash_destroy_ptr);yylval.run=calc_run_sys_runfile;} return CALL; }
"srand"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=SRAND_F;yylval.call->argc=0;yylval.run=calc_run_sys_srand;} return CALL; }
"microtime"                                    { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=MICROTIME_F;yylval.call->argc=0;yylval.run=calc_run_sys_microtime;} return CALL; }
"cos"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=COS_F;yylval.call->argc=1;yylval.run=calc_run_sys_cos;} return CALL; }
"pow"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=POW_F;yylval.call->argc=2;yylval.run=calc_run_sys_pow;} return CALL; }
"rad"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RAD_F;yylval.call->argc=1;yylval.run=calc_run_sys_rad;} return CALL; }
"rand"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RAND_F;yylval.call->argc=0;yylval.run=calc_run_sys_rand;} return CALL; }
"randf"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RANDF_F;yylval.call->argc=0;yylval.run=calc_run_sys_randf;} return CALL; }
"sin"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=SIN_F;yylval.call->argc=1;yylval.run=calc_run_sys_sin;} return CALL; }
"tan"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=TAN_F;yylval.call->argc=1;yylval.run=calc_run_sys_tan;} return CALL; }
"acos"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=ACOS_F;yylval.call->argc=1;yylval.run=calc_run_sys_acos;} return CALL; }
"asin"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=ASIN_F;yylval.call->argc=1;yylval.run=calc_run_sys_asin;} return CALL; }
"atan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=ATAN_F;yylval.call->argc=1;yylval.run=calc_run_sys_atan;} return CALL; }
"ctan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=CTAN_F;yylval.call->argc=1;yylval.run=calc_run_sys_ctan;} return CALL; }
"sqrt"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=SQRT_F;yylval.call->argc=1;yylval.run=calc_run_sys_sqrt;} return CALL; }
"strlen"                                       { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=STRLEN_F;yylval.call->argc=1;yylval.run=calc_run_sys_strlen;} return CALL; }
"passthru"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=PASSTHRU_F;yylval.call->argc=0;yylval.run=calc_run_sys_passthru;} return CALL; }
"const"                                        { return CONST; }
"go"                                           { return RUN; }
"run"                                          { return RUN; }
"exit"                                         { return EXIT; }
"array"                                        { return ARRAY; }
"global"                                       { return GLOBAL_T; }
"ret"                                          { return RET; }
"return"                                       { return RET; }
"echo"                                         { return ECHO_T; }
"list"                                         { return LIST; }
"clear"                                        { return CLEAR; }
"func"                                         { return FUNC; }
"function"                                     { return FUNC; }
"if"                                           { return IF; }
"else"                                         { return ELSE; }
"while"                                        { return WHILE; }
"do"                                           { return DO; }
"for"                                          { return FOR; }
"break"                                        { return BREAK; }
"switch"                                       { return SWITCH; }
"case"                                         { return CASE; }
"default"                                      { return DEFAULT; }
"include"                                      { return INCLUDE; }
"TOP"                                          { /* 忽略TOP关键字 */ }
"int"                                          { /* 忽略int关键字 */ }
"long"                                         { /* 忽略long关键字 */ }
"float"                                        { /* 忽略float关键字 */ }
"double"                                       { /* 忽略double关键字 */ }
"string"                                       { /* 忽略string关键字 */ }
"NULL"                                         { if(EXPECTED(isSyntaxData)) {memset(&yylval, 0, sizeof(exp_val_t));} return NUMBER; }
"__FUNC__"                                     { if(EXPECTED(isSyntaxData)) {if(!linenofuncname){memset(&yylval, 0, sizeof(exp_val_t));return NUMBER;}yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->c=strdup(linenofuncname);yylval.str->n=strlen(linenofuncname);yylval.str->gc=1;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL);}  return STR; }
"__FLINE__"                                    { if(EXPECTED(isSyntaxData)) {yylval.type=INT_T;yylval.ival=linenofunc;} return NUMBER; }
"__FILE__"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->c=strdup(curFileName);yylval.str->n=strlen(curFileName);yylval.str->gc=1;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL);}  return STR; }
"__LINE__"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=INT_T;yylval.ival=yylineno;} return NUMBER; }
"RAND_MAX"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=LONG_T;yylval.lval=RAND_MAX;} return NUMBER; }
"PI"                                           { if(EXPECTED(isSyntaxData)) {yylval.type=DOUBLE_T;yylval.dval=atof("3.141592653589793238462643383279502884197169");} return NUMBER; }
"endl"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->c=strdup("\n");yylval.str->n=1;yylval.str->gc=1;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL);} return STR; }
([0-9]+|[0-9]*\.[0-9]*)[FfDdIiLlRr]?           { if(EXPECTED(isSyntaxData)) {str2val(&yylval, yytext);} return NUMBER; }
[A-Z_][A-Z0-9_]*                               { if(EXPECTED(isSyntaxData)) {exp_val_t *ptr=NULL;memset(&yylval, 0, sizeof(exp_val_t));zend_hash_find(&consts, yytext, strlen(yytext), (void**)&ptr);if(ptr) {memcpy(&yylval, ptr, sizeof(exp_val_t));ref_expr(&yylval);}} return yylval.type==STR_T?STR:NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*                         { if(EXPECTED(isSyntaxData)) {yylval.type=VAR_T;NEW_FREES(yylval.var,var_t);yylval.var->c=strdup(yytext);yylval.var->n=strlen(yytext);yylval.var->h=zend_hash_func(yylval.var->c,yylval.var->n);zend_hash_next_index_insert(&frees, yylval.var->c, 0, NULL);yylval.run = calc_run_variable;} return VARIABLE; }
\"([^"\\]|\\[xX][0-9a-fA-F][0-9a-fA-F]|\\[0-7][0-7][0-7]|\\['"?\\abfnrtv])*\" { yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->c=strndup(yytext+1,strlen(yytext)-2);unescape(yylval.str->c);yylval.str->n=strlen(yylval.str->c);yylval.str->gc=1;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL); return STR; }
"/**\n"(" *"[^\/]([^\n]+\n))*" */"             { if(EXPECTED(isSyntaxData)) {yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->n=strlen(yytext)-8;yylval.str->c=strndup(yytext+4,yylval.str->n);yylval.str->gc=1;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL);} return COMMENT; }
"//"([^\n]+)                                   { /* 单行注释 */ }
[ \t\f\v\r\n]+                                 { /* 忽略空白 */ }

%%

YY_BUFFER_STATE yy_current_buffer() {
	YY_CURRENT_BUFFER->yy_buf_pos = yy_c_buf_p + 1;
	return YY_CURRENT_BUFFER;
}
