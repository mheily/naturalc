/*		$Id: dns.h 11 2007-02-14 02:59:42Z mark $		*/

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

#ifndef _NC_DNS_H
#define _NC_DNS_H 

#include "nc_socket.h"

/** This regular expression should match all syntactically valid domain names */
#define DOMAIN_REGEX "[a-zA-Z0-9.-]+"
#define dns_validate_hostname(result,query)  str_match_regex(result, DOMAIN_REGEX, query)

int dns_library_init(void);

int dns_get_txt_record(list_t *dest, const string_t *host);
int dns_get_mx_list(list_t *dest, const string_t *domain);
int dns_get_nameservers(list_t *dest);
int dns_get_inet_by_name(list_t *dest, const string_t *host);

int dns_log_error(const char *domain, char_t *message);

int dns_get_service_by_name(int *dest, char_t *name, char_t *proto);

/* DNSBL functions */

int dns_query_blocklist(bool *result, list_t *dnsbl, const socket_t *sock);
int dns_ping_blocklist(list_t *dnsbl);

#endif
