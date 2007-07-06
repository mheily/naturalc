/*		$Id: memory.nc 44 2007-04-08 21:28:49Z mark $		*/

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
 *
 * Memory management.
 *
*/
 
#include "config.h"

#include "nc_exception.h"
#include "nc_log.h"
#include "nc_memory.h"

int
mem_realloc(void **dest, size_t old_size, size_t new_size)
{
	char	*p = NULL;

	p = realloc(*dest, new_size);
	if (!p)
		throw("malloc error");
	/* Fill the newly allocated space with zeros */
	if (old_size > 0 && new_size > old_size) {
		memset( *(char **)dest + old_size, 0, new_size - old_size); 
	} else {
		/* If the buffer has shrunk, there is no need to zero
		 * any memory.
		 */
		;
	}
	
	*dest = p;
}
