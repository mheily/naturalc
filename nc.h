/*		$Id: $		*/

/*
 * Copyright (c) 2007 Mark Heily <devel@heily.com>
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
#ifndef _NC_H
#define _NC_H

#ifndef __cplusplus
/** Natural C uses 'class' to indicate a structure that can be 
 *  automatically managed via new() and destroy().
 */
#define class struct
#endif

#include "nc_site.h"

#include "nc_dns.h"
#include "nc_exception.h"
#include "nc_file.h"
#include "nc_hash.h"
#include "nc_host.h"
#include "nc_list.h"
#include "nc_log.h"
#include "nc_memory.h"
#include "nc_passwd.h"
#include "nc_process.h"
#include "nc_signal.h"
#include "nc_string.h"
#include "nc_test.h"
#include "nc_thread.h"

#include "nc_session.h"
#include "nc_socket.h"
#include "nc_server.h"

#endif
