/*		$Id: log.nc 44 2007-04-08 21:28:49Z mark $		*/

/*
 *
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
 * Wrapper for the system log (syslog) functions.
 *
*/
 
#include "config.h"

#include "nc_exception.h"
#include "nc_string.h"
#include "nc_log.h"

#include <stdlib.h>
#include <stdio.h>

/* Non-standard glibc library used to generate backtraces */
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

/* GLOBAL VARIABLES */

/* By default, copy all log output to the standard error */
static bool LOG_TO_STDERR = true;
static bool LOG_TO_SYSLOG = false;
static int  LOG_PRIORITY  = LOG_NOTICE;

     
/** 
 * Print a stack backtrace to the error log.
 *
 * Note that applications must be compiled with the -rdynamic flag to enable
 * the symbol names to be available.
 *
 */
void
log_backtrace (void)
  {
#if HAVE_EXECINFO_H
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	log_warning ("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		log_warning ("%s\n", strings[i]);

	free (strings);
#else
  	log_warning("%s", "This operating system does not provide backtrace_symbols()");
#endif
  }


/*
 * syslog_open(program name, log options)
 *
 * Open the system log. 
 *
 */
int
syslog_open(char_t *program, int logopt)
{
	string_t *program_copy = NULL;

	/* The <program> variable must never go out of scope, so make a copy
	   of it. This creates a small memory leak. 
	 */
	str_new(&program_copy);
	str_cpy(program_copy, program);

	logopt |= LOG_PID | LOG_NDELAY;
	openlog(program, logopt, LOG_MAIL);
	LOG_TO_SYSLOG = true;
	LOG_TO_STDERR = false;
	
	/* SA-NOTE: destroy(program_copy) is never called */
}


int
set_log_level(int priority)
{

	if (priority < 0) 
		priority = LOG_WARNING;
	if (getenv("VERBOSE"))
		priority = LOG_DEBUG;
	if (getenv("QUIET"))
		priority = LOG_ERR;
		
	(void) setlogmask(LOG_UPTO(priority));

	LOG_PRIORITY = priority;

}


/* @bug ncc workaround */
void
_log_vmessage(int level, const char *format, va_list ap)
  {
	//NOTE: cannot log to both at once without calling va_copy to copy the arg list
	if (LOG_TO_STDERR) {
		if (level <= LOG_PRIORITY) {
			(void) vfprintf(stderr, format, ap);
			(void) fputs("\n", stderr);
		}
	} else if (LOG_TO_SYSLOG) {
		vsyslog(level, format, ap);
	}
  }


/* @bug ncc workaround */
void
_log_message(int level, const char *format, ...)
   {
	va_list ap;

	va_start(ap, format);
	_log_vmessage(level, format, ap);
	va_end(ap);

   }


/* @bug ncc workaround */
void
_log_error(const char *format, ...)
   {
	va_list ap;

	va_start(ap, format);
	_log_vmessage(LOG_ERR, format, ap);
	va_end(ap);
   }


/* @bug ncc workaround */
void
_log_warning(const char *format, ...)
   {
	va_list ap;

	va_start(ap, format);
	_log_vmessage(LOG_WARNING, format, ap);
	va_end(ap);
   }


/* @bug ncc workaround */
void
_log_notice(const char *format, ...)
   {
	va_list ap;

	va_start(ap, format);
	_log_vmessage(LOG_NOTICE, format, ap);
	va_end(ap);
   }

   
/* @bug ncc workaround */
void
_log_info(const char *format, ...)
   {
	va_list ap;

	va_start(ap, format);
	_log_vmessage(LOG_INFO, format, ap);
	va_end(ap);
    }


/* @bug ncc workaround */
void
_log_debug(const char *format, ...)
   {
	va_list ap;

	va_start(ap, format);
	_log_vmessage(LOG_DEBUG, format, ap);
	va_end(ap);
   }
