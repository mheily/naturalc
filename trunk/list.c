/*		$Id: list.nc 81 2007-04-28 23:45:32Z mark $		*/

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

/** @file 
    An implementation of doubly-linked lists.
*/

#include "config.h"

#include "nc_exception.h"
#include "nc_hash.h"
#include "nc_log.h"
#include "nc_memory.h"
#include "nc_string.h"

#include "nc_list.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* -------------------------- FORWARD DECLARATIONS -------------------------- */


/* ----------------------------- GLOBAL CONSTANTS -------------------------- */

/* Sorting flags */

const int SORT_ASCENDING  	= 0x0000;
const int SORT_DESCENDING 	= 0x0001;
const int SORT_LEXICOGRAPHIC 	= 0x0000;
const int SORT_NUMERIC		= 0x0010;
const int SORT_DEFAULT		= 0x0000;

int
list_new(list_t **dest)
{
	list_t *l = NULL;

	/* Allocate memory */
	mem_calloc(*dest);
	l = *dest;

finally:
	if (*dest == NULL) 
		destroy(list, &l);
}


/**
 * Release the memory allocated by a list entry.
 *
 * This is an internal list management function not for public use.
 *
 * @see list_entry_delete()
 */ 
inline int
list_entry_destroy(list_entry_t *ent)
{

	if ((str_destroy)(&ent->value) == 0) {
		free(ent);
	} else {
		throw_fatal("str_destroy() failed");
	}
}


/**
 * Append an entry to the end of a list.
 *
 */
inline int
list_entry_append(list_t *dest, list_entry_t *n)
{
	
	/* Place the node at the end of the list */
	n->next = NULL;
	n->prev = dest->tail;
	if (dest->tail) 
		dest->tail->next = n;
	if (!dest->head)
		dest->head = n;
	dest->tail = n;
	dest->count++;
}


int
list_destroy(list_t **dest)
{
	list_t *l = NULL;

	/* Ignore NULL lists */
	if (*dest == NULL)
		return 0;

	l = *dest;

	/* Truncate and then destroy the list */
	list_truncate(l);
	free(*dest);

	*dest = NULL;
}


/**
 * Remove all elements from a list.
 *
 * @param list list to be truncated
*/
int
list_truncate(list_t *list)
{
	list_entry_t *x = NULL, *y = NULL;

	/* SA-NOTE: list_entry_new() was called somewhere else */

	log_debug2("deleting %zu list entries", list->count);

	/* Delete each item, starting at the head */
	x = list->head;
	while (x) {
		y = x->next;
		list_entry_destroy(x);
		x = y;
	}

	list->head = NULL;
	list->tail = NULL;
	list->count = 0;
}


/**
 * Create a new list entry and assign it a value.
 *
 * @param dest list to be modified
 * @param value value of new element
*/
int
list_entry_new(list_entry_t **dest, const string_t *value)
{
	list_entry_t *n = NULL;

	mem_calloc(*dest);
	n = *dest;

	str_new(&n->value);
	str_copy(n->value, value);

	/* Update the entry */
	n->next = NULL;
	n->prev = NULL;

catch:
	(void) list_entry_destroy(n);
}



/**
 * Insert an element at the beginning of a list.
 *
 * @param dest list to be modified
 * @param value value to be prepended
*/
int
list_unshift(list_t *dest, const string_t *value)
{
	list_entry_t *n = NULL;

	/* Create a new entry */
	/* SA-NOTE: list_entry_destroy(&n) called by list destructor */
	list_entry_new(&n, value);

	if (dest->count) {
		n->next = dest->head;
	} else {
		dest->tail = n;
	}
	dest->head = n;
	dest->count++;
}
   

/**
 * Add an element to the end of a list.
 * 
 * @param list list to be modified
 * @param item pointer to the new element
*/
int
list_push(list_t *list, const string_t *item)
{
	list_entry_t *n = NULL;

	/* Create a new entry */
	list_entry_new(&n, item);

	list_entry_append(list, n);
}


/**
 * Remove an item from the end of a list.
 *
 * @param dest string to store the value of the last element
 * @param list list to be modified
*/
int
list_pop(string_t *dest, list_t *list)
{

	/* If the list is empty, return an error */
	if (list->tail == NULL)
		throw("list is empty");

	/* Move the string from the list to the caller */
	str_move(dest, list->tail->value);

	/* Delete the last element from the list */
	list_entry_delete(list, list->tail);
}


/**
 * Append a NUL terminated character string to a list.
 * 
 * @param dest list to be modified
 * @param value NUL terminated string to be appended
*/
int
list_cat(list_t *dest, char_t *value)
{
	string_t *str;

	str_cpy(str, value);
	list_push(dest, str);
}


/**
 * Append a single character to a list.
 * 
 * @param dest list to be modified
 * @param value character value to be appended
*/
int
list_putc(list_t *dest, int value)
{
	string_t *str;

	str_putc(str, value);
	list_push(dest, str);
}


/**
 * Get a specific element in a list.
 *
 * @param dest pointer to the destination pointer
 * @param src list to be accessed
 * @param offset offset of the element within the list, starting at zero
*/ 
int
list_get(string_t *dest, list_t *src, size_t offset)
{
	list_entry_t *ent = NULL;

	list_entry_get(&ent, src, offset);

	/// @todo optimize: use str_clone() instead of making a copy
	str_copy(dest, ent->value);
}


/**
 * Search a list for any occurances of a substring.
 *
 * @param list list to be searched
 * @param substring the substring to search for
 * @returns 0 if a match is found, or -1 if there are no matches.
 *
*/
int
list_find_substr(list_t *list, char_t *substring)
{
	list_entry_t *cur = NULL; 
	bool       found = false;

	/* Examine each list item until a match is found */
	for (cur = list->head; cur; cur = cur->next) {
		str_contains(&found, cur->value, substring);
		if (found)
			break;
	}

	if (!found)
		throw_silent();
}


/**
 * Search a list for any occurances of a string.
 * 
 * @param result stores the result; true if string was found, false if not found.
 * @param list list to be searched
 * @param string the exact string to match
 * @return 0 if a match is found, or -1 if there are no matches.
 *
*/
int
list_find(bool *result, list_t *list, char_t *string)
{
	list_entry_t *cur = NULL; 

	/* Examine each list item until a match is found */
	*result = false;
	for (cur = list->head; cur; cur = cur->next) {
		if (str_cmp(cur->value, string) == 0) {
			*result = true;
			break;
		}
	}
}

///@todo list_find_regex, or just plain list_find() with a third argument

/**
 * Print the contents of a list to the standard output. 
 *
 * @param list list to be printed
*/
int
list_print(list_t *list)
{
	string_t *str = NULL;
	list_entry_t *ent = NULL;

	(void) printf("dumping list of %zu entries..\n", list->count);

	for (ent = list->head; ent != NULL; ent = ent->next) {
		str = ent->value;
		(void) printf("  `%s'\n", str->value);
	}

	(void) printf("done.");
}


/**
 * Get a list entry.
 *
 * @param dest pointer to store the result
 * @param list list
 * @param offset position within the list
 */
int
list_entry_get(list_entry_t **dest, list_t *src, size_t offset)
{
	list_entry_t	*cur = NULL;
	uint32_t	count;

	/* Fail if the requested item is greater than the size of the list */
	if (offset >= src->count) {
		throwf("range error: requested element %u, but list only has %u elements", offset + 1, src->count);
	}

	/* @todo provide a list_study() function to optimize this O(N) linear search */
	cur = src->head;
	count = 0;
	while (cur) {
		if (count++ == offset) {
			*dest = cur;
			goto finally;
		}
		cur = cur->next;
	}

	throw_fatal("entry not found; possible pointer corruption in list");
}


/**
 * Delete an element from a list.
 *
 * @param list list containing the element to be deleted
 * @param ent pointer to the element to be deleted
*/
int
list_entry_delete(list_t *list, list_entry_t *ent)
{

	/* Update the list navigation pointers */
	if (ent->next != NULL)
		ent->next->prev = ent->prev;
	if (ent->prev != NULL)
		ent->prev->next = ent->next;
	if (list->head == ent)
		list->head = ent->next;
	if (list->tail == ent)
		list->tail = ent->prev;
		
	/* Reduce the item count */
	list->count--;

	/* Free the memory associated with the entry */
	/* SA-NOTE: list_entry_new(&ent) was done somewhere else */
	list_entry_destroy(ent);
}



/**
 * Retrieve the first element of a list and then delete the element.
 *
 * @param dest pointer to a string that will be set to the element's value
 * @param list list to be shifted
*/
int
list_shift(string_t *dest, list_t *list)
{
	list_entry_t	*ent = NULL;
	
	/* Special case: empty list */
	if (list->count == 0) 
		throw_silent();

	/* Retrieve the entry */
	list_entry_get(&ent, list, 0);

	/* Move it to the caller */
	str_move(dest, ent->value);

	/* Delete the entry */
	list_entry_delete(list, ent);
}


/**
 * Sort a list.
 *
 * @param list list to be sorted
 * @param flags any combination of SORT_DESCENDING | SORT_ASCENDING, and/or SORT_NUMERIC | SORT_LEXICOGRAPHIC
 * @todo OPTIMIZE: Bubble sort is the least efficient algorithm.
 *
*/
int
list_sort(list_t *list, int flags)
{
	string_t *str1 = NULL,
		 *str2 = NULL,
		 *tmp  = NULL;
	size_t found = 1;
	list_entry_t *ent1 = NULL, *ent2 = NULL;
	string_t *buf;
	int rc;
	bool ascending = (flags & SORT_DESCENDING) ? false : true;
	bool lexicographical = (flags & SORT_NUMERIC) ? false : true;
	uint32_t i, j;

	/* Do not sort an empty list or a list with one item */
	if (list->count < 2) 
		return 0;

	/* Keep sorting until no unsorted elements are found */
	while (found) {
		found = 0;
		ent1 = list->head;
		ent2 = list->head->next;
		str1 = ent1->value;
		str2 = ent2->value;
		while (ent1 != NULL && ent2 != NULL) {
			if (lexicographical) {
				rc = strcmp(str1->value, str2->value);
			} else {
				str_to_uint32(&i, str1);
				str_to_uint32(&j, str2);
				if (i > j) {
					rc = 1;
				} else if (i < j) {
					rc = -1;
				} else {
					rc = 0;
				}	
			}
			if ((ascending && rc > 0) || (!ascending && rc < 0)) {
				/* Swap the elements */
				tmp = ent1->value;
				ent1->value = ent2->value;
				ent2->value = tmp;
				found = 1;
			}
			ent1 = ent1->next;
			ent2 = ent2->next;
		}
	}
}


/**
 * Compare the values of a list to values provided as parameters to this function.
 *
 * @param list list to compare against
 *
 * @bug returns -1 or 0 as boolean truth values 
 *
 * NOTE: The argument list must be NULL terminated
*/
int
list_compare(list_t *list, ...)
{
	size_t        count  = 0;
	va_list	ap;
	string_t      *s1;
	var_char_t    *s2 = NULL;
	
	va_start(ap, list);
	s2 = va_arg(ap, char *);

	/* Special case: Comparing an empty list */
	if (s2 == NULL && list->count != 0)
		return -1;

	// @todo limit this function to TYPE_STRING

	list_get(s1, list, count++);
	while (s2 != NULL) {

		//log_warning("s1=`%s' s2=`%s'", s1->value, s2);

		/* Compare the two */
		if (str_cmp(s1, s2) != 0) {
			log_warning("comparison failed: `%s' != `%s'", s1->value, s2);
			return -1;
		}

		/* Get the next items */
		s2 = va_arg(ap, char *);
		if (s2 == NULL)
			break;
		list_get(s1, list, count++);
	}

	/* Check that the list contains the same number of elements we were given */
	if (count != list->count) {
		log_warning("comparison failed: expecting %zu elements, but list has %zu elements", count, list->count);
		return -1;
	}

finally:
	va_end(ap);
}


/**
 * Join all elements of a list together into a delimited string.
 * Uses Perl semantics. 
 *
 * @param dest string that will store the result
 * @param src list to be joined
 * @param delimiter delimiter to use
*/
int
str_join(string_t *dest, list_t *src, int delimiter)
{
	list_entry_t *cur = NULL; 
	size_t i = 0;

	/* Delete any previous value in the destination */
	str_truncate(dest);

	/* Append the value of each element to the string */
	for (cur = src->head; cur; cur = cur->next) {
		str_append(dest, cur->value);

		/* The last element doesn't get a delimiter */
		if (++i < src->count)
			str_putc(dest, delimiter);
	}

	/* If the delimiter is a newline, add a trailing newline */
	if (delimiter == '\n')
		str_putc(dest, delimiter);
}


/**
 * Copy all elements from one list to another list.
 *
 * After the operation, the destination list will be an exact copy of the source list.
 *
 * @param dest the destination list
 * @param src the source list
*/
int
list_copy(list_t *dest, list_t *src)
{
	list_entry_t *cur = NULL; 

	list_truncate(dest);

	/* Copy each item */
	for (cur = src->head; cur; cur = cur->next) {
		list_push(dest, cur->value);
	}
}


/**
 * Move all elements from one list to the end of another list.
 *
 * After the operation, the destination list will contain all of it's original items
 * plus all items from the source list. The source list will become empty.
 *
 * @param dest the destination list
 * @param src the source list
*/
int
list_merge(list_t *dest, list_t *src)
{

	/* If the destination list is empty, move the whole list */
	if (dest->count == 0) {
		dest->head = src->head;
	} 

	/* Otherwise, connect the src->head to the dest->tail */
	else {
		dest->tail->next = src->head;
	}

	dest->tail = src->tail;
	dest->count += src->count;
	
	/* Make the source list into an empty list without calling free() */
	src->count = 0;
	src->head = NULL;
	src->tail = NULL;
}


/**
 * Move all elements from one list to another list.
 *
 * After the operation, the destination list will be an exact copy of the source,
 * and the source list will become empty.
 *
 * @param dest the destination list
 * @param src the source list
*/
int
list_move(list_t *dest, list_t *src)
{

	/* Remove any contents of the destination list */
	list_truncate(dest);

	/* Copy the control structure */
	dest->head = src->head;
	dest->tail = src->tail;
	dest->count = src->count;
	
	/* Set the old control structure to an empty list */
	src->head = NULL;
	src->tail = NULL;
	src->count = 0;
}


/**
 * Given a list of unterminated lines, write terminated lines to a file descriptor.
 *
 * @param fd file descriptor
 * @param list list of unterminated lines
 * @param eol NUL terminated string to use as the line terminator
 * @todo rename this function and move into file.c
*/
int
list_write(int fd, list_t *list, char_t *eol)
{
	list_entry_t *cur = NULL; 
	size_t    len;

	len = strlen(eol);

	for (cur = list->head; cur; cur = cur->next) {
		if (write(fd, cur->value->value, cur->value->len) < 0) 
			throw_errno("write(2)");
		if (write(fd, eol, len) < 0) 
			throw_errno("write(2)");
	}
}


/**
 * Create a serialized ASCII string from a list.
 *
 * @param dest string to use as a destination buffer
 * @param src list to be serialized
*/
int
list_serialize(string_t *dest, list_t *src)
{
	list_entry_t *cur = NULL; 
	string_t *buf;
	size_t    i = 0;

	str_truncate(dest);
	for (cur = src->head; cur; cur = cur->next) {
		str_escape(buf, cur->value);
		if (i++) {
			str_putc(dest, ' ');
		}
		str_append(dest, buf);
	}
}


/**
 * Create a list from a serialized ASCII string.
 *
 * @param src list to be created
 * @param dest string containing a serialized list
*/
int
list_deserialize(list_t *dest, const string_t *src)
{
	list_entry_t *cur = NULL; 
	list_t   *tok;
	string_t *buf;

	list_truncate(dest);
	str_split(tok, src, ' ');
	for (cur = tok->head; cur; cur = cur->next) {
		str_unescape(buf, cur->value);
		list_push(dest, buf);
	}
}


/**
 * Convert one or more char_t parameters into a list.
 *
 * @param list
*/
int 
list_from_char(list_t *list, ...)
{
	char    *cp     = NULL;
	va_list  ap;
	
	/* Remove any previous items from the list */
	list_truncate(list);

	/* Get the first argument */
	va_start(ap, list);
       	cp = va_arg(ap, char *);

	/* Add each argument to the list */
	while (cp != NULL) {
		list_cat(list, cp);
		cp = va_arg(ap, char *);
	}

finally:
	va_end(ap);
}


/**
 * Reverse the order of all elements in a list.
 *
 * For example, would convert [ 'A', 'B', 'C' ] into [ 'C', 'B', 'A' ]
 *
 * @param list list to be reversed
*/
int
list_reverse(list_t *list)
{
	list_entry_t *ptr = NULL, *tmp = NULL;

	/* Reverse the head and tail pointers */
	ptr = list->head;
	list->head = list->tail;
	list->tail = ptr;

	/* Swap the 'next' and 'prev' pointers for each element */
	for (ptr = list->head; ptr != NULL; ptr = ptr->next) {
		tmp = ptr->prev;
		ptr->prev = ptr->next;
		ptr->next = tmp;
	}
}


/**
 * Remove an item from the list at a given position.
 *
 * @param list list to be modified
 * @param offset position of the item to be deleted (0 is the first item)
 */
int
list_delete(list_t *list, size_t offset)
{
	list_entry_t *ent = NULL;
	size_t        n = 0;

	/* Ensure that the element exists */
	if (offset > list->count)
		throw("invalid offset");

	/* Find the element */
	for (ent = list->head; ent != NULL; ent = ent->next) {
		if (n++ == offset) {
			list_entry_delete(list, ent);
			break;
		}
	}
}
