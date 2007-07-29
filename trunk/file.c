/*		$Id: file.nc 37 2007-03-31 17:29:59Z mark $		*/

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
 * Functions dealing with files
 *
 * @todo make a general process_run_system() varargs function 
 *
*/

#include "config.h"

#include "nc_file.h"
#include "nc_passwd.h"
#include "nc_memory.h"
#include "nc_exception.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_string.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <grp.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


/* Forward declarations */
inline int __file_link(const string_t *src, const string_t *dest, const int symbolic);


int
file_new(file_t **file)
{
	file_t *f = NULL;

	if (*file != NULL)
		throw("double new() detected");

	mem_calloc(f);
	f->fd = -1;
	str_new(&f->path);
	*file = f;

catch:
	(void) file_destroy(&f);
}


/**
 * Wrapper around open(2).
 * 
 * @param file file object to be modified
 * @param path complete path to the file
 * @param flags any combination of O_RDWR, O_RDONLY, etc.
 * @param mode mode of the file, in octal notation
 *
*/
int
file_open(file_t *file, const string_t *path, int flags, mode_t mode)
{
	if (str_len(path) == 0)
		throw("tried to open a file with a zero-length pathname");

	if ((file->fd = open(path->value, flags, mode)) < 0) {
		file->fd = -1;
		log_error("while opening `%s' ... ", path->value);
		throw_errno("open(2)");
	}

	/* Update the object attributes */
	file->flags = flags;
	str_copy(file->path, path);
}


/**
 * Wrapper around close(2).
 *
 * When the file has been closed, the descriptor number will be set to -1.
 *
 * @param f file object
*/
int
file_close(file_t *f)
{

	/* Don't try to close a file more than once. */
	if (f->fd < 0) 
		return 0;

	if (close(f->fd) < 0) {
		throw_errno("close(2)");
	}

	f->fd = -1;
}


/**
 * Wrapper around chmod(2).
 *
 * @param path path to file
 * @param mode file permission bits
 *
*/
int
file_chmod(const string_t *path, mode_t mode)
{

	if (chmod(path->value, mode) < 0) {
		log_error("while changing ownership of `%s' ..", path->value);
		throw_errno("chmod(2)");
	}
}

 
/**
 * Test if a file exists
 *
 * @param result true if the file exists, or false if not
 * @param path path to the file
 *
 */
int
file_exists(bool *result, const string_t *path)
{
	struct stat st;

	if (stat(path->value, &st) < 0) {
		switch (errno) {
			case ENOENT: 
				*result = false;
				break;

			default:
				log_warning("unable to access %s", path->value);
				throw_errno("stat(2)");
				break;
		}
	} else {
		*result = true;
	}
}


/**
 * Test if a path contains unwanted characters or is otherwise illegal.
 *
 * @param path path to the file
 * @returns 0 if pathname is legal, or -1 if it is illegal.
 *
 */
int
valid_pathname(const char *path)
{
	static const char *ftext = "0123456789abcdefghijklmnopqrstuvwxyz./ABCDEFGHIKJLMNOPQRSTUVWXYZ-_:,";
	size_t	len;
	int i;

	/* Validate inputs */
	if (path == NULL)
		throw("NULL input detected");
	len = strlen(path);
	if (len > PATH_MAX)
		throw("invalid path length");

	/* Check each character */
	for (i = 0; i < (int) len; i++) {
                if ( strchr(ftext, path[i]) == NULL ) {
                                throw("invalid path");
                }
        }
}


/**
 * Test that a path exists and has a given set of permissions.
 *
 * @param path path to file
 * @param owner string containing the required file owner's name
 * @param group string containing the required file group's name
 * @param mode file permission bitmask
 * @returns 0 if all conditions are met, -1 if one or more fail
 *
 */
int
test_file_permissions(char_t *path, const uid_t owner, const gid_t group, const mode_t mode)
{
	struct stat st;

	if (valid_pathname(path) < 0)
		throw("invalid filename");

	/* Test that the path exists */
	if (stat(path, &st) < 0) 
		throw("stat(2) failed");

	/* Test the ownership and permissions */
	if (st.st_uid != owner)
		throw("owner does not match");
	if (st.st_gid != group)
		throwf("group does not match (have %d; want %d)", st.st_gid, group);
	if (st.st_mode != mode )
		throwf("mode does not match: want %o, but got %o", mode, st.st_mode);
}


/**
 * Wrapper around chown(2)
 *
 * @param path path to file
 * @param owner symbolic name of owner
 * @param group symbolic name of group
 *
*/
int
file_chown(const string_t *path, const string_t *owner, const string_t *group)
{
	uid_t	uid;
	bool    exists;

	file_exists(&exists, path);
	if (!exists)
		throw("file does not exist");

	/* Change the user ownership */
	/* Only the superuser can change the UID */
	if (getuid() == 0 && owner != NULL && str_len(owner) > 0) {
		passwd_get_id_by_name(&uid, owner);
		if (chown(path->value, uid, (gid_t) -1) < 0)
			throw_errno("chown(2)");
	}

	file_chgrp(path, group);
}


/**
 * Wrapper around chgrp(2)
 *
 * @param path path to file
 * @param group symbolic name of group
 *
*/
int
file_chgrp(const string_t *path, const string_t *group)
{
	gid_t gid;
	
	/* Change the group ownership */
	/* Only the superuser can change the GID (?) */
	if (getuid() == 0 && group != NULL && str_len(group) > 0) {
		group_get_id_by_name(&gid, group);
		if (chown(path->value, (uid_t) -1, gid) < 0)
			throw_errno("chown(2)");
	}
}


/**
 * Get the name of the owner of a file.
 *
 * @param dest string buffer that will store the result
 * @param path path to the file
*/
int
file_get_owner(string_t *dest, const string_t *path)
{
	struct stat st;

	/* Stat the file */
	if (stat(path->value, &st) < 0)
		throwf("stat(2) of file `%s' failed", path->value);

	/* Lookup the owner's name */
	passwd_get_name_by_id(dest, st.st_uid);
}


/**
 * Get the name of the group ownership of a file.
 *
 * @param dest string buffer that will store the result
 * @param path path to the file
*/
int
file_get_group(string_t *dest, const string_t *path)
{
	struct stat st;

	/* Stat the file */
	if (stat(path->value, &st) < 0)
		throwf("stat(2) of file `%s' failed", path->value);

	/* Lookup the group's name and copy it to the caller*/
	group_get_name_by_id(dest, st.st_gid);
}


/**
 * Wrapper to perform fcntl(2) file locking.
 *
 * @param f file object
 * @param lock_type F_WRLCK | F_RDLCK | F_UNLCK 
*/
int
file_lock(file_t *f, int lock_type)
{
	struct flock fl;

	/* convert int to short int */
	fl.l_type   = lock_type;	/* Any of: F_WRLCK | F_RDLCK | F_UNLCK */
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0;
	fl.l_len    = 0;
	fl.l_pid    = getpid();

	/* Make sure the file is open */
	if (f->fd < 0)
		throw("cannot lock an unopened file");

#if FIXME
	/** @bug Check for a double-lock condition */
	if (f->locked && (lock_type ^ F_UNLCK ?!?!)) {
		log_warning("double-lock() condition detected");
		return 0;
	}
	if (!f->locked && (lock_type & F_UNLCK)) {
		log_warning("double-unlock() condition detected");
		return 0;
	}
#endif

	/* Block until the lock request is satisfied */
	if (fcntl(f->fd, F_SETLKW, &fl) < 0) {
		log_error("lock error: path=`%s' fd=%d lock_type=%d", 
				f->path, f->fd, lock_type);
		throw_errno("fcntl(2)");
	}

	f->locked = true;
}


/**
 * Release a fcntl(2) lock on a file.
 *
 * @param fd file descriptor
*/
int
file_unlock(file_t *f) { return file_lock(f, F_UNLCK); }


/**
 * Wrapper around rmdir(2).
 *
 * @param path path to an empty directory
*/
int
file_rmdir(const string_t *path)
{

	if (rmdir(path->value) < 0) {
		log_error("while trying to remove `%s' ... ", path->value);
		throw_errno("rmdir(2)");
	}
}


/**
 * Wrapper around rename(2).
 *
 * @param src old pathname
 * @param dest new pathname
 * @todo make a generic function encompassing rename(2), symlink(2), and link(2)
*/
int 
file_rename(const string_t *src, const string_t *dest)
{

	/* Both pathnames must be non-empty */
	if (str_len(src) == 0 || str_len(dest) == 0)
		throw("path(s) cannot be empty");

	if (rename(src->value, dest->value) < 0) {
		log_error("while renaming `%s' to `%s'..", 
				src->value, dest->value);
		throw_errno("rename(2)");
	}
}


inline int 
__file_link(const string_t *src, const string_t *dest, const int symbolic)
{

	/* Both pathnames must be non-empty */
	if (str_len(src) == 0 || str_len(dest) == 0)
		throw("path(s) cannot be empty");

	if (symbolic) {
		if (symlink(src->value, dest->value) < 0) {
			log_error("while symlinking `%s' to `%s'..", 
					src->value, dest->value);
			throw_errno("symlink(2)");
		}
	} else {
		if (link(src->value, dest->value) < 0) {
			log_error("while linking `%s' to `%s'..", 
					src->value, dest->value);
			throw_errno("link(2)");
		}
	}
}


/**
 * Wrapper around link(2).
 *
 * @param src old pathname
 * @param dest new pathname
*/
int 
file_link(const string_t *src, const string_t *dest) { return __file_link(src, dest, false); }


/**
 * Wrapper around symlink(2).
 *
 * @param src old pathname
 * @param dest new pathname
*/
int
file_symlink(const string_t *src, const string_t *dest) { return __file_link(src, dest, true); }


/**
 * Wrapper around mkdir(2).
 *
 * @param path pathname to be created
 * @todo add mode parameter
*/
int
file_mkdir(const string_t *path)
{

	/* Create the directory */
	if (mkdir(path->value, 0770) < 0) {
		log_error("while trying to create `%s' ...", path->value);
		throw_errno("mkdir(2)");
	}

	/* Force the mode to be 0770 (mkdir applies the umask) */
	file_chmod(path, 0770);
}


/**
 * Get the size, in bytes, of a file.
 *
 * @param dest buffer where the result will be stored
 * @param path path to the file
*/
int
file_get_size(size_t *dest, const string_t *path)
{
        struct stat st;

	if (stat(path->value, &st) < 0) 
		throw_errno("stat(2)");
	*dest = (size_t) st.st_size;
	
catch:
	log_error("while examining `%s' ..", path->value);
}


/**
 * Get the file permissions for a file.
 *
 * @param dest buffer where the result will be stored
 * @param path path to the file
*/
int
file_get_mode(mode_t *dest, const string_t *path)
{
        struct stat st;

	if (stat(path->value, &st) < 0) {
		log_error("while examining `%s' ..", path->value);
		throw_errno("stat(2)");
	}
	*dest = st.st_mode;
}


/**
 * Read the contents of a file into a buffer.
 *
 * @param dest buffer where the result will be stored
 * @param path path to the file
*/
int
file_read(string_t *dest, const string_t *path)
{
	size_t  buf_sz;
	file_t *f;

	/* Open the file */
	file_open(f, path, O_RDONLY, 0);

	/* Ensure that the file is small enough to fit in a string buffer */
	file_fstat(f);
	if (f->st.size > STRING_MAX)
		throw("file is too large to read into a buffer");
	buf_sz = (size_t) f->st.size;

	/* Read the contents of the file */
	str_read(dest, f->fd, buf_sz);

	log_debug2("read %zu bytes from `%s'", buf_sz, path->value);
}


/**
 * Wrapper around unlink(2).
 *
 * @param path path to the file
*/
int
file_unlink(const string_t *path)
{

	if (unlink(path->value) < 0) {
		log_debug("while trying to delete `%s' ... ", path->value);
		throw_errno("unlink(2)");
	}
}


/**
 * Write a line of output to a file descriptor
 *
 * @param fd an open file descriptor
 * @param str buffer to be written
*/
int
file_puts(int fd, const string_t *str)
{
	ssize_t bytes;

	/* Don't write zero-length strings */
	if (str_len(str) == 0)
		return 0;

	/* Write the data to the file */
	bytes = write(fd, str->value, str->len);
	if (bytes < 0) 
		throw_errno("write(2)");
	/** @todo test for short write? */
}


/**
 * Write a buffer to a file, replacing any existing contents of the file.
 *
 * Create a file such that:
 *
 *    1. Multiple concurrent writers cannot intermingle data.
 *    2. If the program crashes in the middle of writing, any
 *       previous file at @a path will not be affected.
 *
 * The file is created using a temporary name, then rename(2) is
 * called to move the file to it's final name. 
 *
 * @param path pathname of the file
 * @param data buffer to be written to the file
*/
int
file_write(const string_t *path, const string_t *data)
{
	string_t *template;
	char buf[PATH_MAX + 1];
	int fd;

	/* Create a temporary file */
	strncpy(buf, "%s.tmpXXXXXXXXXX", sizeof(buf) - 1);
	if ((fd = mkstemp(buf)) < 0) {
		log_error("buf=`%s'", buf);
		throw_errno("mkstemp(3)");
	}
	str_cpy(template, buf);

	/* Write the data to the file */
	file_puts(fd, data);

	/* Close the file */
	if (close(fd) < 0)
		throw_errno("close(2)");

	/* Rename the file to the desired path */
	file_rename(template, path);

	/** @todo Delete the temporary file in the event of failure */
}


/**
 * Generate a list of all files in a directory
 *
 * @param dest list where the result will be stored
 * @param path path to the directory
*/
int
file_ls(list_t *dest, const string_t *path)
{
	struct dirent  ent;
	struct dirent *result;
	DIR           *dirp = NULL;
	int            rc = 0;

	/* Remove any previous list elements */
	list_truncate(dest);

	/* Open the directory handle */
	if ((dirp = opendir(path->value)) == NULL) {
		log_error("while opening %s ...", path->value);
		throw_errno("opendir(3)");
	}

	/* Read past the '.' entry */
	if (readdir_r(dirp, &ent, &result) < 0)
		throw_errno("readdir_r(3)");
	if (result == NULL)
		return 0;
	if (strcmp(ent.d_name, ".") != 0)
		list_cat(dest, ent.d_name);

	/* Read past the '..' entry */
	if (readdir_r(dirp, &ent, &result) < 0) {
		throw_errno("readdir_r(3)");
	} else {
		if (result == NULL)
			return 0;
		if (strcmp(ent.d_name, "..") != 0)
			list_cat(dest, ent.d_name);
	}

	/* Scan the entire directory */
	while (((rc = readdir_r(dirp, &ent, &result)) == 0) && result != NULL) {

		/* Add the entry to the list */
		list_cat(dest, ent.d_name);
	}
	if (rc < 0) {
		log_error("while reading entries from %s ...", path->value);
		throw_errno("readdir_r(3)");
	}

finally:
	if (dirp != NULL)
		(void) closedir(dirp);
}


/**
 * Using glob(2), generate a list of all files that match a given pattern.
 *
 * @param dest list where the result will be stored
 * @param pattern pattern to be supplied to glob(2)
*/
int
file_glob(list_t *dest, const string_t *pattern)
{
	glob_t *g = NULL;
	int i;

	/* Create a new glob_t object */
	mem_calloc(g);

	list_truncate(dest);

	/* Find all matching filenames */
	if ((i = glob(pattern->value, GLOB_NOSORT, NULL, g)) != 0) {
		if (i != GLOB_NOMATCH) {
			log_error("while globbing for pattern `%s' ...", pattern->value);
			switch (i) {
				case GLOB_NOSPACE: 
					throw("glob(3): out of memory");
					break;

				case GLOB_ABORTED:
					throw("glob(3): aborted");
					break;

				case GLOB_NOSYS:
					throw("glob(3): unsupported function");
					break;

				default:
					throw("glob(3): unknown error");
					break;
			}
		}
	} else {
		/* Copy the results into the list */
		for (i = 0; g->gl_pathv[i] != NULL; i++) {
			list_cat(dest, g->gl_pathv[i]);
		}
	}

finally:
	globfree(g);
	free(g);
}


/**
 * Get statistical information about an open file.
 *
 * Wrapper around fstat(2).
 *
 * @param file an open file object
 *
 */
int
file_fstat(file_t *file)
{

	if (file->fd < 0)
		throw("file is not currently open");

	/** @todo actually call fstat(2) here */
	file_stat(&file->st, file->path);
}


/**
 * Wrapper around stat(2).
 *
 * If the file does not exist, the 'type' field is set to FILE_IS_NONEXISTENT
 * and the function returns success.  
 *
 * @param dest pointer to a stat_t buffer that will store the result
 * @param path path to the file
*/
int
file_stat(stat_t *dest, const string_t *path)
{
	struct stat st;

	if (stat(path->value, &st) < 0) {

		/* Check if the file does not exist */
		if (errno == ENOENT) {
			memset(dest, 0, sizeof(dest));
			dest->type = FILE_IS_NONEXISTENT;
			return 0;
		}
		log_error("while examining `%s' ..", path->value);
		throw_errno("stat(2)");
	}

	dest->size = st.st_size;
	dest->mode = st.st_mode;
	if (S_ISREG(st.st_mode)) {
	       dest->type = FILE_IS_REGULAR;
	} else if (S_ISDIR(st.st_mode)) {
	       dest->type = FILE_IS_DIRECTORY;
	} else if (S_ISLNK(st.st_mode)) {
	       dest->type = FILE_IS_SYMLINK;
	} else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
	       dest->type = FILE_IS_DEVICE;
	} else if (S_ISFIFO(st.st_mode)) {
	       dest->type = FILE_IS_FIFO;
	} else if (S_ISSOCK(st.st_mode)) {
	       dest->type = FILE_IS_SOCKET;
	} else {
		throw("unknown file type");
	}
}


/**
 * Get the modification time of a file.
 * 
 * @param dest pointer to a time_t buffer where the result will be stored
 * @param path path to the file
*/
int
file_get_mtime(time_t *dest, const string_t *path)
{
	struct stat st;

	if (stat(path->value, &st) < 0) {
		log_error("while examining `%s' ..", path->value);
		throw_errno("stat(2)");
	}

	*dest = st.st_mtime;
}


/**
 * Check to see if a file mode contains all the members of a mask.
 *
 * @param path path to the file
 * @param mask file permissions bitmask
*/
int
file_mode_contains(const string_t *path, mode_t mask)
{
	struct stat st;
	
	/* Stat the file */
	if (stat(path->value, &st) < 0) {
		log_error("while examining `%s' ..", path->value);
		throw_errno("stat(2)");
	}

	/* Check if the mode includes the mask */
	if ((st.st_mode & mask) != mask)
		throw_silent();
}


/**
 * Test if a pathname points to a directory.
 *
 * @param path pathname to be tested 
 * @returns 0 if the file is a directory, -1 if it is not.
*/
int
file_is_directory(const string_t *path)
{
	stat_t st;
	
	file_stat(&st, path);
	if (st.type != FILE_IS_DIRECTORY)
		throw_silent();
}


/**
 * Test if a pathname points to a regular file.
 *
 * @param path pathname to be tested 
 * @returns 0 if the file is a regular file, -1 if it is not.
*/
int
file_is_regular(const string_t *path)
{
	stat_t st;
	
	file_stat(&st, path);
	if (st.type != FILE_IS_REGULAR)
		throw_silent();
}


/**
 * Test if a pathname points to a symbolic link.
 *
 * @param path pathname to be tested 
 * @returns 0 if the file is a regular file, -1 if it is not.
*/
int
file_is_symlink(const string_t *path)
{
	stat_t st;
	
	file_stat(&st, path);
	if (st.type != FILE_IS_SYMLINK)
		throw_silent();
}


/**
 * Copy a file from one location to another.
 *
 * @param src old location
 * @param dest new location
*/ 
int
file_copy(const string_t *src, const string_t *dest)
{
	int fd;
	string_t *buf, *old_fn, *new_fn, *new_basename, *new_path;
	mode_t    mode = 0;
	size_t    file_size = 0;
	bool      exists;

	/* Check if the source file can be copied */
	if (file_is_regular(src) < 0 && file_is_symlink(src) < 0) 
		throwf("invalid source file type: %s", src->value);
	if (file_is_directory(src) == 0)
		throwf("FIXME - cannot recursively copy directories: %s", src->value);

	/* Get the properties of the old file */
	str_str_regex(src, "([^/]+)$", old_fn, NULL);
	file_get_size(&file_size, src);
	file_get_mode(&mode, src);

	/* Check if the destination exists */
	file_exists(&exists, dest);
	if (exists) {
		if (file_is_symlink(dest) == 0) 
			throwf("FIXME - lstat destination directory: %s", src->value);
		if (file_is_directory(dest) < 0) 
			throwf("destination is not a directory: %s", src->value);
		str_copy(new_basename, dest);
		str_copy(new_fn, old_fn);
	} 

	/* Otherwise, get the filename of the new file */
	else {
		str_str_regex(dest, "(.+)/(.+)$", new_basename, new_fn, NULL);
	}
	//log_warning("new: base=`%s' fn=`%s'", new_basename->value, new_fn->value);

	/* Read the source file into a buffer */
	/* KLUDGE: the string library can't handle binary files */
	str_resize(buf, (size_t) file_size + 1);
	if ((fd = open(src->value, O_RDONLY, 0)) < 0)
		throw_errno("open(2)");
	if (read(fd, (char *) buf->value, (size_t) file_size) < (ssize_t) file_size)
		throw_errno("read(2)");
	if (close(fd) < 0)
		throw_errno("close(2)");

	/* Write the buffer to the new file */
	str_sprintf(new_path, "%s/%s", new_basename->value, new_fn->value);
	log_debug("copying `%s' to `%s'", src->value, new_path->value);
	if ((fd = open(new_path->value, O_WRONLY | O_CREAT, mode)) < 0)
		throw_errno("open(2)");
	if (write(fd, buf->value, (size_t) file_size) < (ssize_t) file_size)
		throw_errno("read(2)");
	if (close(fd) < 0)
		throw_errno("close(2)");
}


int
file_destroy(file_t **file)
{

	if (*file == NULL) {
		log_warning("%s", "double free() detected");
		return 0;
	}

	/* Close the file if it is open */
	if ((*file)->fd >= 0) {
		(void) (file_close)(*file);
	}

	str_destroy(&(*file)->path);

finally:
	if (file != NULL)  
		free(*file);
}


/**
 * Promote a file descriptor into a file object.
 *
 * This should only be used on automatic variables.
 *
 * @param pointer to file object
 * @param fd file descriptor
 */
int
file_from_fd(file_t *fo, const int fd)
{
	const string_t empty = EMPTY_STRING;

	memset(fo, 0, sizeof(*fo));
	fo->path = (string_t *) &empty;
	fo->fd = fd;
}


/**
 * List all files in a given directory that have a specific type.
 *
 * @param dest list to store the result
 * @param path path to examine
 * @param type type of file (FILE_IS_REGULAR, FILE_IS_DIRECTORY, etc..) 
 */
int file_ls_by_type(list_t *dest, const string_t *path, int type)
{
	string_t *item;
	list_t *buf;
	stat_t st;

	list_truncate(dest);

	/* List all files */
	file_ls(buf, path);

	/* Only return ones that match the query */
	for (;;) {
		if (buf->count == 0)
			break;
		list_shift(item, buf);
		file_stat(&st, item);
		if (st.type == (unsigned int) type)
			list_push(dest, item);
	}
}
