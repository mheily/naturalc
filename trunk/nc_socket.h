/*		$Id: socket.h 71 2007-04-21 21:51:37Z mark $		*/

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

#ifndef _NC_SOCKET_H
#define _NC_SOCKET_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#if WITH_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#else
  #define BIO void
  #define SSL_CTX int
  #define SSL int
#endif

#include "nc_list.h"

/* The size (in bytes) of the socket receive buffer */
#define RECV_BUF_SIZE     16*1024

/** A socket address.
 *
 * May contain an IPv4, IPv6, or UNIX-domain socket address.
 * This avoids the need to cast from the generic sockaddr to the specific.
 */
typedef union {
	struct sockaddr a;
	struct sockaddr_in in;
	struct sockaddr_un un;
	struct sockaddr_in6 in6;
} socket_addr_t;

/** A socket. */
typedef struct socket {

	/** Socket address on the local host */
	socket_addr_t local;

	/** Socket address of the remote endpoint */
	socket_addr_t remote;

	/** Directionality: listen(2) or connect(2) */
	enum { 
		UNKNOWN = 0, 
		LISTEN = 1, 
		CONNECT = 2 
	     } direction;


	/** @todo this bitfield probably be converted into individual 'int' or 'bool' */
	struct {
		/** Connection state: 0=not connected, 1=connected */
		int     connected:1;		

		/** If TRUE, non-blocking I/O operations are enabled */
		int     non_blocking:1;

		/** If TRUE, the input buffer contains an incomplete line.
		 *  If FALSE, the input buffer contains at least one complete lines */
		int     fragmented:1;

		/** The results of the most recent select(2) or poll(2) call */
		int	read:1,
			write:1,
			exception:1,
			timeout:1;
	} status;
	
	/** File or socket descriptor */
	int 	fd;

	/** Address family: AF_INET | AF_INET6 | AF_LOCAL */
	int     family;			
 
	/** Inactivity timeout (in seconds) */
	//DEADWOOD: int     timeout;

#if WITH_OPENSSL
	/** TLS variables */
	BIO    *bio;
	SSL    *ssl;
	int	tls_enabled;
#endif
	
	/** AF_LOCAL domain sockets require UNIX-style file permissions */
	uid_t	uid;
	gid_t   gid;
	mode_t  mode;
	
	/** An input buffer used by socket_readline() to store line fragments */
	list_t *read_buf;

} socket_t;

/** Socket I/O event flags for use with socket_select() */
typedef enum {
	SOCK_READ      = 0x00000001,
	SOCK_WRITE     = 0x00000010,
	SOCK_EXCEPTION = 0x00000100
} SOCKET_EVENT_TYPES;

int socket_init_library(void);
int socket_new(socket_t **dest);
int socket_destroy(socket_t **s);
int socket_set_family(socket_t *sock, int family);
int socket_freopen(socket_t *s, FILE *in, FILE *out);
int socket_select(socket_t *s, int flags, int timeout);
int socket_accept(socket_t *dest, socket_t *src);
int socket_bind(socket_t *s, string_t *address, int port);
int socket_connect(socket_t *s, /*@unique@*/ string_t *host, uint16_t port);
int socket_shutdown(socket_t *s);
int socket_set_credentials(socket_t *sock, string_t *user, string_t *group, mode_t mode);
int socket_set_timeout(socket_t *sock, int read_sec, int write_sec);

/* Standard I/O wrapper functions for sockets */
int socket_get_credentials(uid_t *uid, gid_t *gid, socket_t *sock);

int socket_printf(socket_t *sock, const char *format, ...)
	__attribute__((format(printf, 2, 3)));

int socket_readline(string_t *dest, socket_t *sock);
int socket_read(string_t *dest, size_t size, socket_t *sock);
int socket_write(socket_t *sock, const char *src, size_t len);
int socket_close(socket_t *sock);
int socket_get_peer_addr(string_t *dest, socket_t *src);
int socket_get_peer_name(string_t *name, const socket_t *sock);

int socket_starttls(socket_t *s);

/**
 * Write a single character to the output buffer of a socket.
 * 
 * @param sock socket object
 * @param c character to be written
*/
static inline int
socket_putc(socket_t *sock, int c)
{
	return socket_write(sock, (const char *) &c, 1);
}


/**
 * Write a string to a socket.
 * 
 * @param sock socket object
 * @param cptr pointer to a NUL-terminated character array
*/
static inline int
socket_puts(socket_t *sock, const char *cptr)
{
	return socket_write(sock, cptr, strlen(cptr));
}


/**
 * Write a string to a socket.
 * 
 * @param sock socket object
 * @param str a string object
*/
static inline int
socket_put_str(socket_t *sock, string_t *src)
{
	return socket_write(sock, src->value, str_len(src));
}

#endif
