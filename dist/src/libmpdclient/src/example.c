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

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>
#include <mpd/message.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int
handle_error(struct mpd_connection *c)
{
	assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);

	fprintf(stderr, "%s\n", mpd_connection_get_error_message(c));
	mpd_connection_free(c);
	return EXIT_FAILURE;
}

static void
print_tag(const struct mpd_song *song, enum mpd_tag_type type,
	  const char *label)
{
	unsigned i = 0;
	const char *value;

	while ((value = mpd_song_get_tag(song, type, i++)) != NULL)
		printf("%s: %s\n", label, value);
}

int main(int argc, char ** argv) {
	struct mpd_connection *conn;

	conn = mpd_connection_new(NULL, 0, 30000);

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
		return handle_error(conn);

	{
		int i;
		for(i=0;i<3;i++) {
			printf("version[%i]: %i\n",i,
			       mpd_connection_get_server_version(conn)[i]);
		}
	}

	if(argc==1) {
		struct mpd_status * status;
		struct mpd_song *song;
		const struct mpd_audio_format *audio_format;

		mpd_command_list_begin(conn, true);
		mpd_send_status(conn);
		mpd_send_current_song(conn);
		mpd_command_list_end(conn);

		status = mpd_recv_status(conn);
		if (status == NULL)
			return handle_error(conn);

		printf("volume: %i\n", mpd_status_get_volume(status));
		printf("repeat: %i\n", mpd_status_get_repeat(status));
		printf("queue version: %u\n", mpd_status_get_queue_version(status));
		printf("queue length: %i\n", mpd_status_get_queue_length(status));
		if (mpd_status_get_error(status) != NULL)
			printf("error: %s\n", mpd_status_get_error(status));

		if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
		    mpd_status_get_state(status) == MPD_STATE_PAUSE) {
			printf("song: %i\n", mpd_status_get_song_pos(status));
			printf("elaspedTime: %i\n",mpd_status_get_elapsed_time(status));
			printf("elasped_ms: %u\n", mpd_status_get_elapsed_ms(status));
			printf("totalTime: %i\n", mpd_status_get_total_time(status));
			printf("bitRate: %i\n", mpd_status_get_kbit_rate(status));
		}

		audio_format = mpd_status_get_audio_format(status);
		if (audio_format != NULL) {
			printf("sampleRate: %i\n", audio_format->sample_rate);
			printf("bits: %i\n", audio_format->bits);
			printf("channels: %i\n", audio_format->channels);
		}

		mpd_status_free(status);

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
			return handle_error(conn);

		mpd_response_next(conn);

		while ((song = mpd_recv_song(conn)) != NULL) {
			printf("uri: %s\n", mpd_song_get_uri(song));
			print_tag(song, MPD_TAG_ARTIST, "artist");
			print_tag(song, MPD_TAG_ALBUM, "album");
			print_tag(song, MPD_TAG_TITLE, "title");
			print_tag(song, MPD_TAG_TRACK, "track");
			print_tag(song, MPD_TAG_NAME, "name");
			print_tag(song, MPD_TAG_DATE, "date");

			if (mpd_song_get_duration(song) > 0) {
				printf("time: %u\n", mpd_song_get_duration(song));
			}

			printf("pos: %u\n", mpd_song_get_pos(song));

			mpd_song_free(song);
		}

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
		    !mpd_response_finish(conn))
			return handle_error(conn);
	}
	else if(argc==3 && strcmp(argv[1],"lsinfo")==0) {
		struct mpd_entity * entity;

		if (!mpd_send_list_meta(conn, argv[2]))
			return handle_error(conn);

		while ((entity = mpd_recv_entity(conn)) != NULL) {
			const struct mpd_song *song;
			const struct mpd_directory *dir;
			const struct mpd_playlist *pl;

			switch (mpd_entity_get_type(entity)) {
			case MPD_ENTITY_TYPE_UNKNOWN:
				break;

			case MPD_ENTITY_TYPE_SONG:
				song = mpd_entity_get_song(entity);
				printf("uri: %s\n", mpd_song_get_uri(song));
				print_tag(song, MPD_TAG_ARTIST, "artist");
				print_tag(song, MPD_TAG_ALBUM, "album");
				print_tag(song, MPD_TAG_TITLE, "title");
				print_tag(song, MPD_TAG_TRACK, "track");
				break;

			case MPD_ENTITY_TYPE_DIRECTORY:
				dir = mpd_entity_get_directory(entity);
				printf("directory: %s\n", mpd_directory_get_path(dir));
				break;

			case MPD_ENTITY_TYPE_PLAYLIST:
				pl = mpd_entity_get_playlist(entity);
				printf("playlist: %s\n",
				       mpd_playlist_get_path(pl));
				break;
			}

			mpd_entity_free(entity);
		}

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
		    !mpd_response_finish(conn))
			return handle_error(conn);
	}
	else if(argc==2 && strcmp(argv[1],"artists")==0) {
		struct mpd_pair *pair;

		if (!mpd_search_db_tags(conn, MPD_TAG_ARTIST) ||
		    !mpd_search_commit(conn))
			return handle_error(conn);

		while ((pair = mpd_recv_pair_tag(conn,
						 MPD_TAG_ARTIST)) != NULL) {
			printf("%s\n", pair->value);
			mpd_return_pair(conn, pair);
		}

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
		    !mpd_response_finish(conn))
			return handle_error(conn);
	} else if (argc == 2 && strcmp(argv[1], "playlists") == 0) {
		if (!mpd_send_list_playlists(conn))
			return handle_error(conn);

		struct mpd_playlist *playlist;
		while ((playlist = mpd_recv_playlist(conn)) != NULL) {
			printf("%s\n",
			       mpd_playlist_get_path(playlist));
			mpd_playlist_free(playlist);
		}

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
		    !mpd_response_finish(conn))
			return handle_error(conn);
	} else if (argc == 2 && strcmp(argv[1], "idle") == 0) {
		enum mpd_idle idle = mpd_run_idle(conn);
		if (idle == 0 &&
		    mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
			return handle_error(conn);

		for (unsigned j = 0;; ++j) {
			enum mpd_idle i = 1 << j;
			const char *name = mpd_idle_name(i);

			if (name == NULL)
				break;

			if (idle & i)
				printf("%s\n", name);
		}
	} else if (argc == 3 && strcmp(argv[1], "subscribe") == 0) {
		/* subscribe to a channel and print all messages */

		if (!mpd_run_subscribe(conn, argv[2]))
			return handle_error(conn);

		while (mpd_run_idle_mask(conn, MPD_IDLE_MESSAGE) != 0) {
			if (!mpd_send_read_messages(conn))
				return handle_error(conn);

			struct mpd_message *msg;
			while ((msg = mpd_recv_message(conn)) != NULL) {
				printf("%s\n", mpd_message_get_text(msg));
				mpd_message_free(msg);
			}

			if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
			    !mpd_response_finish(conn))
				return handle_error(conn);
		}

		return handle_error(conn);
	} else if (argc == 2 && strcmp(argv[1], "channels") == 0) {
		/* print a list of channels */

		if (!mpd_send_channels(conn))
			return handle_error(conn);

		struct mpd_pair *pair;
		while ((pair = mpd_recv_channel_pair(conn)) != NULL) {
			printf("%s\n", pair->value);
			mpd_return_pair(conn, pair);
		}

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
		    !mpd_response_finish(conn))
			return handle_error(conn);
	} else if (argc == 4 && strcmp(argv[1], "message") == 0) {
		/* send a message to a channel */

		if (!mpd_run_send_message(conn, argv[2], argv[3]))
			return handle_error(conn);
	} else if (argc == 3 && strcmp(argv[1], "fingerprint") == 0) {
		char buffer[8192];

		const char *fingerprint = mpd_run_getfingerprint_chromaprint(conn, argv[2],
									     buffer, sizeof(buffer));
		if (fingerprint == NULL)
			return handle_error(conn);

		printf("%s\n", fingerprint);
	} else if (argc == 3 && strcmp(argv[1], "albumart") == 0) {
		unsigned offset = 0;
		unsigned size = 0;
		FILE *fp = fopen("/tmp/test", "w");
		if (fp == NULL) {
			return 1;
		}
		struct mpd_albumart buffer;
		struct mpd_albumart *albumart;
		while ((albumart = mpd_run_albumart(conn, argv[2], offset, &buffer)) != NULL) {
			fwrite(albumart->data, 1, albumart->data_length, fp);
			offset += albumart->data_length;
			size = albumart->size;
		}
		fclose(fp);
		printf("Wrote file: /tmp/test, size: %u bytes, retrieved: %u bytes\n", size, offset);
	}

	mpd_connection_free(conn);

	return 0;
}
