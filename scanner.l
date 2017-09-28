%option noyywrap yylineno

%{

#include "calc.h"
#include <float.h>
#include <limits.h>

inline void setval(exp_val_t *val) {
	int n=strlen(yytext);
	switch(yytext[n-1]) {
		case 'I':
		case 'i':
			val->type=INT_T;
			val->ival = atol(yytext);
			break;
		case 'L':
		case 'l':
			val->type=LONG_T;
			val->lval = atol(yytext);
			break;
		case 'F':
		case 'f':
			val->type=FLOAT_T;
			val->fval = atof(yytext);
			break;
		case 'D':
		case 'd':
			val->type=DOUBLE_T;
			val->dval = atof(yytext);
			break;
		case 'R':
		case 'r':
			val->type=DOUBLE_T;
			val->dval = atof(yytext) * M_PI / 180.0;
			break;
		default:
			if(strchr(yytext,'.')) {
				double d = atof(yytext);
				if(FLT_MIN <= d && d <= FLT_MAX) {
					val->type = FLOAT_T;
					val->fval = d;
				} else {
					val->type = DOUBLE_T;
					val->dval = d;
				}
			} else {
				long int i = atoi(yytext);
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
">"                                            { yylval.type=LOGIC_GT_T;return LOGIC; }
"<"                                            { yylval.type=LOGIC_LT_T;return LOGIC; }
">="                                           { yylval.type=LOGIC_GE_T;return LOGIC; }
"<="                                           { yylval.type=LOGIC_LE_T;return LOGIC; }
"=="                                           { yylval.type=LOGIC_EQ_T;return LOGIC; }
"!="                                           { yylval.type=LOGIC_NE_T;return LOGIC; }
"<>"                                           { yylval.type=LOGIC_NE_T;return LOGIC; }
"++"                                           { return INC; }
"--"                                           { return DEC; }
"+="                                           { return ADDEQ; }
"-="                                           { return SUBEQ; }
"*="                                           { return MULEQ; }
"/="                                           { return DIVEQ; }
"%="                                           { return MODEQ; }
"array"                                        { return ARRAY; }
"global"                                       { return GLOBAL_T; }
"cos"                                          { yylval.type=FUNC_T;yylval.call.type=COS_F;yylval.call.argc=1;yylval.call.name="cos(double radian)";return CALL; }
"pow"                                          { yylval.type=FUNC_T;yylval.call.type=POW_F;yylval.call.argc=2;yylval.call.name="pow(double base, double exp)";return CALL; }
"rad"                                          { yylval.type=FUNC_T;yylval.call.type=RAD_F;yylval.call.argc=1;yylval.call.name="rad(double angle)";return CALL; }
"rand"                                         { yylval.type=FUNC_T;yylval.call.type=RAND_F;yylval.call.argc=0;yylval.call.name="rand()";return CALL; }
"randf"                                        { yylval.type=FUNC_T;yylval.call.type=RANDF_F;yylval.call.argc=0;yylval.call.name="randf()";return CALL; }
"RAND_MAX"                                     { yylval.type=LONG_T;yylval.lval=RAND_MAX; return CONST_RAND_MAX; }
"ret"                                          { return RET; }
"return"                                       { return RET; }
"sin"                                          { yylval.type=FUNC_T;yylval.call.type=SIN_F;yylval.call.argc=1;yylval.call.name="sin(double radian)";return CALL; }
"srand"                                        { return SRAND; }
"tan"                                          { yylval.type=FUNC_T;yylval.call.type=TAN_F;yylval.call.argc=1;yylval.call.name="tan(double radian)";return CALL; }
"acos"                                         { yylval.type=FUNC_T;yylval.call.type=ACOS_F;yylval.call.argc=1;yylval.call.name="acos(double arg)";return CALL; }
"asin"                                         { yylval.type=FUNC_T;yylval.call.type=ASIN_F;yylval.call.argc=1;yylval.call.name="asin(double arg)";return CALL; }
"atan"                                         { yylval.type=FUNC_T;yylval.call.type=ATAN_F;yylval.call.argc=1;yylval.call.name="atan(double arg)";return CALL; }
"ctan"                                         { yylval.type=FUNC_T;yylval.call.type=CTAN_F;yylval.call.argc=1;yylval.call.name="ctan(double arg)";return CALL; }
"sqrt"                                         { yylval.type=FUNC_T;yylval.call.type=SQRT_F;yylval.call.argc=1;yylval.call.name="sqrt(double num)";return CALL; }
"echo"                                         { return ECHO_T; }
"list"                                         { return LIST; }
"clear"                                        { return CLEAR; }
"endl"                                         { yylval.type=STR_T;yylval.str=strdup("\n");return STR; }
"func"                                         { return FUNC; }
"function"                                     { return FUNC; }
"PI"                                           { return CONST_PI; }
"if"                                           { return IF; }
"else"                                         { return ELSE; }
"while"                                        { return WHILE; }
"do"                                           { return DO; }
"break"                                        { return BREAK; }
([0-9]+|[0-9]*\.[0-9]*)[FfDdIiLlRr]?           { setval(&yylval);return NUMBER; }
[a-zA-Z][a-zA-Z0-9_]*                          { yylval.type=VAR_T;yylval.str=strdup(yytext);return VARIABLE; }
\"([^"\\]|\\['"?\\abfnrtv])*\"                 { yylval.type=STR_T;yylval.str=strndup(yytext+1,strlen(yytext)-2);return STR; }
"//"([^\n]+)                                   { /* 单行注释 */ }
[ \t\f\v\r\n]+                                 { /* 忽略空白 */ }

%%
