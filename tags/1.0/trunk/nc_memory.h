/*		$Id: memory.h 65 2007-04-18 03:53:44Z mark $		*/

/*
 * Copyright (c) 2006 Mark Heily <devel@heily.com>
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

#ifndef __NC_MEMORY_H
#define __NC_MEMORY_H

#include <stdlib.h>

#if USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

/** GCC 4.0 and higher support the non-standard 'sentinel' attribute to check 
 * the presence of a NULL pointer at the end of a variadic function call.
 *
 * Use the ATTRIB_SENTINEL macro to allow older compilers to build
 * the source without emitting warnings.
 */
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define SENTINEL __attribute__ ((sentinel))
#else
#define SENTINEL
#endif
 
/** GCC supports the 'unused' attribute which prevents compiler warnings
 * about unused functions.
 */
#if defined(__GNUC__)
#define UNUSED __attribute__ ((__unused__))
#else
#define UNUSED
#endif

#define mem_alloc(ptr)  ( ((ptr = malloc(sizeof(*ptr))) == NULL) ? -1; 0 )

#define mem_calloc_DEADWOOD(ptr) ( ((ptr = calloc(1, sizeof(*ptr))) == NULL) ? -1 : 0 )

/// @bug FIXME -XXX testing
#define mem_calloc(ptr) { \
		if (ptr != NULL) { \
			throw("double new() detected; pointers must be preset to NULL"); \
		} \
		ptr = calloc(1, sizeof(*ptr)); \
		if (ptr == NULL) \
		    throw("calloc(3) failed"); \
		}

/** When running under Valgrind, ensure that a pointer that is used as an lvalue
 *  does not point to an addressible area of memory. This will detect the
 *  "double new()" class of memory errors, which leads to memory leaks.
 *
 *  @bug not used yet
 */
#define mem_require_fresh(ptr) { \
		if (VALGRIND_CHECK_MEM_IS_ADDRESSABLE(ptr,sizeof(*ptr)) == 0)  \
			throw("double new() detected"); \
		} \

#define mem_malloc(ptr, size) ( ((ptr = malloc(size)) == NULL) ? -1 : 0 )

int mem_realloc(void **dest, size_t old_size, size_t new_size);

#endif

