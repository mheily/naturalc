/*		$Id: test.h 20 2007-03-24 20:48:29Z mark $		*/

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

#ifndef _NC_TEST_H
#define _NC_TEST_H

#include "nc_string.h"

/** Unit testing environment */
typedef struct test_env {
	string_t *tmpdir;       /**< Path to the temporary work directory */
	string_t *account;	/**< The email address of the test account */ 
	string_t *password;	/**< The password for the test account */ 
	struct session *session;	/**< A session object */
} test_env_t;

/* This annotation masks a bizarre Split error saying this function is redefined. */
/*@-redef@*/
int register_test(const char *name, int (*fp)(test_env_t *));
/*@=redef@*/

int write_tempfile(char *dest, char *buf);
int start_test(char *name);
void test_failed(void);
int test_retval(int rv, int wanted);
int test_strcmp(const char *s1, const char *s2);
int test_strncmp(const char *s1, const char *s2, size_t len);
int test_strstr(char *big, char *little);
int start_testing(void);
int run_unit_tests(int (*init_func)(test_env_t *), int (*cleanup_func)(test_env_t *));

#endif
