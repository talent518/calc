#ifndef SMART_STRING_H
#define SMART_STRING_H

#include <stdlib.h>
#include <sys/types.h>

typedef struct {
	char *c;
	size_t len;
	size_t a;
} smart_string;

#define smart_string_0(x) do {										\
	if ((x)->c) {													\
		(x)->c[(x)->len] = '\0';									\
	}																\
} while (0)

#ifndef SMART_STRING_PREALLOC
#define SMART_STRING_PREALLOC 128
#endif

#ifndef SMART_STRING_START_SIZE
#define SMART_STRING_START_SIZE 78
#endif

#define smart_string_alloc4(d, n, newlen) do {				\
	if (!(d)->c) {													\
		(d)->len = 0;												\
		newlen = (n);												\
		(d)->a = newlen < SMART_STRING_START_SIZE 					\
				? SMART_STRING_START_SIZE 							\
				: newlen + SMART_STRING_PREALLOC;					\
		(d)->c = malloc((d)->a + 1);							\
	} else {														\
		if(UNEXPECTED((size_t)n > 8*1024*1024 - (d)->len)) {					\
			yyerror("String size overflow");			\
		}															\
		newlen = (d)->len + (n);									\
		if (newlen >= (d)->a) {										\
			(d)->a = newlen + SMART_STRING_PREALLOC;				\
			(d)->c = realloc((d)->c, (d)->a + 1);						\
		}															\
	}																\
} while (0)

#define smart_string_alloc(d, n) \
	smart_string_alloc4((d), (n), newlen)

/* wrapper */

#define smart_string_appends(dest, src) \
	smart_string_appendl((dest), (src), strlen(src))

#define smart_string_appendc(dest, ch) do {					\
	size_t __nl;													\
	smart_string_alloc4((dest), 1, __nl);						\
	(dest)->len = __nl;												\
	((unsigned char *) (dest)->c)[(dest)->len - 1] = (ch);			\
} while (0)

#define smart_string_free(s) do {								\
	smart_string *__s = (smart_string *) (s);								\
	if (__s->c) {													\
		free(__s->c);										\
		__s->c = NULL;												\
	}																\
	__s->a = __s->len = 0;											\
} while (0)

#define smart_string_appendl(dest, src, nlen) do {			\
	size_t __nl;													\
	smart_string *__dest = (smart_string *) (dest);						\
																	\
	smart_string_alloc4(__dest, (nlen), __nl);					\
	memcpy(__dest->c + __dest->len, (src), (nlen));					\
	__dest->len = __nl;												\
} while (0)

#define smart_string_append_generic(dest, num, func) do {	\
	char __b[32];															\
	char *__t = zend_print##func##_to_buf(__b + sizeof(__b) - 1, (num));	\
	smart_string_appendl((dest), __t, __b + sizeof(__b) - 1 - __t);	\
} while (0)

#define smart_string_append_unsigned(dest, num) \
	smart_string_append_generic((dest), (num), _ulong)

#define smart_string_append_long(dest, num) \
	smart_string_append_generic((dest), (num), _long)

#define smart_string_append(dest, src) \
	smart_string_appendl((dest), ((smart_string *)(src))->c, \
		((smart_string *)(src))->len);


#define smart_string_setl(dest, src, nlen) do {						\
	(dest)->len = (nlen);											\
	(dest)->a = (nlen) + 1;											\
	(dest)->c = (char *) (src);										\
} while (0)

#define smart_string_sets(dest, src) \
	smart_string_setl((dest), (src), strlen(src));

/* buf points to the END of the buffer */
static zend_always_inline char *zend_print_ulong_to_buf(char *buf, zend_ulong num) {
	*buf = '\0';
	do {
		*--buf = (char) (num % 10) + '0';
		num /= 10;
	} while (num > 0);
	return buf;
}

/* buf points to the END of the buffer */
static zend_always_inline char *zend_print_long_to_buf(char *buf, zend_long num) {
	if (num < 0) {
	    char *result = zend_print_ulong_to_buf(buf, ~((zend_ulong) num) + 1);
	    *--result = '-';
		return result;
	} else {
	    return zend_print_ulong_to_buf(buf, num);
	}
}

#endif