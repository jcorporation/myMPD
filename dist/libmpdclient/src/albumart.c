/* libmpdclient
   (c) 2003-2021 The Music Player Daemon Project
   This project's homepage is: http://www.musicpd.org
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   - Neither the name of the Music Player Daemon nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
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

#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include <mpd/connection.h>
#include <mpd/albumart.h>
#include "run.h"
#include "internal.h"
#include "sync.h"
#include "isend.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

bool
mpd_send_albumart(struct mpd_connection *connection, const char *uri, unsigned offset)
{
	return mpd_send_s_u_command(connection, "albumart", uri, offset);
}

int
mpd_recv_albumart(struct mpd_connection *connection, void *buffer, size_t buffer_size)
{
	struct mpd_pair *pair = mpd_recv_pair_named(connection, "binary");
	if (pair == NULL) {
		return -1;
	}

	size_t chunk_size = strtoumax(pair->value, NULL, 10);
	mpd_return_pair(connection, pair);

	size_t retrieve_bytes = chunk_size > buffer_size ? buffer_size : chunk_size;
	if (mpd_recv_binary(connection, buffer, retrieve_bytes) == false) {
		return -1;
	}

	return retrieve_bytes;
}

int
mpd_run_albumart(struct mpd_connection *connection,
				 const char *uri, unsigned offset,
				 void *buffer, size_t buffer_size)
{
	if (!mpd_run_check(connection) ||
		!mpd_send_albumart(connection, uri, offset)) {
			return -1;
	}

	int read_size = mpd_recv_albumart(connection, buffer, buffer_size);
	if (!mpd_response_finish(connection)) {
		return -1;
	}

	return read_size;
}
