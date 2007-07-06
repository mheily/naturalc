/*		$Id: hash.h 44 2007-04-08 21:28:49Z mark $		*/

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

#ifndef _NC_HASH_H
#define _NC_HASH_H

#include "nc_list.h"

/** Hash table */
typedef struct hash {

	/** An array of 'buckets' large enough to support an 8-bit hashing function */
	list_t *bucket[256]; 

	/** The total number of key/value pairs in the hash */
	size_t  count;

} hash_t;


int hash_new(hash_t **dest);
int hash_destroy(hash_t **hash_ref);
int hash_copy(hash_t *dest, hash_t *src);

int hash_get(string_t *dest, hash_t *hash, char_t *key);
int hash_set(hash_t *hash, char_t *key, const string_t *value);
int hash_delete(hash_t *dest, char_t *key);

int hash_truncate(hash_t *dest);
int hash_key_exists(bool *result, const hash_t *hash, char_t *value);
int hash_value_exists(bool *result, const hash_t *hash, char_t *value);
int hash_get_keys(list_t *dest, const hash_t *hash);
int hash_get_values(list_t *dest, const hash_t *hash);
int hash_print(hash_t *hash);

int hash_serialize(string_t *str, const hash_t *hash);
int hash_deserialize(hash_t *hash, const string_t *str);

int hash_from_char(hash_t *hash, ...);
int hash_from_list(hash_t *hash, list_t *list);

#endif
