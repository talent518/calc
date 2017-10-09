#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <complex.h>
#include <float.h>
#include <limits.h>

//#define inline __inline

#include "zend_hash.h"

#define max(a, b) ((a)>(b) ? (a) : (b))

typedef enum _type_enum_t {
	NULL_T=0, // 空
	INT_T, // 整型
	LONG_T, // 长整型
	FLOAT_T, // 浮点型
	DOUBLE_T, // 双精度型
	STR_T, // 字符串
	ARRAY_T, // 数组类型
	VAR_T, // 变量
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
	IF_T // 三目运算 (cond?left:right)
} type_enum_t;

typedef enum _call_enum_f {
	SIN_F, ASIN_F, COS_F, ACOS_F, TAN_F, ATAN_F, CTAN_F, SQRT_F, POW_F, RAD_F, RAND_F, RANDF_F, USER_F, STRLEN_F, MICROTIME_F, SRAND_F
} call_enum_f;

typedef enum _symbol_enum_t {
	ECHO_STMT_T, // echo
	RET_STMT_T, // ret
	ASSIGN_STMT_T, // var = expr
	IF_STMT_T, // if(expr) { stmtlist } else { stmtlist }
	WHILE_STMT_T, // while(expr) { stmtlist }
	DO_WHILE_STMT_T, // do { stmtlist } while(expr)
	BREAK_STMT_T, // break;
	LIST_STMT_T, // list;
	CLEAR_STMT_T, // clear;
	ARRAY_STMT_T, // array var[int][int]...
	INC_STMT_T, // increment var++
	DEC_STMT_T, // decrement var--
	ADDEQ_STMT_T, // var += val
	SUBEQ_STMT_T, // var -= val
	MULEQ_STMT_T, // var *= val
	DIVEQ_STMT_T, // var /= val
	MODEQ_STMT_T, // var %= val
	FUNC_STMT_T, // demo(); demo(1,2,3);
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
	char rawArgs;
	call_args_t *args;
};

struct _func_def_f {
	char *name;
	unsigned argc;
	unsigned minArgc;
	char *names;
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
		struct {
			char *str;
			unsigned int strlen;
		};
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

extern func_symbol_t *topSyms;
extern int yylineno;
extern int yyleng;
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

void calc_free_vars(exp_val_t *expr);
void calc_array_free(exp_val_t *ptr, call_args_t *args);

void str2val(exp_val_t *val, char *str);

#define LIST_STMT(info, funcname, lineno, t) printf(info, funcname, lineno);zend_hash_apply_with_arguments(&vars, (apply_func_args_t)calc_clear_or_list_vars, 1, t)

void seed_rand();
void yyerror(const char *s, ...);
void red_stderr_printf(const char *s, ...);
void red_stderr_vprintf(const char *s, va_list ap);

#define YYSTYPE exp_val_t

#ifdef DEBUG
#define dprintf(...) fprintf(stderr, "\x1b[33m");fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\x1b[0m")
#else
#define dprintf(...)
#endif

typedef struct _wrapStack {
	FILE *fp;
	char *filename;
	int lineno;
	struct _wrapStack *prev;
} wrap_stack_t;

extern HashTable frees;
extern HashTable files;
extern wrap_stack_t *curWrapStack;
extern wrap_stack_t *tailWrapStack;
extern unsigned int includeDeep;
extern char *curFileName;
extern FILE *yyin, *yyout;
extern int isSyntaxData;

int yywrap();
void yyrestart(FILE*);
int yylex(void);
int yylex_destroy();
void yypop_buffer_state(void);

extern int exitCode;
#define INTERRUPT(code, ...) exitCode = code; \
	yyerror(__VA_ARGS__); \
	if(!yywrap()) { \
		yypop_buffer_state(); \
	}

static inline double microtime() {
	struct timeval tp = {0};

	if (gettimeofday(&tp, NULL)) {
		return (double)time((time_t*)NULL);
	}

	return (double)(tp.tv_sec + tp.tv_usec / 1000000.00);
}

#define YYERROR_VERBOSE 1
#define YY_(s) s"\n"
#include "parser.h"
