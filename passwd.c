/*		$Id: passwd.nc 65 2007-04-18 03:53:44Z mark $		*/

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
 *  Routines dealing with /etc/passwd and /etc/group
 *
*/
 
#include "config.h"

#include "nc_exception.h"
#include "nc_file.h"
#include "nc_hash.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_memory.h"
#include "nc_string.h"
#include "nc_thread.h"

#include "nc_passwd.h"

#include <errno.h>
#include <grp.h>
#include <pwd.h>

/* This is from OpenBSD */
#define GETGR_R_SIZE_MAX        (1024+200*sizeof(char*))

/* ----------------- LOCAL FUNCTIONS ------------------------------*/

/* Some systems (e.g. OpenBSD) do not implement getpwuid_r and getpwnam_r */
#if HAVE_GETPWUID_R
# define passwd_getpwnam(a,b,c,d,e)   getpwnam_r(a, b, c, d, e)
# define passwd_getpwuid(a,b,c,d,e)   getpwuid_r(a, b, c, d, e)
#else

mutex_t PWDB_MUTEX = MUTEX_INITIALIZER;

/**
 * Replacement for getpwuid_r(3) and getpwnam_r(3) for some systems.
 *
 */
static int
__getpw_r(bool by_name, void *match, struct passwd *pwbuf, 
		char *buf, size_t buflen, struct passwd **pwbufp)
{
	uid_t          *uid;
	struct passwd  *pwent;

	mutex_lock(PWDB_MUTEX);

	// WORKAROUND - mask warnings
	if (0) { buflen = sizeof(buf); }
		
	if (by_name) {
		pwent = getpwnam((const char *) match);
	} else {
		uid = (uid_t *) match;
		pwent = getpwuid(*uid);
	}

	if (pwent != NULL) {
		memcpy(pwbuf, pwent, sizeof(*pwbuf));
		*pwbufp = pwbuf;
	} else {
		*pwbufp = NULL;
		throw_silent();
	}

finally:
	mutex_unlock(PWDB_MUTEX);
}

static inline int
passwd_getpwnam(const char *name, struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp)
{
        return __getpw_r(true, (void *) name, pwbuf, buf, buflen, pwbufp);
}


static inline int
passwd_getpwuid(uid_t uid, struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp)
{
        return __getpw_r(false, &uid, pwbuf, buf, buflen, pwbufp);
}

#endif

/* ----------------- GLOBAL FUNCTIONS ------------------------------*/


int
passwd_get_symbolic_uid(string_t *dest, hash_t *map, uid_t uid)
{
	string_t *uid_str, *result;

	/* Convert the UID to a string */
	str_sprintf(uid_str, "%u", uid);

	/* Lookup the UID in the map */
	hash_get(result, map, uid_str->value);

	/* Copy the result to the caller */
	str_copy(dest, result);
}


int
passwd_get_uid_map(hash_t *map)
{
	string_t *buf, *path, *line, *uid_num, *uid_sym;
	list_t   *rows, *cols;
	size_t    i    = 0;

	hash_truncate(map);

	/* Read the contents of the /etc/passwd file */
	str_cpy(path, "/etc/passwd");
	file_read(buf, path);

	/* Convert the raw buffer into a list of lines */
	str_chomp(buf);
	str_split(rows, buf, '\n');

	/* Process each row */
	for (i = 0; i < rows->count; i++) {

		list_get(line, rows, i);

		/* Skip lines that are commented out */
		if (str_ncmp(line, "#", 1) == 0) 
			continue;

		/* Split the row into ':' delimited columns */
		str_split(cols, line, ':');

		/* Load the symbolic UID and numeric UID */
		list_get(uid_sym, cols, 0);
		list_get(uid_num, cols, 2);

		//log_debug("%s (uid=%s)", uid_sym->value, uid_num->value);

		/* Add it to the hash */
		hash_set(map, uid_num->value, uid_sym);
	}
}


int
passwd_get_id_by_name(uid_t *uid, const string_t *name)
{
	struct passwd  pwent;
	struct passwd *pwptr;
	char           buf[512];

	if (passwd_getpwnam(name->value, &pwent, buf, sizeof(buf), &pwptr) < 0) {
		//throwf("user does not exist: %s", name->value);
		throw_silent();
	} else {
		if (pwptr != NULL) {
			*uid = pwent.pw_uid;
		} else {
			throw_silent();
		}
	}
}


int
passwd_exists(bool *result, const string_t *name)
{
	uid_t    uid = 0;

	*result = (passwd_get_id_by_name(&uid, name) == 0);
}


static int
__group_get_gev(const int by_name, gid_t *gid, string_t *name)
{
	int i = 0;
	struct group grent;
	struct group *ptr;
	char          buf[GETGR_R_SIZE_MAX + 1];

	/* WORKAROUND: BSD uses the 'wheel' group while Linux uses 'root' */
	if (str_cmp(name, "root") == 0 || str_cmp(name, "wheel") == 0) {
		*gid = 0;
		return 0;
	}

	ptr = &grent;
	memset(ptr, 0, sizeof(struct group));
	if (by_name) {
		i = getgrnam_r(name->value, &grent, buf, sizeof(buf), &ptr);
	} else {
		i = getgrgid_r(*gid, &grent, buf, sizeof(buf), &ptr);
	}

	if (i < 0) {
		log_warning("while resolving gid=%d name=`%s' ..", *gid, name);
		throw_errno("getgrnam_r(3) or getgrgid_r(3)");
	} else {
		if (ptr != NULL) {
			if (by_name) {
				*gid = ptr->gr_gid;
			} else {
				str_cpy(name, ptr->gr_name);
			}
		} else {
			throw_silent();
		}
	}
}


int
group_get_id_by_name(gid_t *gid, const string_t *name)
{

	if (__group_get_gev(true, gid, (string_t *) name) < 0)
		return -1;
}

int
group_get_name_by_id(string_t *name, gid_t gid)
{

	__group_get_gev(false, &gid, name);
}


int
group_exists(bool *result, const string_t *name)
{
	gid_t    gid = 0;

	*result = ((group_get_id_by_name)(&gid, name) == 0);
}


int
passwd_get_name_by_id(string_t *name, uid_t uid)
{
	struct passwd pwent;
	struct passwd *ptr,
		      *result = NULL;
	char          buf[1024];

	ptr = &pwent;
	if (passwd_getpwuid(uid, ptr, buf, sizeof(buf), &result) < 0)
		throw_errno("getpwuid_r(3)");

	str_cpy(name, result->pw_name);
}
