/*		$Id: server.h 56 2007-04-16 00:54:41Z mark $		*/

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

#ifndef _NC_SERVER_H
#define _NC_SERVER_H

#include "nc_string.h"
#include "nc_socket.h"

#include <sys/types.h>

/* Defined in session.h */
struct session;
  
/** The maximum number of simultaneous client connections */
#define MAX_CLIENT_COUNT  10000

/** A server. */
typedef struct server {

	/** The user ID and group ID the server should run under */
 	string_t  *uid,
		  *gid;

	/** The name of the protocol being served, from /etc/services */
	string_t  *service;

	int        family;		/**< Protocol family (AF_INET or AF_INET6) */
	string_t  *address;		/**< Address the socket is bound to */
	int        port;		/**< Port number the socket is bound to */
	mode_t     mode;		/**< File permissions (only for AF_LOCAL) */
        bool       use_tls;		/**< Uses TLS if set to true */
	int        timeout; 		/**< Inactivity timeout, in seconds */

	/** The server socket used for the bind(2) call */
	socket_t   *sock;

	/** A session controller; function pointers for all aspects of a session
	 *
	 * This should be set by the user prior to calling server_run()
	 */
	struct session_controller *controller;

	/** A handle to a session controller.
	 *
	 * This is set automatically by server_run() and should not be modified.
	 */
	unsigned int controller_handle;
} server_t;


int server_new(server_t **srv);
int server_destroy(server_t **srv);
int server_socket(server_t *srv);
int server_multiplex(list_t *bind_addr, int (*constructor[])(server_t *));
int server_dump(server_t *srv);

/** @bug ugly hack */

#define server_accept		server_accept_multithreaded

/* Utility functions */

int daemonize(void);
int drop_privileges(const string_t *uid_str, const string_t *gid_str, const string_t *chrootdir);

#endif
