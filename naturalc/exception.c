/*		$Id: exception.nc 28 2007-03-26 02:14:31Z mark $		*/

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

/** @file 
 *
 * Exception handling library
 *
*/

#include "config.h"

#include "nc_exception.h"
#include "nc_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* Permit functions in this module to manipulate va_list types */
/*@access va_list,@*/

int _throwf(   const char *func,
		const char *file,
		int         lineno, 
		const char *format,
		...)
{
	/* NOTE: A static buffer is used in the event we are out of memory */
	char buf[1024]; 
	va_list ap;

	va_start(ap, format);

	/* Print a shorter trace message when we are moving up the call stack */
	/* FIXME- This never seems to happen.. */
	if (format[0] == '\0') {
		//DEADWOOD - OLD STYLE
		//(void) log_message(LOG_ERR, "   ... in %s() at %s, line %d", func, file, lineno);
		(void) log_message(LOG_ERR, "%s: In function `%s':\n"
				"%s:%d: error: ", file, func, file, lineno);
	}
	
	/* Otherwise, generate the initial 'throw()' message */
	else { 
		/* This mimics GCC's error messages so that
		 * vim can jump to the error line when invoking
		 * ':mak check' or another Makefile target.
		 *
		 * Example:
		 *   exception.c: In function `_throwf':
		 *   exception.c:59: error: blah blah blah
		 */
		(void) memset(&buf, 0, sizeof(buf));
		(void) snprintf(buf, sizeof(buf) - 1, "%s:%d: error: %s", 
				file, lineno, (format[0] == '\0') ? "unhandled exception" : format);
		(void) _log_vmessage(LOG_ERR, buf, ap);
		//(void) log_message(LOG_ERR, "%s: In function `%s':\n"
		//		"%s:%d: error: ", file, func, file, lineno);

		//DEADWOOD-(void) log_message(LOG_ERR, "   ... in %s() at %s, line %d", func, file, lineno);
	}

	va_end(ap);
	return -1;
}


/**
 * Splint does not support variadic macros.. so define an empty variadic function in it's place.
 */
int
splint_variadic_macro(/*@unused@*/void *arg, ...)
{
      return arg == NULL;
}
