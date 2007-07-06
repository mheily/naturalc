/*		$Id: process.h 44 2007-04-08 21:28:49Z mark $		*/

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

#ifndef _NC_PROCESS_H
#define _NC_PROCESS_H

#include "nc_string.h"

#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>


/* Alarm timers */

int test_alarm_timer(void);

/* Subprocesses */

int process_setrlimit(int resource, uint32_t max);
int process_create(pid_t *child_pid, int (*function)(void *), void *data);
int process_run_system(const string_t *cmd);

/* PID file management */

int pidfile_create(const string_t *program);
int pidfile_stat(bool *result, const string_t *program);
int pidfile_kill(const string_t *program, int sig_num);

int process_sleep(unsigned int sec);
int process_daemonize(const string_t *progname, int logopt);

#endif
