/*		$Id: socket.nc 71 2007-04-21 21:51:37Z mark $		*/

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
 *  Berkeley sockets library 
 *
*/
#include "config.h"

#include "nc_dns.h"
#include "nc_exception.h"
#include "nc_file.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_memory.h"
#include "nc_passwd.h"
#include "nc_string.h"

#include "nc_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

/** vasprintf(3) is a GNU extension and not universally visible */
extern int      vasprintf(char **, const char *, va_list);

/*** @todo make this configurable and documented */
#define SOCKET_DEBUG 1

/* Global OpenSSL context object */
#if WITH_OPENSSL
static SSL_CTX *TLS_CTX;
#endif


/* Some platforms (e.g. Linux) don't have getpeereid(2) */
#if ! HAVE_GETPEEREID
static int
getpeereid(int fd, uid_t *euid, gid_t *egid)
{ 
/* @bug NetBSD doesn't have SO_PEERCRED; it uses CRED_LOCAL instead */
#if defined(SO_PEERCRED)
	struct ucred cred;
	socklen_t len;

	len = sizeof(cred);
	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) < 0)
		throw_silent();

	*euid = cred.uid;
	*egid = cred.gid;
#else
	if (fd || euid || egid)  
		throw_fatal("ERROR: porting required");
#endif
}
#endif


/**
 * Start a TLS session on a socket that is already connected.
 *
 * @param s a connected socket
*/
int
socket_starttls(socket_t *s)
{

#if WITH_OPENSSL

	/* Don't call this function multiple times */
	if (s->tls_enabled)
		throw("cannot start TLS session multiple times");

	/* Create the client's SSL object */
	if (!(s->ssl = SSL_new(TLS_CTX)))
		throw("Error creating SSL context");

	/* Create the clients BIO object */
	s->bio = BIO_new(BIO_s_socket());
	if (s->bio == NULL)
		throw("error creating the BIO object");
	if (!BIO_set_fd(s->bio, s->fd, BIO_NOCLOSE))
		throw("error attaching the BIO to the file descriptor");

	/* Perform the TLS/SSL handshake */
	throw("TODO - STUB");

	s->tls_enabled = true;

	log_debug("TLS session established on fd #%d", s->fd);
#else
	if (1 || s != NULL) {
		throw("OpenSSL support was not compiled into the maild(8) binary");
	}
#endif

	/* SA-NOTE: destroy(s->ssl) called during socket_destroy() */
	/* SA-NOTE: destroy(s->bio) called during socket_destroy() */
}


static int
socket_new_inet(socket_t *s, string_t *address, int port)
{

	/* Change the port number to 10000 + port if running as an unprivileged user */
	if (port <= 1024 && getuid() > 0) {
		log_warning("changing reserved port %d to %d", port, port + 10000);
		port += 10000;
	}

	/* Initialize the socket_t structure */
	s->local.in.sin_port = htons(port);
	s->direction = UNKNOWN;
	s->family = PF_INET;

	/* Convert the ASCII network address into a numeric address */
	str_to_inet(&s->local.in.sin_addr, address);

#if WITH_OPENSSL
	/* Initialize the TLS variables */
	if (s->tls_enabled) {
		s->bio = BIO_new_accept("443");
		if (!s->bio)
			throw("error creating server socket");
		if (!BIO_set_bind_mode(s->bio, BIO_BIND_REUSEADDR_IF_UNUSED))
			throw("error setting bind mode");
	}
#endif
}


int
socket_new(socket_t **dest)
{
	socket_t *s = NULL;
	
	/* Allocate memory for thee socket_t structure */
	mem_calloc(*dest);
	s = *dest;

	list_new(&s->read_buf);

	/* Set defaults */
	s->fd = -1;
	s->direction = UNKNOWN;
	s->family = -1;
	s->local.a.sa_family = -1;
}


int
socket_destroy(socket_t **s)
{
	socket_t *cur = NULL;

	if (*s == NULL)
		return 0;

	cur = *s;

#if WITH_OPENSSL
	/* Destroy the TLS objects */
	if (cur->tls_enabled) {
		SSL_free(cur->ssl);
		//FIXME: this seems to cause a double free(): BIO_free(cur->bio);
	}
#endif

	list_destroy(&cur->read_buf);

	/* Free the object */
	free(cur);

	*s = NULL;
}


/**
 * Set the protocol family of a socket
 *
 * @param sock socket object
 * @param family family (PF_INET, PF_INET6, etc.)
 */
int
socket_set_family(socket_t *sock, int family)
{

	if (sock->family >= 0)
		throw("socket family is already set and cannot be changed");

	sock->family = family;
	sock->local.a.sa_family = family;
}


/**
 * Connect to an IPv4 host.
 *
 * @param s a socket object
 * @param host remote host
 * @param port remote port
*/
static int
socket_connect_inet(socket_t *s, string_t *host, const int port)
{
	list_entry_t *cur = NULL; 
	int i;
	struct sockaddr_in sa;
	list_t   *inet;
	string_t  *buf;

	/* Initialize variables */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = PF_INET;
	sa.sin_port = htons(port);

	/* Get the address of the remote host */
	dns_get_inet_by_name(inet, host);
			
	/* Create a socket descriptor */
	if ((s->fd = socket(s->family, SOCK_STREAM, 0)) < 0)
        	throw_errno("socket(2)");

	/* Use non-blocking I/O */
	if (fcntl(s->fd, F_SETFL, O_NONBLOCK) < 0)
		throw_errno("fcntl(2)");

	/* Try to connect to each possible IP address */
	for (cur = inet->head; cur; cur = cur->next) {
		str_to_inet(&sa.sin_addr, cur->value);
		log_debug("connecting to %s port %d", cur->value->value, port);

		/* Initiate the connection to the remote server */
		i = connect(s->fd, (struct sockaddr *) &sa, 
				sizeof(struct sockaddr_in));
				
		/* In non-blocking mode, connect(2) returns EINPROGRESS */
		if (i < 0 && errno != EINPROGRESS) {
			log_warning("connect(2) failed with errno %d", errno);
			continue;
		}

		/* Set the socket to connected so that socket_select() does't fail */
		s->status.connected = 1;

		/* Wait up to 30 seconds for the connection to be established */
		/** @bug no error handling ! */
		socket_select(s, SOCK_WRITE, 30);
		
		/* We now have a valid connection. */
		memcpy(&s->local.in, &sa, sizeof(struct sockaddr_in));

		goto finally;
	}

	/* Unable to connect to any of the remote addresses */
	throw("connect(2)");
}


/**
 * Bind a socket to a port on an IPv4 interface.
 *
 * This function is a basically a wrapper around bind(2).
 *
 * @param s socket object
*/
static int
socket_bind_inet(socket_t *s)
{
	int i;

	/* Non-root users cannot bind to ports lower than 1024 */
	if (getuid() > 0 && s->local.in.sin_port <= 1024) {
		s->local.in.sin_port += 1000;
	}

#if WITH_OPENSSL
	// FIXME: why is this accept() and not bind() ?
	if (s->tls_enabled) {
		if (BIO_do_accept(s->bio) <= 0)
			throw("error binding server socket");
		s->fd = BIO_get_fd(s->bio, NULL);
		log_debug("bound to a TLS socket on fd #%d", s->fd);
		return 0;
	}
#endif

	i = bind(s->fd, &s->local.a, sizeof(struct sockaddr));
	if (i < 0 && errno == EADDRINUSE) {
		log_error("on port %d ...", ntohs(s->local.in.sin_port));
		throw("address is already in use");
	}
	if (i < 0)
		throw_errno("bind(2)");

	log_debug("bound fd %d to port %d", s->fd, ntohs(s->local.in.sin_port));
}


static int
socket_local_operation(socket_t *s)
{
	string_t path = EMPTY_STRING;
	bool     exists;

	log_debug("local socket addr=`%s'", s->local.un.sun_path);

	/* Create a socket descriptor */
	if ((s->fd = socket(s->family, SOCK_STREAM, 0)) < 0)
        	throw_errno("socket(2)");

	/* Bind to the socket */
	if (s->direction == LISTEN) {

		str_alias(&path, s->local.un.sun_path);

		/* Remove any existing socket file */
		file_exists(&exists, &path);
		if (exists) {
			file_unlink(&path);
		}

		/* Bind to the socket file */
		if (bind(s->fd, &s->local.a, SUN_LEN(&s->local.un)) < 0)
			throw_errno("bind(2)");

		/* Assign filesystem credentials */
		log_debug("setting mode of %s to %d; setting ownership to %d:%d",
			s->local.un.sun_path, s->uid, s->gid, s->mode);
		if (chown(s->local.un.sun_path, s->uid, s->gid) < 0)
			throw_errno("chown(2)");
		file_chmod(&path, s->mode);

		log_debug("bound to local socket at %s (uid=%d gid=%d)"
				, s->local.un.sun_path, s->uid, s->gid);

	/* Connect the socket */
	} else if (s->direction == CONNECT) {
		if (connect(s->fd, &s->local.a, SUN_LEN(&s->local.un)) < 0)
			throw_errno("connect(2)");

		log_debug("connected to local socket at %s", s->local.un.sun_path);
		s->status.connected = true;

	} else {
		throw("unsupported socket direction");
	}
}


/**
 * Set a timeout for read(2) and write(2) operations on a socket.
 *
 * @param sock a socket
 * @param read_sec the socket read(2) timeout, in seconds
 * @param write_sec the socket write(2) timeout, in seconds
 */
int
socket_set_timeout(socket_t *sock, int read_sec, int write_sec)
{
	struct timeval tv;

	tv.tv_usec = 0;

	/* Set the read(2) timeout */
	tv.tv_sec = read_sec;
	if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
		throw_errno("setsockopt(2)");

	/* Set the write(2) timeout */
	tv.tv_sec = write_sec;
	if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0)
		throw_errno("setsockopt(2)");
}


/**
 * Set the file permissions and ownership for a PF_LOCAL socket.
 *
 * @param sock socket object
 * @param user user name to set as the file owner
 * @param group group name to set as the file group
 * @param mode file permissions for the socket
*/
int
socket_set_credentials(socket_t *sock,
		string_t *user,
		string_t *group,
		mode_t mode)
{

	/* Validate inputs */
	if (sock->family != PF_LOCAL)
		throw("invalid socket type (must be PF_LOCAL)");
	if (sock->direction == CONNECT)
		throw("socket must be set to listen mode");
	if (sock->status.connected)
		throw("socket has already been opened");

	/* Update the socket structure */
	passwd_get_id_by_name(&sock->uid, user);
	group_get_id_by_name(&sock->gid, group);
	sock->mode = mode;
}


/**
 * Toggle the non-blocking mode for a socket.
 *
 * @param sock socket to be modified
 * @param enabled if TRUE, non-blocking I/O operations will be enabled
 * @bug 0L is wrong; need to save flags first using fctnl, then restore them
 */
static int UNUSED
socket_set_blocking_mode(socket_t *sock, bool enabled)
{
	if (sock->status.non_blocking != enabled) {
		if (fcntl(sock->fd, F_SETFL, enabled ? O_NONBLOCK : 0L ) < 0)
			throw_errno("fcntl(2)");
		sock->status.non_blocking = enabled;
	}
}

/**
 * Accept a pending connection on socket and create a new client socket.
 *
 * Before calling this function, there should be at least one pending 
 * connection in the server socket's accept(2) queue. The caller should
 * check the value of client->status.connected in case the connection was aborted.
 * 
 * @param dest client socket to be created
 * @param src server socket 
*/
int
socket_accept(socket_t *dest, socket_t *src)
{
	struct sockaddr_storage addr;
	socklen_t len = (socklen_t) sizeof(addr);
	string_t *buf;

#if WITH_OPENSSL
	/* Special case: TLS servers use the BIO_* routines */
	if (src->tls_enabled) {
		if (BIO_do_accept(src->bio) <= 0)
			throw("error binding server socket");
		dest->bio = BIO_pop(src->bio);
		if (!(dest->ssl = SSL_new(TLS_CTX)))
			throw("error creating SSL context");

		SSL_set_bio(dest->ssl, dest->bio, dest->bio);
		dest->fd = BIO_get_fd(dest->bio, NULL);
		dest->tls_enabled = true;

		goto established;
	}
#endif

	/* Block until a connection is ready */
	do {
		dest->fd = accept(src->fd, (struct sockaddr *) &addr, &len);
		if (dest->fd >= 0) {
			goto established;
		} else {
			switch (errno) {

				/* Retry if accept(2) was interrupted by a signal */
				case EINTR: 
					continue;

				case ECONNABORTED:
					dest->status.connected = 0;
					goto finally;

				default:
					log_error("while accepting a connection on fd# %d",
							src->fd);
					throw_errno("accept(2)");

			}
		}
	} while (1);
	
established:

	/* Remote socket is now valid */
	dest->status.connected = true;
	dest->direction = CONNECT;
	dest->family = src->family;

	/* Determine the IP address of the client */
	if (src->family == PF_INET) {
		if (getpeername(dest->fd, (struct sockaddr *) &addr, &len) < 0) 
			throw_errno("getpeername(2)");
		memcpy(&dest->remote, &addr, (size_t) len);

		str_from_inet(buf, dest->remote.in.sin_addr);
		log_info("incoming connection on port %d (fd #%d) from %s", 
				ntohs(src->local.in.sin_port),
				dest->fd,
				buf->value);
	}

}


/**
 * Bind an IPv4 socket to a given address and port.
 *
 * @param s socket object
 * @param address string containing the IPv4 address to bind to, or "0.0.0.0" to bind to all addresses.
 * @param port port to bind to
*/
int
socket_bind(socket_t *s, string_t *address, int port)
{
	int	one = 1;
	
	log_debug("binding to %s port %d", address->value, port);

	/// @todo guess the family from the address and then set_family()
	
	/* Create an INET socket on the requested interface */
	if (s->family == AF_INET) {

		socket_new_inet(s, address, port);

	/* For AF_LOCAL sockets, set the path */
	} else if (s->family == AF_LOCAL) {
		memset(s->local.un.sun_path, 0, sizeof(s->local.un.sun_path));
		strncpy(s->local.un.sun_path, address->value,
					sizeof(s->local.un.sun_path) - 1);

	/* Otherwise, fail if the family type is unknown */
	} else {
		throw("unknown socket family");
	}

	s->direction = LISTEN;

	/* Create a socket descriptor */
	if ((s->fd = socket(s->family, SOCK_STREAM, 0)) < 0)
        	throw("socket(2)");

	/* Permit addresses to be reused */
	setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR,
			(char *) &one, sizeof(one));

	/* Bind to the socket address */
	switch (s->family) {
		case PF_INET:
			socket_bind_inet(s);
			break;

		case PF_LOCAL:
			socket_local_operation(s);
			break;

		default:
			throw("unsupported socket domain");
	}

	/* Set the backlog */
	if (listen(s->fd, 300) < 0)
		throw("listen(2)");
}


/**
 * Connect an IPv4 socket to a given host and port.
 *
 * @param s socket object
 * @param host remote hostname
 * @param port remote port
*/
int
socket_connect(socket_t *s, string_t *host, uint16_t port)
{

	s->direction = CONNECT;

	/// @todo guess the family from the address and then set_family()
	
	/* Connect to an Internet socket */
	if (s->family == AF_INET) {
		s->local.in.sin_port = htons( (int) port );
		if (socket_connect_inet(s, host, (int) port) < 0) {
			throwf("unable to connect to %s:%d", host->value, port);
		}

	/* Connect to a local socket */
	} else if (s->family == AF_LOCAL) {
		strncpy(s->local.un.sun_path, 
					host->value, 
					sizeof(s->local.un.sun_path) - 1);
		if (socket_local_operation(s) < 0) {
			throwf("local connection to `%s' failed", host->value);
		}

	/* No other types are supported */
	} else {
		throw("unsupported address family");
	}
}


/**
 * Call select(2) to wait for an event on a socket.
 *
 * Waits up to @a timeout seconds for an event matching the @a event_mask 
 * to occur on a @a socket.
 * 
 * @param s socket object
 * @param flags an event mask := SOCK_READ | SOCK_WRITE |SOCK_EXCEPTION
 * @param timeout number of seconds to wait for an event, or -1 to wait forever
 *
 * @return 0 if a matching event has occured or the timeout occurred
 *         -1 if a socket error occurred.
 *
*/
int
socket_select(socket_t *s, int flags, int timeout)
{
	bool want_read, want_write, want_except;
	fd_set	 fds, fds_read, fds_write, fds_except;
	int	 n;
	struct timeval	tv;

	/** @todo clear previous status flags except for status.connected */

	/* Convert the flags into boolean options */
	want_read = flags & SOCK_READ;
	want_write = flags & SOCK_WRITE;
	want_except = flags & SOCK_EXCEPTION;

	/* Abort if the socket is disconnected */
	/* Assuming that a non-infinite timeout is an established connection */
	if (timeout > 0 && s->status.connected == 0)
		throw("socket is not connected");

	/* Create the fd_set with one socket in the list */
	FD_ZERO(&fds);
	FD_SET(s->fd, &fds);

retry:
	/* Set the timeout structure */
	if (timeout >= 0) {
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
	}

	/* Copy the stored fd set into the working sets */
	if (want_read)
		memcpy(&fds_read, &fds, sizeof(fds));
	if (want_write)
		memcpy(&fds_write, &fds, sizeof(fds));
	if (want_except)
		memcpy(&fds_except, &fds, sizeof(fds));

	/* Wait for data on the fd set */
	log_debug2("waiting %d seconds for data on fd #%d", timeout, s->fd);
	n = select(FD_SETSIZE, 
			want_read ? &fds_read : NULL,
			want_write ? &fds_write : NULL,
			want_except ? &fds_except : NULL,
			(timeout >= 0 ? &tv : NULL)
			);

	/* Retry if interrupted by a signal */
	if (n < 0 && errno == EINTR) {
		log_debug("interrupted by signal, retrying... %d", 0);
		goto retry; 
	}

	/* Fail if any other error */
	if (n < 0)
		throw_errno("select(2)");

	/* If select(2) returns zero, the timeout was reached */
	if (n == 0) {
		log_debug(" ... timeout on fd #%d after %d seconds", s->fd, timeout);
		s->status.timeout = 1;
		return 0;
	}

	/* Check for 'ready to read' events */
	if ( want_read && FD_ISSET(s->fd, &fds_read) ) {
		//log_debug(" ... fd #%d is ready for reading", s->fd);
		s->status.read = 1;
	}

	/* Check for 'ready to write' events */
	if ( want_write && FD_ISSET(s->fd, &fds_write) ) {
		//log_debug(" ... fd #%d is ready for writing", s->fd);
		s->status.write = 1;
	}

	/* Check for 'exceptional condition' events */
	if ( want_except && FD_ISSET(s->fd, &fds_except) ) {
		log_debug(" ... fd #%d is ready to throw an exception", s->fd);
		s->status.exception = 1;
	}
}


static int
socket_vprintf(socket_t *sock, const char *format, va_list ap)
{
	var_char_t    *buf = NULL;
	size_t         len;

	/* Generate the result buffer */
	if ((len = vasprintf(&buf, format, ap)) < 0)
		throw("memory error");

	/* Write the buffer to the socket */
	socket_write(sock, (const char *) buf, len);

finally: 
	free(buf);
}


/**
 * Print a formatted string to a socket.
 *
 * Uses printf(3) formatting syntax.
 *
 * @param sock socket object
 * @param format format string
*/
int
socket_printf(socket_t *sock, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	socket_vprintf(sock, format, ap);

finally: 
	va_end(ap);
}


#if WITH_OPENSSL

static int
socket_read_tls(socket_t *sock, var_char_t *buf, size_t size)
{
	ssize_t i;

	i = 0;

	require (size > 2);
	
	i = SSL_read(sock->ssl, buf, size - 1);
	switch (SSL_get_error(sock->ssl, i)) {

		case SSL_ERROR_NONE: 
			break;

		case SSL_ERROR_ZERO_RETURN:
			throw("TLS socket was unexpectedly closed");
			break;

		case SSL_ERROR_WANT_READ:
			throw("want read");
			break;

		case SSL_ERROR_WANT_WRITE:
			throw("want write");
			break;

		case SSL_ERROR_WANT_CONNECT:
			throw("want connect");
			break;

		case SSL_ERROR_WANT_ACCEPT:
			throw("want accept");
			break;

		case SSL_ERROR_WANT_X509_LOOKUP:
			throw("want X.509 lookup");
			break;

		case SSL_ERROR_SYSCALL:
			//ERR_print_errors_fp(stderr);
			throw("SSL_read(3) returned SSL_ERROR_SYSCALL (Some I/O error occurred)");
			throw("error in syscall");
			break;
			
		case SSL_ERROR_SSL:
			//ERR_print_errors_fp(stderr); 
			throw("SSL_read(3) returned SSL_ERROR_SSL (A failure in the SSL library occurred)");
			break;

		default:
			throwf("SSL_read(3) returned an undefined error %d", i);

	}

	log_debug("SSL_read(3) returned `%s'", buf);
}
#endif


/**
 * Read a single line of input from a socket.
 *
 * @param dest buffer where the result will be stored
 * @param sock socket object
*/
int
socket_readline(string_t *dest, socket_t *sock)
{
	ssize_t     i = 0;
	string_t   *line, *fragment;
	list_t     *tok;
	var_char_t  buf[RECV_BUF_SIZE];

	/* If there is a complete line on the input buffer, return it */
	if (sock->read_buf->count > 0 && sock->status.fragmented == 0) {
			list_shift(dest, sock->read_buf);
			return 0;
	}

	/* Read all of the data the caller has sent to us */
read_loop:
	do {

		/* Clear the contents of the temporary buffer */
		memset(buf, 0, sizeof(buf));

		/* Read  data */
		i = read(sock->fd, buf, sizeof(buf) - 1);
		if (i < 0 ) {
			switch (errno) {

				/** @todo handle signals */
				case EINTR:
					goto read_loop;

				case EAGAIN: 
					sock->status.timeout = true;
					return 0;
					break;

				case EBADF: 
					sock->status.connected = 0;
					sock->fd = -1;
					break;

				default: 
					throw_errno("read(2)");
			}
		}

		if (i > 0) {
			str_cat(line, buf);

		} else if (i == 0) {
			/* WORKAROUND: Is this correct behavior? */
			sock->status.connected = false;
			break;
		} 

		/* Continue reading... */
		continue;
	} while (str_len(line) == 0 && sock->status.connected);

	/* If no data was read, the socket has gone bad */
	if (str_len(line) == 0) {
		sock->status.connected = 0;
		//list_print(sock->read_buf);
		throw("truncated read(2), connection aborted");
	}

	/* If there is not a complete line, keep reading.. */
	if (sock->status.connected && strchr(line->value, '\n') == NULL) {
		goto read_loop;
	}

	/* Convert it to a list of lines */
	log_debug2("line=`%s'", line->value);
	str_split(tok, line, '\n');

	/* If the last element of the input buffer is a fragment,
	   merge it with the first element of the new list */
	if (sock->status.fragmented) {

		//@todo don't touch list internals here
		/* Validate list internal pointers */
		if (sock->read_buf->tail == NULL)
			throw("logic error; an empty list cannot contain a fragment");
		if (tok->head == NULL)
			throw("logic error; the token list has no head");

		/* Copy the first element of the new list onto the end of the tail */
		str_append(sock->read_buf->tail->value, tok->head->value);

		/* Remove the first entry of the new list */
		list_shift(fragment, tok);
	}

	/* Append the list of lines to the socket input buffer */
	list_merge(sock->read_buf, tok);

	/* If the line isn't LF-terminated, set the fragmentation indicator */
	sock->status.fragmented = (str_cmp_terminator(line, '\n') != 0);

	/* Shift the first line from the list and move it to the caller */
	list_shift(dest, sock->read_buf);

	log_debug("<<< %s\n", dest->value);
}


/**
 * Read from a socket.
 *
 * @param dest a string buffer to hold the result
 * @param size the maximum number of bytes to read
 * @param socket the socket to read from
 */
int
socket_read(string_t *dest, size_t size, socket_t *sock)
{

	str_read(dest, sock->fd, size);
}


/**
 * Write a string to a socket.
 * 
 * @param sock socket object
 * @param src string to be written
 * @param len length of the string
*/
int
socket_write(socket_t *sock, const char *src, size_t len)
{
	ssize_t    bytes;

	/* Do not write empty strings */
	if (len == 0)  
		return 0;
	
	/* Write the entire buffer */
	bytes = write(sock->fd, src, len);
	if (bytes <= 0) {
		/* No data could be written to the file descriptor */
		log_error("while writing to fd %d family %d ...", 
				sock->fd,
				sock->family
			 );
		throw_errno("write(2)");
	}

	/* Check for a short write */
	if (bytes != len) 
		throw("short write(2) detected");
}


/**
 * Close a socket.
 *
 * @param sock socket object
*/
int
socket_close(socket_t *sock)
{

	/* Don't close a socket twice */
	if (sock->fd < 0)
		return 0;

	log_debug("closing transmission %s", "channel");

#if DEADWOOD
	// this breaks the unit test

	/* Shutdown the socket */
	if (shutdown(sock->fd, SHUT_RDWR) < 0)
		throw_errno("shutdown(2)");
#endif

	if (close(sock->fd) < 0)
		throw_errno("close(2)");
	sock->fd = -1;
}


/**
 * Retrieve the user ID and group ID of the remote end of a local domain socket.
 *
 * @param uid pointer to a user ID that will store the result
 * @param gid pointer to a group ID that will store the result
 * @param sock socket object belonging to the PF_LOCAL domain.
*/
int
socket_get_credentials(uid_t *uid, gid_t *gid, socket_t *sock)
{

	if (sock->family != PF_LOCAL)
		throw("cannot get socket credentials: not an PF_LOCAL socket");

	if (getpeereid(sock->fd, uid, gid) < 0)
		throw_errno("getpeereid(2)");
}

/**
 * Get the IP address of the remote peer of a connected socket.
 *
 * @param dest string to store the resulting IP address
 * @param src a connected socket object
*/
int
socket_get_peer_addr(string_t *dest, socket_t *src)
{

	switch (src->family) {

		case PF_INET:
			str_from_inet(dest, src->remote.in.sin_addr);
			break;

		/* Pretend that local-domain sockets are loopback IPv4 */
		case PF_LOCAL:
			str_cpy(dest, "127.0.0.1");
			break;

		default:
			throwf("socket family %d not supported", src->family);
	}
}


/**
 * Lookup the DNS hostname of the remote peer of a connected socket.
 *
 * @bug not threadsafe
 *
 * @todo add support for IPv6
 * @param name stores the result; the IPv4 hostname
 * @param addr a connected socket
 */
int
socket_get_peer_name(string_t *result, const socket_t *sock)
{
	//int  gai_errno;
	char hbuf[NI_MAXHOST];

	if (!sock->status.connected)
		throw("invalid argument: socket is not connected");

	/* For local-domain sockets, pretend the hostname is 'localhost' */
	if (sock->family == PF_LOCAL) {
		str_cpy(result, "localhost");
		return 0;
	}

#if FIXME
	// broken on BSD
	
	/* Perform a DNS lookup */
	gai_errno = getnameinfo(&sock->remote.a, 
				sizeof (struct sockaddr_in),
				hbuf, sizeof(hbuf), 
				NULL, 0,
				NI_NAMEREQD);
	if (gai_errno != 0) {
		log_error("%s", gai_strerror(gai_errno));
		throw("could not resolve hostname for socket peer");
	}
#else 
	if (inet_ntop(AF_INET, &sock->remote.in.sin_addr, (char *) &hbuf, sizeof(hbuf))== NULL)
		throw_errno("inet_ntop(3)");
	
#endif

	/* Copy the result to the caller */
	str_cpy(result, hbuf);
}

/**
 * Initialize socket-related libraries, such as OpenSSL and libevent.
 *
*/
int
socket_init_library()
{

#if WITH_OPENSSL

	string_t  *item = NULL;
	SSL_CTX   *ctx  = NULL;
	//size_t     count = 0;

	/* Don't initialize OpenSSL unless the 'tls_enabled' option is set */
	if (hash_exists(options, "tls_enabled") < 0)
		return 0;

	/* Load all OpenSSL strings into memory */
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	/* Initialize the SSL library */
	SSL_library_init();

	/* Create a new TLS server context */
	ctx = SSL_CTX_new(SSLv23_server_method());

	/* Load the certificate */
	hash_get(&item, options, "tls_cert_file");
	if (SSL_CTX_use_certificate_chain_file(ctx, item->value) != 1)
		throwf("error loading TLS certificate from `%s'", item->value);

	/* Load the private key */
	hash_get(&item, options, "tls_key_file");
	if (SSL_CTX_use_PrivateKey_file(ctx, item->value, SSL_FILETYPE_PEM) != 1)
		throwf("error loading TLS certificate from `%s'", item->value);
	
	/* Update the global TLS context object */
	TLS_CTX = ctx;

	log_debug("OpenSSL initialized (%d)", 0);

#endif

	/* SA-NOTE: destroy(SSL_CTX) is never called. */
}


