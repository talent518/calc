CC = gcc

CC_FLAGS = -O3 -DYYSTYPE_IS_DECLARED $(CFLAGS)
CL_FLAGS = -lm $(LFLAGS)

TOOLS_DIR = $(PWD)/tools
BISON_FN = bison-3.0.4
FLEX_FN = flex-2.6.4

all: ./tools/bin/bison ./tools/bin/flex calc

.PHONY:

test: calc
	@echo $@
	@./calc exp.txt exp2.txt exp3.txt exp4.txt exp5.txt exp6.txt exp7.txt

calc: parser.o scanner.o zend_hash.o calc.o
	@echo $@
	@$(CC) $(CC_FLAGS) -o $@ $? $(CL_FLAGS)
	@sh -c "echo version '`./calc -v`'"

parser.c: ./tools/bin/bison
	@echo parser.y
	@./tools/bin/bison -v -d -Wnone -o$@ parser.y

scanner.c: ./tools/bin/flex
	@echo scanner.l
	@./tools/bin/flex -o$@ scanner.l

%.o: %.c
	@echo $?
	@$(CC) $(CC_FLAGS) -o $@ -c $?

clean:
	@echo $@
	@rm -f calc calc.exe calc.exe.stackdump parser.c parser.h scanner.c *.o *.output

./tools/build:
	@mkdir ./tools/build

./tools/build/$(BISON_FN):  ./tools/build
	@tar -xvf ./tools/$(BISON_FN).tar.gz -C ./tools/build

./tools/build/$(BISON_FN)/Makefile: ./tools/build/$(BISON_FN)
	@sh -c "cd ./tools/build/$(BISON_FN) && ./configure --prefix=$(TOOLS_DIR)"

./tools/build/$(FLEX_FN):  ./tools/build
	@tar -xvf ./tools/$(FLEX_FN).tar.gz -C ./tools/build

./tools/build/$(FLEX_FN)/Makefile: ./tools/build/$(FLEX_FN)
	@sh -c "cd ./tools/build/$(FLEX_FN) && ./configure --prefix=$(TOOLS_DIR)"

./tools/bin/bison:
	@echo build bison
	@sh -c "make ./tools/build/$(BISON_FN)/Makefile"
	@sh -c "cd ./tools/build/$(BISON_FN) && make all install"

./tools/bin/flex:
	@echo build flex
	@sh -c "make ./tools/build/$(FLEX_FN)/Makefile"
	@sh -c "cd ./tools/build/$(FLEX_FN) && make all install"

