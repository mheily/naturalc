/*		$Id: test.nc 22 2007-03-24 21:16:33Z mark $		*/

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
 * Unit testing framework.
 *
*/

#include "config.h"

#include "nc_exception.h"
#include "nc_file.h"
#include "nc_log.h"
//#include "options.h"
#include "nc_string.h"

#include "nc_test.h"

#include <errno.h>

/** mkdtemp(3) is an OpenBSD extension */
extern char      *mkdtemp(char *);

/* GLOBAL VARIABLES */

#define TESTS_MAX	25
#define TEST_NAME_MAX	15

int	num_tests = 0;
int	passed = 0;
int	failed = 0;

/* Test name -> function pointer */
int	   test_count = 0;
string_t  *test_name[TESTS_MAX + 1];
int	 (*test_func[TESTS_MAX + 1])(test_env_t *);

int
register_test(const char *name, int (*fp)(test_env_t *))
{

	str_new(&test_name[test_count]);
	//@todo where to str_destroy(test_name)?

	str_cat(test_name[test_count], name);
	test_func[test_count] = fp;
	test_count++;
}

int
write_tempfile(char *dest, char *buf)
{
	FILE *f = NULL;

	if ((f = fopen("tmp.file", "w")) == NULL)
		return -1;

	if (fputs(buf, f) < 0)
		return -1;
	if (fclose(f) < 0)
		return -1;
	strncpy(dest, "tmp.file", 15);
}

int
start_test(char *name)
{

	(void)printf("%50s ... \n", name);
	num_tests++;
}

void
test_failed()
  {

	(void)printf("FAILED\n\n*** Aborting due to a test failure\n"); 
	abort();
  }

int
start_testing()
{
	(void)printf("Starting test suite...\n");
}


int
run_unit_tests(int (*init_func)(test_env_t *), int (*cleanup_func)(test_env_t *))
{
	char    *module;
	char    template[80];
	char	*tmpdir = NULL;
	int	use_tmpdir = 1;
	int     i, rc, run_all;
	test_env_t env;

	/* If the TESTDIR variable is provided, use it */
	if (getenv("TESTDIR")) {
		use_tmpdir = 0;
		tmpdir = strdup(getenv("TESTDIR"));
	}

	/* Create a temporary work directory */
	if (use_tmpdir) {
		strncpy(template, "/tmp/openmta.XXXXXXXXXX", sizeof(template));
		tmpdir = mkdtemp(template);
		if (!tmpdir) {
			(void)printf("unable to create the temporary directory\n");
			exit(EXIT_FAILURE);
		}
		(void)printf("Created a temporary directory at %s..\n", tmpdir);
	}
	str_new(&env.tmpdir);
	str_cpy(env.tmpdir, tmpdir);

	/* Initialize variables */
	module = getenv("TEST");
	run_all = (strcmp(module, "all") == 0);

	/* Open the system log and display messages on the standard error */
#if FIXME
	// sys doesnt exist
        closelog();
        openlog("maild-unit-tests", sys.logopt | LOG_PERROR, sys.log_facility);
        set_log_level(sys.log_level);
#endif
	
	/* Initialize Recvmail test environment */
	if (strcmp(module, "all") == 0) {
		if (init_func(&env) < 0)
			throw("failed to initialize test environment");
	}

	(void) start_testing();

	/* Check each test in the array */
	for (i = 0; i < test_count; i++) {
		if (run_all || (strcmp(module, test_name[i]->value) == 0)) {

			/* Print the test name */
			if (i > 0)
				(void)printf("\n");
			(void)printf("%s:\n", test_name[i]->value);

			/* Run the test function */
			rc = (*test_func[i])(&env);
			if (rc < 0) {
				(void)printf("TEST FAILED (some tests may have been skipped..)\n\n* Aborting *\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	/* Cleanup the test environment */
	if (strcmp(module, "all") == 0) {
		if (cleanup_func(&env) < 0)
			throw("failed to cleanup the test environment");
	}

	/* Remove the temporary directory */
	if (use_tmpdir) {
		if ((file_rmdir)(env.tmpdir) < 0) {
			(void)printf("ERROR: Unable to remove %s: %s\n", tmpdir, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

finally:
	if (tmpdir)
		free(tmpdir);
	destroy(str, &env.tmpdir);
}


int
test_retval(int rv, int wanted)
{

	if (rv != wanted) {
		(void)printf(" ==> Invalid return value: "
			     "expecting %d but get %d", wanted, rv);
		throw("test failed");
	}
}


/*
 * test_strcmp(s1, s2)
 *
 * Compare two strings, <s1> and <s2>, and print a warning if they don't match.
 *
 */
int
test_strcmp(const char *s1, const char *s2)
{

	if (strcmp(s1, s2) != 0) {
		(void)printf(" ==> Comparison failed: `%s' != `%s'\n", s1, s2);
		throw("test failed");
	}
}


/*
 * test_strncmp(s1, s2, n)
 *
 * Compare two strings, <s1> and <s2>, and print a warning if they don't match.
 *
 */
int
test_strncmp(const char *s1, const char *s2, size_t len)
{

	if (strncmp(s1, s2, len) != 0) {
		(void)printf("NO\n\t==> Comparison failed: first %u bytes of `%s' != `%s'\n", 
				(unsigned int) len, s1, s2);
		throw("test failed");
	}
}

/*
 * test_strstr(big, little)
 *
 * Check if <little> exists inside of <big> and print a warning if it doesn't.
 *
 */
int
test_strstr(char *big, char *little)
{

	if (strstr(big, little) == NULL) {
		(void)printf("NO\n\t==> Comparison failed: string `%s` does not contain `%s'\n", 
				big, little);
		throw("test failed");
	}
}

