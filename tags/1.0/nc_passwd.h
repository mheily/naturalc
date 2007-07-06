/*		$Id: passwd.h 11 2007-02-14 02:59:42Z mark $		*/

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

#ifndef _NC_PASSWD_H
#define _NC_PASSWD_H

#include "nc_string.h"
#include "nc_hash.h"

#include <sys/types.h>
#include <unistd.h>

/* System /etc/passwd and /etc/group functions */

int passwd_get_symbolic_uid(string_t *dest, hash_t *map, uid_t uid);
int passwd_get_uid_map(hash_t *map);

int passwd_get_name_by_id(string_t *name, const uid_t uid);
int passwd_get_id_by_name(uid_t *uid, const string_t *name);
int passwd_exists(bool *result, const string_t *name);

int group_get_id_by_name(gid_t *gid, const string_t *name);
int group_get_name_by_id(string_t *name, const gid_t gid);
int group_exists(bool *result, const string_t *name);

#endif
