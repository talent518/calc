%option yywrap yylineno
%option lex-compat

%{

#include "calc.h"

int linenofunc;

zend_always_inline void unescape(char *p) {
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
"srand"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=SRAND_F;yylval.call->argc=0;yylval.call->name="srand()";yylval.run=calc_run_sys_srand;} return CALL; }
"microtime"                                    { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=MICROTIME_F;yylval.call->argc=0;yylval.call->name="microtime()";yylval.run=calc_run_sys_microtime;} return CALL; }
"cos"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=COS_F;yylval.call->argc=1;yylval.call->name="cos(double radian)";yylval.run=calc_run_sys_cos;} return CALL; }
"pow"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=POW_F;yylval.call->argc=2;yylval.call->name="pow(double base, double exp)";yylval.run=calc_run_sys_pow;} return CALL; }
"rad"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RAD_F;yylval.call->argc=1;yylval.call->name="rad(double angle)";yylval.run=calc_run_sys_rad;} return CALL; }
"rand"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RAND_F;yylval.call->argc=0;yylval.call->name="rand()";yylval.run=calc_run_sys_rand;} return CALL; }
"randf"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=RANDF_F;yylval.call->argc=0;yylval.call->name="randf()";yylval.run=calc_run_sys_randf;} return CALL; }
"sin"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=SIN_F;yylval.call->argc=1;yylval.call->name="sin(double radian)";yylval.run=calc_run_sys_sin;} return CALL; }
"tan"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=TAN_F;yylval.call->argc=1;yylval.call->name="tan(double radian)";yylval.run=calc_run_sys_tan;} return CALL; }
"acos"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=ACOS_F;yylval.call->argc=1;yylval.call->name="acos(double arg)";yylval.run=calc_run_sys_acos;} return CALL; }
"asin"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=ASIN_F;yylval.call->argc=1;yylval.call->name="asin(double arg)";yylval.run=calc_run_sys_asin;} return CALL; }
"atan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=ATAN_F;yylval.call->argc=1;yylval.call->name="atan(double arg)";yylval.run=calc_run_sys_atan;} return CALL; }
"ctan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=CTAN_F;yylval.call->argc=1;yylval.call->name="ctan(double arg)";yylval.run=calc_run_sys_ctan;} return CALL; }
"sqrt"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=SQRT_F;yylval.call->argc=1;yylval.call->name="sqrt(double num)";yylval.run=calc_run_sys_sqrt;} return CALL; }
"strlen"                                       { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;NEW_FREES(yylval.call, func_call_t);yylval.call->type=STRLEN_F;yylval.call->argc=1;yylval.call->name="strlen(string str)";yylval.run=calc_run_sys_strlen;} return CALL; }
"array"                                        { return ARRAY; }
"global"                                       { return GLOBAL_T; }
"ret"                                          { return RET; }
"return"                                       { return RET; }
"echo"                                         { return ECHO_T; }
"list"                                         { return LIST; }
"clear"                                        { return CLEAR; }
"func"                                         { linenofunc = yylineno;return FUNC; }
"function"                                     { linenofunc = yylineno;return FUNC; }
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
"RAND_MAX"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=LONG_T;yylval.lval=RAND_MAX;}  return CONST_RAND_MAX; }
"PI"                                           { return CONST_PI; }
"endl"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->c=strdup("\n");yylval.str->n=1;yylval.str->gc=0;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL);} return STR; }
([0-9]+|[0-9]*\.[0-9]*)[FfDdIiLlRr]?           { if(EXPECTED(isSyntaxData)) {str2val(&yylval, yytext);} return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*                         { if(EXPECTED(isSyntaxData)) {yylval.type=VAR_T;yylval.var=strdup(yytext);zend_hash_next_index_insert(&frees, yylval.str, 0, NULL);yylval.run = calc_run_variable;} return VARIABLE; }
\"([^"\\]|\\[xX][0-9a-fA-F][0-9a-fA-F]|\\[0-7][0-7][0-7]|\\['"?\\abfnrtv])*\" { yylval.type=STR_T;NEW_FREES(yylval.str,string_t);yylval.str->c=strndup(yytext+1,strlen(yytext)-2);unescape(yylval.str->c);yylval.str->n=strlen(yylval.str->c);yylval.str->gc=0;zend_hash_next_index_insert(&frees, yylval.str->c, 0, NULL); return STR; }
"//"([^\n]+)                                   { /* 单行注释 */ }
[ \t\f\v\r\n]+                                 { /* 忽略空白 */ }

%%
