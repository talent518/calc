CC = gcc

CFLAGS = -O3 -g -DYYSTYPE_IS_DECLARED `pkg-config --cflags glib-2.0`
LFLAGS = -lm `pkg-config --libs glib-2.0`

all: calc

.PHONY:

test: calc
	@echo -e "\E[35m"$@"\E[m"
	@tput sgr0
	@./calc < exp.txt

test2: calc
	@echo -e "\E[35m"$@"\E[m"
	@tput sgr0
	@./calc < exp2.txt

calc: calc.h parser.o scanner.o calc.o
	@echo -e "\E[31m"$@"\E[m"
	@tput sgr0
	@$(CC) $(CFLAGS) -o $@ $? $(LFLAGS)

parser.c: parser.y
	@echo -e "\E[34m"$?"\E[m"
	@tput sgr0
	@bison -v -d -o$@ $?

scanner.c: scanner.l
	@echo -e "\E[33m"$?"\E[m"
	@tput sgr0
	@flex -o$@ $?

%.o: %.c
	@echo -e "\E[32m"$?"\E[m"
	@tput sgr0
	@$(CC) $(CFLAGS) -o $@ -c $?

clean:
	@echo -e "\E[32m"$@"\E[m"
	@tput sgr0
	@rm -f scanner.c parser.h parser.c calc.exe *.o parser.output calc.exe.stackdump
