	.file	"file.c"
	.section	.rodata
	.align 4
.LC0:
	.string	"tried to open a file with a zero-length pathname"
.LC1:
	.string	"%s"
.LC2:
	.string	"file.c"
	.align 4
.LC3:
	.string	"%s(%s:%d): ERROR: while opening `%s' ... "
.LC4:
	.string	"open(2)"
.LC5:
	.string	"%s: %s (errno=%d)"
.LC6:
	.string	"close(2)"
	.align 4
.LC7:
	.string	"%s(%s:%d): ERROR: while changing ownership of `%s' .."
.LC8:
	.string	"chmod(2)"
	.align 4
.LC9:
	.string	"%s(%s:%d): WARNING: unable to access %s"
.LC10:
	.string	"stat(2)"
.LC11:
	.string	"NULL input detected"
.LC12:
	.string	"invalid path length"
.LC13:
	.string	"invalid path"
.LC14:
	.string	"invalid filename"
.LC15:
	.string	"stat(2) failed"
.LC16:
	.string	"owner does not match"
	.align 4
.LC17:
	.string	"group does not match (have %d; want %d)"
	.align 4
.LC18:
	.string	"mode does not match: want %o, but got %o"
.LC19:
	.string	"file does not exist"
.LC20:
	.string	"chown(2)"
.LC21:
	.string	"stat(2) of file `%s' failed"
.LC22:
	.string	"cannot lock an unopened file"
	.align 4
.LC23:
	.string	"%s(%s:%d): ERROR: lock error: path=`%s' fd=%d lock_type=%d"
.LC24:
	.string	"fcntl(2)"
	.align 4
.LC25:
	.string	"%s(%s:%d): ERROR: while trying to remove `%s' ... "
.LC26:
	.string	"rmdir(2)"
.LC27:
	.string	"path(s) cannot be empty"
	.align 4
.LC28:
	.string	"%s(%s:%d): ERROR: while renaming `%s' to `%s'.."
.LC29:
	.string	"rename(2)"
	.align 4
.LC30:
	.string	"%s(%s:%d): ERROR: while symlinking `%s' to `%s'.."
.LC31:
	.string	"symlink(2)"
	.align 4
.LC32:
	.string	"%s(%s:%d): ERROR: while linking `%s' to `%s'.."
.LC33:
	.string	"link(2)"
	.align 4
.LC34:
	.string	"%s(%s:%d): ERROR: while trying to create `%s' ..."
.LC35:
	.string	"mkdir(2)"
	.align 4
.LC36:
	.string	"%s(%s:%d): ERROR: while examining `%s' .."
	.align 4
.LC37:
	.string	"file is too large to read into a buffer"
	.align 4
.LC38:
	.string	"%s(%s:%d): while trying to delete `%s' ... "
.LC39:
	.string	"unlink(2)"
.LC40:
	.string	"write(2)"
.LC41:
	.string	"%s.tmpXXXXXXXXXX"
.LC42:
	.string	"%s(%s:%d): ERROR: buf=`%s'"
.LC43:
	.string	"mkstemp(3)"
	.align 4
.LC44:
	.string	"%s(%s:%d): ERROR: while opening %s ..."
.LC45:
	.string	"opendir(3)"
.LC46:
	.string	"readdir_r(3)"
.LC47:
	.string	"."
.LC48:
	.string	".."
	.align 4
.LC49:
	.string	"%s(%s:%d): ERROR: while reading entries from %s ..."
	.align 4
.LC50:
	.string	"double new() detected; pointers must be preset to NULL"
.LC51:
	.string	"calloc(3) failed"
	.align 4
.LC52:
	.string	"%s(%s:%d): ERROR: while globbing for pattern `%s' ..."
.LC53:
	.string	"glob(3): out of memory"
.LC54:
	.string	"glob(3): aborted"
.LC55:
	.string	"glob(3): unsupported function"
.LC56:
	.string	"glob(3): unknown error"
.LC57:
	.string	"file is not currently open"
.LC58:
	.string	"unknown file type"
.LC59:
	.string	"invalid source file type: %s"
	.align 4
.LC60:
	.string	"FIXME - cannot recursively copy directories: %s"
.LC61:
	.string	"([^/]+)$"
	.align 4
.LC62:
	.string	"FIXME - lstat destination directory: %s"
	.align 4
.LC63:
	.string	"destination is not a directory: %s"
.LC64:
	.string	"(.+)/(.+)$"
.LC65:
	.string	"read(2)"
.LC66:
	.string	"%s/%s"
	.align 4
.LC67:
	.string	"%s(%s:%d): copying `%s' to `%s'"
.LC68:
	.string	"double new() detected"
.LC69:
	.string	"double free() detected"
.LC70:
	.string	"%s(%s:%d): WARNING: %s"
