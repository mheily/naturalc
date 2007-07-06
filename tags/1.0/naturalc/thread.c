/*		$Id: thread.nc 68 2007-04-20 02:13:10Z mark $		*/

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
 * Abstractions for threads or processes.
 *
 * @todo make threads and processes separate classes and not interchangeable.
 * 
*/

#include "config.h"

#include "nc_exception.h"
//#include "options.h"
#include "nc_log.h"
#include "nc_thread.h"

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>


/** Default thread attributes */

pthread_attr_t DEFAULT_THREAD_ATTRIB;
pthread_attr_t DETACHED_THREAD_ATTRIB;


/**
 * Create a new thread.
 *
 * The thread will start excution at @a function with @a data as a parameter.
 * @bug data may be null, but auto-assertion will fail
*/
int
thread_create(thread_t *dest, callback_t func, void *data)
{
	int 		i;

	i = pthread_create(dest, &DEFAULT_THREAD_ATTRIB, (void*(*)(void*)) func, data);
	if (i != 0) 
		throw_errno("pthread_create(3)");

	
	log_debug("created thread %lu", (unsigned long) dest);
}


/**
 * Create a new detached thread.
 *
 * The thread will start excution at @a function with @a data as a parameter.
 * @bug data may be null, but auto-assertion will fail
*/
int
thread_create_detached(callback_t func, void *data)
{
	int 		i;
	pthread_t       thr;

	i = pthread_create(&thr, &DETACHED_THREAD_ATTRIB, (void*(*)(void*)) func, data);
	if (i != 0) 
		throw_errno("pthread_create(3)");

	
	log_debug("created detached thread %lu", (unsigned long) thr);
}

#if FIXME
	// port to apr and move these into thread_pool.nc

/**
 * Set the callback function for the thread pool.
 *
 * @param pool a thread pool object
 * @param callback a callback of type GFunc
 * @bug user_data parameter is not supported by this wrapper
 */
int
thread_pool_set_callback(thread_pool_t *pool, GFunc callback)
{
	if (pool->tpool)
		throw("callback is already set and cannot be changed");

	pool->tpool = g_thread_pool_new(callback, pool, -1, FALSE, NULL);
}
 
 
/**
 * Create a new thread pool.
 *
 * @param p reference to new thread pool
 *
 */
int
thread_pool_new(thread_pool_t **pool)
{

	mem_calloc(*pool);
}


/**
 * Shutdown and destroy a thread pool after all queued tasks are completed.
 *
 * @param pool thread pool object
 */
int
thread_pool_destroy(thread_pool_t **pool)
{
 
	(void) g_thread_pool_free((*pool)->tpool, FALSE, TRUE);
	free(*pool);
	*pool = NULL;
}


/**
 * Add a unit of work to the thread pool's work queue.
 *
 * @param pool thread pool object
 * @param work pointer to an opaque work object
 */
int
thread_pool_add_work(thread_pool_t *pool, void *work)
{

	(void) g_thread_pool_push(pool->tpool, work, NULL);
}

#endif


/**
 * Initialize the threading library.
 *
*/
int
thread_library_init()
{
        pthread_t tid;

	/* Initialize attribute structures */
	pthread_attr_init(&DEFAULT_THREAD_ATTRIB); 
	pthread_attr_init(&DETACHED_THREAD_ATTRIB); 

	/* Create an attribute structure for detached threads */
	if (pthread_attr_setdetachstate(&DETACHED_THREAD_ATTRIB, PTHREAD_CREATE_DETACHED) != 0)
		throw_errno("pthread_attr_setdetachstate(3)");

	/* Set the stack size of newly created threads */
	if (pthread_attr_setstacksize(&DEFAULT_THREAD_ATTRIB, 1024 * 64) != 0)
		throw_errno("pthread_attr_setstacksize(3)");
	if (pthread_attr_setstacksize(&DETACHED_THREAD_ATTRIB, 1024 * 64) != 0)
		throw_errno("pthread_attr_setstacksize(3)");

	/* This is needed under GNU/Linux to prime the linker prior to chroot() */
	/*@-type@*/
	(void) pthread_create(&tid, &DETACHED_THREAD_ATTRIB, (void*(*)(void*)) pthread_exit, NULL);
	/*@=type@*/

	atexit(thread_library_atexit);
}


void
thread_library_atexit(void)
  {
	(void) pthread_attr_destroy(&DEFAULT_THREAD_ATTRIB); 
	(void) pthread_attr_destroy(&DETACHED_THREAD_ATTRIB); 
  }
