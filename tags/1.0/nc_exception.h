/*		$Id: exception.h 20 2007-03-24 20:48:29Z mark $		*/

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

#ifndef _NC_EXCEPTION_H
#define _NC_EXCEPTION_H

#include <errno.h>

/** Throw an exception and print a format-string error message
 *
 */
#define throwf(fmt,...) return _throwf(__func__, __FILE__, __LINE__, fmt, __VA_ARGS__)
//DEADWOOD: used to contain: if (__retval == 0) goto catch; } while (0)


/**
 * Throw an exception and print an error message.
 */
 #define throw(str) return _throwf(__func__, __FILE__, __LINE__, "%s", str)

/**
 * Throw an exception but do not print any message to the log file.
 *
 */
#define throw_silent()  do { if (__retval == 0) goto catch; } while (0)


/**
 * Throw an error and report the errno.
 *
 * @todo for threadsafety, use strerror_r instead of strerror
 */
#define throw_errno(syscall) do { _throwf(__func__, __FILE__, __LINE__, "%s: %s (errno=%d)", syscall, strerror(errno), errno); if (__retval == 0) goto catch; } while (0)

/**
 * Throw an exception if a condition is true
 */
#define throw_if(condition) do { if (condition) { throw("error condition detected"); } } while (0)


/**
 * Throw a fatal, non-catchable exception and terminate the program.
 */
#define throw_fatal(msg) \
	    do { log_error("FATAL: %s", msg); abort(); } while (0)

/**
 * This function should appear as the first statement in each function.
 *
 * It provides a better assert() macro for use with testing function arguments.
 * 
 * Splint doesn't like pointers being used as boolean values, so
 * annotations are used to disable the warnings.
 *
 */
#define require(a) \
		/*@+ptrnegate@*/ \
		/*@-boolops@*/ \
		if (!(a)) \
   		    throw("invalid argument(s)"); \
		/*@=ptrnegate@*/ \
		/*@=boolops@*/ 

/* You are not expected to understand this. */

#define destroy(ref_type,ref) if (ref_type##_destroy(ref) != 0) { \
	     log_warning("destructor returned %d", -1); \
	     return -1; \
     }


#define try(a) \
     if ((a) != 0) throw("")

int _throwf(   const char *func,
		const char *file,
		int         lineno, 
		const char *format,
		...);
 
//DEADWOOD: int splint_variadic_macro(/*@unused@*/ void *arg, ...);

#endif
