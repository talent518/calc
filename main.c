#include "calc.h"

#define USAGE() printf("Usage: %s { -v | -h | --varname=varvalue | - | files.. }\n" \
	"\n" \
	"  author             zhang bao cai\n" \
	"  email              talent518@yeah.net\n" \
	"  git URL            https://github.com/talent518/calc.git\n" \
	"\n" \
	"  options:\n" \
	"    -                from stdin input source code.\n" \
	"    -v               Version number.\n" \
	"    -h               This help.\n" \
	"    -i               Isolate run.\n" \
	"    -I               Not isolate run.\n" \
	"    --varname=varvalue  define a variable, variable name is varname, variable varvalue is value.\n" \
	"    files...         from file input source code for multiple.\n" \
	, argv[0])

int main(int argc, char **argv) {
	calc_init();
	
	if(argc > 1) {
		register int i;
		exp_val_t expr = {NULL_T}, *ptr;
		
		for(i = 1; i<argc; i++) {
			if(argv[i][0] == '-') {
				if(argv[i][1] == '-') {
					char *p = strchr(argv[i]+2,'=');
					CNEW01(ptr, exp_val_t);
					
					if(p) {
						CNEW01(ptr->str, string_t);
						ptr->type = STR_T;
						ptr->str->c = strdup(p+1);
						ptr->str->n = strlen(p);
						zend_hash_add(&vars, argv[i]+2, p-(argv[i]+2), ptr, 0, NULL);
					} else {
						ptr->type = INT_T;
						ptr->ival = 1;
						zend_hash_add(&vars, argv[i]+2, strlen(argv[i]+2), ptr, 0, NULL);
					}
				} else if(argv[i][1] && !argv[i][2]) {
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
				} else if(!argv[i][1]) {
					printf("> ");
					calc_runfile(&expr, "-", NULL, NULL);
					break;
				} else {
					USAGE();
					break;
				}
			} else {
				calc_runfile(&expr, argv[i], NULL, NULL);
			}
		}

		calc_free_expr(&expr);
	} else {
		USAGE();
	}

	calc_destroy();

	return exitCode;
}
