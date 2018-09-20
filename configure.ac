#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
m4_define([M4_CALC_NAME], [calc])
m4_define([M4_CALC_VERSION], [1.1])
AC_INIT(M4_CALC_NAME, M4_CALC_VERSION, [talent518@yeah.net])

AC_DEFINE([CALC_NAME],["M4_CALC_NAME"],[application name])
AC_DEFINE([CALC_VERSION],["M4_CALC_VERSION"],[application version])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE(M4_CALC_NAME, M4_CALC_VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h stdarg.h string.h time.h sys/time.h math.h complex.h float.h limits.h sys/types.h sys/stat.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero getcwd gettimeofday memchr memset strchr strdup strerror strrchr strstr strtol strtoul])

sinclude(acinclude.m4)

CFLAGS="-O3 -DYYSTYPE_IS_DECLARED -DSYS_CONF_DIR=\"${sysconfdir}\" -D_GNU_SOURCE -Wno-unused-result"

AC_ARG_ENABLE(debug, [  --enable-debug turn on debug], CFLAGS="$CFLAGS -g -DDEBUG")
AC_ARG_ENABLE(expr-type, [  --enable-expr-type turn on expr type print], CFLAGS="$CFLAGS -DHAVE_EXPR_TYPE")
AC_ARG_ENABLE(no-func-run-args, [  --enable-no-func-run-args turn off function call of run arguments], CFLAGS="$CFLAGS -DNO_FUNC_RUN_ARGS")
AC_CHECK_LIB([m], pow,,[AC_MSG_ERROR([math support requested, but not found])])

\cp Makefile.am.in Makefile.am

no_undef=
if test "$build_os" = "cygwin"; then
	no_undef=-no-undefined
fi

enable_exts=yes
AC_ARG_ENABLE(exts, [  --enable-exts turn on extensions [[default=yes]]], enable_exts=$enableval)

let extsize=0

sinclude(ext/Makefile.m4)

[
	find ext -type f | xargs grep -rin FUNC_DEF | awk -F : '{l=index($3,"(");r=index($3,")");files[$1]=1;funcs[substr($3,l+1,r-l-1)]=1;}END{for(k in files){print "#include \""k"\"";}print "void ext_funcs() {";for(k in funcs){print "    if(zend_hash_add(&funcs, \""k"\", "length(k)", &calc_def_"k", 0, NULL) == FAILURE) printf(\"function "k"() exists.\\n\");";}print "}";}' > ext.c
	find ext -type f | xargs grep -rin CONST_DEF | awk -F : '{l=index($3,"(");r=index($3,")");files[$1]=1;consts[substr($3,l+1,r-l-1)]=1;}END{for(k in files){def=toupper(k);gsub("\\.","_",def);gsub("/", "_",def);print "#ifndef "def;print "#define "def;print "#include \""k"\"";print "#endif";}print "void ext_consts() {";for(k in consts){print "    if(zend_hash_add(&consts, \""k"\", "length(k)", &calc_const_"k", 0, NULL) == FAILURE) printf(\"const "k" exists.\\n\");";}print "}";}' >> ext.c

	sed -e "s'@extheaders@'$extheaders'" -e "s'@extlibs@'$extlibs'" -i Makefile.am
	automake --add-missing --foreign 2> /dev/null
]

AC_CONFIG_FILES([Makefile $CALC_OUTPUT_FILES])

AC_OUTPUT
