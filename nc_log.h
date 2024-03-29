/*		$Id: log.h 11 2007-02-14 02:59:42Z mark $		*/

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

#ifndef _NC_LOG_H
#define _NC_LOG_H

#include <syslog.h>

#include "nc_string.h"

#define _log_all(level, format,...) _log_message(level, "%s(%s:%d): "format, __func__, __FILE__, __LINE__, __VA_ARGS__)

#define log_message(level, format,...) _log_all(level, format, __VA_ARGS__)
#define log_error(format,...) _log_all(LOG_ERR, "ERROR: "format, __VA_ARGS__)
#define log_warning(format,...) _log_all(LOG_WARNING,"WARNING: "format,__VA_ARGS__)
#define log_notice(format,...) _log_all(LOG_NOTICE, format, __VA_ARGS__)
#define log_info(format,...) _log_all(LOG_INFO, format, __VA_ARGS__)
#define log_debug(format,...) _log_all(LOG_DEBUG, format, __VA_ARGS__)
#define log_debug2(...) if (0) { ; }

void _log_vmessage(int level, const char *format, va_list ap);
void _log_message(int level, const char *format, ...);
void _log_error(const char *format, ...);
void _log_warning(const char *format, ...);
void _log_notice(const char *format, ...);
void _log_info(const char *format, ...);
void _log_debug(const char *format, ...);

void log_backtrace (void);
int syslog_open(char_t *program, int logopt);
int set_log_level(int priority);

#endif
