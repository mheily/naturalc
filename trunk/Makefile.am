#	$Id: Makefile.am 24 2006-12-10 18:43:41Z mark $
#
# Copyright (c) 2007 Mark Heily <devel@heily.com>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

SUBDIRS=		doc
AM_CFLAGS=		-g -Wall -Werror $(NCLIBDEP_CFLAGS)
AM_LDFLAGS=		$(NCLIBDEP_LIBS)

if WITH_LIBRESOLV
AM_LDFLAGS+=		-lresolv
endif

lib_LTLIBRARIES=	libnc.la

bin_SCRIPTS=		ncc
EXTRA_DIST=		ncc

pkginclude_HEADERS=	nc_dns.h \
			nc_exception.h \
			nc_file.h \
			nc_hash.h \
			nc_host.h \
			nc_list.h \
			nc_log.h \
			nc_memory.h \
			nc_passwd.h \
			nc_process.h \
			nc_server.h \
			nc_session.h \
			nc_signal.h \
			nc_string.h \
			nc_socket.h \
			nc_site.h \
			nc_test.h \
			nc_thread.h \
			nc.h

libnc_la_SOURCES=	file.c dns.c \
			exception.c \
			hash.c \
			host.c \
			list.c \
			log.c \
			memory.c \
			passwd.c \
			process.c \
			signal.c \
			server.c \
			session.c \
			socket.c \
			string.c \
			test.c \
			thread.c 

libnc_la_LIBADD=	$(NCLIBDEP_LIBS)

pkgconfigdir=		@pkgconfigdir@
pkgconfig_DATA=		libnc.pc

#
# Unit testing program
# 
TESTS=			selftest
check_PROGRAMS=		selftest
selftest_SOURCES=       selftest.c
selftest_LDADD=         $(NCLIBDEP_LIBS) libnc.la -lpthread
