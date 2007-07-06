/*		$Id: process.nc 44 2007-04-08 21:28:49Z mark $		*/

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
 * Abstractions for processes.
 *
*/

#include "config.h"

#include "nc.h"
//#include "options.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nc_signal.h"
#include "nc_process.h"

/** 
 * Test if a SIGALRM signal has been recieved.
 *
 * @todo refactor this 
 */
int
test_alarm_timer() { return SIGALRM_triggered ? 1 : 0; }


/**
 * Test for the existence of a pidfile for a given program.
 * 
 * @param program program name to check for
 */
	int
pidfile_stat(bool *result, const string_t *program)
{
	struct stat st;
	string_t   *path = NULL;

	str_new(&path);

	/* Generate the pidfile path */
	str_sprintf(path, "%s/%s.pid", NC_IPCDIR, program->value);
	log_debug("pidfile=`%s'", path->value);

	/* Check if the pidfile exists */
	// FIXME crap
	*result = stat(path->value, &st) == 0;
	/** @todo check for ENOENT vs other errors */

finally: 
	destroy(str, &path);
}


/**
 * Create a pidfile for a program.
 * 
 * @todo document this strange func
 *
 * @param program program name to check for
 * @return 0 if there is no exsting pidfile, or the pidfile is stale,
 *          -1 if an existing valid pidfile was found.
 *
 */
int
pidfile_create(const string_t *program)
{
	string_t *path    = NULL,
	         *pid_str = NULL;
	bool      exists;

	/* Allocate storage */
	str_new(&path);
	str_new(&pid_str);

	/* Generate the pidfile path */
	str_sprintf(path, "%s/%s.pid", NC_IPCDIR, program->value);

	/* Do not overwrite an existing pidfile */
	/* Instead, try sending it signal 0 to see if it is valid */
	pidfile_stat(&exists, program);
	if (exists) {
		if (pidfile_kill(program, 0) == 0)
			throwf("another instance of %s is already running", program->value);
	}

	/* Create a new PID file */
	str_sprintf(pid_str, "%u\n", getpid());
	file_write(path, pid_str);

finally:
	destroy(str, &path);
	destroy(str, &pid_str);
}


/**
 * Send a signal to a program.
 *
 * The specified @a signal will be sent to the process number contained
 * in the pidfile at /var/run/@a program.
 *
 * @param program name of the program
 * @param signal signal number := SIGTERM | SIGKILL | ...
*/
int
pidfile_kill(const string_t *program, int signal_num)
{
	string_t *abspath = NULL;
	string_t *pid_str = NULL;
	bool      exists;
	pid_t	  pid;

	/* Don't do anything if there isn't a pidfile */
	pidfile_stat(&exists, program);
	if (!exists) {
		log_debug("pidfile `%s.pid' does not exist", program->value);
		return 0;
	}

	/* Resolve the pidfile path */
	str_new(&abspath);
	str_sprintf(abspath, "%s/%s.pid", NC_IPCDIR, program->value);

	/* Read the contents of the pidfile */
	str_new(&pid_str);
	file_read(pid_str, abspath);
	str_chomp(pid_str);

	/* Parse the PID */
	str_to_uint32((uint32_t *) &pid, pid_str);

	log_info("sending signal %u to process id %u", signal_num, pid);

	/* Attempt to kill the process */
	if (kill(pid, signal_num) < 0) {
		switch (errno) {
			case 0: 
				break;

			case ESRCH:
				log_warning("unable to send a signal to process #%d: no such process", pid);
				break;

			default:
				log_warning("while sending signal %d to pid # %d ..", signal_num, pid);
				throw_errno("kill(2)");
		}
	}

	/* Remove the pidfile */
	if (unlink(abspath->value) < 0)
		throw_errno("unlink(2)");

finally:
	destroy(str, &abspath);
	destroy(str, &pid_str);
}


/**
 * Wrapper for setrlimit(2) to set per-process resource limits
 *
 * @param resource resource to control := RLIMIT_NOFILE | RLIMIT_NPROC | ...
 * @param max the hard and soft limit to set
 *
 * @see setrlimit(2)
*/
int
process_setrlimit(int resource, uint32_t max)
{
	struct rlimit   limit;

	if (getuid() > 0) 
		throw("setrlimit failed: not running as root");

	limit.rlim_cur = max;
	limit.rlim_max = max;
	if (setrlimit(resource, &limit) != 0)
		throw_errno("setrlimit(2)");
}


/**
 * Create a new process.
 *
 * The new process will start execution at @a function with @a data as a parameter.
 *
 * @param child_pid buffer to store the process ID of the child process
 * @param function function pointer
 * @param data argument to be passed to the function pointer
*/
int
process_create(pid_t *child_pid, int (*function)(void *), void *data)
{
	int    retries = 0;
	pid_t  pid;

	/* Fork a child to handle the connection */
try_fork:
	pid = fork();

	if (pid < 0) {
		if (errno == EAGAIN && retries++ < 30) {
			/** @todo check for SIGHUP? */
			if (sleep(2) > 0)
				log_warning("signal caught while sleeping %d", 0);
			goto try_fork;
		} else {
			throw("Unable to fork(2)");
		}
	}

	if (pid > 0) {
		/* Parent */
		log_debug("created process %lu", (unsigned long) pid);
		if (child_pid != NULL)
			*child_pid = pid;

		return 0;
	}
	if (pid == 0) {
		/* Child */

		/* Restore the default signal handlers */
		signal_library_init();

		/** @todo some way to send the return code to the parent */
		(void) (*function)(data);
	}
}

	
/**
 * Run a system command and check the return value for errors.
 *
 * @param cmd command to be executed via system(2)
*/
int
process_run_system(const string_t *cmd)
{
	int status = 0;

	log_debug("exec: `%s'", cmd->value);

	status = system(cmd->value);
	if (! WIFEXITED(status)) 
		throwf("subprocess `%s' reported abnormal termination", cmd->value);
	if (WEXITSTATUS(status) != 0)
		throwf("subprocess `%s' exited with a non-zero return value (retval=%d)", 
				cmd->value, WEXITSTATUS(status)
				);
}


/**
 * Sleep for a specified period of time
 *
 * @todo handle interruption by a signal
*/
int
process_sleep(unsigned int sec)
{
	unsigned int slept;

	slept = sleep(sec);

	if (slept > 0)
		throw("sleep interrupted");
}


/**
 * Detatch from the controlling terminal and become a daemon.
 * 
 * @todo document parameter
*/
int
process_daemonize(const string_t *progname, int logopt)
{
	pid_t	pid, sid;

	/* Create a new process */
	if ((pid = fork()) < 0)
		throw_errno("fork(2)");
	if (pid > 0)
		exit(0);

	/* Create a new session and become the session leader */
	if ((sid = setsid()) < 0)
		throw_errno("setsid(2)");

	/* Close all inherited STDIO file descriptors */
	(void) close(0);
	(void) close(1);
	(void) close(2);

	/* Re-open the system log */
	syslog_open(progname->value, logopt);
	log_debug("pid %d detatched from the controlling terminal", getpid());

	/* Create a pidfile */
	pidfile_create(progname);
}


