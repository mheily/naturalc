/*		$Id: host.nc 34 2007-03-29 04:07:35Z mark $		*/

/*
 * Copyright (c) 2007 Mark Heily <devel@heily.com>
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
 *  Utility routines to determine information about the local host.
 *
*/

#include "config.h"

#include "nc_host.h"

#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <unistd.h>


char _HNAME[HOST_NAME_MAX + 1];
int  _HNAME_CACHED = 0;

/**
 * Compatibility shim for convenience..
 * @bug not threadsafe
 */
const char *
hostname(void) {
	  if (!_HNAME_CACHED) { 
		  if (gethostname(_HNAME, sizeof(_HNAME)) < 0) {
			  abort();
		  }
		_HNAME_CACHED = 1;
	  } return (const char *) _HNAME;
  }

/**
 *
 * Retrieve the current hostname.
 *
 * @deprecated access the global hostname instead
 * 
 * @param dest string to store the hostname in
 */
int
host_get_name(string_t *dest)
{
	char buf[HOST_NAME_MAX + 1];

	/* Get the short name of the host */
	if (gethostname(buf, sizeof(buf)) < 0) {
		throw_errno("gethostname(3)");
	}
	str_cpy(dest, buf);

#if DEADWOOD
	// Not sure why this is a problem 

	/* Make sure the hostname is fully qualified */
	if (str_contains(dest, ".") < 0)
		throwf("the hostname `%s' is not fully qualified", dest->value);
#endif
}


/**
 * Retrieve a list of addresses for the current host.
 *
 * @param dest list that will store the result
 * @param family socket family, AF_INET or AF_INET6
 * @todo figure out if getifaddrs(3) may or may not be threadsafe.
*/
int
host_get_ifaddrs(list_t *dest, int family)
{
	struct ifaddrs *head = NULL;
	struct ifaddrs *cur = NULL;
	struct sockaddr_in *sain = NULL;
	string_t *buf;

	if (getifaddrs(&head) != 0)
		throw_errno("getifaddrs(3)");

	for (cur = head; cur != NULL; cur = cur->ifa_next) {

		/* Skip interfaces that don't match the requested family */
		if (cur->ifa_addr == NULL || cur->ifa_addr->sa_family != family)
			continue;

		switch (family) {
			case AF_INET:
				sain = (struct sockaddr_in *) cur->ifa_addr;
				str_from_inet(buf, sain->sin_addr);
				list_push(dest, buf);
				break;
				
			default:
				throw("address family not supported");
		}

		//log_debug("%s", cur->ifa_name);
	}
	//list_print(dest); abort();

finally:
	if (head != NULL)
		freeifaddrs(head);
}
