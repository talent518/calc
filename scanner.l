%option yywrap yylineno
%option lex-compat

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
"array"                                        { return ARRAY; }
"global"                                       { return GLOBAL_T; }
"cos"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=COS_F;yylval.call.argc=1;yylval.call.name="cos(double radian)";} return CALL; }
"pow"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=POW_F;yylval.call.argc=2;yylval.call.name="pow(double base, double exp)";} return CALL; }
"rad"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=RAD_F;yylval.call.argc=1;yylval.call.name="rad(double angle)";} return CALL; }
"rand"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=RAND_F;yylval.call.argc=0;yylval.call.name="rand()";} return CALL; }
"randf"                                        { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=RANDF_F;yylval.call.argc=0;yylval.call.name="randf()";} return CALL; }
"RAND_MAX"                                     { if(EXPECTED(isSyntaxData)) {yylval.type=LONG_T;yylval.lval=RAND_MAX;}  return CONST_RAND_MAX; }
"ret"                                          { return RET; }
"return"                                       { return RET; }
"sin"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=SIN_F;yylval.call.argc=1;yylval.call.name="sin(double radian)";} return CALL; }
"srand"                                        { return SRAND; }
"tan"                                          { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=TAN_F;yylval.call.argc=1;yylval.call.name="tan(double radian)";} return CALL; }
"acos"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=ACOS_F;yylval.call.argc=1;yylval.call.name="acos(double arg)";} return CALL; }
"asin"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=ASIN_F;yylval.call.argc=1;yylval.call.name="asin(double arg)";} return CALL; }
"atan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=ATAN_F;yylval.call.argc=1;yylval.call.name="atan(double arg)";} return CALL; }
"ctan"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=CTAN_F;yylval.call.argc=1;yylval.call.name="ctan(double arg)";} return CALL; }
"sqrt"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=FUNC_T;yylval.call.type=SQRT_F;yylval.call.argc=1;yylval.call.name="sqrt(double num)";} return CALL; }
"echo"                                         { return ECHO_T; }
"list"                                         { return LIST; }
"clear"                                        { return CLEAR; }
"endl"                                         { if(EXPECTED(isSyntaxData)) {yylval.type=STR_T;yylval.str=strdup("\n");zend_hash_next_index_insert(&frees, yylval.str, 0, NULL);} return STR; }
"func"                                         { return FUNC; }
"function"                                     { return FUNC; }
"PI"                                           { return CONST_PI; }
"if"                                           { return IF; }
"else"                                         { return ELSE; }
"while"                                        { return WHILE; }
"do"                                           { return DO; }
"break"                                        { return BREAK; }
"include"                                      { return INCLUDE; }
"TOP"                                          { /* 忽略TOP关键字 */ }
([0-9]+|[0-9]*\.[0-9]*)[FfDdIiLlRr]?           { if(EXPECTED(isSyntaxData)) {setval(&yylval);} return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*                         { if(EXPECTED(isSyntaxData)) {yylval.type=VAR_T;yylval.str=strdup(yytext);zend_hash_next_index_insert(&frees, yylval.str, 0, NULL);} return VARIABLE; }
\"([^"\\]|\\[xX][0-9a-fA-F][0-9a-fA-F]|\\[0-7][0-7][0-7]|\\['"?\\abfnrtv])*\" { yylval.type=STR_T;yylval.str=strndup(yytext+1,strlen(yytext)-2);unescape(yylval.str);zend_hash_next_index_insert(&frees, yylval.str, 0, NULL); return STR; }
"//"([^\n]+)                                   { /* 单行注释 */ }
[ \t\f\v\r\n]+                                 { /* 忽略空白 */ }

%%

int yywrap() {
	dprintf("--------------------------\n");
	dprintf("END INPUT: %s\n", curFileName);
	dprintf("==========================\n");

	if(includeDeep>0) {
		zend_hash_del(&files, curFileName, strlen(curFileName));

		includeDeep--;

		if(EXPECTED(isSyntaxData)) {
			free(curFileName);
		}

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
