TOOLS_DIR = $(PWD)
BISON_FN = bison-3.0.4
FLEX_FN = flex-2.6.4

all: ./bin/bison ./bin/flex ../parser.c ../scanner.c

.PHONY:

../parser.c: ./bin/bison ../parser.y
	@echo parser.c
	@./bin/bison -v -d -Wnone -o$@ ../parser.y

../scanner.c: ./bin/flex ../scanner.l
	@echo scanner.c
	@./bin/flex --header-file=../scanner.h --outfile=$@ ../scanner.l

./build:
	@mkdir ./build

./build/$(BISON_FN):  ./build
	@tar -xvf ./$(BISON_FN).tar.gz -C ./build

./build/$(BISON_FN)/Makefile: ./build/$(BISON_FN)
	@sh -c "cd ./build/$(BISON_FN) && ./configure --prefix=$(TOOLS_DIR)"

./build/$(FLEX_FN):  ./build
	@tar -xvf ./$(FLEX_FN).tar.gz -C ./build

./build/$(FLEX_FN)/Makefile: ./build/$(FLEX_FN)
	@sh -c "cd ./build/$(FLEX_FN) && ./configure --prefix=$(TOOLS_DIR)"

./bin/bison:
	@echo build bison
	@sh -c "make ./build/$(BISON_FN)/Makefile"
	@sh -c "cd ./build/$(BISON_FN) && make all install"

./bin/flex:
	@echo build flex
	@sh -c "make ./build/$(FLEX_FN)/Makefile"
	@sh -c "cd ./build/$(FLEX_FN) && make all install"

