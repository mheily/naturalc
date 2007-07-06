/*		$Id: string.h 44 2007-04-08 21:28:49Z mark $		*/

/*
 * Copyright (c) 2006, 2007 Mark Heily <devel@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _NC_STRING_H
#define _NC_STRING_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/** @bug workaround for <string.h> not being parsed correctly */
//extern size_t strlen(const char *s);
//extern void *memset(void *s, int c, size_t n);

/** 
 * Sentinel for variadic functions.
 *
 * Variadic functions that are marked with '__attribute__ ((sentinel))'
 * require that they be called with a trailing (char *) NULL.
 *
 * Unfortunately, NULL by itself is (void *) which causes a type warning.
 *
 * Using 'szNULL' instead of NULL avoids the type warning. 
 */
#define szNULL (char *) NULL

/** 
 * Wherever possible, use the string_t type instead of (const char *).
 * When using the (const char *) type deliberately, use char_t.
 * This way, you can grep for 'char *' to find code that needs to be fixed.
 */
#define char_t      const char
#define var_char_t  char

/* String options for str_compare() */
#define STR_DEFAULT	0x00000000
#define STR_RAW		0x00000001
#define STR_NO_CASE	0x00000010
#define STR_SINGLE_LINE	0x00000100
#define STR_MULTI_LINE	0x00001000
#define STR_CRLF_EOL	0x00010000
#define STR_LF_EOL	0x00100000

/** Maximum size of a string_t value (== 512 Megabytes) */
#define STRING_MAX      536870912

/** Maximum size of a format specifier */
#define FORMAT_MAX	80

/** Cast a NUL terminated (char *) to a string_t */
#define STR(x)        (string_t *) { x, 0, 0 }

/** UNIX v.s. DOS EOL conventions */
typedef enum {
	EOL_UNKNOWN = 0,
	EOL_LF = 1,
	EOL_CRLF = 2
} eol_type_t;

/** A string object. */
typedef struct str {

	/* Pointer to a buffer containing the value of the string. */
	const char *value;

	/* The size of the buffer as returned by malloc(3), in bytes */
	size_t	size;

	/* The length of the string, not including the terminating NUL character */ 
	size_t  len;

	/** If TRUE, this object owns the memory and is responsible for free()'ing
	   it when it is destroyed
	 */
	bool    owner;          
} string_t;

/** A string that is always empty. */
extern const string_t EMPTY_STRING;

/**
 * New, fancy C99 way to convert a (const char *) to a (const string *)
 */
#define CSTRING(x)	(&(const string_t){ x, sizeof(x) - 1, sizeof(x), false })

#define str_assert(s)	(s != NULL && s->value != NULL)

int str_new(string_t **str);
int str_destroy(string_t **str);

int str_alias(string_t *dest, const char *src);
int str_contains(bool *result, const string_t *haystack, char_t *needle);
int str_cmp(const string_t *s1, const char *s2);
int str_case_compare(const string_t *s1, const string_t *s2);
int str_compare(const string_t *s1, const string_t *s2);
int str_copy(/*@unique@*/ string_t *dest, const string_t *src);
int str_count(size_t *dest, const string_t *s, int c);
int str_cpy(string_t *dest, const char *src);
int str_escape(string_t *dest, const string_t *src);
int str_unescape(string_t *dest, const string_t *src);
int str_move(/*@unique@*/ string_t *dest, string_t *src);
int str_ncpy(string_t *dest, const char *src, size_t len);
int str_ncmp(const string_t *s, char_t *c, size_t len);
int str_casecmp(const string_t *dest, const char *src);
int str_resize(string_t *str, size_t new_size);
int str_cat(string_t *dest, /*@unique@*/ const char *src);
int str_prepend(string_t *dest, /*@unique@*/ const string_t *src);
int str_append(string_t *dest, const string_t *src);
int str_putc(string_t *dest, const int c);
int str_get_char(var_char_t *dest, string_t *str, size_t position);
int str_divide(/*@unique@*/ string_t *left, /*@unique@*/ string_t *right, /*@unique@*/ const string_t *src, size_t position);

int str_sprintf(string_t *dest, const char *format, ...)
	__attribute__((format(printf, 2, 3)));

int str_vprintf(string_t *dest, const char *format, va_list argv);
int str_chomp(string_t *str);
int str_read(string_t *dest, int fd, size_t size);
int str_set_len(string_t *dest, size_t new_len);
int str_to_upper(string_t *s);
int str_set_path(string_t *path, string_t *prefix, const char *filename);
int str_swap(string_t *s1, string_t *s2);
int str_truncate(string_t *dest);
int str_truncate_at(string_t *src, size_t position);

/* Numeric conversion */

int str_to_int(int *dest, string_t *src);
int str_to_int32(int32_t *dest, string_t *src);
int str_to_uint32(uint32_t *dest, const string_t *src);
int str_to_ulong(unsigned long *dest, const string_t *src);

/* System UID and GID conversion */
// DEADWOOD - Moved to passwd.c
//int str_to_gid(gid_t *dest, string_t *src);
//int str_to_uid(uid_t *dest, string_t *src);
//int gid_to_str(string_t *dest, gid_t src);
//int uid_to_str(string_t *dest, uid_t src);

/* Line endings */

int str_set_terminator(string_t *str, int terminator);
int str_get_terminator(char_t **dest, const string_t *str);
int str_cmp_terminator(string_t *str, int terminator);

/* Date/Time conversion */

int str_time(string_t *dest, time_t *tloc, char_t *format);
int str_from_time(string_t *dest, time_t *tloc);
int str_from_localtime(string_t *dest, time_t *tloc);
int str_to_time(time_t *dest, string_t *src);

/* Network address conversion */

int str_from_inet(string_t *dest, struct in_addr addr);
int str_to_inet(struct in_addr *addr, const string_t *src);

/* Simple pattern replacement */

int str_translate(string_t *s, int old, int new);

/* Regular expressions */

int str_match_regex(bool *result, const string_t *src, const char *pattern);
int str_subst_regex(string_t *src, const char *pattern, const char *replacement);
int str_str_regex(const string_t *src, const char *pattern, ...);

/* Functions that operate on (char *) */
int cbuf_len(size_t *dest, char_t *s);

/** Return the length (in bytes) of a string */
#define str_len(s)       s->len

/** Return a pointer to the string's value */
#define str_val(s)       s->value

/** Convert from a pointer to a string */
#define str_from_pointer(s,p)	str_sprintf(s, "%p", (void *) p)

/** Convert from a string to a pointer */
#define str_to_pointer(p, s)	(void) sscanf(s->value, "%p", (void **) &p)

#endif
