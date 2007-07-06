/*		$Id: file.h 37 2007-03-31 17:29:59Z mark $		*/

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

#ifndef _NC_FILE_H
#define _NC_FILE_H

#include "nc_list.h"
#include "nc_string.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

/** A filetype corresponding to the POSIX stat(2) macros */
typedef enum {

	/** Nonexistent file.
	 *
	 * If stat(2) returns -1 and errno is set to ENOENT
	 */
	FILE_IS_NONEXISTENT = 0,

	/** Regular file.
	 *
	 * Corresponds to S_ISREG(m)
	 */
	FILE_IS_REGULAR,

	/** Directory.
	 *
	 * Corresponds to S_ISDIR(m)
	 */
	FILE_IS_DIRECTORY,

	/** Device node.
	 *
	 * Corresponds to S_ISCHR(m) or S_ISBLK(m)
	 */
	FILE_IS_DEVICE,

	/** Named pipe.
	 *
	 * Corresponds to S_ISFIFO(m)
	 */
	FILE_IS_FIFO,

	/** Symbolic link.
	 *
	 * Corresponds to S_ISLNK(m)
	 */
	FILE_IS_SYMLINK,

	/** Socket.
	 *
	 * Corresponds to S_ISSOCK(m)
	 */
	FILE_IS_SOCKET

} file_type_t;

/** Store the results of the stat(2) system call */
typedef struct {
	uid_t     uid;
	string_t *owner;
	gid_t     gid;
	string_t *group;
	off_t     size;
	mode_t    mode;
	file_type_t type;
} stat_t;


/** File object
 *
 * @todo Convert some functions to use this instead of always calling stat(2)
 */
typedef struct file {

	/** File descriptor. Invalid or uninitialized descriptors are set to -1. */
	int       fd;

	/** The flags that were passed to open(2) */
	int       flags;

	/** Absolute or relative path to the file */
	string_t *path;

	/** Cached stat(2) information about the file */
	stat_t    st;

	/** If TRUE, the file has been locked by the current process */
	bool      locked;

} file_t;

int file_new(file_t **file);
int file_destroy(file_t **file);
int file_from_fd(file_t *fo, const int fd);

int valid_pathname(const char *pathname);
int test_file_permissions(const char *path, const uid_t owner, const gid_t group, const mode_t mode);


/* Locking functions */

int file_lock(file_t *f, int lock_type);
#define file_lock_as_reader(f) file_lock(f, F_RDLCK)
#define file_lock_as_writer(f) file_lock(f, F_WRLCK)
int file_unlock(file_t *f);

/* File metadata functions */

int file_get_owner(string_t *dest, const string_t *path);
int file_get_group(string_t *dest, const string_t *path);
int file_get_size(size_t *dest, const string_t *path);
int file_get_mode(mode_t *dest, const string_t *path);
char *get_filename(const char *path);

/* Move, copy, delete, and rename operations */

int file_exists(bool *result, const string_t *path);
int file_copy(const string_t *src, const string_t *dest);
int file_move(const string_t *src, const string_t *dest);
int file_rename(const string_t *src, const string_t *dest);
int file_link(const string_t *src, const string_t *dest);
int file_symlink(const string_t *src, const string_t *dest);
int file_unlink(const string_t *path);

/* Ownership operations */

int file_chmod(const string_t *path, mode_t mode);
int file_chown(const string_t *path, const string_t *user, const string_t *group);
int file_chgrp(const string_t *path, const string_t *group);

/* Directory operations */

int file_mkdir(const string_t *path);
int file_rmdir(const string_t *path);
int file_ls(list_t *dest, const string_t *path);
int file_ls_by_type(list_t *dest, const string_t *path, int type);
int file_glob(list_t *dest, const string_t *pattern);

/* Basic I/O */

int file_open(file_t *file, const string_t *path, int flags, mode_t mode);
int file_read(string_t *dest, const string_t *path);
int file_write(const string_t *path, const string_t *data);
int file_puts(int fd, const string_t *str);
int file_close(file_t *file);

/* stat(2) calls */

int file_fstat(file_t *file);
int file_stat(stat_t *dest, const string_t *path);
int file_is_directory(const string_t *path);
int file_is_regular(const string_t *path);
int file_is_symlink(const string_t *path);
int file_get_mtime(time_t *dest, const string_t *path);
int file_mode_contains(const string_t *path, mode_t mask);

#define file_is_user_readable(f)	file_mode_contains(f, S_IRUSR)
#define file_is_user_writable(f)	file_mode_contains(f, S_IWUSR)
#define file_is_user_executable(f)	file_mode_contains(f, S_IXUSR)

#define file_is_group_readable(f)	file_mode_contains(f, S_IRGRP)
#define file_is_group_writable(f)	file_mode_contains(f, S_IWGRP)
#define file_is_group_executable(f)	file_mode_contains(f, S_IXGRP)

#define file_is_world_readable(f)	file_mode_contains(f, S_IROTH)
#define file_is_world_writable(f)	file_mode_contains(f, S_IWOTH)
#define file_is_world_executable(f)	file_mode_contains(f, S_IXOTH)

#define file_is_sticky(f)		file_mode_contains(f, S_ISVTX)

#endif
