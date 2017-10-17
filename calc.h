#ifndef _CALC_H
#define CALC_H
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
#include <unistd.h>

#include "zend_hash.h"
#include "smart_string.h"

#define max(a, b) ((a)>(b) ? (a) : (b))
#define NEW(t,n) (t*)malloc(sizeof(t)*(n))
#define NEW1(t) NEW(t,1)
#define CNEW(p,t,n) p = NEW(t,n)
#define CNEW0(p,t,n) CNEW(p,t,n);memset(p,0,sizeof(t)*(n))
#define CNEW1(p,t) CNEW(p,t,1)
#define CNEW01(p,t) CNEW0(p,t,1)
#define DNEW(p,t,n) t *p = NEW(t,n)
#define DNEW0(p,t,n) DNEW(p,t,n);memset(p,0,sizeof(t)*(n))
#define DNEW1(p,t) DNEW(p,t,1)
#define DNEW01(p,t) DNEW0(p,t,1)

#define NEW_FREES(dst, type) dst=NEW1(type);zend_hash_next_index_insert(&frees, dst, 0, NULL)
#define MEMDUP(dst,src,type) NEW_FREES(dst, type);memcpy(dst,src,sizeof(type))

typedef enum _type_enum_t {
	NULL_T=0, // 空
	INT_T, // 整型
	LONG_T, // 长整型
	FLOAT_T, // 浮点型
	DOUBLE_T, // 双精度型
	STR_T, // 字符串
	ARRAY_T, // 数组类型
	PTR_T, // 指针类型
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
	SIN_F, ASIN_F, COS_F, ACOS_F, TAN_F, ATAN_F, CTAN_F, SQRT_F, POW_F, RAD_F, RAND_F, RANDF_F, USER_F, STRLEN_F, MICROTIME_F, SRAND_F, RUNFILE_F, PASSTHRU_F
} call_enum_f;

typedef enum _symbol_enum_t {
	ECHO_STMT_T, // echo
	RET_STMT_T, // ret
	ASSIGN_STMT_T, // var = expr
	ACC_STMT_T, // var += expr
	IF_STMT_T, // if(expr) { stmtlist } else { stmtlist }
	WHILE_STMT_T, // while(expr) { breakStmtlist }
	DO_WHILE_STMT_T, // do { breakStmtlist } while(expr)
	BREAK_STMT_T, // break;
	LIST_STMT_T, // list;
	CLEAR_STMT_T, // clear;
	ARRAY_STMT_T, // array var[int][int]...
	FUNC_STMT_T, // demo(); demo(1,2,3);
	FOR_STMT_T, // for(;;) { breakStmtlist }
	SWITCH_STMT_T, // switch(expr) { switchStmtList }
	CASE_STMT_T, // case number: / case str:
	DEFAULT_STMT_T, // default:
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

typedef void (*expr_run_func_t)(exp_val_t *expr);
typedef status_enum_t (*sym_run_func_t)(exp_val_t *ret, func_symbol_t *syms);

typedef struct {
	char *c;
	unsigned int n;
	unsigned long int h;
} var_t;

struct _func_def_f {
	expr_run_func_t run;
	var_t *name;
	unsigned char argc;
	unsigned char minArgc;
	char *names;
	func_args_t *args;
	func_symbol_t *syms;
	unsigned int lineno;
	char *filename;
	char *desc;
	unsigned int varlen; // global 变量数
	var_t *vars; // global 变量指针
	//HashTable frees;
};

typedef struct {
	call_enum_f type;
	unsigned char argc;
	var_t *name;
	call_args_t *args;
	HashTable *ht;
} func_call_t;

typedef struct {
	char *c;
	unsigned int n;
	unsigned int gc; // 垃圾回收计数
} string_t;

typedef struct {
	unsigned int gc; // 垃圾回收计数
	unsigned int arrlen;
	struct _exp_val_t *array;
	unsigned int args[16];
	unsigned char dims; // 维数
} array_t;

typedef struct _ptr_t ptr_t;
typedef void (*ptr_free_func_t)(ptr_t *ptr);

struct _ptr_t {
	unsigned int gc; // 垃圾回收计数
	union {
		void *ptr;
		FILE *fptr;
		int fp;
		HashTable *ht;
	};
	char *desc;
	ptr_free_func_t dtor;
};

typedef struct {
	struct _exp_val_t *cond;
	struct _exp_val_t *left;
	struct _exp_val_t *right;
} exp_def_t;

struct _exp_val_t {
	type_enum_t type;
	union {
		int ival;
		long int lval;
		float fval;
		double dval;
		var_t *var;
		string_t *str;
		array_t *arr;
		struct _exp_val_t *ref;
		func_call_t *call;
		call_args_t *callArgs;
		func_def_f *def;
		func_args_t *defArgs;
		func_symbol_t *syms;
		exp_def_t *defExp;
		ptr_t *ptr;
	};
	struct _exp_val_t *result;
	expr_run_func_t run;
};

struct _call_args_t {
	exp_val_t val;
	struct _call_args_t *next;
	struct _call_args_t *tail;
};

struct _func_args_t {
	var_t *name;
	exp_val_t val;
	struct _func_args_t *next;
	struct _func_args_t *tail;
};

struct _func_symbol_t {
	symbol_enum_t type;
	union {
		exp_val_t *expr;
		struct {
			exp_val_t *var;
			union {
				call_args_t *args;
				exp_val_t *val;
			};
		};
		struct {
			exp_val_t *cond;
			func_symbol_t *lsyms;
			func_symbol_t *rsyms;
			func_symbol_t *forSyms;
			HashTable *ht;
		};
	};
	unsigned int lineno;
	char *filename;
	struct _func_symbol_t *next;
	struct _func_symbol_t *tail;
	sym_run_func_t run;
};

typedef struct _pool_t {
	void *ptr;
	dtor_func_t run;
} pool_t;

extern func_symbol_t *topSyms;
extern HashTable topFrees;
extern HashTable results;
extern char *types[];
extern HashTable vars;
extern HashTable funcs;
extern HashTable pools;
extern HashTable consts;

void append_pool(void *ptr, dtor_func_t run);
void zend_hash_destroy_ptr(HashTable *ht);

#define free_frees free

#define CALC_CONV_op(op1, op2, n) calc_conv_to_##n(op1);calc_conv_to_##n(op2)

void calc_conv_to_int(exp_val_t *src);
void calc_conv_to_long(exp_val_t *src);
void calc_conv_to_float(exp_val_t *src);
void calc_conv_to_double(exp_val_t *src);
void calc_conv_to_str(exp_val_t *src);
void calc_conv_to_array(exp_val_t *src);

#define calc_echo(val)  \
	do { \
		smart_string buf = {NULL,0,0}; \
		calc_sprintf(&buf, val); \
		if(buf.c) { \
			fwrite(buf.c, 1, buf.len, stdout); \
			free(buf.c); \
		} \
	} while(0)

void calc_sprintf(smart_string *buf, exp_val_t *src);
void calc_func_def(func_def_f *def);
void calc_free_args(call_args_t *args);
void calc_free_syms(func_symbol_t *syms);
void calc_free_func(func_def_f *def);

void calc_run_variable(exp_val_t *expr); // 读取变量
void calc_run_add(exp_val_t *expr); // 加法运算
void calc_run_sub(exp_val_t *expr); // 减法运算
void calc_run_mul(exp_val_t *expr); // 乘法运算
void calc_run_div(exp_val_t *expr); // 除法运算
void calc_run_mod(exp_val_t *expr); // 求余运算
void calc_run_pow(exp_val_t *expr); // 乘方运算
void calc_run_abs(exp_val_t *expr); // 绝对值运算
void calc_run_minus(exp_val_t *expr); // 相反数(负)运算
void calc_run_iif(exp_val_t *expr); // ?:运算
void calc_run_gt(exp_val_t *expr); // >运算
void calc_run_lt(exp_val_t *expr); // <运算
void calc_run_ge(exp_val_t *expr); // >=运算
void calc_run_le(exp_val_t *expr); // <=运算
void calc_run_eq(exp_val_t *expr); // ==运算
void calc_run_ne(exp_val_t *expr); // !=运算
void calc_run_array(exp_val_t *expr); // 读取数组元素值
void calc_run_func(exp_val_t *expr); // 执行用户函数
void calc_run_sys_runfile(exp_val_t *expr);
void calc_run_sys_passthru(exp_val_t *expr);
void calc_run_sys_sqrt(exp_val_t *expr);
void calc_run_sys_pow(exp_val_t *expr);
void calc_run_sys_sin(exp_val_t *expr);
void calc_run_sys_asin(exp_val_t *expr);
void calc_run_sys_cos(exp_val_t *expr);
void calc_run_sys_acos(exp_val_t *expr);
void calc_run_sys_tan(exp_val_t *expr);
void calc_run_sys_atan(exp_val_t *expr);
void calc_run_sys_ctan(exp_val_t *expr);
void calc_run_sys_rad(exp_val_t *expr);
void calc_run_sys_rand(exp_val_t *expr);
void calc_run_sys_randf(exp_val_t *expr);
void calc_run_sys_strlen(exp_val_t *expr);
void calc_run_sys_microtime(exp_val_t *expr);
void calc_run_sys_srand(exp_val_t *expr);
status_enum_t calc_run_syms(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_echo(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_ret(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_if(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_while(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_do_while(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_break(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_list(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_clear(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_array(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_null(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_func(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_variable_assign(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_array_assign(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_for(exp_val_t *ret, func_symbol_t *syms);
status_enum_t calc_run_sym_switch(exp_val_t *ret, func_symbol_t *syms);

void calc_free_expr(exp_val_t *expr);
void calc_free_vars(exp_val_t *expr);

#define calc_run_expr(expr) \
	if(EXPECTED((expr)->run!=NULL)) { \
		(expr)->run((expr)); \
	}

#define ref_expr(dst) \
	if((dst)->type == STR_T) { \
		(dst)->str->gc++; \
	} \
	if((dst)->type == ARRAY_T) { \
		(dst)->arr->gc++; \
	} \
	if((dst)->type == PTR_T) { \
		(dst)->ptr->gc++; \
	}

#define memcpy_ref_expr(dst, src) \
	calc_free_expr(dst); \
	memcpy(dst, src, sizeof(exp_val_t)); \
	ref_expr(dst)

#define str2num(val) \
	if((val)->type == STR_T) { \
		string_t *str = (val)->str; \
		(val)->type = INT_T; \
		 \
		memset((val), 0, sizeof(exp_val_t)); \
		str2val((val), str->c); \
		 \
		(val)->result = (val); \
		(val)->run = NULL; \
		free_str(str); \
	}

void str2val(exp_val_t *val, char *str);
void unescape(char *p);

int calc_list_funcs(func_def_f *def);
int calc_clear_or_list_vars(exp_val_t *val, int num_args, va_list args, zend_hash_key *hash_key);

#define free_str(s) if(!(s->gc--)) { \
		free(s->c); \
		free(s); \
	}

typedef struct _linenostack {
	unsigned int lineno;
	char *funcname;
	char *filename;
	func_symbol_t *syms;
} linenostack_t;

extern linenostack_t linenostack[];
extern int linenostacktop;
extern int linenofunc;
extern char *linenofuncname;
extern char *errmsg;

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
extern int isolate;

int yywrap();
void yyrestart(FILE*);
int yylex(void);
int yylex_destroy();
void yypop_buffer_state(void);

extern int exitCode;
#define EXIT_CODE_FUNC_EXISTS 1 // 函数已存在错误
#define EXIT_CODE_FUNC_ERR_ARG 2 // 函数的有默认值参数后还存在无默认值参数
#define ABORT(code, ...) exitCode = code; \
	yyerror(__VA_ARGS__); \
	if(!yywrap()) { \
		yypop_buffer_state(); \
	}

zend_always_inline static double microtime() {
	struct timeval tp = {0};

	if (gettimeofday(&tp, NULL)) {
		return (double)time((time_t*)NULL);
	}

	return (double)(tp.tv_sec + tp.tv_usec / 1000000.00);
}

typedef void (*top_syms_run_before_func_t)(void *ptr);

void calc_init();
int calc_runfile(exp_val_t *expr, char *filename, top_syms_run_before_func_t before, void *ptr);
void calc_destroy();
#endif