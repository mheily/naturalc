/*		$Id: string.nc 75 2007-04-22 15:41:19Z mark $		*/

/*
 * Copyright (c) 2004, 2005, 2006, 2007 Mark Heily <devel@heily.com>
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

/** @file
 *
 * String library.
 *
 * @todo Make each function require() non-overlapping arguments
 *
*/

#include "config.h"

/* Needed for glibc to provide strptime(3) */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "nc_exception.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_memory.h"
#include "nc_string.h"

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/** @file

    String library.

*/

/** vasprintf(3) is a GNU extension and not universally visible */
extern int      vasprintf(char **, const char *, va_list);

const string_t EMPTY_STRING = { "", 0, 0, true };

/* Allow functions to access members of the 'regex_t' abstract type */
/*@access regex_t@*/

/**
 * Copy from a NUL terminated character array to a string object.
 *
 * @param dest string object that will contain the result
 * @param src pointer to a NUL terminated character array
*/
int
str_cpy(string_t *dest, const char *src)
{
	size_t src_len = 0;

	require (src != dest->value);

	cbuf_len(&src_len, src);
	str_ncpy(dest, src, src_len);
}


/**
 * Remove all characters from a string.
 *
 * @param dest string to be truncated
*/ 
int
str_truncate(string_t *dest)
{

	dest->len = 0;
	if (dest->size > 0)
		memset((char *) dest->value, 0, 1);
}


/**
 * Append a single character to the end of a string.
 *
 * @param dest the string to be modified
 * @param c character to be appended
*/
int
str_putc(string_t *dest, int c)
{
	char buf[2];

	/* Convert the char to a simple string */
	buf[0] = (char) c;
	buf[1] = '\0';

	/* Append it to the buffer */
	str_cat(dest, buf);
}


/**
 * Get a single character at a given position within a string.
 *
 * If @a position is zero or positive, it is relative to the beginning.
 * If @a position is negative, it is relative to the end of the string.
 *
 * @param dest pointer to buffer that will store the result
 * @param str string to be examined
 * @param position position of desired character within the string
*/
int
str_get_char(var_char_t *dest, string_t *str, size_t position)
{
	if (position >= str_len(str))
		throw("invalid position: attempted to seek beyond the end of the string");

	/* Copy the result to the caller */
	*dest = (var_char_t) *(str->value + position);
}


/**
 * Divide a string into a left part and a right part at a given position.
 *
 * @param left buffer that will store the left part
 * @param right buffer that will store the right part
 * @param src string that will be split
 * @param position character position to split the string at
*/
int
str_divide(/*@unique@*/ string_t *left, /*@unique@*/ string_t *right, /*@unique@*/ const string_t *src, size_t position)
{
	/* Make sure the <position> isn't greater than the length of the string */
	if (position >= str_len(src))
		throw("invalid position: is greater than string length");

	/* Special case: if the <position> is zero, store the whole string in <right> */
	if (position == 0) {
		str_truncate(left);
		str_copy(right, src);
		return 0;
	}

	/* The <position> is a positive offset from zero */
	/* Copy the result to the caller */
	str_ncpy(left, src->value, position);
	str_cpy(right, src->value + position + 1);
}


/**
 * Truncate a string at a given character position.
 *
 * @param src string to be truncated
 * @param position position (starting at zero) to truncate at
 */
int
str_truncate_at(string_t *src, size_t position)
{
	if (position > src->len)
		throw("invalid request: position exceeds length of string");

	memset((char *) src->value + position, 0, 1);
	src->len = position;
}


/**
 * Create a string that points to an existing character buffer.
 *
 * This is useful in when used with automatic variables.
 * @deprecated this seems wrong
 * @param dest string
 * @param src string
*/
int 
str_alias(string_t *dest, const char *src)
{

	dest->owner = false;
	dest->value = (char *) src;
	cbuf_len(&dest->len, src);
	dest->size = dest->len + 1;
}


/**
 * Return a pointer to the end-of-line terminator in a string.
 *
 * In most cases, this will return a pointer to CR+LF+NUL or LF+NUL
 *
 * @param dest indirect pointer that will be set to point at the result
 * @param str string to be examined
*/
int
str_get_terminator(char_t **dest, const string_t *str)
{

	if (str->len > 0) {
		*dest = str->value + str->len - 1;
	} else {
		*dest = str->value;
	}
}


/**
 * @bug this function stinks
 *
 */
int
str_cmp_terminator(string_t *str, int terminator)
{

	/* Compare the terminating character */
	if (str->len > 0 && 
			((int) *(str->value + str->len - 1) != terminator)
	   )
		throw_silent();
}


/**
 * Set the end-of-line terminator for a string.
 *
 * @param str string to be modified
 * @param terminator new end-of-line terminator
*/
int
str_set_terminator(string_t *str, int terminator)
{
	
	if (str->len > 0) {
		memset((char *) str->value + str->len - 1, terminator, 1);
		if (terminator == '\0')
			str->len--;
	} else {
		str_putc(str, terminator);
	}
}


/**
 * Copy the value of one string to another string.
 *
 * @param dest string that will store the result
 * @param src string to be copied
*/
int
str_copy(/*@unique@*/ string_t *dest, const string_t *src)
{
	/* Don't copy two strings that share the same memory address */
	if (src == dest)
		throw("src and dest cannot be equal");

	/* Don't copy an empty string */
	if (src->len == 0 || src->size == 0) 
		return str_truncate(dest);
	
	/* Free any previously allocated memory in the destination */
	if (dest->owner) 
		free((char *) dest->value);
	dest->size = 0;

	/* Allocate memory for the copy */
	if ((dest->value = malloc(src->size)) == NULL)
		throwf("malloc error: want %u bytes", src->size);

	/* Copy the source to the destination */
	memcpy((char *) dest->value, src->value, src->len + 1);

	/* Set the destination size/length to be equal to the source */
	dest->size = src->size;
	dest->len = src->len;

	/* The destination is a unique and modifiable copy */
	dest->owner = true;
}


/**
 * Move the value from one string to another string.
 *
 * After the operation is complete, the destination string will point at
 * the value of the source string, and the source string will point at an
 * empty string.
 * 
 * @param dest destination string
 * @param src source string
*/
int
str_move(/*@unique@*/ string_t *dest, string_t *src)
{
	require (src != dest);

	/* Don't move two strings that share the same memory address */
	if (src == dest)
		throw("src and dest cannot be equal");

	/* WORKAROUND: Copy, then truncate the original string */
	str_copy(dest, src);
	str_truncate(src);

#ifdef TODO
	// This code caused problems

	/* Optimize this by moving the pointer without copying the memory */

	/* Free any previously allocated memory in the destination */
	if (dest->owner) 
		free(dest->value);

	/* Set the destination to be equal to the source */
	memcpy(dest, src, sizeof(*dest));

	/* Set the source to be an empty string */
	src->value = "";	// PROBLEM: this can't be free()d later
	src->size = 0;
	src->len = 0;
#endif
}


/**
 * Generate a composite path given a basename and a filename
 *
 * As an example, passing "/etc" as the basename and "hosts" as 
 * the filename would produce the string "/etc/hosts".
 *
 * @param path string that will contain the complete pathname
 * @param prefix basename (or directory prefix), without any trailing slash
 * @param filename filename, as a constant character array
*/
int
str_set_path(string_t *path, string_t *prefix, char_t *filename)
{

	str_sprintf(path, "%s/%s", prefix->value, filename);
}


/**
 * Set the length of a string.
 *
 * This would typically be used to shrink a string to a specific length
 * If the string is longer than @a new_len, it will be truncated.
 *
 * @param dest string to be modified
 * @param new_len new length
*/
int
str_set_len(string_t *dest, size_t new_len)
{
	
	if (new_len > STRING_MAX)
		throw("string exceeds STRING_MAX");

	if (new_len >= dest->size)
		throwf("string would exceed the size of it's buffer (buf_size=%zu len_request=%zu)",
				dest->size, new_len);

	/* Set the new length */
	dest->len = new_len;

	/* Set the NUL terminator according to the new length */
	memset((char *) dest->value + new_len, 0, 1);
}


/**
 * Convert a string to upper case.
 *
 * @param s string to be converted
*/ 
int
str_to_upper(string_t *s)
{
	size_t i, len;
	
	for (i = 0, len = str_len(s); i < len; i++) {
		memset((char *) s->value + i, toupper((int) s->value[i]), 1);
	}
}


/**
 * Compute the length of a NUL terminated character array.
 *
 * @param dest destination buffer that will store the result
 * @param s character pointer to be examined
*/
int
cbuf_len(size_t *dest, char_t *s)
{
	size_t i;

	/* Find the end of the string */
	for (i = 0; i < STRING_MAX; i++) {
		if ((char) *(s + i) == '\0') {
			*dest = i;
			return 0;
		}
	}

	*dest = 0;
	throw("character buffer too large");
}


int
str_new(string_t **dest)
{
	size_t    initial_size = 16;
	string_t *str = NULL;

	mem_calloc(*dest);
	str = *dest;

	if ((str->value = malloc(initial_size)) == NULL)
		throw("malloc error");
	memset((char *) str->value, 0, sizeof(str->value));
	
	str->size = initial_size;
	str->owner = true;
}


int
str_destroy(string_t **str)
{ 
	if (*str == NULL)
		return 0;

	if ((*str)->value)
		free((char *) (*str)->value);
	free(*str);
	*str = NULL;
}


/**
 * Resize the memory allocated to contain the value of a string.
 *
 * @param str string to be resized
 * @param new_size new size, in bytes
 * @see str_set_len()
*/ 
int
str_resize(string_t *str, size_t new_size)
{
	char	*c = NULL;

	/* Check the bounds of the new string size */
	if (new_size == 0)
		throw("strings cannot have zero size");
	if (new_size > STRING_MAX)
		throwf("string too large (%zu > %zu)", new_size, STRING_MAX);

	/* Resize the buffer if it is not already large enough */
	if (str->size < new_size) {
		if ((c = realloc((char *) str->value, new_size)) == NULL)
			throw("malloc error");
		str->value = c;
		memset((char *) str->value + new_size - 1, 0, 1);
		str->size = new_size;
	}
}


/**
 * Prepend one string to the beginning of another string.
 *
 * @param dest destination string
 * @param src source string
*/
int
str_prepend(string_t *dest, /*@unique@*/ const string_t *src)
{
	string_t *buf;

	/* Don't prepend two strings that share the same memory address */
	if (src == dest)
		throw("src and dest cannot be equal");

	str_copy(buf, dest);
	str_copy(dest, src);
	str_append(dest, buf);
}


/**
 * Append one string to the end of another string.
 *
 * @param dest destination string
 * @param src source string
 * @todo Optimize this, don't just call str_cat().. 
*/
int
str_append(string_t *dest, const string_t *src)
{
	/* Don't append two strings that share the same memory address */
	if (src == dest)
		throw("src and dest cannot be equal");

	str_cat(dest, src->value);
}


/**
 * Append a NUL terminated character array to the end of a string.
 *
 * @param dest destination string
 * @param src pointer to a NUL terminated character array
 * @todo Optimize this, don't just call str_cat().. 
*/
int
str_cat(string_t *dest, /*@unique@*/ const char *src)
{
	size_t	src_len,
                new_size;

	/* Compute the input length */
	src_len = strlen(src);
	if (src_len == 0)
		return 0;
	if (src_len >= STRING_MAX)
		throw("input string is too long");

	/* Compute the required buffer size */
	new_size = dest->len + src_len + 1;
	if (new_size <= dest->len)
		throw("operation would overflow");

	/* Resize the buffer as needed */
	if (new_size > dest->size) {
		str_resize(dest, new_size);
	}

	/* Append the new string to the existing string */
	memcpy((char *) dest->value + dest->len, src, src_len);
	memset((char *) dest->value + dest->len + src_len, 0, 1);
	str_set_len(dest, new_size - 1);
}


/**
 * Generate a string from a format string and a variable number of arguments.
 *
 * @param dest destination string
 * @param format format string
 * @see printf(3)
*/
int
str_sprintf(string_t *dest, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	str_vprintf(dest, format, ap);

finally:
	va_end(ap);
}


/**
 * Generate a string from a format string and a va_list argument.
 *
 * @param dest destination string
 * @param format format string
 * @param argv variadic argument list
 * @see printf(3)
*/
int
str_vprintf(string_t *dest, const char *format, va_list argv)
{
	/* Deallocate any previous string contents */
	if (dest->size > 0) {
		free((char *) dest->value);
		dest->value = NULL;
		dest->size = 0;
		dest->len = 0;
	}

	if (vasprintf((char **) &dest->value, format, argv) < 0)
		throw_errno("vasprintf(3)");

	dest->len = strlen(dest->value);
	if (dest->len > STRING_MAX) {
		free((char *) dest->value);
		dest->value = NULL;
		dest->size = 0;
		dest->len = 0;
		throw("result too large");
	}
	dest->size = dest->len + 1;
}


/**
 * Look for a needle in a haystack.
 *
 * @param result destination to store the result
 * @param haystack the string to be scanned
 * @param needle the substring to match
 * @see strstr(3)
*/
int
str_contains(bool *result, const string_t *haystack, char_t *needle)
{

	*result = strstr(haystack->value, needle) == NULL ? false : true;
}


/**
 * Compare a string with a NUL terminated character array.
 *
 * @param s1 string object
 * @param s2 pointer to a NUL terminated character array
 * @see strcmp(3)
 * @bug this function should return 0 or -1, not 0|1|-1
 * @return an integer greater than, equal to, or  less than 0, 
 *          according to whether the string s1 is greater than, equal
 *           to, or less than the string s2.
*/ 
int
str_cmp(const string_t *s1, const char *s2)
{

	return strcmp(s1->value, s2);
}


/**
 * Compare a string and a character array, without case sensitivity.
 *
 * @param s1 string object
 * @param s2 character array
 * @bug would like to abort() instead of returning -2
 * @see strncmp(3)
*/ 
int
str_casecmp(const string_t *s1, const char *s2)
{

	return (strcasecmp(s1->value, s2) == 0 ? 0 : -1);
}


/**
 * Compare two strings lexicographically.
 *
 * @param s1 string object
 * @param s2 string object
*/
int
str_compare(const string_t *s1, const string_t *s2)
{
	char *c1, *c2;

	/* Don't compare two strings that share the same memory address */
	//FIXME: why not?
	require (s1 != s2 && s1->value != s2->value);

	/* Compare each character */
	c1 = (char *) s1->value;
	c2 = (char *) s2->value;
	while (*c1 != '\0' && *c2 != '\0') {
		if (*c1 != *c2)
			return -1;
		c1++;
		c2++;
	}
	/* This tests if the strings are of different lengths */
	if (*c1 != *c2)
		return -1;
}


/**
 * Compare two strings lexicographically, without case sensitivity.
 *
 * @param s1 string object
 * @param s2 string object
*/
int
str_case_compare(const string_t *s1, const string_t *s2)
{
	/* Don't compare two strings that share the same memory address */
	//FIXME: why not?
	require (s1 != s2 && s1->value != s2->value);

	if (strcasecmp(s1->value, s2->value) != 0)
		return -1;
}



/**
 * Wrapper for read(2).
 *
 * Reads a set number of @a bytes from a @a fd, storing it in @a dest.
 *
 * @param dest destination buffer
 * @param fd file descriptor
 * @param size number of bytes to read
 * @bug returns 1 if EAGAIN is encountered
*/
int
str_read(string_t *dest, int fd, size_t size)
{
	ssize_t   bytes_read;
	size_t    new_len;

	if (size > STRING_MAX)
		throw("requested size is too large to read into a buffer");

	/* Delete any previous contents of the buffer */
	str_truncate(dest);

	/* Allocate memory for the new data and a NUL terminator */
	str_resize(dest, size + 1);
		
	/* Read from the file descriptor */
	bytes_read = read(fd, (char *) dest->value, size);
	if (bytes_read < 0) {
		switch (errno) {
			case EAGAIN: 
				return 1;
				break;

			default:
				log_warning("error reading from fd #%d", fd);
				throw_errno("read(2)");
		}
	}


	/* Update the string length */
	new_len = (size_t) bytes_read;
	str_set_len(dest, new_len);
}


/**
 * Convert a string into a signed integer.
 *
 * @param dest pointer to an int to store the result
 * @param src string to be converted
*/ 
int
str_to_int(int *dest, string_t *src)
{
	long l;

	errno = 0;

	l = strtol(src->value, (char **)NULL, 10);

	if (errno != 0)
		throwf("error converting string `%s' to an integer", src->value);

	if (l > INT_MAX)
		throw("conversion error: out of range");

	*dest = (int) l;
}


/**
 * Convert a string into an signed 32-bit integer.
 *
 * @param dest pointer to an int32_t to store the result
 * @param src string to be converted
 * @bug - this is a copy/paste of str_to_int() 
*/ 
int
str_to_int32(int32_t *dest, string_t *src)
{
	long l;

	errno = 0;

	l = strtol(src->value, (char **)NULL, 10);

	if (errno != 0)
		throwf("error converting string `%s' to an integer", src->value);

	if (l > INT32_MAX)
		throw("conversion error: out of range");

	*dest = (int32_t) l;
}


/**
 * Convert a string into an unsigned 32-bit integer.
 *
 * @param dest pointer to an uint32_t to store the result
 * @param src string to be converted
*/ 
int
str_to_uint32(uint32_t *dest, const string_t *src)
{
	unsigned long ul;
	
	if (str_to_ulong(&ul, src) < 0)
		throw("conversion error");

	if (ul > UINT_MAX)
		throw("conversion error: out of range");
	
	*dest = (uint32_t) ul;
}


/**
 * Convert a string into an unsigned long.
 *
 * @param dest pointer to an unsigned long to store the result
 * @param src string to be converted
*/ 
int
str_to_ulong(unsigned long *dest, const string_t *src)
{
	char *ep;

	errno = 0;
	*dest = strtoul(src->value, &ep, 10);
	if (src->value[0] == '\0' || *ep != '\0')
		throwf("conversion error: string is not numeric: `%s'", src->value);
	if (errno == ERANGE && *dest == ULONG_MAX)
		throw("conversion error: string exceeds numeric range");
}


/**
 * Copy part of a character buffer into a string.
 *
 * @param dest string to store the result
 * @param src pointer to a NUL terminated character array
 * @param len number of bytes to be copied
*/ 
int
str_ncpy(string_t *dest, const char *src, size_t len)
{
	/* Don't copy two strings that share the same memory address */
	if (src == dest->value)
		throw("src and dest cannot be equal");

	/* Don't copy more than STRING_MAX characters */
	if (len > STRING_MAX)
		throw("string too long");

	/* A zero-length copy is the same thing as a truncate */
	if (len == 0) 
		return str_truncate(dest);

	/* Delete any previous value */
	str_truncate(dest);

	/* Resize the destination buffer to fit the source */
	str_resize(dest, len + 1);

	/* Copy the string and set the NUL terminator */
	memcpy((char *) dest->value, src, len);
	memset((char *) dest->value + len, 0, 1);

	/* Update the string header */
	dest->owner = true;
	dest->len = len;
}



/**
 * Compare a portion of a string with a character buffer.
 *
 * @param s string object
 * @param c pointer to a NUL terminated character array
 * @param len number of bytes to compare
 * @bug should abort() instead of returning -1
*/
int
str_ncmp(const string_t *s, char_t *c, size_t len)
{
	require (len > 0 && len < STRING_MAX);
	
	return strncmp(s->value, c, len);
}


/**
 * Convert from human-readable time to UNIX time.
 *
 * The time string must be in this format: `12 Aug 2006 22:49:40 -0400'.
 *
 * @param dest destination buffer
 * @param src formatted time string
*/
int
str_to_time(time_t *dest, string_t *src)
{
	string_t *buf, *offset_hours, *offset_mins;
	int       offset;
	struct tm result;

	/* Make a copy of the string without the time zone */
	if (str_str_regex(src, "^([0-9]{1,2} [A-Za-z]{3} [0-9]{4} "
				"[0-9]{2}:[0-9]{2}:[0-9]{2}) ([+-][0-9][0-9])([0-9][0-9])$"
				,
				buf, offset_hours, offset_mins, NULL) < 0)
		throwf("invalid time string: `%s'", src->value);

	/* PORTABILITY: many strptime(3) functions can not handle the time zone */
	/* WORKAROUND: ignore the time zone */
	if (strptime(buf->value, "%d %b %Y %T", &result) == NULL)
		throwf("unable to convert time string `%s'", buf->value);

	/* Convert the broken-down time into UNIX time */
	*dest = mktime(&result);

	/* Adjust for the time zone by adding or subtracting hours from <dest> */
	offset = atoi(offset_hours->value) * 60 * 60;
	if (offset < 0) {
		offset -= atoi(offset_mins->value) * 60;
	} else {
		offset += atoi(offset_mins->value) * 60;
	}
}


/**
 * Convert from UNIX time to a formatted time string.
 *
 * @param dest buffer to store the result
 * @param tloc time, in seconds, since the Epoch (1/1/1970)
 * @param format format string to pass to strftime(3)
 * @see strftime(3)
*/
int
str_time(string_t *dest, time_t *tloc, char_t *format)
{
	var_char_t  buf[230];
	struct tm timeval;
	time_t    now;
	
	/* If a time is not given, use the current time */
	if (tloc == NULL) {
		now = time(NULL);
		tloc = &now;
	}

	/* Convert UNIX time into GMT time */
	localtime_r(tloc, &timeval);

	/* Generate a formatted date string.
	 */
	memset(buf, 0, sizeof(buf));
	if (strftime((char *) &buf, sizeof(buf) - 1, format, &timeval) == 0)
		throw("date format error");
	str_cpy(dest, buf);
}


/**
 * Convert from UNIX time to locale-specific human-readable time.
 *
 * Uses non-standard user-friendly time format; 
 * eg. "07 Feb 2006 5:03pm" 
 *
 * @param dest buffer to store the result
 * @param tloc time, in seconds, since the Epoch (1/1/1970)
*/
int
str_from_localtime(string_t *dest, time_t *tloc)
{
	/** @todo Add a profile option for Default-Time-Zone 
	   	since the client's time zone may differ from the server */

	str_time(dest, tloc, "%d %b %Y %r");
}


/**
 * Convert from UNIX time to human-readable time.
 *
 * Uses standard mailsystem time format; eg. "07 Feb 2006 16:03:10 -0500" 
 *
 * @param dest buffer to store the result
 * @param tloc time, in seconds, since the Epoch (1/1/1970)
*/
int
str_from_time(string_t *dest, time_t *tloc)
{

	str_time(dest, tloc, "%d %b %Y %T %z");
}


/**
 * Print a regular expression error message to the system log.
 *
 * @param regex_errno errno as returned by a regex function call
 * @param preg the compiled regular expression that caused the error
 * @returns This function always returns -1.
*/
static inline int
regex_error(int regex_errno, regex_t *preg)
{
	char errbuf[80];

	(void) regerror(regex_errno, preg, (char *) &errbuf, sizeof(errbuf));
	log_error("regex error: %s", errbuf);

	throw_silent();
}


/**
 * Compile a regular expression.
 *
 * @param preg a regex object 
 * @param pattern regular expression pattern
 * @param cflags flags passed to regcomp(3)
*/ 
static inline int
regex_compile(regex_t *preg, const char *pattern, int cflags)
{
	int i;

	i = regcomp(preg, pattern, REG_EXTENDED | REG_ICASE | cflags );
	if (i != 0) 
		return regex_error(i, preg);

}


/**
 * Perform a regular expression search and replace.
 *
 * @param preg a compiled regular expression
 * @param buf the string to be modified
 * @param replacement the replacement text
*/ 
static int
regex_replace_all(regex_t *preg,
		string_t *buf, 
		const char *replacement
		)
{
	string_t   *result;
	regmatch_t  pmatch[1];
	int i;

	i = regexec(preg, buf->value, 1, pmatch, 0);
	if (i != 0) {
		if (i == REG_NOMATCH) { 
			goto finally;
		} else {
			return regex_error(i, preg);
		}
	}

	/* NOTE: rm_so and rm_eo are 'long long' (a.k.a regoff_t) type 
	   and must be downcast to 'size_t' type
	 */
	
	/* Copy the beginning part of the string */
	str_ncpy(result, buf->value, (size_t) pmatch[0].rm_so);

	/* Insert the replacement text */
	str_cat(result, replacement);

	/* Copy the end part of the original string */
	str_cat(result, buf->value + (size_t) pmatch[0].rm_eo);

	/* Replace the input buffer with the result */
	str_move(buf, result);

finally:
	regfree(preg);
}


/**
 * Test if a string matches a regular expression.
 *
 * @param result store the result; matched (true) or not matched (false)
 * @param src string to be examined
 * @param pattern regular expression
 * @return 0 if the regular expression matches, 0 if there is no match
*/ 
int
str_match_regex(bool *result, const string_t *src, char_t *pattern)
{
	regex_t preg;
	int i = 0;

	/* Compile the pattern */
	if (regex_compile(&preg, pattern, REG_NOSUB) < 0)
		throw("regex compilation error");
	//log_debug2("pattern=`%s' val=`%s' i=%d", pattern, src->value, i);
	
	/* Execute the query */
	i = regexec(&preg, src->value, 0, NULL, 0);
	if (i != 0) {
		if (i == REG_NOMATCH) { 
			*result = false;
			goto finally;
		} else {
			(void) regex_error(i, &preg);
			throw("regex error");
		}
	}

	*result = true;

finally:
	/* Free any auxiliary storage used by the regex */
	regfree(&preg);
}


/**
 * Perform substring replacement using a regular expression.
 * 
 * Whatever the @a pattern matches will be replaced.
 *
 * @param src string to be modified
 * @param pattern regular expression
 * @param replacement replacement text
*/
int
str_subst_regex(string_t *src, const char *pattern, const char *replacement)
{
	regex_t preg;
	int i;
	
	if ((i = regex_compile(&preg, pattern, 0)) < 0) {
		(void) regex_error(i, &preg);
		throw("error compiling regular expression");
	}

	regex_replace_all(&preg, src, replacement);

	/* Recalculate the string length */
	src->len = strlen(src->value);
}


/* Ignore warnings about accessing members of a regex_t */
/*@-type@*/
/**
 * Retrieve one or more substrings from a string.
 *
 * @todo better documentation
 * @param src string to be examined
 * @param pattern regular expression
*/
int
str_str_regex(const string_t *src, const char *pattern, ...)
{
	va_list ap;
	regmatch_t  pmatch[16];
	regex_t preg;
	string_t *buf = NULL;
	unsigned int i;

	/* Compile the regex */
	if ((i = regex_compile(&preg, pattern, 0)) != 0) {
		log_warning("regex compilation failed for `%s'", pattern);
		return regex_error(i, &preg);
	}

	/* Check if pmatch is large enough to hold all the results */
	if (preg.re_nsub > 16)
		throw("too many sub-expressions");

	/* Execute the regex */
	i = regexec(&preg, src->value, 16, pmatch, 0);
	if (i != 0) {
		if (i == REG_NOMATCH) { 
			return -1;
		} else {
			log_warning("regexec failed for `%s'", pattern);
			return regex_error(i, &preg);
		}
	}

	/* Retreive the results and copy them to the caller */
	va_start(ap, pattern);
	for (i = 1; i <= preg.re_nsub; i++) {
		buf = va_arg(ap, string_t *);
		str_ncpy(buf, 
					src->value + pmatch[i].rm_so, 
					(size_t) pmatch[i].rm_eo - (size_t) pmatch[i].rm_so
			     );
	}
	va_end(ap);

finally:
	regfree(&preg);
}
/*@=type@*/


/**
 * Remove the last character from the end of a string.
 *
 * If the string ends with CR+LF, both characters will be removed.
 *
 * @param str string to be modified
 * @todo make rtrim function that does all trailing whitespace
*/ 
int
str_chomp(string_t *str)
{
	var_char_t *cp;

	/* Ignore empty strings */
	if (str->len == 0)
		return 0;

	str_get_terminator((char_t **) &cp, str);

	if (*cp == '\n') {
		*cp-- = '\0';
		str->len--;
	}

	/* Special case: Treat CRLF the same as LF */
	if (str->len > 0 && *cp == '\r') {
		*cp = '\0';
		str->len--;
	}
}


/**
 * Perform character substitution within a string.
 * 
 * Every occurrance of the @a old character will be changed to the
 * @a new character.
 *
 * @param s string to be modified
 * @param old old character
 * @param new new character
*/
int
str_translate(string_t *s, int old, int new)
{
	char  *cp;

	cp = (char *) s->value;
	while (*(cp++) != '\0') {
		if (*cp == (char) old) 
			*cp = (char) new;
	}
}


/**
 * Generate a URI-encoded string by escaping special characters.
 *
 * @todo document the RFC for this encoding
 * @param dest buffer to store the result
 * @param src string to be URI-encoded
*/ 
int
str_escape(string_t *dest, const string_t *src)
{
	string_t      *buf;
	var_char_t     code[4];
	size_t 	       i, len;

	str_truncate(dest);

	/* Ignore empty strings */
	if (str_len(src) == 0)
		return 0;

	len = str_len(src);
	for (i = 0; i < len; i++) {
		if ( isalnum((int)src->value[i]) || strchr( "/_.-~", src->value[i] ) != NULL )
		{
			str_putc(buf, (int) src->value[i]);
		}
		else
		{
			if (snprintf(code, 
						sizeof(code), 
						"%%%02x", 
						(unsigned int) src->value[i] & 0xff) < 0)
				throw_errno("snprintf(3)");
			str_cat(buf, code);
		}
	}

	str_move(dest, buf);
}


#ifndef S_SPLINT_S
	// splint and gcc seem to disagree about the signedness of code
/**
 * Compute the original string from a URI-encoded string.
 *
 * @param dest buffer to store the result
 * @param src URI-encoded string
*/
int
str_unescape(string_t *dest, const string_t *src)
{
	string_t *buf;
	size_t i = 0;
	int code = 0;

	/* Ignore empty strings */
	if (str_len(src) == 0)
		return 0;

	/* Process each character of the source string */
	while (src->value[i] != '\0') {
		if (src->value[i] == '%' &&
			sscanf(&src->value[i], "%%%02x", (unsigned int *) &code) == 1)  {

			i += 2;
			str_putc(buf, code);
		} else {
			str_putc(buf, (int) src->value[i]);
		}
		i++;
	}

	str_move(dest, buf);
}
#endif


/**
 * Count the number of occurances of a character within a string.
 *
 * @param dest store the result
 * @param s string to be examined
 * @param c character to look for
 * @return the number of times that @a c appears within the string
*/
int
str_count(size_t *dest, const string_t *s, int c)
{
	var_char_t *cp = NULL;

	*dest = 0;

	cp = (char *) s->value;
	while (*cp != '\0') {
		if (*cp++ == (char) c)
			(*dest)++;
	}
}


/**
 * Convert a binary IPv4 address into an ASCII string.
 *
 * @param dest pointer to a buffer that will store the result
 * @param addr a 32-bit IPv4 address
*/ 
int
str_from_inet(string_t *dest, struct in_addr addr)
{
	char buf[INET_ADDRSTRLEN + 1];

	memset(&buf, 0, sizeof(buf));
	/*@-type@*/
	if (inet_ntop(PF_INET, &addr, (char *) &buf, sizeof(buf) - 1) == 0)
		throw_errno("inet_ntop(3)");
	/*@=type@*/
	str_cpy(dest, buf);
}


/**
 * Convert a string into an IPv4 address.
 *
 * @param addr pointer to a buffer that will store the result
 * @param src string containing an ASCII representation of an IPv4 address
*/ 
int
str_to_inet(struct in_addr *addr, const string_t *src)
{
#if BROKEN
	// openbsd complains about inet_aton
	
	/** 
	 * inet_aton returns 1 if the address was valid
	 * for the specified address family; 0 if the address wasn't parseable in
	 * the specified address family; or -1 if some system error occurred (in
	 * which case errno will have been set).
	 */
	rc = inet_aton(src->value, addr);
	if (rc < 0)
		throw_errno("inet_aton(3)");
	if (rc == 0)
		throwf("address `%s' was not parsable", src->value);
#else
	// workaround
        /* Convert the ASCII address to the numeric address */
	/*@-type@*/
        addr->s_addr = inet_addr(src->value);

        /* Special case: inet_addr returns -1 if an error occurred
         *               which happens to be 255.255.255.255.
         */
	if ((int) addr->s_addr == -1) {
		if (str_cmp(src, "255.255.255.255") != 0)
			throw("parse error");
	}
	/*@=type@*/
#endif
}

/**
 * Split a multi-line string into a list of strings using a delimiter.
 *
 * @param dest list that will hold the result of the split operation
 * @param src delimited string that will be split
 * @param delimiter the delimiter to split on
*/
int
str_split(list_t *dest, const string_t *src, int delimiter)
{
	string_t *buf, *src_copy, *delim;
	var_char_t *tok   = NULL,
		   *cp    = NULL;

	/* Do nothing if the string is empty */
	if (str_len(src) == 0)
		return 0;

	/* Make a copy of the input string */
	str_copy(src_copy, (string_t *) src);

	/* strspn(3) expects a NUL terminated string */
	str_sprintf(delim, "%c", delimiter);

	/* Delete any items that are currently in the destination list */
	list_truncate(dest);

	/* Special case: remove the trailing delimiter if it exists */
	str_chomp(src_copy);

	//log_debug("delim=`%c' src=`%s'", delimiter, src_copy->value);

	/* Parse each token and add it to the list */
	cp = (char *) src_copy->value;
	while ((tok = strsep(&cp, delim->value)) != NULL) {
		//log_debug("tok=`%s'", tok);

		/* Copy the string to the list */
		list_cat(dest, tok);

		/* Remove the CRLF line terminator if the LF delimiter is chosen */
		if (delimiter == '\n') {
			str_chomp(dest->tail->value);
		}
	}

	//list_print(dest);
}


/**
 * Swap the contents of two strings.
 *
 * @param s1 the first string
 * @param s2 the second string
 */
int str_swap(string_t *s1, string_t *s2)
{
	struct str tmp;		

	memcpy(&tmp, s1, sizeof(tmp));
	memcpy(s1, s2, sizeof(tmp));
	memcpy(s2, &tmp, sizeof(tmp));
}
