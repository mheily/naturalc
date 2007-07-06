/*		$Id: hash.nc 65 2007-04-18 03:53:44Z mark $		*/

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
 * Hash tables.
 *
*/

#include "nc_exception.h"
#include "nc_hash.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_memory.h"
#include "nc_string.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* -------------------------------- FUNCTIONS -------------------------------- */

/** @todo Use a public domain hashing function instead of this crap */
static inline int
hash_function(uint8_t *result, char_t *key)
{
	size_t i, len;
	uint8_t x = 0;

	len = strlen(key);
	for (i = 0; i < len; i++) {
		 x = x ^ key[i];
	}
	//log_debug("hashed `%s' to %d", src->value, result);
	*result = x;
}

static inline int
hash_entry_search(list_t **dest, list_entry_t **ent, hash_t *hash, char_t *key)
{
	list_entry_t *cur = NULL; 
	uint8_t   slot;
	bool      found = false;

	/* Hash the key into a bucket number */
	hash_function(&slot, key);

	/* If the bucket doesn't exist, the search fails */
	if (hash->bucket[slot] == NULL) 
		throw_silent();

	/* Perform an O(N) search for the key */
	for (cur = hash->bucket[slot]->head; cur; cur = cur->next) {
		if (str_cmp(cur->value, key) == 0) {
			*dest = hash->bucket[slot];
			*ent = cur;
			found = true;
			break;
		}
	}

	/* Fail unless the key was found */
	if (!found) {
		throw_silent();
	}
}


/**
 * Delete an element from a hash table.
 *
 * @param hash hash table
 * @param key key to be deleted
*/
int
hash_delete(hash_t *hash, char_t *key)
{
	list_t   *bucket = NULL;
	list_entry_t *ent = NULL;

	/* Get a pointer to the bucket that has the element */
	if (hash_entry_search(&bucket, &ent, hash, key) == 0) {

		/* Delete the value */
		list_entry_delete(bucket, ent->next);

		/* Delete the key */
		list_entry_delete(bucket, ent);

		hash->count--;
	} else {
		throw_silent();
	}
}


/**
 * Retrieve an element from the hash. 
 *
 * @param dest buffer to store the result
 * @param hash hash table
 * @param key key to be retrieved
*/
int
hash_get(string_t *dest, hash_t *hash, char_t *key)
{
	list_t       *bucket = NULL;
	list_entry_t *ent = NULL;

	/* Get a pointer to the bucket that has the element */
	if (hash_entry_search(&bucket, &ent, hash, key) == 0) {

		/// @todo optimize: str_clone() instead of copy()
		str_copy(dest, ent->next->value);

	} else {
		throw_silent();
	}
}


/**
 * Set the value of an element in the hash table.
 *
 * @param hash hash table
 * @param key key to be set
 * @param value string value to be associated with the key
*/
int
hash_set(hash_t *hash, char_t *key, const string_t *value)
{
	list_entry_t *cur = NULL; 
	string_t    str = EMPTY_STRING;
	uint8_t     slot;

	/* Hash the key into a bucket number */
	hash_function(&slot, key);

	/* Create a new bucket, if needed */
	if (hash->bucket[slot] == NULL) {
		list_new(&hash->bucket[slot]);
		/* SA-NOTE: list_destroy() called by hash destructor */
	}

	/* Perform an O(N) search for the key */
	for (cur = hash->bucket[slot]->head; cur; cur = cur->next) {
		if (str_cmp(cur->value, key) == 0) {
			/* Update the value */
			str_copy(cur->next->value, value);
			goto finally;
		}
	}

	/* If a match isn't found, try to insert a new value */
	/* Add the new key+value pair to the list */
	str_alias(&str, key);
	list_push(hash->bucket[slot], &str);
	if (list_push(hash->bucket[slot], value) < 0) {

		/* To prevent corruption, remove the key from the list */
		list_entry_delete(hash->bucket[slot], hash->bucket[slot]->tail);
		throw("error inserting a new entry into the hash table");
	}
	hash->count++;
}


/**
 * Create a new hash table.
 *
 * @param dest indirect pointer to a hash table
*/
int
hash_new(hash_t **dest)
{

	/* Allocate memory */
	mem_calloc(*dest);
}


/**
 * Delete all elements from a hash table.
 *
 * @param hash hash table
*/
int
hash_truncate(hash_t *hash)
{
	int   i;

	/* Delete all in-memory keypairs */
	for (i = 0; i < 256; i++) {
		if (hash->bucket[i] != NULL)
			list_truncate(hash->bucket[i]);
	}

	hash->count = 0;
}


int
hash_destroy(hash_t **hash_ref)
{
	hash_t *hash = NULL;
	int  i;

	/* Don't destroy hashes twice */
	hash = *hash_ref;

	/* Delete all elements */
	for (i = 0; i < 256; i++) {
		if (hash->bucket[i] != NULL) {
			list_destroy(&hash->bucket[i]);
		}
	}

	/* Delete the hash */
	free(hash);
	*hash_ref = NULL;
}


/**
 * Copy all elements from one hash table into another hash table.
 * Any pre-existing elements in the destination will be removed.
 *
 * @param dest destination hash table
 * @param src source hash table
*/
int
hash_copy(hash_t *dest, hash_t *src)
{
	int i;

	/* Copy each bucket */
	memset (dest->bucket, 0, sizeof(dest->bucket));
	for (i = 0; i < 256; i++) {
		if (src->bucket[i] != NULL) {
			list_copy(dest->bucket[i], src->bucket[i]);
		}
	}
	dest->count = src->count;
}


/**
 * Get a list of all the keys in a hash table.
 *
 * @param dest list where the result will be stored
 * @param hash hash table
 */
int
hash_get_keys(list_t *dest, const hash_t *hash)
{
	list_entry_t *ent = NULL;
	size_t        i;

	/* Remove any previous members of the destination */
	list_truncate(dest);

	/* Examine each bucket .. */
	for (i = 0; i < 256; i++) {

		/* Skip empty buckets */
		if (!hash->bucket[i])
			continue;

		/* Add each value to the list */
		for (ent = hash->bucket[i]->head; ent; ent = ent->next->next) {
			list_push(dest, ent->value);
		}
	}
}


/**
 * Get a list of all the values in a hash table.
 *
 * @param dest list where the result will be stored
 * @param hash hash table
 */
int
hash_get_values(list_t *dest, const hash_t *hash) 
{
	list_entry_t *ent = NULL;
	size_t        i;

	/* Remove any previous members of the destination */
	list_truncate(dest);

	/* Examine each bucket .. */
	for (i = 0; i < 256; i++) {

		/* Skip empty buckets */
		if (!hash->bucket[i])
			continue;

		/* Add each value to the list */
		for (ent = hash->bucket[i]->head; ent; ent = ent->next->next) {
			list_push(dest, ent->next->value);
		}
	}
}


/**
 * Test if a key exists within a hash table.
 *
 * @param result store the result of the search; true if found
 * @param hash hash table to be searched
 * @param key key to look for
 */
int
hash_key_exists(bool *result, const hash_t *hash, char_t *key)
{
	string_t *dest;

	/** @todo fix hash_get()s side effects for tied hashes so this cast is not needed */
        if (hash_get(dest, (hash_t *) hash, key) < 0) {
		*result = false;
	} else {
		*result = true;
	}
}


/**
 * Test if a value exists in a hash table.
 * @todo Optimize; this can be a slow O(N) search.
 *
 * @param result store the result of the search; true if found
 * @param hash hash table to be searched
 * @param value value to look for
*/
int
hash_value_exists(bool *result, const hash_t *hash, char_t *value)
{
	list_t  *values;

	hash_get_values(values, hash);
	list_find(result, values, value);
}


/**
 * Print a dump of a hash table to the standard output.
 *
 * @param hash hash table
*/
int
hash_print(hash_t *hash)
{
	list_t *keys;
	string_t *key, *item;
	size_t i;

	log_warning("dumping hash at %p..", hash);

	hash_get_keys(keys, hash);

	for (i = 0; i < keys->count; i++) {
		list_get(key, keys, i);
		hash_get(item, hash, key->value);
		log_warning("   [%s] -> `%s'", key->value, item->value);
	}
	log_warning ("done; hash contains %zu elements", keys->count);
}


/**
 * Serialize an in-memory hash table into a linear byte stream.
 *
 * @param str destination buffer to hold the serialized hash table
 * @param hash hash table to be serialized
*/
int
hash_serialize(string_t *str, const hash_t *hash)
{
	string_t     *buf, *val;
	list_entry_t *ent = NULL;
	string_t     *ptr;
	size_t        i;

	for (i = 0; i < 256; i++) {
		if (hash->bucket[i] != NULL) {

			ent = hash->bucket[i]->head;
			while (ent) {
				ptr = ent->value;
				/* Add the key */
				str_sprintf(buf, "%s: ", ptr->value);

				ent = ent->next;
				ptr = ent->value;

				/* Special case: multiline data is backslash terminated */
				if (strchr(ptr->value, '\n') != NULL) {
					str_copy(val, ptr);
					str_subst_regex(val, "\n", "\\\n");
					str_append(buf, val);
				} 
				/* Normal case: no padding is necessary */
				else {
					str_append(buf, ptr);
				}
				str_putc(buf, '\n');
				str_append(str, buf);
				ent = ent->next;
			}
		}
	}

	//hash_print(hash);
	//log_warning("s=`%s'", str->value);
	//abort();
}


/**
 * Deserialize an in-memory hash table from a linear byte stream.
 *
 * @param hash hash table to store the result
 * @param str destination buffer that holds a serialized hash table
*/
int
hash_deserialize(hash_t *hash, const string_t *str)
{
	list_entry_t *cur = NULL; 
	char_t   *eol   = NULL;
	string_t *key, *val;
	string_t *line = NULL;
	list_t   *lines;
	bool      multiline = false;

	/* Split the buffer into a list of lines */
	str_split(lines, str, '\n');

	/* Examine each line */
	for (cur = lines->head; cur; cur = cur->next) {
		line = cur->value;
		log_debug("line=`%s'", line->value);

		/* Ignore blank lines */
		if (str_len(line) == 0)
			continue;

		/* If there is no key, look for one */
		log_warning("line=`%s' key=`%s' val=`%s'", 
				line->value, key->value, val->value);
		if (str_len(key) == 0) {
			str_str_regex(line, "([A-Za-z0-9._-]+): (.*)", key, val);
		} 

		/* Otherwise, append the current line to the value */
		else {
			str_append(val, line);
		}

		/* If the line has a '\' terminator, continue reading lines */
		log_warning("val=`%s'", val->value);
		str_get_terminator(&eol, val);
		if (*eol == '\\') {
			multiline = true;
			/* Convert the '\\' to a '\n' */
			str_set_terminator(val, '\n');
		} 

		/* Otherwise, add the key/value pair and clear the variables */
		else {
			/* Multiline values should be newline-terminated */
			if (multiline) {
				str_putc(val, '\n');
				multiline = false;
			}
			hash_set(hash, key->value, val);
			str_truncate(key);
			str_truncate(val);
		}

	}

	/* Check for an unterminated value */
	if (str_len(key) > 0)
		throw("parse error");
}


/**
 * Convert one or more char_t parameters into a hash.
 *
 * Note that hash values cannot be NULL.
 *
 * @param list
*/
int 
hash_from_char(hash_t *hash, ...)
{
	char    *key   = NULL,
		*val   = NULL;
	string_t str = EMPTY_STRING;
	va_list  ap;
	
	/* Remove any previous items from the hash table */
	hash_truncate(hash);

	/* Get the first key/value pair */
	va_start(ap, hash);
       	key = va_arg(ap, char *);
	if (key == NULL)
		throw("insufficient arguments");
       	val = va_arg(ap, char *);
	if (val == NULL)
		throw("insufficient arguments");

	/* Add each argument to the list */
	while (key != NULL) {
		val = va_arg(ap, char *);
		if (val == NULL)
			throw("insufficient arguments");
		str_alias(&str, val);
		hash_set(hash, key, &str);
		key = va_arg(ap, char *);
	}

finally:
	va_end(ap);
}


/**
 * Coerce a list of values into a hash table.
 *
 * @param hash hash to store the result
 * @param list list := (key, value)*
 */
int
hash_from_list(hash_t *hash, list_t *list)
{
	list_entry_t *cur = NULL;
	string_t *key, *val = NULL;

	/* Ensure that there is an even number of items in the list */
	if (list->count % 2 != 0)  
		throw("odd number of elements in list; cannot create a hash"); 
	
	/* Examine the list */
	for (cur = list->head; cur != NULL; cur = cur->next) {

		/* Get the key */
		key = cur->value;
		cur = cur->next;
		if (key == NULL || cur == NULL)
			throw("list error");

		/* Get the value */
		val = cur->value;
		if (val == NULL)
			throw("list error");

		/* Create a new keypair */
		hash_set(hash, key->value, val);
	}
}
