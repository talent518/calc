%option yywrap yylineno
%option lex-compat

%{

#include "calc.h"

inline void unescape(char *p) {
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
">"                                            { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_GT_T;} return LOGIC; }
"<"                                            { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_LT_T;} return LOGIC; }
">="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_GE_T;} return LOGIC; }
"<="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_LE_T;} return LOGIC; }
"=="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_EQ_T;} return LOGIC; }
"!="                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_NE_T;} return LOGIC; }
"<>"                                           { if(EXPECTED(isSyntaxData)) {yylval.type=LOGIC_NE_T;} return LOGIC; }
"++"                                           { return INC; }
"--"                                           { return DEC; }
"+="                                           { return ADDEQ; }
"-="                                           { return SUBEQ; }
"*="                                           { return MULEQ; }
"/="                                           { return DIVEQ; }
"%="                                           { return MODEQ; }
"srand"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=SRAND_F;yylval.callArgc=0;yylval.callName="srand()";yylval.callRawArgs=0;} return CALL; }
"microtime"                                    { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=MICROTIME_F;yylval.callArgc=0;yylval.callName="microtime()";yylval.callRawArgs=0;} return CALL; }
"cos"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=COS_F;yylval.callArgc=1;yylval.callName="cos(double radian)";yylval.callRawArgs=0;} return CALL; }
"pow"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=POW_F;yylval.callArgc=2;yylval.callName="pow(double base, double exp)";yylval.callRawArgs=0;} return CALL; }
"rad"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=RAD_F;yylval.callArgc=1;yylval.callName="rad(double angle)";yylval.callRawArgs=0;} return CALL; }
"rand"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=RAND_F;yylval.callArgc=0;yylval.callName="rand()";yylval.callRawArgs=0;} return CALL; }
"randf"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=RANDF_F;yylval.callArgc=0;yylval.callName="randf()";yylval.callRawArgs=0;} return CALL; }
"sin"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=SIN_F;yylval.callArgc=1;yylval.callName="sin(double radian)";yylval.callRawArgs=0;} return CALL; }
"tan"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=TAN_F;yylval.callArgc=1;yylval.callName="tan(double radian)";yylval.callRawArgs=0;} return CALL; }
"acos"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=ACOS_F;yylval.callArgc=1;yylval.callName="acos(double arg)";yylval.callRawArgs=0;} return CALL; }
"asin"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=ASIN_F;yylval.callArgc=1;yylval.callName="asin(double arg)";yylval.callRawArgs=0;} return CALL; }
"atan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=ATAN_F;yylval.callArgc=1;yylval.callName="atan(double arg)";yylval.callRawArgs=0;} return CALL; }
"ctan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=CTAN_F;yylval.callArgc=1;yylval.callName="ctan(double arg)";yylval.callRawArgs=0;} return CALL; }
"sqrt"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=SQRT_F;yylval.callArgc=1;yylval.callName="sqrt(double num)";yylval.callRawArgs=0;} return CALL; }
"strlen"                                       { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.callType=STRLEN_F;yylval.callArgc=1;yylval.callName="strlen(string str)";yylval.callRawArgs=1;} return CALL; }
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
"break"                                        { return BREAK; }
"include"                                      { return INCLUDE; }
"TOP"                                          { /* 忽略TOP关键字 */ }
"int"                                          { /* 忽略int关键字 */ }
"long"                                         { /* 忽略long关键字 */ }
"float"                                        { /* 忽略float关键字 */ }
"double"                                       { /* 忽略double关键字 */ }
"string"                                       { /* 忽略string关键字 */ }
"RAND_MAX"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=LONG_T;yylval.lval=RAND_MAX;}  return CONST_RAND_MAX; }
"PI"                                           { return CONST_PI; }
"endl"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=STR_T;yylval.str=strdup("\n");zend_hash_next_index_insert(&frees, yylval.str, 0, NULL);} return STR; }
([0-9]+|[0-9]*\.[0-9]*)[FfDdIiLlRr]?           { if(EXPECTED(isSyntaxData)) {str2val(&yylval, yytext);} return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*                         { if(EXPECTED(isSyntaxData)) {yylval.type=VAR_T;yylval.str=strdup(yytext);zend_hash_next_index_insert(&frees, yylval.str, 0, NULL);} return VARIABLE; }
\"([^"\\]|\\[xX][0-9a-fA-F][0-9a-fA-F]|\\[0-7][0-7][0-7]|\\['"?\\abfnrtv])*\" { yylval.type=STR_T;yylval.str=strndup(yytext+1,strlen(yytext)-2);unescape(yylval.str);yylval.strlen=strlen(yylval.str);zend_hash_next_index_insert(&frees, yylval.str, 0, NULL); return STR; }
"//"([^\n]+)                                   { /* 单行注释 */ }
[ \t\f\v\r\n]+                                 { /* 忽略空白 */ }

%%

