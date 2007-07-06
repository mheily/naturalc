/*		$Id: session.nc 93 2007-05-09 04:24:26Z mark $		*/

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
 *  Session handling.
 *
*/

#include "config.h"

#include "nc_exception.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_session.h"
#include "nc_server.h"
#include "nc_socket.h"
#include "nc_string.h"
#include "nc_thread.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>

/** The maximum number of unique session controllers that may be defined */
#define CONTROLLER_MAX  10

/** The current number of defined session controllers 
 * NOTE: CONTROLLER[0] is reserved for the null controller; any session
 * that sets it's controller_handle to 0 will not use any hooks.
 */
static unsigned int CONTROLLER_COUNT = 1;

/** An array of session controllers, to allow an application to serve
 * multiple types of sessions, different protocols, etc., within the 
 * same application.
 */
static session_controller_t CONTROLLER[CONTROLLER_MAX];

/**
 * Invoke a session controller hook function.
 *
 * @bug if a hook function is undefined, we will crash.
 */
int
session_controller_invoke(session_t *s, session_hook_t func_num, void *arg)
{
	int rc = 0;

	/* CONTROLLER[0] is a special, NOOP-like controller used by
	 * connect(2) sockets
	 */
	if (s->controller_handle == 0)
		return 0;

	/* Validate the controller handler */
	if (s->controller_handle >= CONTROLLER_COUNT) {
		throwf("invalid controller handle (hnum=%u)", s->controller_handle);
	}

	/* Validate the function number */
	if (func_num >= SESSION_HOOK_MAX)
		throw("invalid function number");

	/** @todo optimize:
	 *
	 * cast CONTROLLER to an array of function pointers and use
	 *        func_num in an equation like:
	 *
	 *        	CONTROLLER[s->contreller_handle][func_num]
	 *
	 * to eliminate the switch table. the only problem is, some functions
	 * have different parameter types.
	 *
	 */
	switch (func_num) {
		case SESSION_REQUEST_HANDLER:
			rc = CONTROLLER[s->controller_handle].request_handler_func(s, (string_t *) arg);
			break;

		case SESSION_RESPONSE_HANDLER:
			rc = CONTROLLER[s->controller_handle].response_handler_func(s, (long) arg);
			break;

		case SESSION_INIT:
			if (CONTROLLER[s->controller_handle].session_init_func) {
				rc = CONTROLLER[s->controller_handle].session_init_func(s);
			}
			break;

		case SESSION_RESET:
			if (CONTROLLER[s->controller_handle].session_reset_func) {
				rc = CONTROLLER[s->controller_handle].session_reset_func(s);
			}
			break;

		case SESSION_TIMEOUT:
			if (CONTROLLER[s->controller_handle].timeout_handler_func)
			{
				rc = CONTROLLER[s->controller_handle].timeout_handler_func(s);
			}
			break;

		case SESSION_OVERLOAD:
			if (CONTROLLER[s->controller_handle].overload_handler_func)
			{
				rc = CONTROLLER[s->controller_handle].overload_handler_func(s);
			}
			break;

		case SESSION_DESTROY:
			if (CONTROLLER[s->controller_handle].session_destroy_func) {
				rc = CONTROLLER[s->controller_handle].session_destroy_func(s);
			}
			break;

		case SESSION_HOOK_MAX:
			throw("invalid function id");
			break;
	}

	if (rc != 0)
		throw("controller: hook function returned an error");
}


/**
 * Register a controller with the global CONTROLLER table.
 *
 * @param handle destination buffer to store a handle to the newly created entry
 * @param ctl session controller object to be registered
 */
int
session_controller_register(unsigned int *handle, session_controller_t *ctl)
{
	if (CONTROLLER_COUNT >= CONTROLLER_MAX)
		throw("CONTROLLER_MAX limit has been reached");

	*handle = CONTROLLER_COUNT++;
	memcpy(&CONTROLLER[*handle], ctl, sizeof(*ctl));
}


static inline int
session_greeting(session_t *s)
{

	/* Send the greeting message */
	if (session_controller_invoke(s, SESSION_RESPONSE_HANDLER, (void *) 1) < 0)
		throw("error sending greeting");

	/* Wait for the client to respond */
	s->session_state = SESSION_READ;
}


int
response_reset(session_t *s)
{

	str_truncate(s->response.header);
	str_truncate(s->response.body);
	s->response.asis = false;
	s->response.code = 0;
}

int
response_set(session_t *s, int code, char_t *header, char_t *body)
{

	s->response.code = code;
	str_cpy(s->response.header, header);
	str_cpy(s->response.body, body);
	log_debug2("response: code=%u header=`%s' body=`%s'", s->response.code, s->response.header->value,
			s->response.body->value);
}


int
session_new(session_t **dest) 
{
	session_t *s      = NULL;
	string_t  *path   = NULL;

	/* Allocate memory for a new session */
	mem_calloc(*dest);
	s = *dest;

	/* Initialize the object members */
	list_new(&s->argv);
	list_new(&s->context);
	list_new(&s->groups);
	str_new(&s->response.body);
	str_new(&s->response.header);
	str_new(&s->user);
	s->response.code = 0;
	s->response.asis = false;
	s->session_state = SESSION_OPEN;
	s->start_time = time(NULL);
	s->controller_handle = (unsigned int) -1;

finally:
	destroy(str, &path);
	if (*dest == NULL) 
		(void) session_destroy(&s);
}


/**
 * Initiate a connection to another host.
 *
 * @param session session object
 * @param family address family := AF_INET | AF_INET6 | AF_LOCAL
 * @param host remote hostname 
 * @param port remote port 
 * @return 0 if connection succeeded, -1 if an error occurred.
*/
int
session_connect(session_t *session, int family, string_t *host, uint16_t port)
{

	/* Outgoing sessions do not use the controller mechanism */
	session->controller_handle = 0;

	/* Create a socket object as needed */
	if (!session->sock) {
		socket_new(&session->sock);
		socket_set_family(session->sock, family);
	}

	/* Make the connection */
	socket_connect(session->sock, host, port);

	/* SA-NOTE: destroy(session->sock) will be called during the session destructor */
	
catch:
	log_error("could not connect to `%s'", host->value);
}


/**
 * Close all mailbox, folder, and message objects associated with a session.
 * This is called prior to session_destroy().
 *
 * @param s session to be closed
 * @todo remove maild-specific actions, make them hooks instead
*/
int
session_close(session_t *s)
{

	/* Check the session state */
	if (s->session_state == SESSION_UNDEF)
		throw("tried to close an undefined session");
	if (s->session_state == SESSION_CLOSED) {
		log_warning("tried to close an already closed session (%d)", 0);
		return 0;
	}

	/* Remove the authentication information */
	str_truncate(s->user);
	list_truncate(s->groups);

	/* Close the mailbox */
#if FIXME
	//XXX-FIXME
	if (s->mbox != NULL)
		mailbox_close(s->mbox);
#endif

	/* Close the client socket */
	socket_close(s->sock);

	s->session_state = SESSION_CLOSED;
}


int
session_destroy(session_t **session_ref)
{
	session_t *s = NULL;

	/* Don't free the session twice, or a segfault will occur */
	s = *session_ref;

	/* Deallocate memory */
	list_destroy(&s->argv);
	list_destroy(&s->context);
	list_destroy(&s->groups);
	str_destroy(&s->response.body);
	str_destroy(&s->response.header);
	str_destroy(&s->user);
	
	/* Run the protocol-specific cleanup hook */
	session_controller_invoke(s, SESSION_DESTROY, NULL);

	socket_destroy(&s->sock);
	free(*session_ref);

	*session_ref = NULL;
}


/**
 * Reset the variables of the session to allow for pipelining requests.
 *
 * @param s session to be reset
*/
int
session_reset(session_t *s)
{

	s->error_count = 0;

#if FIXME	
	//XXX-FIXME
	/* Delete the contents of the previous message object */
	message_reset(s->msg);
#endif

	/* Run the protocol-specific reset hook */
	session_controller_invoke(s, SESSION_RESET, NULL);
}


/* Process a single line of input from the remote client */
int
session_process_request(session_t *s)
{
	long        rc;
	string_t  *line;

	/*
	//log_debug("socket input buffer follows: %d", 0);
	//list_print(s->sock->input);
	 */

#if DEADWOOD
	// now using set_socket_timeout() and setsockopt(..SO_RCVTIMEO) instead
	
	/* Block until data is ready or the timeout expires */
	socket_select(s->sock, SOCK_READ, s->srv->timeout);

#endif

	/* Read a line */
	socket_readline(line, s->sock);

	/* Check if a timeout occurred */
	if (s->sock->status.timeout) {
		s->session_state = SESSION_TIMEOUT;
		return 0;
	}

	/* Reset the response */
	response_reset(s);

	/* Run the request handler */
	rc = (long) session_controller_invoke(s, SESSION_REQUEST_HANDLER, line); 

	/* Run the response handler */
	session_controller_invoke(s, SESSION_RESPONSE_HANDLER, (void *) rc); 
}


/**
 * Create a new client session based on a pending server connection.
 *
 * @param s new session
 * @param srv server
*/
int
session_accept(session_t *s, struct server *srv)
{

	/* Create a socket object for the new client */
	/* SA-NOTE: socket_destroy(&s->sock) called by session_destroy() */
	socket_new(&s->sock);
	socket_set_family(s->sock, srv->family);
	s->srv = srv;

	/* Copy the session controller handle from the server */
	s->controller_handle = srv->controller_handle;

	/* Accept(2) an incoming connection */
	socket_accept(s->sock, srv->sock);

	/* Set the socket timeout */
	socket_set_timeout(s->sock, srv->timeout, 60);

	/* Determine the DNS hostname of the client */
	/** @bug need to understand evdns_callback_type */
	//evdns_resolve_reverse(&s->sock->addr.in.sin_addr, 0, session_greeting, s);
}


/*
 * session_handler(session)
 *
 * Handle all line-oriented client/server communication for an entire <session>.
 *
 * Intended to be called by pthread_create(3) after a successful session_accept() call.
 *
 */
int
session_handler(session_t *s)
{

	/* Send the greeting */
	s->session_state = SESSION_GREETING;
	session_greeting(s);

	/* In the SESSION_READ state, cycle through input from the remote client */
	while (s->session_state == SESSION_READ) {

		/* Process the input buffer */
		(void) session_process_request(s);

	}

	/* Send the 'timed out' error message to the client */
	if (s->session_state == SESSION_TIMEOUT && s->sock->status.connected) {
		(void) session_controller_invoke(s, SESSION_TIMEOUT, NULL); 
	}	

	log_debug("terminating session %lu", (unsigned long) thread_get_id());
	if (s) {
		session_close(s);
		destroy(session, &s);
	}
	thread_exit(0);
}


/**
 * Determine if a session is being conducted via a loopback interface.
 *
 * @todo OPTIMIZE: unnecessary ASCII conversion.
 * @todo is this function even needed?
 *
*/
int
session_is_loopback(session_t *s)
{
	string_t *buf;

	str_from_inet(buf, s->sock->remote.in.sin_addr);
	if (str_cmp(buf, "127.0.0.1") < 0)
		throw_silent();
}


/**
 * Simulate the reciept of a line of input from a socket and get the response code.
 *
 * @param s session object
 * @param line line of simulated client input
 * @param expected_response response code expected from the server
 * @todo this function should replace session_simulate_request()
 */
int
session_test(session_t *s, char_t *line, int expected_response)
{
	string_t *buf;

	str_cpy(buf, line);
	str_putc(buf, '\n');
	if (session_controller_invoke(s, SESSION_REQUEST_HANDLER, (void *) line) < 0)
		throw("error in handler function");
	
	/* Check the response code */
	if (s->response.code != expected_response) {
		log_error("expecting response of %d but got %d (header=`%s' body=`%s')", 
				expected_response, 
				s->response.code,
				s->response.header->value,
				s->response.body->value
				);
		throw("test failed");
	}
}
