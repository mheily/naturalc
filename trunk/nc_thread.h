/*		$Id: thread.h 68 2007-04-20 02:13:10Z mark $		*/

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

#ifndef _NC_THREAD_H
#define _NC_THREAD_H

#include "nc_list.h"

#if _REENTRANT
#include <pthread.h>
#include <semaphore.h>
#endif

/** A generic function pointer suitable for use as a callback */
typedef void (*callback_t) (void *);

/* Mutexes */
/// @bug check the return value of mutex_lock()
#define mutex_t			pthread_mutex_t
#define MUTEX_INITIALIZER       PTHREAD_MUTEX_INITIALIZER
#define mutex_init(m)		((pthread_mutex_init(m, NULL) == 0) ? 0 : -1)
#define mutex_lock(a)           (void) pthread_mutex_lock(&a)
#define mutex_unlock(a)         (void) pthread_mutex_unlock(&a)

/* Condition variables */
/// @todo since these always require a mutex, make cond_t a struct containing a cond and mutex
#define cond_t                  pthread_cond_t
#define COND_INITIALIZER        PTHREAD_COND_INITIALIZER;
#define cond_init(c)		((pthread_cond_init(&c, NULL) == 0) ? 0 : -1)
#define cond_wait(c,m)		((pthread_cond_wait(&c, &m) == 0) ? 0 : -1)
#define cond_signal(c)		((pthread_cond_signal(&c) == 0) ? 0 : -1)


/** A thread. */
#define thread_t		pthread_t

/* Thread control operations */

#define thread_join(t,rc)	pthread_join(t, (void **) &rc)
#define thread_cancel(t)	pthread_cancel(t)
#define thread_exit(i)		pthread_exit(NULL)
#define thread_get_id()		pthread_self()

/* Reader/writer locks */

#define rwlock_t                pthread_rwlock_t
#define rwlock_init(a)		pthread_rwlock_init(&a, NULL)
#define rwlock_rdlock(a)	pthread_rwlock_rdlock(&a)
#define rwlock_wrlock(a)	pthread_rwlock_wrlock(&a)
#define rwlock_unlock(a)	pthread_rwlock_unlock(&a)
#define rwlock_destroy(a)	pthread_rwlock_destroy(&a)

/* thread_create() is a syscall in Darwin, so this shim is needed to avoid linker problems. */
#define thread_create(a,b,c)	nc_thread_create(a,b,c)

// FIXME: workaround for openbsd 4.0 wierdness
//int nc_thread_create(thread_t *dest, callback_t func, void *data);

int thread_create_detached(callback_t func, void *data);

int thread_library_init(void);
void thread_library_atexit(void);

#endif
