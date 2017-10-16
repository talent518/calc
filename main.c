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

int main(int argc, char **argv) {
	calc_init();
	
	if(argc > 1) {
		register int i;
		exp_val_t expr = {NULL_T};
		
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
					calc_runfile(&expr, "-", NULL, NULL);
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
