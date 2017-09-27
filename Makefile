CC = gcc

CFLAGS = -O3 -g -DYYSTYPE_IS_DECLARED
LFLAGS = -lm

all: calc

.PHONY:

test: calc
	@echo $@
	@./calc < exp.txt

test2: calc
	@echo $@
	@./calc < exp2.txt

calc: parser.o scanner.o zend_hash.o calc.o
	@echo $@
	@$(CC) $(CFLAGS) -o $@ $? $(LFLAGS)

parser.c: parser.y
	@echo $?
	@bison -v -d -o$@ $?

scanner.c: scanner.l
	@echo $?
	@flex -o$@ $?

%.o: %.c
	@echo $?
	@$(CC) $(CFLAGS) -o $@ -c $?

clean:
	@echo $@
	@rm -f calc calc.exe calc.exe.stackdump parser.c parser.h scanner.c *.o *.output

