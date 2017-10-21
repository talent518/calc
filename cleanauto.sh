#!/bin/sh

test -f "Makefile" && make uninstall clean > /dev/null 2>&1

#svn propget svn:ignore | xargs echo

rm -f calc calc.exe calc.exe.stackdump parser.c parser.h scanner.c scanner.h *.output ext.c
rm -rf *.exe .deps Makefile Makefile.am Makefile.in aclocal.m4 autom4te.cache compile config.h config.h.in config.log config.status configure depcomp install-sh missing stamp-h1 *.o *.a *.la *.lo *.so config.guess ltmain.sh config.sub libtool confdefs.h conftest.c conftest.err

rm -f ext/Makefile.m4

