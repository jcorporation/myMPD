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

#include <mpd/albumart.h>
#include <mpd/send.h>
#include <mpd/recv.h>
#include <mpd/pair.h>
#include <mpd/response.h>
#include <mpd/connection.h>
#include "run.h"
#include "internal.h"
#include "sync.h"
#include "isend.h"

#include <string.h>
#include <stdlib.h>

bool
mpd_send_albumart(struct mpd_connection *connection, const char *uri, const unsigned offset)
{
	return mpd_send_s_u_command(connection, "albumart", uri, offset);
}

struct mpd_albumart *
mpd_recv_albumart(struct mpd_connection *connection, struct mpd_albumart *buffer)
{
        struct mpd_albumart *result = NULL;

        //size pair
	struct mpd_pair *pair = mpd_recv_pair(connection);
	if (pair == NULL) {
	      return NULL;
        }
        buffer->size = atoi(pair->value);
        mpd_return_pair(connection, pair);

        //binary pair
	pair = mpd_recv_pair(connection);
	if (pair == NULL) {
	       return NULL;
        }
        buffer->data_length = atoi(pair->value);
        mpd_return_pair(connection, pair);

        //binary data
        buffer->data_length = buffer->data_length < MPD_BINARY_CHUNK_SIZE ? buffer->data_length : MPD_BINARY_CHUNK_SIZE;
        if (mpd_recv_binary(connection, buffer->data, buffer->data_length) != buffer->data_length) {
                mpd_error_code(&connection->error, MPD_ERROR_STATE);
                mpd_error_message(&connection->error,
                                  "Error reading binary data");
                return NULL;
        }
        
        result = buffer;
	return result;
}

struct mpd_albumart *
mpd_run_albumart(struct mpd_connection *connection, const char *uri, const unsigned offset, struct mpd_albumart *buffer)
{
	if (!mpd_run_check(connection) ||
	    !mpd_send_albumart(connection, uri, offset)) {
		return NULL;
        }
        
        struct mpd_albumart *result = mpd_recv_albumart(connection, buffer);
        if (!mpd_response_finish(connection) || 
            result->data_length == 0) {
                return NULL;
        }
	return result;
}
