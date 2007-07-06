/* 		$Id: dns.nc 45 2007-04-09 04:05:23Z mark $		*/

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
 
    DNS stub resolver.

*/ 
#include "config.h"

#include <errno.h>
#include <netdb.h>

#include "nc_exception.h"
#include "nc_file.h"
#include "nc_log.h"
#include "nc_list.h"
#include "nc_hash.h"
#include "nc_host.h"
//#include "nc_options.h"
#include "nc_socket.h"
#include "nc_string.h"
#include "nc_thread.h"

#include "nc_dns.h"

/* Splint cannot parse these system header files */
#ifndef S_SPLINT_S
#include <arpa/nameser.h>		
#if HAVE_ARPA_NAMESER_COMPAT_H
#include <arpa/nameser_compat.h>		
#endif
#include <resolv.h>
#endif

/* GLOBAL PRIVATE VARIABLES */

/** Set to true after the library has been initialized */
static bool DNS_LIB_INIT = false;

/** Global DNS resolver mutex.
 *
 * This mutex should be acquired before any calls to non-reentrant
 * resolver routines, including getaddrinfo(3) and gethostbyname(3).
 */
mutex_t DNS_RESOLVER_MUTEX = MUTEX_INITIALIZER;

/**
 *
 * Initialize the DNS stub resolver library.
 *
 * Runs a gethostname(2) and getaddrinfo(3) query. 
 * This should ensure that the resolver libraries are loaded
 * into memory prior to a chroot(2) call.
 *
*/
int
dns_library_init(void)
{
	list_t *result;
	string_t *buf;

	/* Don't initialize the library twice */
	if (DNS_LIB_INIT)
		return 0;

	/* Ensure that the local hostname is preloaded */
	host_get_name(buf);

	/* Ensure that 'gethostbyname' is preloaded */
	(void) gethostbyname("localhost");

	/* Run a DNS lookup prior to chroot(2) but ignore the response */
	str_cpy(buf, "netsol.com");
	(void) dns_get_inet_by_name(result, buf);

	DNS_LIB_INIT = true;
}


/** @todo WORKAROUND: Splint doesn't like this function, so skip it.. */
#ifndef S_SPLINT_S
/**
 *
 * Get a list of the MX records for a domain.
 * 
 * Caveats:	the caller must invoke free_mx_list() when finished
 *
 * @param dest list that will store the result
 * @param domain domain name to look up the MX records for
 *
 * @see http://www.woodmann.com/fravia/DNS.htm
 * 
 */
int
dns_get_mx_list(list_t *dest, const string_t *domain)
{
	union {
		HEADER hdr;
		unsigned char buf[PACKETSZ];
	} response;
	HEADER	 	*hp;
	int	 	pkt_len, len;
	unsigned int	i, answer_count;
	char 		buf[MAXDNAME + 1];
	unsigned char	*cp, *end;
	string_t        str_p = EMPTY_STRING;
	//string_t       *item;
	unsigned short	rec_type, rec_len, rec_pref;

	require (str_len(domain) > 0);

	/* Initialize variables */
	memset(&buf, 0, sizeof(buf));

	/* Remove any previous values in the list */
	list_truncate(dest);

#if FIXME
	/** @todo move this to smtp/queue.c to not pollute this library module */

	/* Special case: if RELAYHOST is defined, use it */
	hash_key_exists(&match, options, "relayhost");
	if (options != NULL && match) {
		hash_get(item, options, "relayhost");
		list_push(dest, item);
		return 0;
	}
#endif

	/* Lookup the MX records for <domain> */
	log_debug("looking up MX record for `%s'", domain->value);
	pkt_len = res_query(domain->value, C_IN, T_MX, 
		(unsigned char *) &response, sizeof(response));
	if (pkt_len < 0)
		return dns_log_error(domain->value, "MX lookup failed");
	if ((unsigned long) pkt_len > sizeof(response))
		throw("DNS response too large");

	/* Move <cp> to the answer portion.. */
	
	/* Skip the header portion */
	hp = (HEADER *) &response;
	cp = (unsigned char *) &response + HFIXEDSZ;
	end = (unsigned char *) &response + pkt_len;

	/* Skip over each question */
	i = ntohs((unsigned short)hp->qdcount);
	while (i > 0) {
		if ((len = dn_skipname(cp, end)) < 0)
			throw("bad hostname in question portion of dns packet");

		cp += len + QFIXEDSZ;
		i--;
	}

	/* Process each answer */
	answer_count = ntohs((unsigned short)hp->ancount);
	for (i = 0; i < answer_count; i++) {
		len = -1;
            	len = dn_expand((unsigned char *) &response, end, cp, (char *) &buf, sizeof(buf) - 1);
		if (len < 0)
			throw("error expanding hostname in answer portion");
		if (strncmp(buf, domain->value, strlen(domain->value) + 1) != 0)
			throw("extraneous response");

		/* Jump to the record type */
		cp += len;
		
		/* Check the record type */
		GETSHORT(rec_type, cp);
		if (rec_type != T_MX)
			throw("bad response record: expecting type MX");

		/* Skip over the class and TTL entries */
		cp += INT16SZ + INT32SZ;

		/* Get the RR length and MX pref */
		GETSHORT(rec_len, cp);
		GETSHORT(rec_pref, cp);
		//log_debug("mx pref == %d", rec_pref);

		/* Decode the MX hostname */
		len = -1;
		len = dn_expand((unsigned char *) &response, end, cp,
				(char *) &buf, sizeof(buf) - 1);
		if (len < 0)
			throw("error decompressing RR");

		/* Add the MX record to the list */
		str_alias(&str_p, buf);
		list_push(dest, &str_p);

		/* Jump to the next record */
		cp += len;
	}

	/** @todo Sort the resulting MX list by priority */

}
#endif


/**
 *
 * Print an error message to the system log along with a DNS error message.
 *
 * @param domain the domain name that caused the lookup error
 * @param message additional information to be appended to the log message
 *
 */
int
dns_log_error(const char *domain, const char *message)
{
	size_t sz = PATH_MAX;
	char buf[PATH_MAX + 1];

	memset(&buf, 0, sizeof(buf));

	switch (h_errno) {
		case HOST_NOT_FOUND:
			(void) snprintf((char *) &buf, sz, "Host not found");
			break;
		case NO_DATA:
			(void) snprintf((char *) &buf, sz, "No records found");
			break;
		case TRY_AGAIN:
			(void) snprintf((char *) &buf, sz, "No response");
			break;
		default:
			(void) snprintf((char *) &buf, sz, "Unknown error");
		}

	/* Print an error message to the system log */
	log_error("%s: error: %s: %s\n", 
			message, (char *) &buf, domain);

	/* This function always returns -1 */
	throw("caught DNS error");
}

/**
 *
 * Get a list of the system's DNS name servers from /etc/resolv.conf
 *
 * @param dest list that will store the result of the lookup operation
 *
*/
int
dns_get_nameservers(list_t *dest)
{
	list_entry_t *cur = NULL;
	string_t *line  = NULL;
	string_t *buf, *path, *ns;
	list_t   *lines;
	bool      match;

	list_truncate(dest);
	
	str_cpy(path, "/etc/resolv.conf");
	file_read(buf, path);
	str_split(lines, buf, '\n');

	/* Examine each line */
	for (cur = lines->head; cur; cur = cur->next) {
		line = cur->value;

		/* Skip lines that don't start with 'nameserver' */
		str_match_regex(&match, line, "^nameserver");
		if (!match)
			continue;

		/* Parse the nameserver value and add it to the list */
		str_str_regex(line, "nameserver"
					"[[:space:]]+"
					"([A-Za-z0-9._-]+)"
					"[[:space:]]*$", ns, NULL);
		list_push(dest, ns);
	}
}
		

/**
 * Convert a protocol name into a numeric port number by consulting /etc/services.
 * @param dest pointer to an integer that will store the result
 * @param name official name of the service
 * @param proto protocol to use when contacting the service (tcp or udp)
 *
*/
int
dns_get_service_by_name(int *dest, char_t *name, char_t *proto)
{
	struct servent *ent;
	struct servent ent_copy;

	/* Query /etc/services in a threadsafe manner */
	mutex_lock(DNS_RESOLVER_MUTEX);
	ent = getservbyname(name, proto);
	if (ent != NULL)
		memcpy(&ent_copy, ent, sizeof(ent_copy));
	mutex_unlock(DNS_RESOLVER_MUTEX);

	/* Check for errors */
	if (ent == NULL)
		throw_errno("getservbyname(3)");

	*dest = ntohs(ent_copy.s_port);
}


/**
 *
 * Lookup the A records for a host and store the IP addresses in a list.
 *
 * @param dest list that will contain the resulting IP addresses
 * @param host fully qualified domain name to query
 *
*/
int
dns_get_inet_by_name(list_t *dest, const string_t *host)
{
	struct addrinfo    *ai, *ai_list;
	struct addrinfo     hint;
	struct sockaddr_in *sa;
	string_t           *buf;
	int	            gai_errno;

	/* Initialize variables */
	ai_list = NULL;
	memset(&hint,0,sizeof(struct addrinfo));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	list_truncate(dest);

	/* Get the address of the remote host */
	mutex_lock(DNS_RESOLVER_MUTEX);
	gai_errno = getaddrinfo(host->value, NULL, &hint, &ai_list);
	mutex_unlock(DNS_RESOLVER_MUTEX);
	if (gai_errno != 0) {
		log_error("while resolving `%s' ...", host->value);
		throwf("getaddrinfo(3): %s: %s", host, gai_strerror(gai_errno));
	}
			
	/* Build a list containing each possible IP address */
	for ( ai = ai_list; ai != NULL; ai = ai->ai_next) {
		sa = (struct sockaddr_in *) ai->ai_addr;
		str_from_inet(buf, sa->sin_addr);
		list_push(dest, buf);
	}

finally:
	if (ai_list != NULL)
		freeaddrinfo(ai_list);
}


/**
 * Ping one or more DNS blocklists to make sure they are alive.
 *
 * Blocklists are supposed to respond affirmatively to any queries
 * for 127.0.0.1.  Any blocklist that does not answer will be removed 
 * from the list.
 *
 * @param list list of DNSBL hostnames
 * @see dns_query_blocklist()
 */
int
dns_ping_blocklist(list_t *dnsbl)
{
	list_entry_t *cur = NULL;
	string_t   *suffix  = NULL;
	string_t   *query;
	list_t     *valid;
	struct hostent *answer  = NULL;
	int             rc;  

	/* For each DNSBL domain ... */
	for (cur = dnsbl->head; cur; cur = cur->next) {

		/* Generate an A record query */
		suffix = cur->value;
		str_sprintf(query, "2.0.0.127.%s", suffix->value);

		/* Call gethostbyname(3) to determine if the client is listed */
		mutex_lock(DNS_RESOLVER_MUTEX);
		answer = gethostbyname(query->value);
		rc = h_errno;
		mutex_unlock(DNS_RESOLVER_MUTEX);

		/* Check for authoritative "host not found" responses */
		if (answer == NULL && rc == HOST_NOT_FOUND) {
			log_warning("DNSBL `%s' appears to be down; it will not be used", suffix->value);
		} else {
			list_push(valid, suffix);
		}
	}

	/* Replace the input list with the list of valid DNSBL addresses */
	list_truncate(dnsbl);
	list_copy(dnsbl, valid);
}


/**
 * Query one or more DNS blocklists for the IP address of the client.
 *
 * @param result result of the lookup; true, if the client is listed
 * @param dnsbl list of DNSBL domain names to query (e.g. `bl.spamcop.net')
 * @param sock socket with an active client
 * @see dns_ping_blocklist()
 */
int
dns_query_blocklist(bool *result, list_t *dnsbl, const socket_t *sock)
{
	list_entry_t  *cur      = NULL;
	string_t      *domain   = NULL;
	struct hostent *answer  = NULL;
	string_t      *addr, *rev_addr, *query;
	list_t        *quad;
	int             rc;  

	*result = false;

	/* Generate the forward and reverse address strings */
	str_from_inet(addr, sock->remote.in.sin_addr);
	str_split(quad, addr, '.');
	list_reverse(quad);
	str_join(rev_addr, quad, '.');
	log_debug2("addr=`%s' rev_addr=`%s'", addr->value, rev_addr->value);

	/* For each DNSBL domain .. */
	for (cur = dnsbl->head; cur; cur = cur->next) {
		domain = cur->value;

		/* Combine the reverse address and the DNSBL domain */
		str_sprintf(query, "%s.%s", rev_addr->value, domain->value);
		log_debug2("query=`%s'", query->value);

		/* Call gethostbyname(3) to determine if the client is listed */
		mutex_lock(DNS_RESOLVER_MUTEX);
		answer = gethostbyname(query->value);
		rc = h_errno;
		mutex_unlock(DNS_RESOLVER_MUTEX);

		/* If there was an answer, stop looking. */
		if (answer != NULL) {
			*result = true;
			break;
		}
	}
}


/**
 * Get the TXT record(s) associated with a hostname.
 *
 * @param result buffer to store the result
 * @param name hostname to look up the TXT record for
 * @bug doesn't handle multiple responses
 */
int
dns_get_txt_record(list_t *dest, const string_t *host)
{
	unsigned char response[PACKETSZ];
	char buf[128];
	//HEADER 	*hp;
	unsigned char *cp, *end;
	int    pkt_len, exp_len;
	int    ans_type, ans_size, txt_size;
	int          ttl;
	string_t    *s;

	/* Initialize variables */
	memset(&response, 0, sizeof(response));
	list_truncate(dest);

	pkt_len = res_search(host->value, C_IN, T_TXT, 
			(unsigned char *) &response, sizeof(response));
	if (pkt_len < 0)
		return dns_log_error(host->value, "TXT lookup failed");

	/* Setup pointers to parts of the response */
	cp = ((unsigned char *) &response);
	end = (unsigned char *) &response + pkt_len;

	/* Skip over the header */
       	cp += sizeof(HEADER);

	/* Uncompress and skip the question portion */
	exp_len = dn_expand(response, response + pkt_len, cp, buf, sizeof(buf));
	if (exp_len < 0) 
		throw("dn_expand(3) failed");
	cp += exp_len;

#if TODO
	// This doesn't work because the *hp (HEADER struct) gives nonsense values
	
	/* Process each answer */
	log_warning("processing %u answers", (unsigned short) hp->ancount);
	for (i = 0; i < (unsigned short) hp->ancount; i++) {
#endif

		/* Make sure the answer is of type T_TXT */
		GETSHORT(ans_type, cp);
		if (ans_type != T_TXT)
			throwf("invalid answer type: %d", ans_type);

		/* Ignore the `class' field */
		cp += INT16SZ;

		/* Uncompress the answer portion */
		exp_len = dn_expand(response, response + pkt_len, cp, buf, sizeof(buf));
		if (exp_len < 0) 
			throw("dn_expand(3) failed");
		cp += exp_len;

		/* Make sure the answer is of type T_TXT */
		GETSHORT(ans_type, cp);
		if (ans_type != T_TXT)
			throwf("invalid answer type: %d", ans_type);

		/* Ignore the `class' field */
		cp += INT16SZ;

		/* Get the `TTL' and `size' fields */
		GETLONG(ttl, cp);
		GETSHORT(ans_size, cp);
		txt_size = (int) *cp++;
		if (txt_size >= ans_size || !txt_size)
			throw("invalid TXT response");

		/* Add the result to the list */
		str_ncpy(s, (char *) cp, (size_t) txt_size);
		list_push(dest, s);
		//log_warning("resolver: txt=`%s'", s->value);
#if TODO
	}
#endif

	/** @bug some hosts (e.g. AOL) have more than 1 txt record, not handled yet */
}

