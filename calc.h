#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <complex.h>

#include "zend_hash.h"

typedef enum _type_enum_t {
	NULL_T, // 空
	INT_T, // 整型
	LONG_T, // 长整型
	FLOAT_T, // 浮点型
	DOUBLE_T, // 双精度型
	VAR_T, // 变量
	STR_T, // 字符串
	FUNC_T, // 函数
	FUNC_CALL_T, // 函数调用参数
	FUNC_DEF_T, // 函数定义信息
	ADD_T, // 加法
	SUB_T, // 减法
	MUL_T, // 乘法
	DIV_T, // 除法
	MOD_T, // 求余
	POW_T, // b的n次方
	ABS_T, // 绝对值
	MINUS_T, // 负号
	LOGIC_GT_T, // 大于 left > right
	LOGIC_LT_T, // 小于 left < right
	LOGIC_GE_T, // 大于等于 left >= right
	LOGIC_LE_T, // 小于等于 left <= right
	LOGIC_EQ_T, // 等于 left == right
	LOGIC_NE_T, // 不等于 left != right
	IF_T, // 三目运算 (cond?left:right)
	ARRAY_T // 数组类型
} type_enum_t;

typedef enum _call_enum_f {
	SIN_F, ASIN_F, COS_F, ACOS_F, TAN_F, ATAN_F, CTAN_F, SQRT_F, POW_F, RAD_F, RAND_F, RANDF_F, USER_F
} call_enum_f;

typedef enum _symbol_enum_t {
	ECHO_STMT_T, // echo
	RET_STMT_T, // ret
	ASSIGN_STMT_T, // var = expr
	IF_STMT_T, // if(expr) { stmtlist } else { stmtlist }
	WHILE_STMT_T, // while(expr) { stmtlist }
	DO_WHILE_STMT_T, // do { stmtlist } while(expr)
	BREAK_STMT_T, // break;
	ARRAY_STMT_T, // array var[int][int]...
	INC_STMT_T, // increment var++
	DEC_STMT_T, // decrement var--
	ADDEQ_STMT_T, // var += val
	SUBEQ_STMT_T, // var -= val
	MULEQ_STMT_T, // var *= val
	DIVEQ_STMT_T, // var /= val
	MODEQ_STMT_T, // var %= val
	NULL_STMT_T
} symbol_enum_t;

typedef enum _status_enum_t {
	RET_STATUS, BREAK_STATUS, NONE_STATUS
} status_enum_t;

typedef struct _call_args_t call_args_t;
typedef struct _func_args_t func_args_t;
typedef struct _func_symbol_t func_symbol_t;
typedef struct _exp_val_t exp_val_t;
typedef struct _func_def_f func_def_f;
typedef struct _func_call_f func_call_f;

struct _func_call_f {
	call_enum_f type;
	char *name;
	unsigned argc;
	call_args_t *args;
};

struct _func_def_f {
	char *name;
	unsigned argc;
	unsigned minArgc;
	func_args_t *args;
	func_symbol_t *syms;
};

struct _exp_val_t {
	type_enum_t type;
	union {
		int ival;
		long int lval;
		float fval;
		double dval;
		char *str;
		func_call_f call;
		func_def_f def;
		struct {
			struct _exp_val_t *cond;
			struct _exp_val_t *left;
			struct _exp_val_t *right;
		};
		struct {
			unsigned int arrlen;
			struct _exp_val_t *array;
			call_args_t *arrayArgs;
		};
	};
};

struct _call_args_t {
	exp_val_t val;
	struct _call_args_t *next;
	struct _call_args_t *tail;
};

struct _func_args_t {
	char *name;
	exp_val_t val;
	struct _func_args_t *next;
	struct _func_args_t *tail;
};

struct _func_symbol_t {
	symbol_enum_t type;
	union {
		call_args_t *args;
		exp_val_t *expr;
		struct {
			exp_val_t *var;
			exp_val_t *val;
		};
		struct {
			exp_val_t *cond;
			func_symbol_t *lsyms;
			func_symbol_t *rsyms;
		};
	};
	unsigned int lineno;
	struct _func_symbol_t *next;
	struct _func_symbol_t *tail;
};

extern int yylineno;
extern size_t yyleng;
extern char *types[];
extern HashTable vars;
extern HashTable funcs;

void calc_add(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2);
void calc_sub(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2);
void calc_mul(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2);
void calc_div(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2);
void calc_mod(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2);
void calc_abs(exp_val_t *dst, exp_val_t *src);
void calc_minus(exp_val_t *dst, exp_val_t *src);
void calc_pow(exp_val_t *dst, exp_val_t *op1, exp_val_t *op2);
void calc_sqrt(exp_val_t *dst, exp_val_t *src);
void calc_echo(exp_val_t *src);
void calc_sprintf(char *buf, exp_val_t *src);
void calc_print(char *p);
int calc_clear_or_list_vars(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key);
void calc_func_print(func_def_f *def);
void calc_func_def(func_def_f *def);
void calc_free_args(call_args_t *args);
void calc_free_syms(func_symbol_t *syms);
void calc_free_func(func_def_f *def);
void calc_run_expr(exp_val_t *ret, exp_val_t *expr);
void calc_free_expr(exp_val_t *expr);
status_enum_t calc_run_syms(exp_val_t *ret, func_symbol_t *syms);
void call_func_run(exp_val_t *ret, func_def_f *def, call_args_t *args);
void calc_call(exp_val_t *ret, call_enum_f ftype, char *name, unsigned argc, call_args_t *args);

void call_free_vars(exp_val_t *expr);
void calc_array_free(exp_val_t *ptr, call_args_t *args);

void seed_rand();
void yyerror(char *s, ...);

#define YYSTYPE exp_val_t

//#define DEBUG

#ifdef DEBUG
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

int yylex (void);
int yylex_destroy();

#include "parser.h"

