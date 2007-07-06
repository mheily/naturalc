/*		$Id: list.h 56 2007-04-16 00:54:41Z mark $		*/

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

#ifndef _NC_LIST_H
#define _NC_LIST_H

#include "nc_string.h"
#include "nc_memory.h"

extern const int LIST_ZEROCOPY;

extern const int SORT_DEFAULT,
       SORT_ASCENDING,
       SORT_DESCENDING,
       SORT_LEXICOGRAPHIC,
       SORT_NUMERIC;

/** An element in a doubly linked list. */
typedef struct list_entry_t {
	/** Pointer to the item's value */
	string_t *value;			

	/** List navigation pointers */
	struct list_entry_t 
		 *next, *prev;
} list_entry_t;


/** A doubly linked list. */
typedef struct list {

	/** The total number of entries in the list */
	size_t	count;		

	/** Pointer to the first item on the list */
	list_entry_t  *head; 

	/** Pointer to the last item on the list */
	list_entry_t  *tail; 

} list_t;

int list_new(list_t **dest);
int list_destroy(list_t **dest);

int list_truncate(list_t *list);

/* Adding and removing elements */

int list_push(list_t *list, const string_t *item);
int list_pop(string_t *dest, list_t *src);
int list_shift(string_t *dest, list_t *list);
int list_unshift(list_t *dest, const string_t *value);
int list_get(string_t *dest, list_t *src, size_t offset);
int list_delete(list_t *dest, size_t offset);
int list_cat(list_t *dest, char_t *value);
int list_putc(list_t *dest, int value);
int list_from_char(list_t *list, ...) SENTINEL;

/* Searching */

int list_find(bool *result, list_t *list, char_t *string);
int list_find_substr(list_t *list, char_t *substring);
int list_entry_get(list_entry_t **dest, list_t *src, size_t offset);

int list_print(list_t *list);
int str_join(string_t *dest, list_t *src, int delimiter);
int list_reverse(list_t *list);

/* Multi-list functions */

int list_move(list_t *dest, list_t *src);
int list_merge(list_t *dest, list_t *src);
int list_copy(list_t *dest, list_t *src);
int list_compare(list_t *list, ...) SENTINEL;

int list_parse(list_t **dest, char *src, char *delimiter);
int list_sort(list_t *list, int flags);
int list_write(int fd, list_t *list, char_t *eol);

/* Serialization */

int list_serialize(string_t *dest, list_t *src);
int list_deserialize(list_t *dest, const string_t *src);

/* Direct access to list entries */

int list_entry_new(list_entry_t **dest, const string_t *value);
int list_entry_append(list_t *dest, list_entry_t *n);
int list_entry_delete(list_t *list, list_entry_t *ent);
int list_entry_destroy(list_entry_t *ent);

/* FIXME - this creates a circular dependency btw string.h and list.h */
int str_split(list_t *list, const string_t *src, int delimiter);

/* ---------------------------- INLINE FUNCTIONS ------------------------------ */


/**
 * Convenience function for manually iterating over a list 
 */
static inline int UNUSED
list_next(string_t **cursor, list_t *list, list_entry_t **cur)
{
	if (!cursor || !list || !cur)
		return -1;

	*cur = (*cur)->next;
	if (!*cur)
		return -1;

	*cursor = (*cur)->value;
	return 0;
}

#endif
