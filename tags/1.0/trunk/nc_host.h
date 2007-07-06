/*		$Id: host.h 11 2007-02-14 02:59:42Z mark $		*/

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

#ifndef _NC_HOST_H
#define _NC_HOST_H

#include "nc.h"

#include <inttypes.h>

/** Workaround: OpenBSD 4.0 does not define this POSIX constant */
#ifndef HOST_NAME_MAX
# define HOST_NAME_MAX 256
#endif

typedef uint8_t mac_addr_t[6];

const char *hostname();

int host_get_name(string_t *dest);
//TODO: int host_get_mac_addr(mac_addr_t *result);
int host_get_ifaddrs(list_t *dest, int family);

#endif
