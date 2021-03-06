dnl
dnl CALC_RUN_ONCE(namespace, variable, code)
dnl
dnl execute code, if variable is not set in namespace
dnl
AC_DEFUN([CALC_RUN_ONCE],[
	changequote({,})
	unique=`echo $2|sed 's/[^a-zA-Z0-9]/_/g'`
	changequote([,])
	cmd="echo $ac_n \"\$$1$unique$ac_c\""
	if test -n "$unique" && test "`eval $cmd`" = "" ; then
		eval "$1$unique=set"
		$3
	fi
])

dnl
dnl CALC_EXPAND_PATH(path, variable)
dnl
dnl expands path to an absolute path and assigns it to variable
dnl
AC_DEFUN([CALC_EXPAND_PATH],[
	if test -z "$1" || echo "$1" | grep '^/' >/dev/null ; then
		$2=$1
	else
		changequote({,})
		ep_dir=`echo $1|sed 's%/*[^/][^/]*/*$%%'`
		changequote([,])
		ep_realdir=`(cd "$ep_dir" && pwd)`
		$2="$ep_realdir"/`basename "$1"`
	fi
])

dnl
dnl CALC_ADD_INCLUDE(path [,before])
dnl
dnl add an include path. 
dnl if before is 1, add in the beginning of CFLAGS.
dnl
AC_DEFUN([CALC_ADD_INCLUDE],[
	if test "$1" != "/usr/include"; then
		CALC_EXPAND_PATH($1, ai_p)
		CALC_RUN_ONCE(INCLUDEPATH, $ai_p, [
			if test "$2"; then
				CFLAGS="-I$ai_p $CFLAGS"
			else
				CFLAGS="$CFLAGS -I$ai_p"
			fi
		])
	fi
])

dnl
dnl CALC_ADD_LIBRARY_WITH_PATH(path [,before])
dnl
dnl add an library path. 
dnl if before is 1, add in the beginning of LDFLAGS.
dnl
AC_DEFUN([CALC_ADD_LIBRARY_WITH_PATH],[
	if test "$1" != "/usr/lib"; then
		CALC_EXPAND_PATH($1, ai_p)
		CALC_RUN_ONCE(LIBRARYPATH, $ai_p, [
			export PKG_CONFIG_PATH="$ai_p/pkgconfig:$PKG_CONFIG_PATH"
			if test "$2"; then
				LDFLAGS="-L$ai_p -Wl,-rpath,$ai_p $LDFLAGS"
			else
				LDFLAGS="$LDFLAGS -L$ai_p -Wl,-rpath,$ai_p"
			fi
		])
	fi
])

dnl
dnl CALC_ADD_LIBRARY(path [,before])
dnl
dnl add an library link. 
dnl if before is 1, add in the beginning of LDFLAGS.
dnl
AC_DEFUN([CALC_ADD_LIBRARY],[
	CALC_EXPAND_PATH($1, ai_p)
	CALC_RUN_ONCE(LIBRARYLINK, $ai_p, [
		if test "$2"; then
			LDFLAGS="-l$ai_p $LDFLAGS"
		else
			LDFLAGS="$LDFLAGS -l$ai_p"
		fi
	])
])

dnl
dnl CALC_OUTPUT(file)
dnl
dnl Adds "file" to the list of files generated by AC_OUTPUT
dnl This macro can be used several times.
dnl
AC_DEFUN([CALC_OUTPUT],[
	CALC_EXPAND_PATH($1, ai_p)
	CALC_RUN_ONCE(OUTPUTPATH, $ai_p, [
		CALC_OUTPUT_FILES="$CALC_OUTPUT_FILES $1"
	])
])

dnl
dnl CALC_NEW_EXT(extname, sources [, shared [, cflags [, ldflags] [, ldadd [, libadd]]]])
dnl
dnl Includes an extension in the build.
dnl
dnl "extname" is the name of the ext/ subdir where the extension resides.
dnl "sources" is a list of files relative to the subdir which are used
dnl to build the extension.

AC_DEFUN([CALC_NEW_EXT],[
	extheaders="$extheaders ext/$1/$1.h"

	files=""
	for file in $2;do files="$files ext/$1/$file";done
	
	extlibs="$extlibs libext_$1.a"

	echo >> Makefile.am
	echo "libext_$1_a_SOURCES = $files" >> Makefile.am
	echo "libext_$1_a_CFLAGS = -fPIC $4" >> Makefile.am
	
	if test ! "$no_undef $5" = " "; then
		echo "libext_$1_a_LDFLAGS = $no_undef $5" >> Makefile.am
	fi
])

