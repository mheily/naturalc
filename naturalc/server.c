/*		$Id: server.nc 68 2007-04-20 02:13:10Z mark $		*/

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
 *  Server class methods.
 *
*/
 
#include "nc_dns.h"
#include "nc_exception.h"
#include "nc_file.h"
//#include "nc_options.h"
#include "nc_hash.h"
#include "nc_host.h"
#include "nc_list.h"
#include "nc_memory.h"
#include "nc_log.h"
#include "nc_passwd.h"
#include "nc_process.h"
#include "nc_session.h"
#include "nc_socket.h"
#include "nc_string.h"
#include "nc_thread.h"
#include "nc_server.h"

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>


/********************* PRIVATE FUNCTIONS **********************/

static int server_destroy_all(void);


/********************* PUBLIC FUNCTIONS **********************/

/** This interface was withdrawn, according to OpenBSD unistd.h */
extern int chroot(const char *);

int
server_new(server_t **srv)
{
	server_t *s = NULL;

	/* Allocate memory for the object */
	mem_calloc(*srv);
	s = *srv;
	mem_calloc(s->controller);
	str_new(&s->uid);
	str_new(&s->gid);
	str_new(&s->service);
	str_new(&s->address);
	socket_new(&s->sock);

	/* Initialize strings */
	// FIXME: nobody/nogroup is not portable; use nc_site.h
	str_cpy(s->uid, "nobody");
	str_cpy(s->gid, "nogroup");
	str_cpy(s->service, "undef-proto");

	/* Initialize socket defaults */
	s->port = -1;
	s->mode = 0660;
	s->family = PF_INET;
	s->timeout = 5 * 60;

	*srv = s;
}


int
server_destroy(server_t **srv)
{
	server_t *s = NULL;

	s = *srv;

	/** @todo Wait for all clients to terminate */

	str_destroy(&s->uid);
	str_destroy(&s->gid);
	str_destroy(&s->service);
	str_destroy(&s->address);
	socket_destroy(&s->sock);
	free(s->controller);

	/* Free the object */
	free(s);
	*srv = NULL;
}


/**
 * Initialize a server socket.
 *
 * This function emulates the socket(2) system call in that it creates
 * a new socket, but does not bind(2) or listen(2).
 *
 * @param srv a server object
 */
int
server_socket(server_t *srv)
{
	/* Create a server socket */
	socket_set_family(srv->sock, srv->family);

#if WITH_OPENSSL
	/* Set the TLS flag */
	srv->sock->tls_enabled = srv->use_tls;
#endif

	/* Set the file permissions for PF_LOCAL sockets */
	if (srv->family == PF_LOCAL) {
		socket_set_credentials(srv->sock, srv->uid, srv->gid, srv->mode);
	}
}


/**
 * Destroy all instantiated server_t objects.
 *
 * @bug this was used for valgrind and is broken now
 */
static int
server_destroy_all(void)
{
#if FIXME
	server_t *obj = NULL;

	// array type is deprecated
	
	while (SERVER_OBJ->len) {
		array_pop(obj, SERVER_OBJ);
		(void) server_destroy(&obj);
	}
	array_destroy(&SERVER_OBJ);

	/* Destroy the global options table */
	sys_destroy();

	/* Destroy thread library structures */
	thread_pool_destroy(&TPOOL); 
#endif

	log_debug("cleanup: %s", "all server objects destroyed");
}


/**
 * Remove root privileges from the current process.
 * 
 * If the process is running as root, change the UID and GID to the ID's of
 * @a uid and @a gid, respectively.  If @a chrootdir is specified, the
 * process will be placed in a chroot(2) jail.
 * 
 * @param uid_str name of the user to run as
 * @param gid_str name of the group to run as
 * @param chrootdir root of the chroot(2) jail, or an empty string if chroot is not desired
*/
int
drop_privileges(const string_t *uid_str, const string_t *gid_str, const string_t *chrootdir)
{
	string_t *buf, *path, *regex;
	uid_t     uid;
	gid_t     gid;
	bool      match;

	if (getuid() > 0) {
		log_warning("cannot drop privileges when running as non-root user #", getuid());
		return 0;
	}

	/* Run getaddrinfo(3) to force ld.so to pull in libresolv.so */
	dns_library_init();

	/* Run pthread_create(3) to force ld.so to pull in libpthread.so */
	thread_library_init();

	/* Convert the symbolic group to numeric GID */
	group_exists(&match, gid_str);
	if (match) {
		group_get_id_by_name(&gid, gid_str);
	} else {
		throwf("a group named '%s' does not exist", gid_str->value);
	}

	/* Convert the symbolic user-id to the numeric UID */
	passwd_exists(&match, gid_str);
	if (match) {
		passwd_get_id_by_name(&uid, uid_str);
	} else {
		throwf("a user named '%s' does not exist", uid_str->value);
	}

	/* Change to the chrootdir directory */
	if (chdir(chrootdir->value) < 0)
		throwf("chdir(2) to %s", chrootdir->value);

	/* Relocate to a chroot(2) jail if possible */
	if (chroot(chrootdir->value) < 0)
		throwf("chroot(2) to %s", chrootdir->value);
	log_debug("chroot(2) to %s", chrootdir->value);

	/* Remove the chroot prefix from system paths */
	str_sprintf(regex, "^%s", chrootdir->value);
#if FIXME
	// XXX-FIXME options.h is no more
	str_subst_regex(sys.queuedir, regex->value, "");
	str_subst_regex(sys.mailboxdir, regex->value, "");
	str_subst_regex(sys.ipcdir, regex->value, "");
	str_cpy(sys.chrootdir, "/");
#endif

	/* Set the real GID */
	if (setgid(gid) < 0) 
		throw_errno("setgid(2)");
	log_debug("setgid(2) to %s(%d)", gid_str->value, gid);

	/* Set the real UID */
	if (setuid(uid) < 0)
		throw_errno("setuid(2)");
	log_debug("setuid(2) to %s(%d)", uid_str->value, uid);

	/* Make sure that getpwnam(3) still works within the chroot environment */
	str_cpy(buf, "root");
	passwd_exists(&match, buf);
	if (!match)
		throw("a problem with the chroot(2) jail "
				"is preventing getpwnam(3) from working");
}


/**
 * Accept an incoming connection on a server and create a new client session.
 *
 * @param srv a server object
*/
static int
server_accept(server_t *srv)
{
	session_t *session = NULL;

	/* Initialize a session object */
	session_new(&session);

	/* Run the protocol-specific initialization hook */
	session->controller_handle = srv->controller_handle;
	session_controller_invoke(session, SESSION_INIT, NULL);

	/* Accept a client connection */
	session_accept(session, srv);

	/* Reject the connection if the maximum number of clients has been reached */
	if (session->sock->fd >= MAX_CLIENT_COUNT)
		throw("connection limits exceeded");

	/* Ensure that fd is not negative */
	if (session->sock->fd < 0 )
		throw("invalid session fd");

	/* Create a new thread */
	/** @todo proper casting */
	/*@-type@*/
	if (thread_create_detached((callback_t) session_handler, session) < 0 ) {
		session_controller_invoke(session, SESSION_OVERLOAD, NULL);
		(void) session_close(session);
		session_destroy(&session);
		return 0;
	}
	/*@=type@*/

#if DEADWOOD
	// old event-driven code
	
	/* Create the I/O event, but do not activate it */
	event_set(&session->io_event, 
			session->sock->fd, 
			EV_READ,
			(void (*)(int,short, void *))server_wake_idle_session,
			session);

	/* Create the timeout event, but do not activate it */
	evtimer_set(&session->timeout_event, 
			(void (*)(int,short, void *))server_terminate_idle_session,
			session);

	/* Do not fork a separate process when running under a debugger */
	if ( sys.no_fork ) {
		session_handler(session);
	}

	/* Otherwise, create a new process or thread */
	else {
		(void) thread_pool_add_work(TPOOL, session);	
	}
#endif

catch:
	if (session) {
		log_warning("destroying session %d", 0);
		/// @todo tell the client why we are hanging up?
		(void) session_close(session);
		destroy(session, &session);
	}
}


/** @bug this should be called somewhere!!! */
static int UNUSED
server_prepare(server_t *srv)
{

#if FIXME
	/* When running as a daemon .. */
	if (sys.daemon && !sys.testing) {

		/* Detatch from the controlling terminal */
		process_daemonize();

	}

	/* Set resource limits for the server process */
	if (getuid() == 0) {

		/* Increase the allowable number of child processes */
		process_setrlimit(RLIMIT_NPROC, MAX_CLIENT_COUNT);

		/* Increase the allowable number of file descriptors */
		process_setrlimit(RLIMIT_NOFILE, RLIM_INFINITY);

	}

	/* Initialize signal handlers to ignore SIGCHLD and SIGPIPE */
	(void) signal(SIGCHLD, SIG_IGN);
	(void) signal(SIGPIPE, SIG_IGN);

	/* Drop privileges and chroot() to the jail directory */
	if (drop_privileges(srv->uid, srv->gid, sys.chrootdir) < 0)
		throw("unable to drop privileges");

#endif

}


/**
 * Process the receipt of a signal (SIGINT, SIGTERM, etc.)
 *
 */
static void UNUSED
server_signal_handler(int fd UNUSED, short event_type UNUSED, void *arg)
  {
  	long signum = (long) arg;
  	
	log_warning("hi %l", signum);
	(void) server_destroy_all();
	exit(EXIT_FAILURE);
  }


/**
 * Run multiple servers inside a single process
 *
 * @param constructor a NULL terminated array of function pointers to instantiate each server
*/
int
server_multiplex(list_t *bind_addr, int (*constructor[])(server_t *))
{
	list_entry_t *cur = NULL; 
	string_t  *item;
	server_t  *srv;
	size_t     num_servers = 0, pfd_count = 0;
	struct pollfd *pfd;
	struct server **handler;
	int        i, j, n;
	unsigned int controller_handle;

	/* If no addresses are provided, get a list of all addresses */
	if (bind_addr->count == 0) {
		host_get_ifaddrs(bind_addr, AF_INET);
	}

	/* Determine how many servers to run */
	for (; constructor[num_servers] != NULL; num_servers++) {}
	log_debug("%zu server objects", num_servers);

	/* Allocate memory for the poll(2) descriptor set */
	pfd_count = bind_addr->count * num_servers;
	mem_malloc(pfd, pfd_count * sizeof(struct pollfd));

	/* Allocate memory for the server handler table */
	mem_malloc(handler, pfd_count * sizeof(struct server));

	/* Mask all signals and use kernel events instead */
	//DEADWOOD:mask_signals();
	//FIXME:signal_set(&signals[0], SIGINT, server_signal_handler, (void *) SIGINT);
	//signal_add(&signals[0], NULL);

	/* Instantiate each server ... */
	for (pfd_count = 0, i = 0, j = 0; constructor[i] != NULL; i++) {

		/* For each network address we listen on ... */
		for (cur = bind_addr->head; cur; cur = cur->next) {
			item = cur->value;

			/* Create a new server object */
			srv = NULL;
			server_new(&srv);

			/* Call the subclass constructor */
			if (constructor[i] != NULL) {
				if (constructor[i](srv) < 0)
					throw("server constructor failed");
			}

			/* Initialize the server socket */
			server_socket(srv);

			/* SA-NOTE: server_destroy(&srv) is never called */

			/* Register the server's session controller */
			/* Only do this once per server, not per-interface */
			if (cur == bind_addr->head) {
				session_controller_register(&controller_handle, 
						srv->controller);
			}
			srv->controller_handle = controller_handle;

			/* Copy the IP address of the current interface */
			if (srv->family != PF_LOCAL) {
				str_copy(srv->address, item);
			}

			/* Bind to the socket and start listening */
			if (socket_bind(srv->sock, srv->address, srv->port) < 0) {
				log_error("while binding to %s port %d",
						srv->address->value, srv->port);
				throw("unable to bind to socket");
			}

			/* Initialize the poll(2) descriptor */
			memset(&pfd[j], 0, sizeof(struct pollfd));
			pfd[j].fd = srv->sock->fd;
			pfd[j].events = POLLIN;

			/* Register the handler */
			handler[j] = srv;
			log_debug2("handler %d registered", j);
			//server_dump(handler[j]);

			/* Increment the server object counters */
			pfd_count++;
			j++;

			/* Don't create multiple PF_LOCAL server instances */
			if (srv->family == PF_LOCAL) 
				break;
		}
	}

	for (;;) {

		/* Wait for a connection on one of the socket descriptors */
		if ((i = poll(pfd, pfd_count, -1)) < 0) {

			/** @todo signal handling */
			if (errno == EINTR) 
				continue;

			throw_errno("poll(2)");
		}

		log_debug("%d connections waiting", i);

		/* Examine each poll(2) descriptor */
		for (j = 0, n = 0; n <= pfd_count && j < i; n++) {

			/* Test if a client is waiting */
			if (pfd[n].revents & POLLIN) {
				log_debug("accepting client for server #%d", n);
				
				(void) server_accept(handler[n]);
				j++;
				continue;
			}

			/* Test for an error condition */
			if (pfd[n].revents & POLLERR ||
					pfd[n].revents & POLLHUP ||
					pfd[n].revents & POLLNVAL) {
				log_error("poll(2) error or hangup on server #%d", n);
				server_dump(handler[n]);
			}
		}

		/* Ensure that all connections were handled */
		if (j < i) {
			throwf("error: %d unhandled connection(s)", (i-j));
		}
		
	}
}


/**
 * Dump information about a server object to the error log
 *
 * @param srv server object
 */
int
server_dump(server_t *srv)
{

	log_warning("dumping server object:\n"
			"uid=`%s' gid=`%s' "
			"service=`%s'"
			"address=`%s'"
			"port=%d"
			,
			srv->uid->value, srv->gid->value,
			srv->service->value,
			srv->address->value,
			srv->port
		   );
}
