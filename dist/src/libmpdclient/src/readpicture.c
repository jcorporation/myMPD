/* libmpdclient
   (c) 2003-2018 The Music Player Daemon Project
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
#include <mpd/readpicture.h>
#include "run.h"
#include "internal.h"
#include "sync.h"
#include "isend.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

void
mpd_free_readpicture(struct mpd_readpicture *buffer)
{
        if (buffer->mime_type != NULL) {
                free(buffer->mime_type);
                buffer->mime_type = NULL;
        }
}

bool
mpd_send_readpicture(struct mpd_connection *connection, const char *uri, unsigned offset)
{
	return mpd_send_s_u_command(connection, "readpicture", uri, offset);
}

bool
mpd_recv_readpicture(struct mpd_connection *connection, struct mpd_readpicture *buffer)
{
        buffer->size = 0;
        buffer->data_length = 0;
        buffer->mime_type = NULL;

        struct mpd_pair *pair = mpd_recv_pair_named(connection, "size");
        if (pair == NULL) {
                return false;
        }
        buffer->size = strtoumax(pair->value, NULL, 10);
        mpd_return_pair(connection, pair);
        
        pair = mpd_recv_pair_named(connection, "type");
        if (pair != NULL) {
                buffer->mime_type = strdup(pair->value);
                mpd_return_pair(connection, pair);
        }

        pair = mpd_recv_pair_named(connection, "binary");
        if (pair == NULL) {
               return false;
        }
        buffer->data_length = strtoumax(pair->value, NULL, 10);
        mpd_return_pair(connection, pair);

        //binary data
        buffer->data_length = buffer->data_length < MPD_BINARY_CHUNK_SIZE ? buffer->data_length : MPD_BINARY_CHUNK_SIZE;
        if (mpd_recv_binary(connection, buffer->data, buffer->data_length) == false) {
                return false;
        }
        
	return true;
}

bool
mpd_run_readpicture(struct mpd_connection *connection, const char *uri, unsigned offset, struct mpd_readpicture *buffer)
{
	if (!mpd_run_check(connection) ||
	    !mpd_send_readpicture(connection, uri, offset)) {
		return false;
        }
        
        if (mpd_recv_readpicture(connection, buffer) == false) {
                if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
                       mpd_connection_clear_error(connection);
                }
                mpd_response_finish(connection);
                return false;
        }
        else if (!mpd_response_finish(connection)) {
                return false;
        }
	return true;
}
