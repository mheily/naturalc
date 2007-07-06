/*		$Id: session.h 93 2007-05-09 04:24:26Z mark $		*/

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

#ifndef _NC_SESSION_H
#define _NC_SESSION_H

#include "nc_socket.h"

// @bug workaround for glibc problem
#define u_char unsigned char

#include <sys/types.h>

/** session_data is an opaque struct that must be defined by the user of this library */
struct session_data;

/* Defined in server.h */
struct server;

#define throw_response(session, code, msg) do { (void)response_set(session, code, msg, ""); log_warning("%s", msg); goto catch; } while (0)
#define throwf_response(session, fmt, arg) do { (void)printf_response(session, fmt, arg); log_warning(fmt, arg); goto catch; } while (0)
#define throwf_response(session, fmt, arg) do { (void)printf_response(session, fmt, arg); log_warning(fmt, arg); goto catch; } while (0)

typedef enum {
	SESSION_OK = 0,
	SESSION_BAD_REQUEST = -1,
	SESSION_FATAL_ERROR = -666,
	SESSION_QUIT = 42
} SESSION_RETURN_CODES;

typedef enum {
	SESSION_UNDEF = 0,
	SESSION_GREETING,
	SESSION_OPEN,
	SESSION_READ,
	SESSION_IDLE,
	SESSION_WRITE,
	SESSION_CLOSED,
} SESSION_STATE_CODES;

/** A client session */
typedef struct session {

	/** The login name of the user */
	string_t  *user;

	/** List of groups that the user belongs to */
	list_t    *groups;

	/** The server associated with the session (optional) */
	struct server *srv;

	/** Socket object */
	socket_t  *sock;

	/* Status information */
	time_t	start_time;		/**< Time the session was created */ 	
	time_t  expire_time;		/**< Time the session should be closed if idle */
	int     session_state;		/**< Current session state */
	int     protocol_state;		/**< Current protocol state */
	int 	error_count;		/**< Number of errors from the client */

	/** Argument vector for the current command */
	list_t       *argv;

	/** List containing context lines.
	 * For use with multi-line requests like in HTTP, some IMAP requests,
	 * and for parsing SMTP headers which can be multi-line. 
	 */
	list_t      *context;

	/** String containing the text of the response 
	 * @todo Convert all modules to set this instead of using print_response()
	 */
	struct {
		int        code;
		string_t  *header;
		string_t  *body;
		bool       asis;
	} response;

	/** Pointer to an opaque, protocol-specific data structure */
	struct session_data	*data;

	/** Handle to a session_controller_t object, as returned by session_controller_add() */
	unsigned int controller_handle;
} session_t;

/** References to elements within a session_controller_t
 *
 * NOTE: This must exactly match the order of the function pointers in session_controller_t
 */
typedef enum SESSION_HOOKS {
	SESSION_REQUEST_HANDLER = 0,
	SESSION_RESPONSE_HANDLER,
	SESSION_INIT,
	SESSION_RESET,
	SESSION_TIMEOUT,
	SESSION_OVERLOAD,
	SESSION_DESTROY,

	/* Not a function hook, this sets an upper limit for a function number */
	SESSION_HOOK_MAX
} session_hook_t;

/** Hooks for important points in a session's lifetime 
 *
 * NOTE: If you change this structure, you *MUST* modify the SESSION_HOOKS enum to match.
 */
typedef struct session_controller {
	int      (*request_handler_func)(struct session *, string_t *),
		 (*response_handler_func)(struct session *, int),
		 (*timeout_handler_func)(struct session *),
		 (*overload_handler_func)(struct session *),
		 (*session_init_func)(struct session *),
		 (*session_reset_func)(struct session *),
		 (*session_destroy_func)(struct session *);
} session_controller_t;

int session_controller_register(unsigned int *handle, session_controller_t *ctl);
int session_controller_invoke(session_t *s, session_hook_t func_num, void *arg);

/* Session functions */

int session_new(session_t **dest);
int session_connect(session_t *session, int family, string_t *address, uint16_t port);
int session_accept(session_t *s, struct server *srv);
int session_handler(session_t *s);
int session_destroy(session_t **s);
int session_reset(session_t *s);
int session_authenticate(bool *result, session_t *s, const string_t *account, const string_t *password);
int session_close(session_t *s);

/* Response functions */

int response_print(session_t *s, const char *msg);
int response_set(session_t *s, int code, char_t *header, char_t *body);
int response_reset(session_t *s);

int session_send_greeting(session_t *s);
int session_process_request(session_t *s);
int session_test(session_t *s, char_t *line, int expected_response);
int session_is_loopback(session_t *s);

/** @todo this is here because of a circular header dependency */
int server_accept(session_t **session_ref, struct server *srv);

#endif
