/* libmpdclient
   (c) 2003-2019 The Music Player Daemon Project
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MPD_ISEARCH_H
#define MPD_ISEARCH_H

#include <stdbool.h>
#include <stdlib.h>

struct mpd_connection;

char *
mpd_sanitize_arg(const char *src);

bool
mpd_request_begin(struct mpd_connection *connection);

bool
mpd_request_command(struct mpd_connection *connection, const char *cmd);

char *
mpd_request_prepare_append(struct mpd_connection *connection,
			   size_t add_length);

bool
mpd_request_add_sort(struct mpd_connection *connection,
		     const char *name, bool descending);

bool
mpd_request_add_window(struct mpd_connection *connection,
		       unsigned start, unsigned end);

bool
mpd_request_commit(struct mpd_connection *connection);

void
mpd_request_cancel(struct mpd_connection *connection);

#endif
