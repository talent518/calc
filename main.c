#include "calc.h"

#define USAGE() printf("Usage: %s { -v | -h | - | files.. }\n" \
	"\n" \
	"  author           zhang bao cai\n" \
	"  email            talent518@yeah.net\n" \
	"  git URL          https://github.com/talent518/calc.git\n" \
	"\n" \
	"  options:\n" \
	"    -              from stdin input source code.\n" \
	"    -v             Version number.\n" \
	"    -h             This help.\n" \
	"    -i             Isolate run.\n" \
	"    -I             Not isolate run.\n" \
	"    files...       from file input source code for multiple.\n" \
	, argv[0])

void free_pools(pool_t *p) {
	p->run(p->ptr);
}

void ext_funcs();

int main(int argc, char **argv) {
	zend_hash_init(&files, 2, NULL);
	zend_hash_init(&frees, 20, NULL);
	zend_hash_init(&topFrees, 20, (dtor_func_t)free_frees);
	zend_hash_init(&vars, 20, (dtor_func_t)calc_free_vars);
	zend_hash_init(&funcs, 20, (dtor_func_t)calc_free_func);
	zend_hash_init(&pools, 2, (dtor_func_t)free_pools);
	zend_hash_init(&results, 20, (dtor_func_t)calc_free_vars);
	
	if(argc > 1) {
		register int i;
		exp_val_t expr = {NULL_T};
		
		ext_funcs();
		for(i = 1; i<argc; i++) {
			if(argv[i][0] == '-') {
				if(argv[i][1] && !argv[i][2]) {
					switch(argv[i][1]) {
						case 'v':
							printf("v1.1\n");
							break;
						case 'h':
							USAGE();
							break;
						case 'i':
							isolate = 1;
							break;
						case 'I':
							isolate = 0;
							break;
						case 's':
							isSyntaxData = 1;
							break;
						case 'S':
							isSyntaxData = 0;
							break;
						EMPTY_SWITCH_DEFAULT_CASE()
					}
				} else {
					printf("> ");
					runfile(&expr, "-", NULL, NULL);
					break;
				}
			} else {
				runfile(&expr, argv[i], NULL, NULL);
			}
		}

		calc_free_expr(&expr);
	} else {
		USAGE();
	}
	
	yylex_destroy();

	zend_hash_destroy(&topFrees);
	zend_hash_destroy(&files);
	zend_hash_destroy(&vars);
	zend_hash_destroy(&funcs);
	zend_hash_destroy(&pools);
	zend_hash_destroy(&results);
	zend_hash_destroy(&frees);
	
	if(errmsg) {
		free(errmsg);
	}

	return exitCode;
}
