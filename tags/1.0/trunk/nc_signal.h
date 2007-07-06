/*		$Id: signal.h 44 2007-04-08 21:28:49Z mark $		*/

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

#ifndef _NC_SIGNAL_H
#define _NC_SIGNAL_H

#include <inttypes.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

/// @bug workaround
extern int kill(pid_t pid, int sig);

/* Signal handling */

extern volatile sig_atomic_t 	SIGALRM_triggered;
extern volatile sig_atomic_t 	SIGHUP_triggered;
extern volatile sig_atomic_t 	SIGUSR1_triggered;
extern volatile sig_atomic_t 	SIGUSR2_triggered;
extern volatile sig_atomic_t 	SIGTERM_triggered;
void default_sigchld_handler(void);

int signal_library_init(void);
int signal_mask_all(void);
int signal_unmask_all(void);

#endif
