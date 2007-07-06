/*		$Id: signal.nc 22 2007-03-24 21:16:33Z mark $		*/

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
 *  Signal handling.
 *
*/
 
#include "config.h"

#include "nc_exception.h"
#include "nc_log.h"
#include "nc_signal.h"

#include <signal.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>


/* 	SIGNAL HANDLERS		*/

volatile sig_atomic_t 	SIGALRM_triggered = 0;
volatile sig_atomic_t 	SIGHUP_triggered = 0;
volatile sig_atomic_t 	SIGUSR1_triggered = 0;
volatile sig_atomic_t 	SIGUSR2_triggered = 0;
volatile sig_atomic_t 	SIGTERM_triggered = 0;

static void
default_signal_handler(int signum) 
  {  
	  switch (signum) {
		  case SIGINT: 
			  exit(EXIT_FAILURE);		

		  case SIGHUP:
			  SIGHUP_triggered = 1;	
			  break;

		  case SIGUSR1:
			  SIGUSR1_triggered = 1;	
			  break;
	  }
  }


static int
signal_handler_install(int sigcatch, void (*func_ptr)(int))
{
	struct sigaction sig_act;

        sig_act.sa_handler = func_ptr;
        sig_act.sa_flags   = 0;
        if (sigemptyset(&sig_act.sa_mask) != 0)
		throw_errno("sigemptyset(3)");
        if (sigaction(sigcatch, &sig_act, NULL) != 0)
		throw_errno("sigaction(3)");
}


/**
 * Set all signal handlers to update a global variable when a signal is caught.
 *
*/
int
signal_library_init()
{

	signal_handler_install(SIGHUP, default_signal_handler);
	signal_handler_install(SIGUSR1, default_signal_handler);
	signal_handler_install(SIGINT, default_signal_handler);

	//@todo Make the server_run() loop aware that it should check for SIGTERM
	//      Otherwise the app will ignore SIGTERM, which is not a good idea.
	//signal_handler_install(SIGTERM, sigterm_handler);
}


/**
 * Tell the process to ignore all incoming signals.
 */
int
signal_mask_all(void)
{
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);
	(void) signal(SIGUSR1, SIG_IGN);
	(void) signal(SIGUSR2, SIG_IGN);
}


/**
 * Restore the default signal handler for all incoming signals.
 */
int
signal_unmask_all()
{
	(void) signal(SIGHUP, SIG_DFL);
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTSTP, SIG_DFL);
	(void) signal(SIGUSR1, SIG_DFL);
}


/**
 * Reap all child processes after they terminate and ignore their exit value.
*/
void
default_sigchld_handler()
  {
	do { } while (waitpid(0, 0, WNOHANG) > 0);
  }

