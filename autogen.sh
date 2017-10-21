#!/bin/sh

./cleanauto.sh

for ext in $(ls ext)
do
	cat >> ext/Makefile.m4 <<EOF
enable_ext_${ext}=\$enable_exts
AC_ARG_ENABLE($ext, [  --enable-$ext turn on $ext], \$enable_ext_${ext}=\$enableval)
if test "\$enable_ext_${ext}" = "yes"; then
	sinclude(ext/$ext/Makefile.m4)
fi

EOF
done

make -C tools && \
aclocal && \
autoheader && \
autoconf && \
libtoolize --automake --copy --debug --force 2> /dev/null > /dev/null && \
automake --add-missing --foreign 2> /dev/null

exit 0
