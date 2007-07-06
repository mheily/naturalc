/*		$Id: selftest.nc 92 2007-05-09 04:22:47Z mark $		*/

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

#include "config.h"

#include "nc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if DEADWOOD
static int
array_run_tests(void)
{
	array_t *a = NULL;
	int	x, *y;

	x = 1;
	y = &x;

	start_test("array_new()");
	array_new(&a);

	start_test("array_push()");
	array_push(a, y);

	start_test("array_pop()");
	y = NULL;
	array_pop(y, a);
	test_retval(*y, 1);

	start_test("array_destroy()");
	array_destroy(&a);

}

static int UNUSED
server_run_tests(void)
{
	thread_t thread;
	long      rc;

        static int       (*constructor[])(server_t *) = 
        {
                init_echo_server,
                NULL
        };

	set_log_level(LOG_DEBUG);

	/// @todo need a thread to communicate with the server
	/** @test server_multiplex() */
	start_test("server_multiplex()");
	(void) server_multiplex(constructor);
	thread_create(&thread, (callback_t) server_multiplex, constructor);
	thread_join(thread, rc);
}


static int
cidr_run_tests(void)
{
	string_t *cidr, *addr;
	bool result;

	start_test("cidr_contains_addr()");
	str_cpy(cidr, "192.168.0.0/24");
	str_cpy(addr, "192.168.0.1");
	cidr_contains_addr(&result, cidr, addr);
	if (!result)
		throw("incorrect result");
	str_cpy(cidr, "10.10.10.0/24");
	str_cpy(addr, "192.168.0.1");
	cidr_contains_addr(&result, cidr, addr);
	if (result)
		throw("incorrect result");
	str_cpy(cidr, "1.2.3.4");
	str_cpy(addr, "1.2.3.4");
	cidr_contains_addr(&result, cidr, addr);
	if (!result)
		throw("incorrect result");
}


static int
exception_run_tests(void)
{
	/** @todo */

	return 0;
}

static int
base64_run_tests(void)
{
	string_t *result = NULL;
	
	str_new(&result);

	start_test("base64_decode()");
       	base64_decode(result, "bWFya0ByZWN2bWFpbC5vcmc=");
	test_strcmp(result->value, "mark@recvmail.org");
	
finally:
	destroy(str, &result);
}


static int
acl_run_tests(void)
{
	string_t *buf = NULL;
	acl_t    *acl = NULL;
	list_t   *group = NULL;
	bool      result;

	acl_new(&acl);
	list_new(&group);
	str_new(&buf);

	start_test("acl_parse()");
	str_cpy(buf, "user:foo@bar.com:r-x");
	acl_parse(acl, buf);

	start_test("acl_compare() - allowed");
	str_cpy(buf, "foo@bar.com");
	acl_compare(&result, acl, ACL_READ, buf, group);
	test_retval(result, true);

	start_test("acl_compare() - denied");
	str_cpy(buf, "foo@bar.com");
	acl_compare(&result, acl, ACL_WRITE, buf, group);
	test_retval(result, false);
}

static int
db_run_tests(test_env_t *env)
{
	string_t *path = NULL;
	string_t *sql = NULL;
	string_t *cell = NULL;
	list_t   *row = NULL;
	db_t	*db = NULL;
	row_id_t  id = 0;

	str_new(&sql);
	str_new(&cell);
	str_new(&path);
	list_new(&row);
	str_sprintf(path, "%s/test.db", env->tmpdir->value);

#if ! HAVE_SQLITE3
	log_warning("%s", "SQLite3 not detected; skipping database tests");
	return 0;
#endif

	start_test ("db_new()");
       	db_new(&db);

	start_test ("db_open()");
	db_open(db, path, DB_SQLITE);

	start_test ("db_create_table()");
	db_create_table(db, "map",
			"id", SQL_TYPE_INT32, "",
			"name", SQL_TYPE_TEXT, "",
			szNULL
		       );

	start_test ("db_exec()");
	str_cpy(sql, "INSERT INTO map (id, name) VALUES (1, \"foo\")");
	db_exec(db, sql);

	start_test ("db_insert_id()");
	db_insert_id(&id, db);
	test_retval((int) id, 1);

	start_test ("db_get_next_row()");
	str_cpy(sql, "SELECT id, name FROM map WHERE id = 1;");
	db_exec(db, sql);
	db_get_next_row(row, db);
	list_compare(row, "1", "foo", szNULL);
	//db_print_results(sql, db); log_warning(sql->value); abort();

	/* Test prepared statements */
	db_exec(db, CSTRING("DELETE FROM map"));
	str_cpy(cell, "test");
	start_test ("db_prepare()");
	db_prepare(db, "INSERT INTO map (id, name) VALUES (?, ?)");

	start_test ("db_bind..()");
	db_bind_int(db, 1, 2);
	db_bind_string(db, 2, cell);

	start_test ("db_step()");
       	db_step(db);

	start_test ("db_reset()");
       	db_reset(db);

	start_test ("db_finalize()");
	db_finalize(db);
	
	//@todo start_test ("db_create()", ...);

	/* NOTE: This runs twice to check for an obscure bug */
	start_test("db_get_id()");
	db_get_id(&id, db, "map", "name", cell);
	test_retval((int) id, 2);
	db_get_id(&id, db, "map", "name", cell);
	test_retval((int) id,2);

	/* Test table functions */

	start_test ("db_create_table()");
	db_create_table(db, "test_table",
			"id", SQL_TYPE_SERIAL, "",
			"name", SQL_TYPE_TEXT, "NULL",
			"salary", SQL_TYPE_INT32, "",
			szNULL);

	/* Finished */

	start_test ("db_close()");
       	db_close(db);

	start_test ("db_destroy()");
       	db_destroy(&db);

	(void) unlink(path->value);

finally:
	destroy(str, &cell);
	destroy(str, &sql);
	destroy(str, &path);
}

#endif

static int
dns_run_tests(void)
{
	list_t	 *result   = NULL;
	list_t	 *ns       = NULL;
	string_t *host     = NULL;
	string_t *domain   = NULL;
	string_t *query    = NULL;
	string_t *buf      = NULL;
	int       port     = 0;

	/* Initialize local variables */
	str_new(&domain);
	str_new(&host);
	str_new(&query);
	str_new(&buf);
	list_new(&result);

	start_test("dns_library_init()");
       	dns_library_init();

	start_test("host_get_name()");
	host_get_name(host);

	start_test("dns_get_mx_list() - google.com");
	str_cpy(domain, "google.com");
	dns_get_mx_list(result, domain);

	/** @todo:
	start_test("dns_get_mx_list() - check results", 
			mx.count == 1 && 
			strcmp(mx.hostname[0], "mail.openmta.org"));
			*/

	start_test("dns_get_nameservers()");
	list_new(&ns);
	dns_get_nameservers(ns);

	start_test("dns_get_txt_record()");
	str_cpy(query, "recvmail.org");
	dns_get_txt_record(ns, query);

	start_test("dns_get_service_by_name()"); 
	dns_get_service_by_name(&port, "smtp", "tcp");
	if (port != 25) 
		throwf("%u", port);

#if FIXME
	// causes problems when building on recvmail.org -- returns 127.0.0.1
	// also causes an internet lookup which might be down/slow

	start_test("dns_get_inet_by_name()",
			str_cpy(query, "recvmail.org");
			dns_get_inet_by_name(result, query) == 0&&
			list_compare(result, "66.150.225.145", szNULL) == 0
		);
#endif
}


static int
file_run_tests(test_env_t *env)
{
	string_t *subdir = NULL,
		 *path = NULL,
		 *path2 = NULL,
		 *buf = NULL,
		 *pattern = NULL;
	list_t   *list = NULL;
	bool      exists;

	str_new(&subdir);
	str_new(&path);
	str_new(&path2);
	str_new(&buf);
	str_new(&pattern);
	list_new(&list);
	
	start_test("file_mkdir()");
	str_sprintf(subdir, "%s/testdir", env->tmpdir->value);
	file_mkdir(subdir); 
	
	start_test("file_rmdir()");
		str_sprintf(subdir, "%s/testdir", env->tmpdir->value);
		file_rmdir(subdir);

	start_test("file_glob()");
	str_cpy(pattern, "file.[oh]");
	file_glob(list, pattern);
	test_retval((int)list->count, 1);

	start_test("file_glob() - no matches");
		str_cpy(pattern, "FILE.NOT.FOUND");
		file_glob(list, pattern);
		test_retval((int)list->count, 0);

	/** @test file_exists() */
	start_test ("file_exists()"); 
	str_cpy(pattern, "FILE.NOT.FOUND");
	file_exists(&exists, pattern);
	if (exists)
		throw("file_exists() failed");

	/** @test file_copy() */
	start_test ("file_copy()"); 
	str_sprintf(path, "%s/file1", env->tmpdir->value);
	str_sprintf(path2, "%s/file2", env->tmpdir->value);
	str_cpy(buf, "test");
	file_write(path, buf);
	file_copy(path, path2);
	file_write(buf, path2);
	test_strcmp(buf->value, "test");
	file_unlink(path);
	file_unlink(path2);

	/** @test file_write() and file_read()
	 *
	 * Write a string to a file, then read it back again.
	 *
	 */
	start_test ("file_write() and file_read()");
	str_sprintf(path, "%s/fwfr", env->tmpdir->value);
	str_cpy(buf, "test");
	file_write(path, buf);
	str_truncate(buf);
	file_read(buf, path);
	test_strcmp(buf->value, "test");
}


static int
hash_run_tests(void)
{
	hash_t	*hash = NULL;
	string_t *ptr, *key, *data, *buf;
	list_t	*list;

	str_cpy(key, "key");
	str_cpy(data, "data");

	/** @test hash_new() */
	hash_new(&hash);

	/** @test hash_set() */
	hash_set(hash, "key", data);

	/** @test hash_get() */
	hash_truncate(hash);
	str_cpy(data, "data");
	hash_set(hash, "key", data);
	hash_get(ptr, hash, "key");
	test_strcmp(ptr->value, "data");

	/** @test hash_set() */
	hash_truncate(hash);
	str_cpy(data, "more.new.data");
	hash_set(hash, "key", data);
	hash_get(ptr, hash, "key");
	test_strcmp(ptr->value, "more.new.data"); 

	start_test( "hash_get_keys()"); 
	hash_truncate(hash);
	hash_set(hash, "key1", data);
	hash_get_keys(list, hash);
	list_compare(list, "key1", (char *) szNULL);

	start_test( "hash_get_values()"); 
	hash_truncate(hash);
	str_cpy(data, "more.new.data");
	hash_set(hash, "key1", data);
	hash_get_values(list, hash);
	list_compare(list, "more.new.data", (char *) szNULL);

#if FIXME
	start_test( "hash_value_exists()", 
			hash_value_exists(hash, "nonexistant") < 0 &&
			hash_value_exists(hash, "more.new.data") == 0 
		);	
#endif

#if FIXME
	// XXX-BUGBUG - crashes due to list_truncate() fix

	start_test( "hash_serialize()"); 
			str_cpy(data, "multiline\ndata");
			hash_set(hash, "key2", data); 
			hash_serialize(buf, hash);

	start_test( "hash_deserialize()"); 
			str_cpy(buf, "Single-Key: abc\nMulti-Key: d\\\ne\\\nf\n");
			hash_truncate(hash); 
			hash_deserialize(hash, buf);
			hash_get_keys(list, hash);
			list_sort(list, SORT_DESCENDING);
			list_compare(list, "Single-Key", "Multi-Key", szNULL);
			hash_get(ptr, hash, "Single-Key");
			test_strcmp(ptr->value, "abc");
			hash_get(ptr, hash, "Multi-Key");
			test_strcmp(ptr->value, "d\ne\nf\n");
#endif

	start_test ("hash_delete()"); 
			hash_truncate(hash); 
			str_cpy(data, "simple data");
			hash_set(hash, "key", data); 
			hash_delete(hash, "key");
			//TODO:if ((hash_get)(ptr, hash, "key") < 0 &&
			//TODO:hash_delete(hash, "key") < 0

	//TODO: start_test("hash_from_char()", ...);
	
	start_test ("hash_truncate()"); 
	hash_set(hash, "key", data);
	hash_truncate(hash);
	if (hash_get(ptr, hash, "key") == 0)
		throw("error");

	start_test ("hash_destroy()"); 
	hash_destroy(&hash);
}


#if DEADWOOD
static int
html_run_tests(void)
{
	html_t   *h = NULL;
	string_t *buf = NULL;

	str_new(&buf);

	start_test ("html_new()");
       	html_new(&h);

	start_test ("html_render()");
	str_cat(h->body, "hello");
	html_render(buf, h);

	start_test ("html_destroy()");
	html_destroy(&h);
	//@todo start_test("html_escape()", ...);

finally:
	destroy(str, &buf);
}
#endif


static int
list_run_tests(void)
{
	list_t *list = NULL;
	list_t *list2 = NULL;
	list_entry_t *ent = NULL;
	string_t     *str = NULL;
	string_t     *str_ptr = NULL;

	str_new(&str);

	start_test( "list_new()");
		list_new(&list);
		list_new(&list2);

	start_test( "str_join()"); 
			list_truncate(list);
			list_cat(list, "foo");
			list_cat(list, "bar");
			str_join(str, list, ' ');
			test_strcmp(str->value, "foo bar");

	start_test("str_split()");
	str_cpy(str, "foo bar baz");
	str_split(list, str, ' ');

	start_test("str_split() - trailing LF");
	str_cpy(str, "foo\nbar\nbaz\n");
	str_split(list, str, '\n');
	list_compare(list, "foo", "bar", "baz", szNULL);

	start_test( "str_split() - multiple empty LF"); 
			str_cpy(str, "\n\n\n\n");
			str_split(list, str, '\n');
			list_compare(list, "", "", "", "", szNULL);

	start_test( "str_split() - LF + empty item"); 
	str_cpy(str, "GET / HTTP/1.0\nUser-Agent: Lynx\n\nsomedata\n");
	str_split(list, str, '\n');
	list_compare(list, "GET / HTTP/1.0", "User-Agent: Lynx", "", "somedata", szNULL);

	start_test( "str_split() - RFC2822 via SMTP");
	str_cpy(str, "HELO a\nDATA\nTo: You\nFrom: Me\n"
			"Subject: Test\n\nMessage here.\n");
	str_split(list, str, '\n');
	list_compare(list, "HELO a", "DATA", "To: You", "From: Me",
			"Subject: Test", "", "Message here.", szNULL);

	start_test( "list_compare()"); 
	str_cpy(str, "foo bar baz");
	str_split(list, str, ' ');
	list_compare(list, "foo", "bar", "baz", szNULL);

	start_test( "list_shift()"); 
	list_shift(str, list);
	list_compare(list, "bar", "baz", szNULL);
	test_strcmp(str->value, "foo");
	list_shift(str, list);
	str_cmp(str, "bar");
	list_compare(list, "baz", szNULL);
	list_shift(str, list);
	str_cmp(str, "baz");

	/* Re-create the list after list_shift() killed it */
	str_cpy(str, "foo bar baz");
	str_split(list, str, ' ');
	start_test("list_compare() #2");
       	list_compare(list, "foo", "bar", "baz", szNULL);


	start_test("list_entry_get()");
       	list_entry_get(&ent, list, 0);

	str_ptr = ent->value;
	start_test("list_entry_get() - value matches");
	test_strcmp(str_ptr->value, "foo");

	start_test("list_entry_delete()");
	list_entry_delete(list, ent);

	start_test("list_truncate()");
	list_truncate(list);

	start_test("list_move()");
	list_truncate(list);
	list_truncate(list2);
	list_cat(list, "a");
	list_cat(list, "b");
	list_cat(list, "c");
	list_move(list2, list);
	list_compare(list2, "a", "b", "c", szNULL);
	if (list->count != 0)
		throw("error");

	start_test("list_sort() - ascending");
	list_truncate(list);
	list_cat(list, "c");
	list_cat(list, "a");
	list_cat(list, "b");
	list_sort(list, SORT_ASCENDING | SORT_LEXICOGRAPHIC);
	list_compare(list, "a", "b", "c", szNULL);

#if DEADWOOD
	start_test("list_append_unique()");
	list_truncate(list);
	str_cpy(str, "foo");
			list_append_unique(list, str);
			list_append_unique(list, str);
			str_cpy(str, "bar");
			list_append_unique(list, str);
			list_compare(list, "foo", "bar", szNULL) == 0
		);
#endif

	start_test("list_sort() - descending");
			list_truncate(list);
			list_cat(list, "x");
			list_cat(list, "z");
			list_cat(list, "y");
			list_sort(list, SORT_DESCENDING);
			list_compare(list, "z", "y", "x", szNULL);

	start_test("list_sort() - ascending numeric");
			list_truncate(list);
			list_cat(list, "99");
			list_cat(list, "100");
			list_sort(list, SORT_ASCENDING | SORT_NUMERIC);
			list_compare(list, "99", "100", szNULL);

	start_test("list_sort() - only one item");
			list_truncate(list);
			list_cat(list, "100");
			list_sort(list, SORT_ASCENDING | SORT_NUMERIC);
			list_compare(list, "100", szNULL);

	start_test("list_find_substr()");
			list_truncate(list);
			list_cat(list, "foo");
			list_cat(list, "bar");
			list_find_substr(list, "ba"); 
			list_find_substr(list, "bar"); 
			if ((list_find_substr)(list, "baz") == 0)
			       throw("error");	

	start_test("list_serialize()");
			list_truncate(list);
			list_cat(list, "s p a c e");
			list_cat(list, "foo");
			list_cat(list, "bar");
			list_serialize(str, list);
			test_strcmp(str->value, "s%20p%20a%20c%20e foo bar");

	start_test("list_deserialize()");
			str_cpy(str, "one%20two three");
			list_deserialize(list, str);
			list_compare(list, "one two", "three", szNULL);

	start_test("list_from_char()");
			list_from_char(list, "one", "two", "three", szNULL);
			list_compare(list, "one", "two", "three", szNULL);

#if FIXME
	start_test("list_reverse()");
	list_from_char(list, "one", "two", "three", szNULL);
	list_reverse(list);
	list_compare(list, "three", "two", "one", szNULL);
#endif

finally:
	destroy(str, &str);
	destroy(list, &list);
	destroy(list, &list2);
}


static int
passwd_run_tests(void)
{
	uid_t uid = 1;
	gid_t gid = 1;
	string_t *user = NULL,
		 *group = NULL;
	string_t *buf = NULL;
	bool      match;

	str_new(&buf);
	str_new(&user);
	str_new(&group);

	start_test("passwd_get_id_by_name()");
	str_cpy(user, "root");
	passwd_get_id_by_name(&uid, user);
	test_retval((int) uid, 0);

	start_test("passwd_exists() - nonexistent account");
	str_cpy(user, "r8-)t");
	passwd_exists(&match, user);
	if (match)
		throw("");

	start_test("group_get_id_by_name()");
	str_cpy(group, "daemon");
	group_get_id_by_name(&gid, group);
	test_retval((int) gid, 1);

	start_test("group_exists() - nonexistent account");
	str_cpy(group, "r8-)t");
	group_exists(&match, group); 
	if (match)
		throw("");

	start_test("group_get_name_by_id()");
	group_get_name_by_id(group, 1);
	test_strcmp(group->value, "daemon");
}



static int
socket_server_test(void UNUSED *a)
{
	socket_t *srv, *client;
	string_t *addr;

	log_warning("hello%s","a");
	socket_set_family(srv, PF_INET);
	str_cpy(addr, "127.0.0.1");
	socket_bind(srv, addr, 1234);
	socket_accept(client, srv);
	socket_puts(client, "hello\n");
}

static int
socket_client_test(void)
{
	socket_t *sock;
	string_t *addr;
	string_t *buf;

	socket_set_family(sock, PF_INET);
	str_cpy(addr, "127.0.0.1");

	start_test ("socket_connect()");
	socket_connect(sock, addr, 1234);

#if FIXME
	/** @test socket_get_peer_name()
	 *
	 * Note that the 'peer' in this case is reversed from
	 * the normal reverse lookup that is done when a client connects
	 * to us. That is why we first copy sock->local to sock->remote.
	 */
	start_test ("socket_get_peer_name()");
	memcpy(&sock->remote, &sock->local, sizeof(sock->local));
	socket_get_peer_name(buf, sock);
	throw(buf->value);
#endif

	start_test ("socket_readline()");
	socket_readline(buf, sock);

}

static int
socket_run_tests(void)
{
	string_t *buf  = NULL;
	socket_t *sock = NULL;
	list_t *list = NULL;
	bool      match;

	str_new(&buf);
	list_new(&list);

	start_test ("socket_init_library()");
	socket_init_library();

	start_test ("socket_new()");
	socket_new(&sock);
	socket_set_family(sock, PF_INET);

	start_test ("socket_bind() loopback test");
	thread_create_detached((callback_t) socket_server_test, &match);
	sleep(1);
	socket_client_test();

	/** @test dns_query_blocklist() and dns_ping_blocklist().
	 *
	 * Query a list of DNS blocklists.
	 * This is done here since we have a connected "peer" from
	 * the above test.
	 */
	start_test("dns_query_blocklist()");
	list_from_char(list, "bl.spamcop.net", "sbl-xbl.spamhaus.org", (char *) NULL);
	dns_ping_blocklist(list);
	dns_query_blocklist(&match, list, sock);

	start_test ("socket_close()");
	socket_close(sock);

	start_test ("socket_destroy()");
	socket_destroy(&sock);

	start_test ("host_get_ifaddrs()");
	host_get_ifaddrs(list, PF_INET);

finally:
	destroy(str, &buf);
	destroy(list, &list);
}


static int
str_run_tests(void)
{
	bool            result;
	//char		s2[100];
	string_t 	*str = NULL;
	string_t 	*str2 = NULL;
	string_t 	*str3 = NULL;
	var_char_t       c = 'X';
	int              i = 0;
	int UNUSED	*j = NULL;
	size_t           sz;

	start_test ("str_new()"); 
	str_new(&str);
	str_new(&str2);
	str_new(&str3);

	start_test ("str_cpy()"); 
	str_truncate(str);
	str_cpy(str, "xyz");
	test_strcmp(str->value, "xyz");

	start_test ("str_cat())");
	str_truncate(str);
       	str_cat(str, "test");
	test_retval((int) str_len(str), 4);

	start_test ("str_chomp() #1"); 
			str_cpy(str, ".\r\n");
			str_chomp(str);
			test_strcmp(str->value, ".");

	start_test ("str_chomp() #2"); 
			str_cpy(str, ".\n");
			str_chomp(str);
			test_strcmp(str->value, ".");

	start_test ("str_chomp() #3"); 
			str_cpy(str, ".\r");
			str_chomp(str);
			test_strcmp(str->value, ".");


	//start_test ("str_gets()", str_append(str, "test");
	start_test ("str_copy()"); 
			str_truncate(str);
			str_cat(str, "test");
			str_copy(str2, str);
			test_strcmp(str2->value, "test");

	start_test ("str_ncpy()"); 
			str_truncate(str);
			str_ncpy(str, "abcd", 3);
			test_strcmp(str->value, "abc");

	start_test ("str_cpy() - empty src"); 
			str_cpy(str, "abc");
			str_cpy(str, "");
			test_strcmp(str->value, "");

	start_test ("str_subst_regex()");
			str_cpy(str, "abcde");
			str_subst_regex(str, "bcd", ".NEW.");
			test_strcmp(str->value, "a.NEW.e");

	start_test ("str_escape()"); 
			str_cpy(str, " . ");
			str_escape(str2, str);
			test_strcmp(str2->value, "%20.%20");

	start_test ("str_unescape()"); 
			str_unescape(str, str2);
			test_strcmp(str->value, " . ");

	//http://recvmail.org:8025/compose?submit=Send&to=bugs%40recvmail.org&subj=test2&body=the+%27something%27+hth
	start_test ("str_unescape() - query string"); 
			str_cpy(str, "a %27quoted%27 word");
			str_unescape(str2, str);
			test_strcmp(str2->value, "a 'quoted' word");

	start_test ("str_unescape() - invalid input"); 
			str_cpy(str, "%");
			str_unescape(str2, str);
			test_strcmp(str2->value, "%");

	start_test ("str_translate()");
			str_cpy(str, "a+b+c");
			str_translate(str, '+', ' ');
			test_strcmp(str->value, "a b c");

				start_test("str_contains()");
			str_cpy(str, "foo bar baz");
			str_contains(&result, str, "bar");
			test_retval((int) result, true);

	start_test("str_count()"); 
			str_cpy(str, "zzzaaazzz");
			str_count(&sz, str, 'z');
			if (sz != 6)
				throwf("expecting 6; got %zu", sz);

	start_test ("str_cmp_terminator()");
	str_cpy(str, "foo\n");
	test_retval( str_cmp_terminator(str, '\n'), 0);

	start_test("str_match_regex()");
		str_cpy(str, "foo");
		str_match_regex(&result, str, "(foo|bar)");
		test_retval( result, true);
		str_cpy(str, "baz");
		str_match_regex(&result, str, "(foo|bar)");
		test_retval( result, false);

#ifdef TODO
	/* This works fine on OpenBSD. 
	   On Linux, it returns values that are 1 hour apart. */
	
	/* Time and date functions */
	time_t          timeval, timeval2;
	(void) time(&timeval);
	start_test ("str_from_time()", 
			str_from_time(str, &timeval);
			str_to_time(&timeval2, str);
			test_retval(timeval, timeval2) == 0
		 );
#endif


	start_test ("str_to_int()"); 
			str_cpy(str, "-36");
			str_to_int(&i, str);
			if (i != -36)
				throw("error");

	start_test ("str_divide()");
			str_cpy(str, "abc");
			str_divide(str2, str3, str, 1);
			test_strcmp(str2->value, "a");
			test_strcmp(str3->value, "c");

	start_test ("str_truncate_at()");
	str_cpy(str, "abc");
	str_truncate_at(str, 1);
	test_strcmp(str->value, "a");
	
	start_test ("str_swap()");
	str_cpy(str, "abc");
	str_cpy(str2, "cba");
	str_swap(str, str2);
	test_strcmp(str->value, "cba");
	test_strcmp(str2->value, "abc");

	start_test ("str_get_char()");
			str_cpy(str, "abc");
			str_get_char(&c, str, 1);
			if (c != 'b')
				throw("error");

	start_test("str_append()");
	str_cpy(str, "def");
	str_cpy(str2, "abc");
	str_prepend(str, str2);
	test_strcmp(str->value, "abcdef");

#if FIXME
	//FIXME: causes warning: str_to_pointer(j, str);
	start_test("str_from_pointer()");
	start_test("str_to_pointer()");
	str_from_pointer(str, &i);
	if (j != &i)
		throw("pointer conversion failed");
#endif

	start_test ("str_destroy()");
	str_destroy(&str);
}



/* A global variable used by the thread test suite */
static int GLOBAL_INT = 0;

/* A simple callback to test the thread pool */
void thread_callback(void *a UNUSED, void *b UNUSED);
void
thread_callback(void *a UNUSED, void *b UNUSED)
  {
	GLOBAL_INT++;
  }

static int
thread_run_tests(void)
{
	string_t *str = NULL;
	//bool      exists;

	str_new(&str);

	start_test("thread_library_init()"); 
	thread_library_init(); 

	
#if DEADWOOD
	/* -------- Thread pool tests ----------- */

	thread_pool_t *tpool = NULL;

	start_test("thread_pool_new()"); 
	thread_pool_new(&tpool);

	start_test("thread_pool_set_callback()");
	thread_pool_set_callback(tpool, (GFunc) thread_callback);

	start_test("thread_pool_add_work()");
	thread_pool_add_work(tpool, tpool);

	start_test("thread_pool_destroy()"); 
	thread_pool_destroy(&tpool);

	/* After performing it's work, the thread pool modifies the GLOBAL_INT variable */
	test_retval(GLOBAL_INT, 1);

	start_test("pidfile_create()"); 
	str_cpy(str, "testprog");
	pidfile_create(str);

	start_test("pidfile_stat()"); 
	str_cpy(str, "testprog");
	pidfile_stat(&exists, str);
	if (!exists)
		throw("error");

	start_test("pidfile_kill()"); 
	str_cpy(str, "testprog");
	pidfile_kill(str, 0);

#endif

#if EXTRA_UNIT_TESTS
	/** @todo - make a way for this to be enabled via ./configure() */

	start_test("thread_sleep()");
			thread_sleep(1) == 0
		);

#endif
}


int
main(void)
{
	string_t  *cmd = NULL,
		  *conffile = NULL;
	test_env_t te;

	// FIXME: nc_library_init();
	thread_library_init();

	memset(&te, 0, sizeof(te));
	str_new(&cmd);
	str_new(&conffile);

	/* Clean up the temporary directory */
	str_cpy(cmd, "rm -rf .check/");
	(void) process_run_system (cmd);

	/* Create the test directory */
	str_new(&te.tmpdir);
	str_cpy(te.tmpdir, ".check");
	(void) mkdir(".check", 0755);

	/* Redirect system prefixes to the temporary directory */
	// FIXME: no sys
	//str_cpy(sys.pidfiledir, ".check");
	//str_cpy(sys.mailboxdir, ".check");

	/* Parse a non-existent options file, which loads the default settings */
	str_cpy(conffile, ".check/nonexistent");
	// FIXME: options_parse(conffile);

#if FIXME
	/** @todo always run this test once it is finished. */
	if (getenv("TEST_SERVER")) {
		server_run_tests();
		abort();
	}
#endif

	/* Test the base-level libraries that higher up modules depend on*/
	str_run_tests();
	list_run_tests();	
	hash_run_tests();	

	//acl_run_tests();
	//array_run_tests();
	//base64_run_tests();
	//cidr_run_tests();
	//db_run_tests(&te);
	dns_run_tests();
	file_run_tests(&te);
	//html_run_tests();
	passwd_run_tests();
	socket_run_tests();
	thread_run_tests();

	/* Clean up the temporary directory */
	str_cpy(cmd, "rm -rf .check/");
	(void) process_run_system (cmd);
}
