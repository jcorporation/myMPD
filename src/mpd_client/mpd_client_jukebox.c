/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_jukebox.h"

#include "../lib/filehandler.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/random.h"
#include "../lib/sds_extras.h"
#include "../lib/sticker_cache.h"
#include "../lib/utility.h"
#include "../mympd_api/mympd_api_queue.h"
#include "../mympd_api/mympd_api_sticker.h"
#include "mpd_client_errorhandler.h"
#include "mpd_client_search.h"
#include "mpd_client_search_local.h"
#include "mpd_client_tags.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//private definitions
static bool _mpd_client_jukebox(struct t_partition_state *partition_state);
static struct t_list *mpd_client_jukebox_get_last_played(struct t_partition_state *partition_state,
        enum jukebox_modes jukebox_mode);
static bool mpd_client_jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool _mpd_client_jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static long mpd_client_jukebox_unique_tag(struct t_partition_state *partition_state, const char *uri,
        const char *value, bool manual, struct t_list *queue_list);
static long mpd_client_jukebox_unique_album(struct t_partition_state *partition_state, const char *album,
        const char *albumartist, bool manual, struct t_list *queue_list);
static bool add_album_to_queue(struct t_partition_state *partition_state, struct mpd_song *album);
static long _fill_jukebox_queue_songs(struct t_partition_state *partition_state, long add_songs,
        const char *playlist, bool manual, struct t_list *queue_list, struct t_list *add_list);
static long _fill_jukebox_queue_albums(struct t_partition_state *partition_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list);

enum jukebox_uniq_result {
    JUKEBOX_UNIQ_IN_QUEUE = -2,
    JUKEBOX_UNIQ_IS_UNIQ = -1
};

//public functions

enum jukebox_modes mpd_client_parse_jukebox_mode(const char *str) {
    if (strcmp(str, "off") == 0) {
        return JUKEBOX_OFF;
    }
    if (strcmp(str, "song") == 0) {
        return JUKEBOX_ADD_SONG;
    }
    if (strcmp(str, "album") == 0) {
        return JUKEBOX_ADD_ALBUM;
    }
    return JUKEBOX_UNKNOWN;
}

const char *mpd_client_lookup_jukebox_mode(enum jukebox_modes mode) {
	switch (mode) {
        case JUKEBOX_OFF:
            return "off";
        case JUKEBOX_ADD_SONG:
            return "song";
        case JUKEBOX_ADD_ALBUM:
            return "album";
        default:
            return NULL;
    }
	return NULL;
}

bool mpd_client_rm_jukebox_entry(struct t_list *list, long pos) {
    struct t_list_node *node = list_node_at(list, pos);
    if (node == NULL) {
        return false;
    }
    node->user_data = NULL;
    return list_remove_node(list, pos);
}

void mpd_client_clear_jukebox(struct t_list *list) {
    list_clear(list);
}

sds mpd_client_get_jukebox_list(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        const long offset, const long limit, sds searchstr, const struct t_tags *tagcols)
{
    long entity_count = 0;
    long entities_returned = 0;
    long real_limit = offset + limit;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    if (partition_state->jukebox_mode == JUKEBOX_ADD_SONG) {
        struct t_list_node *current = partition_state->jukebox_queue.head;
        while (current != NULL) {
            bool rc = mpd_send_list_meta(partition_state->conn, current->key);
            if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_meta") == true) {
                struct mpd_song *song;
                if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
                    if (search_mpd_song(song, searchstr, tagcols) == true) {
                        if (entity_count >= offset &&
                            entity_count < real_limit)
                        {
                            if (entities_returned++) {
                                buffer = sdscatlen(buffer, ",", 1);
                            }
                            buffer = sdscatlen(buffer, "{", 1);
                            buffer = tojson_long(buffer, "Pos", entity_count, true);
                            buffer = get_song_tags(buffer, partition_state, tagcols, song);
                            if (partition_state->mpd_shared_state->feat_mpd_stickers == true &&
                                partition_state->mpd_shared_state->sticker_cache.cache != NULL)
                            {
                                buffer = sdscatlen(buffer, ",", 1);
                                buffer = mympd_api_sticker_list(buffer, &partition_state->mpd_shared_state->sticker_cache, mpd_song_get_uri(song));
                            }
                            buffer = sdscatlen(buffer, "}", 1);
                        }
                        entity_count++;
                    }
                    mpd_song_free(song);
                }
            }
            mpd_response_finish(partition_state->conn);
            mympd_check_error_and_recover(partition_state);
            current = current->next;
        }
    }
    else if (partition_state->jukebox_mode == JUKEBOX_ADD_ALBUM) {
        struct t_list_node *current = partition_state->jukebox_queue.head;
        sds album_lower = sdsempty();
        sds artist_lower = sdsempty();
        while (current != NULL) {
            album_lower = sdscatsds(album_lower, current->key);
            sds_utf8_tolower(album_lower);
            artist_lower = sdscatsds(artist_lower, current->value_p);
            sds_utf8_tolower(album_lower);
            if (strstr(album_lower, searchstr) != NULL ||
                strstr(artist_lower, searchstr) != NULL)
            {
                if (entity_count >= offset &&
                    entity_count < real_limit)
                {
                    if (entities_returned++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    buffer = sdscatlen(buffer, "{", 1);
                    buffer = tojson_long(buffer, "Pos", entity_count, true);
                    buffer = tojson_char(buffer, "uri", "Album", true);
                    buffer = tojson_char(buffer, "Title", "", true);
                    buffer = tojson_raw(buffer, "Album", current->key, true);
                    buffer = tojson_raw(buffer, "AlbumArtist", current->value_p, true);
                    buffer = tojson_raw(buffer, "Artist", current->value_p, false);
                    buffer = sdscatlen(buffer, "}", 1);
                }
                entity_count++;
            }
            sdsclear(album_lower);
            sdsclear(artist_lower);
            current = current->next;
        }
        FREE_SDS(album_lower);
        FREE_SDS(artist_lower);
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "jukeboxMode", partition_state->jukebox_mode, true);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_respond_end(buffer);

    return buffer;
}

bool mpd_client_jukebox(struct t_partition_state *partition_state) {
    for (int i = 1; i < 3; i++) {
         if (_mpd_client_jukebox(partition_state) == true) {
             return true;
         }
         if (partition_state->jukebox_mode == JUKEBOX_OFF) {
             return false;
         }
         MYMPD_LOG_ERROR("Jukebox: trying again, attempt %d", i);
    }
    return false;
}

static bool _mpd_client_jukebox(struct t_partition_state *partition_state) {
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status == NULL) {
        mympd_check_error_and_recover(partition_state);
        return false;
    }
    long queue_length = (long)mpd_status_get_queue_length(status);
    mpd_status_free(status);

    time_t now = time(NULL);
    time_t add_time = partition_state->song_end_time - (partition_state->crossfade + 10);

    MYMPD_LOG_DEBUG("Queue length: %ld", queue_length);
    MYMPD_LOG_DEBUG("Min queue length: %ld", partition_state->jukebox_queue_length);

    if (queue_length >= partition_state->jukebox_queue_length && now < add_time) {
        MYMPD_LOG_DEBUG("Jukebox: Queue length >= %ld and add_time not reached", partition_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    long add_songs = partition_state->jukebox_queue_length > queue_length ? partition_state->jukebox_queue_length - queue_length : 0;

    if (now > add_time &&
        add_time > 0 &&
        queue_length <= partition_state->jukebox_queue_length)
    {
        if (add_songs == 0) {
            add_songs++;
        }
        MYMPD_LOG_DEBUG("Time now %lld greater than add_time %lld, adding %ld song(s)", (long long)now, (long long)add_time, add_songs);
    }

    if (add_songs < 1) {
        MYMPD_LOG_DEBUG("Jukebox: nothing to do");
        return true;
    }

    if (add_songs > 99) {
        MYMPD_LOG_WARN("Jukebox: max songs to add set to %ld, adding max. 99 songs", add_songs);
        add_songs = 99;
    }

    if (partition_state->mpd_shared_state->feat_mpd_playlists == false && strcmp(partition_state->jukebox_playlist, "Database") != 0) {
        MYMPD_LOG_WARN("Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = mpd_client_jukebox_add_to_queue(partition_state, add_songs, partition_state->jukebox_mode, partition_state->jukebox_playlist, false);

    //update playback state
    mympd_api_queue_status(partition_state, NULL);
    if (partition_state->play_state != MPD_STATE_PLAY) {
        MYMPD_LOG_DEBUG("Jukebox: start playback");
        rc = mpd_run_play(partition_state->conn);
        mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_play");    
    }

    if (rc == true) {
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX);
        return true;
    }

    MYMPD_LOG_DEBUG("Jukebox mode: %d", partition_state->jukebox_mode);
    MYMPD_LOG_ERROR("Jukebox: Error adding song(s)");
    return false;
}

bool mpd_client_jukebox_add_to_queue(struct t_partition_state *partition_state, long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    if (manual == false) {
        MYMPD_LOG_DEBUG("Jukebox queue length: %ld", partition_state->jukebox_queue.length);
    }
    if ((manual == false && add_songs > partition_state->jukebox_queue.length) ||
        (manual == true))
    {
        bool rc = mpd_client_jukebox_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
        if (rc == false) {
            return false;
        }
    }
    long added = 0;
    struct t_list_node *current;
    if (manual == false) {
        current = partition_state->jukebox_queue.head;
    }
    else {
        current = partition_state->jukebox_queue_tmp.head;
    }
    while (current != NULL &&
        added < add_songs)
    {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	        bool rc = mpd_run_add(partition_state->conn, current->key);
            if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_add") == true) {
	            MYMPD_LOG_NOTICE("Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("Jukebox adding song %s failed", current->key);
            }
        }
        else {
            bool rc = add_album_to_queue(partition_state, (struct mpd_song *)current->user_data);
            if (rc == true) {
                MYMPD_LOG_NOTICE("Jukebox adding album: %s - %s", current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("Jukebox adding album %s - %s failed", current->value_p, current->key);
            }
        }
        if (manual == false) {
            partition_state->jukebox_queue.head->user_data = NULL;
            mpd_client_rm_jukebox_entry(&partition_state->jukebox_queue, 0);
            current = partition_state->jukebox_queue.head;
        }
        else {
            partition_state->jukebox_queue_tmp.head->user_data = NULL;
            mpd_client_rm_jukebox_entry(&partition_state->jukebox_queue_tmp, 0);
            current = partition_state->jukebox_queue_tmp.head;
        }
    }
    if (added == 0) {
        MYMPD_LOG_ERROR("Error adding song(s)");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, "Addings songs from jukebox to queue failed");
        return false;
    }
    if (manual == false) {
        if ((jukebox_mode == JUKEBOX_ADD_SONG && partition_state->jukebox_queue.length < 25) ||
            (jukebox_mode == JUKEBOX_ADD_ALBUM && partition_state->jukebox_queue.length < 5))
        {
            bool rc = mpd_client_jukebox_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
            if (rc == false) {
                return false;
            }
        }
        MYMPD_LOG_DEBUG("Jukebox queue length: %ld", partition_state->jukebox_queue.length);
    }
    return true;
}

//private functions
static bool add_album_to_queue(struct t_partition_state *partition_state, struct mpd_song *album) {
    bool rc = mpd_search_add_db_songs(partition_state->conn, true);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_search_add_db_songs") == false) {
        mpd_search_cancel(partition_state->conn);
        return false;
    }

    const char *value = NULL;
    unsigned i = 0;
    sds expression = sdsnewlen("(", 1);
    while ((value = mpd_song_get_tag(album, partition_state->mpd_shared_state->tag_albumartist, i)) != NULL) {
        expression = escape_mpd_search_expression(expression, mpd_tag_name(partition_state->mpd_shared_state->tag_albumartist), "==", value);
        expression = sdscat(expression, " AND ");
        i++;
    }
    expression = escape_mpd_search_expression(expression, "Album", "==", mpd_song_get_tag(album, MPD_TAG_ALBUM, 0));
    expression = sdscatlen(expression, ")", 1);

    rc = mpd_search_add_expression(partition_state->conn, expression);
    FREE_SDS(expression);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_search_add_expression") == false) {
        mpd_search_cancel(partition_state->conn);
        return false;
    }
    rc = mpd_search_commit(partition_state->conn);
    mpd_response_finish(partition_state->conn);
    return mympd_check_rc_error_and_recover(partition_state, rc, "mpd_search_commit");
}

static struct t_list *mpd_client_jukebox_get_last_played(struct t_partition_state *partition_state, enum jukebox_modes jukebox_mode) {
    struct mpd_song *song;
    struct t_list *queue_list = list_new();

    bool rc = mpd_send_list_queue_meta(partition_state->conn);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_queue_meta") == false) {
        FREE_PTR(queue_list);
        return NULL;
    }

    sds tag_value = sdsempty();
    sds album = sdsempty();
    sds albumartist = sdsempty();

    while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
            }
            list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
            sdsclear(tag_value);
        }
        else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
            album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
            albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_shared_state->tag_albumartist, albumartist);
            list_push(queue_list, album, 0, albumartist, NULL);
            sdsclear(album);
            sdsclear(albumartist);
        }
        mpd_song_free(song);
    }

    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);

    //append last_played to queue list
    struct t_list_node *current = partition_state->mpd_shared_state->last_played.head;
    while (current != NULL) {
        rc = mpd_send_list_meta(partition_state->conn, current->key);
        if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_meta") == true) {
            song = mpd_recv_song(partition_state->conn);
            if (song != NULL) {
                if (jukebox_mode == JUKEBOX_ADD_SONG) {
                    if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                        tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
                    }
                    list_push(queue_list, current->key, 0, tag_value, NULL);
                    sdsclear(tag_value);
                }
                else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                    album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
                    albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_shared_state->tag_albumartist, albumartist);
                    list_push(queue_list, album, 0, albumartist, NULL);
                    sdsclear(album);
                    sdsclear(albumartist);
                }
                mpd_song_free(song);
            }
        }
        mpd_response_finish(partition_state->conn);
        mympd_check_error_and_recover(partition_state);
        current = current->next;
    }
    //get last_played from disc
    if (queue_list->length < 20) {
        sds line = sdsempty();
        char *data = NULL;
        sds lp_file = sdscatfmt(sdsempty(), "%S/state/last_played", partition_state->mpd_shared_state->config->workdir);
        errno = 0;
        FILE *fp = fopen(lp_file, OPEN_FLAGS_READ);
        if (fp != NULL) {
            while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0 &&
                   queue_list->length < 20)
            {
                int value = (int)strtoimax(line, &data, 10);
                if (value > 0 && strlen(data) > 2) {
                    data = data + 2;
                    rc = mpd_send_list_meta(partition_state->conn, data);
                    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_meta") == true) {
                        song = mpd_recv_song(partition_state->conn);
                        if (song != NULL) {
                            if (jukebox_mode == JUKEBOX_ADD_SONG) {
                                if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                                    tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
                                }
                                list_push(queue_list, data, 0, tag_value, NULL);
                                sdsclear(tag_value);
                            }
                            else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                                album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
                                albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_shared_state->tag_albumartist, albumartist);
                                list_push(queue_list, album, 0, albumartist, NULL);
                                sdsclear(album);
                                sdsclear(albumartist);
                            }
                            mpd_song_free(song);
                        }
                    }
                    mpd_response_finish(partition_state->conn);
                    mympd_check_error_and_recover(partition_state);
                }
                else {
                    MYMPD_LOG_ERROR("Reading last_played line failed");
                    MYMPD_LOG_DEBUG("Erroneous line: %s", line);
                }
            }
            (void) fclose(fp);
            FREE_SDS(line);
        }
        else {
            //ignore missing last_played file
            MYMPD_LOG_DEBUG("Can not open \"%s\"", lp_file);
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(lp_file);
    }
    FREE_SDS(album);
    FREE_SDS(albumartist);
    FREE_SDS(tag_value);
    MYMPD_LOG_DEBUG("Jukebox last_played list length: %ld", queue_list->length);
    return queue_list;
}

static bool mpd_client_jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
    long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_INFO, "Filling jukebox queue");
    MYMPD_LOG_DEBUG("Jukebox queue to small, adding entities");
    if (partition_state->mpd_shared_state->feat_mpd_tags == true) {
        if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            enable_mpd_tags(partition_state, &partition_state->jukebox_unique_tag);
        }
        else {
            disable_all_mpd_tags(partition_state);
        }
    }
    bool rc = _mpd_client_jukebox_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
    if (partition_state->mpd_shared_state->feat_mpd_tags == true) {
        enable_mpd_tags(partition_state, &partition_state->mpd_shared_state->tag_types_mympd);
    }

    if (rc == false) {
        MYMPD_LOG_ERROR("Filling jukebox queue failed, disabling jukebox");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, "Filling jukebox queue failed, disabling jukebox");
        partition_state->jukebox_mode = JUKEBOX_OFF;
        return false;
    }
    return true;
}

static bool _mpd_client_jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
    long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    long added = 0;

    if (manual == true) {
        mpd_client_clear_jukebox(&partition_state->jukebox_queue_tmp);
    }

    //get last_played and current queue
    struct t_list *queue_list = mpd_client_jukebox_get_last_played(partition_state, jukebox_mode);
    if (queue_list == NULL) {
        return false;
    }

    struct t_list *add_list = manual == false ? &partition_state->jukebox_queue : &partition_state->jukebox_queue_tmp;

    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        added = _fill_jukebox_queue_songs(partition_state, add_songs, playlist, manual, queue_list, add_list);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        added = _fill_jukebox_queue_albums(partition_state, add_songs, manual, queue_list, add_list);
    }

    if (added < add_songs) {
        MYMPD_LOG_WARN("Jukebox queue didn't contain %ld entries", add_songs);
        if (partition_state->jukebox_enforce_unique == true) {
            MYMPD_LOG_WARN("Disabling jukebox unique constraints temporarily");
            partition_state->jukebox_enforce_unique = false;
            send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_WARN, "Playlist to small, disabling jukebox unique constraints temporarily");
        }
    }

    list_free(queue_list);
    return true;
}

static long _fill_jukebox_queue_albums(struct t_partition_state *partition_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list)
{
    if (partition_state->mpd_shared_state->album_cache.cache == NULL) {
        MYMPD_LOG_WARN("Album cache is null, jukebox can not add albums");
        return -1;
    }

    long start_length = 0;
    if (manual == false) {
        start_length = partition_state->jukebox_queue.length;
        add_albums = 10 - partition_state->jukebox_queue.length;
        if (add_albums <= 0) {
            return 0;
        }
    }

    long skipno = 0;
    long nkeep = 0;
    long lineno = 1;
    raxIterator iter;
    raxStart(&iter, partition_state->mpd_shared_state->album_cache.cache);
    raxSeek(&iter, "^", NULL, 0);
    sds album = sdsempty();
    sds albumartist = sdsempty();
    while (raxNext(&iter)) {
        struct mpd_song *song = (struct mpd_song *)iter.data;
        sdsclear(album);
        sdsclear(albumartist);
        album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
        albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_shared_state->tag_albumartist, albumartist);
        long is_uniq = JUKEBOX_UNIQ_IS_UNIQ;
        if (partition_state->jukebox_enforce_unique == true) {
            is_uniq = mpd_client_jukebox_unique_album(partition_state, album, albumartist, manual, queue_list);
        }

        if (is_uniq == JUKEBOX_UNIQ_IS_UNIQ) {
            if (randrange(0, lineno) < add_albums) {
                if (nkeep < add_albums) {
                    if (list_push(add_list, album, lineno, albumartist, song) == true) {
                        nkeep++;
                    }
                    else {
                        MYMPD_LOG_ERROR("Can't push jukebox_queue element");
                    }
                }
                else {
                    long pos = add_albums > 1 ? start_length + randrange(0, add_albums -1) : 0;
                    struct t_list_node *node = list_node_at(add_list, pos);
                    if (node != NULL) {
                        node->user_data = NULL;
                        if (list_replace(add_list, pos, album, lineno, albumartist, song) == false) {
                            MYMPD_LOG_ERROR("Can't replace jukebox_queue element pos %ld", pos);
                        }
                    }
                    else {
                        MYMPD_LOG_ERROR("Can't replace jukebox_queue element pos %ld", pos);
                    }
                }
            }
            lineno++;
        }
        else {
            skipno++;
        }
    }
    FREE_SDS(album);
    FREE_SDS(albumartist);
    raxStop(&iter);
    MYMPD_LOG_DEBUG("Jukebox iterated through %ld albums, skipped %ld", lineno, skipno);
    return (int)nkeep;
}

static long _fill_jukebox_queue_songs(struct t_partition_state *partition_state, long add_songs, const char *playlist,
        bool manual, struct t_list *queue_list, struct t_list *add_list)
{
    unsigned start = 0;
    unsigned end = start + MPD_RESULTS_MAX;
    long skipno = 0;
    long nkeep = 0;
    long lineno = 1;
    time_t since = time(NULL);
    since = since - (partition_state->jukebox_last_played * 3600);

    if (partition_state->mpd_shared_state->sticker_cache.cache == NULL) {
        MYMPD_LOG_WARN("Sticker cache is null, jukebox doesn't respect last played constraint");
    }

    long start_length = 0;
    if (manual == false) {
        start_length = partition_state->jukebox_queue.length;
        add_songs = (long)50 - start_length;
        if (add_songs <= 0) {
            return 0;
        }
    }
    bool from_database = strcmp(playlist, "Database") == 0 ? true : false;
    sds tag_value = sdsempty();
    do {
        MYMPD_LOG_DEBUG("Jukebox: iterating through source, start: %u", start);

        if (from_database == true) {
            if (mpd_search_db_songs(partition_state->conn, false) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_db_songs");
            }
            else if (mpd_search_add_uri_constraint(partition_state->conn, MPD_OPERATOR_DEFAULT, "") == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_add_uri");
                mpd_search_cancel(partition_state->conn);
            }
            else if (mpd_search_add_window(partition_state->conn, start, end) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_add_window");
                mpd_search_cancel(partition_state->conn);
            }
            else if (mpd_search_commit(partition_state->conn) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_commit");
                mpd_search_cancel(partition_state->conn);
            }
        }
        else {
            if (mpd_send_list_playlist_meta(partition_state->conn, playlist) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_playlist_meta");
            }
        }

        if (mympd_check_error_and_recover(partition_state) == false) {
            FREE_SDS(tag_value);
            return -1;
        }
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            sdsclear(tag_value);
            tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
            const char *uri = mpd_song_get_uri(song);
            time_t last_played = 0;
            struct t_sticker *sticker = get_sticker_from_cache(&partition_state->mpd_shared_state->sticker_cache, uri);
            if (sticker != NULL) {
                last_played = sticker->lastPlayed;
            }

            long is_uniq = JUKEBOX_UNIQ_IS_UNIQ;
            if (last_played > since) {
                //song was played too recently
                is_uniq = JUKEBOX_UNIQ_IN_QUEUE;
            }
            else if (partition_state->jukebox_enforce_unique == true) {
                is_uniq = mpd_client_jukebox_unique_tag(partition_state, uri, tag_value, manual, queue_list);
            }

            if (is_uniq == JUKEBOX_UNIQ_IS_UNIQ) {
                if (randrange(0, lineno) < add_songs) {
                    if (nkeep < add_songs) {
                        if (list_push(add_list, uri, lineno, tag_value, NULL) == true) {
                            nkeep++;
                        }
                        else {
                            MYMPD_LOG_ERROR("Can't push jukebox_queue element");
                        }
                    }
                    else {
                        long pos = add_songs > 1 ? start_length + randrange(0, add_songs - 1) : 0;
                        if (list_replace(add_list, pos, uri, lineno, tag_value, NULL) == false) {
                            MYMPD_LOG_ERROR("Can't replace jukebox_queue element pos %ld", pos);
                        }
                    }
                }
                lineno++;
            }
            else {
                skipno++;
            }
            mpd_song_free(song);
        }
        mpd_response_finish(partition_state->conn);
        if (mympd_check_error_and_recover(partition_state) == false) {
            FREE_SDS(tag_value);
            return -1;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (from_database == true && lineno + skipno > (long)start);
    FREE_SDS(tag_value);
    MYMPD_LOG_DEBUG("Jukebox iterated through %ld songs, skipped %ld", lineno, skipno);
    return (int)nkeep;
}

static long mpd_client_jukebox_unique_tag(struct t_partition_state *partition_state, const char *uri,
        const char *value, bool manual, struct t_list *queue_list)
{
    struct t_list_node *current = queue_list->head;
    while(current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        if (value != NULL &&
            strcmp(current->value_p, value) == 0)
        {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        current = current->next;
    }

    current = manual == false ? partition_state->jukebox_queue.head : partition_state->jukebox_queue_tmp.head;
    long i = 0;
    while (current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return false;
        }
        if (value != NULL &&
            strcmp(current->value_p, value) == 0)
        {
            return i;
        }
        current = current->next;
        i++;
    }
    return JUKEBOX_UNIQ_IS_UNIQ;
}

static long mpd_client_jukebox_unique_album(struct t_partition_state *partition_state, const char *album,
        const char *albumartist, bool manual, struct t_list *queue_list)
{
    struct t_list_node *current = queue_list->head;
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 &&
            strcmp(current->value_p, albumartist) == 0)
        {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        current = current->next;
    }

    current = manual == false ? partition_state->jukebox_queue.head : partition_state->jukebox_queue_tmp.head;
    long i = 0;
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 &&
            strcmp(current->value_p, albumartist) == 0)
        {
            return i;
        }
        current = current->next;
        i++;
    }
    return JUKEBOX_UNIQ_IS_UNIQ;
}
