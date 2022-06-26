/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_jukebox.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mympd_configuration.h"
#include "../lib/random.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mympd_api/mympd_api_queue.h"
#include "../mympd_api/mympd_api_utility.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//private definitions
static bool _mpd_client_jukebox(struct t_mympd_state *mympd_state);
static struct t_list *mpd_client_jukebox_get_last_played(struct t_mympd_state *mympd_state,
        enum jukebox_modes jukebox_mode);
static bool mpd_client_jukebox_fill_jukebox_queue(struct t_mympd_state *mympd_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool _mpd_client_jukebox_fill_jukebox_queue(struct t_mympd_state *mympd_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static long mpd_client_jukebox_unique_tag(struct t_mympd_state *mympd_state, const char *uri,
        const char *value, bool manual, struct t_list *queue_list);
static long mpd_client_jukebox_unique_album(struct t_mympd_state *mympd_state, const char *album,
        const char *albumartist, bool manual, struct t_list *queue_list);
static bool add_album_to_queue(struct t_mympd_state *mympd_state, struct mpd_song *album);
static long _fill_jukebox_queue_songs(struct t_mympd_state *mympd_state, long add_songs,
        const char *playlist, bool manual, struct t_list *queue_list, struct t_list *add_list);
static long _fill_jukebox_queue_albums(struct t_mympd_state *mympd_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list);

enum jukebox_uniq_result {
    JUKEBOX_UNIQ_IN_QUEUE = -2,
    JUKEBOX_UNIQ_IS_UNIQ = -1
};

//public functions
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

sds mpd_client_get_jukebox_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
        const long offset, const long limit, sds searchstr, const struct t_tags *tagcols)
{
    long entity_count = 0;
    long entities_returned = 0;
    long real_limit = offset + limit;

    sds_utf8_tolower(searchstr);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    if (mympd_state->jukebox_mode == JUKEBOX_ADD_SONG) {
        struct t_list_node *current = mympd_state->jukebox_queue.head;
        while (current != NULL) {
            bool rc = mpd_send_list_meta(mympd_state->mpd_state->conn, current->key);
            if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == true) {
                struct mpd_song *song;
                if ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
                    if (filter_mpd_song(song, searchstr, tagcols) == true) {
                        if (entity_count >= offset &&
                            entity_count < real_limit)
                        {
                            if (entities_returned++) {
                                buffer = sdscatlen(buffer, ",", 1);
                            }
                            buffer = sdscatlen(buffer, "{", 1);
                            buffer = tojson_long(buffer, "Pos", entity_count, true);
                            buffer = get_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
                            if (mympd_state->mpd_state->feat_mpd_stickers == true &&
                                mympd_state->sticker_cache != NULL)
                            {
                                buffer = sdscatlen(buffer, ",", 1);
                                buffer = mpd_shared_sticker_list(buffer, mympd_state->sticker_cache, mpd_song_get_uri(song));
                            }
                            buffer = sdscatlen(buffer, "}", 1);
                        }
                        entity_count++;
                    }
                    mpd_song_free(song);
                }
            }
            mpd_response_finish(mympd_state->mpd_state->conn);
            check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false);
            current = current->next;
        }
    }
    else if (mympd_state->jukebox_mode == JUKEBOX_ADD_ALBUM) {
        struct t_list_node *current = mympd_state->jukebox_queue.head;
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
    buffer = tojson_long(buffer, "jukeboxMode", mympd_state->jukebox_mode, true);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);

    return buffer;
}

bool mpd_client_jukebox(struct t_mympd_state *mympd_state) {
    for (int i = 1; i < 3; i++) {
         if (_mpd_client_jukebox(mympd_state) == true) {
             return true;
         }
         if (mympd_state->jukebox_mode == JUKEBOX_OFF) {
             return false;
         }
         MYMPD_LOG_ERROR("Jukebox: trying again, attempt %d", i);
    }
    return false;
}

static bool _mpd_client_jukebox(struct t_mympd_state *mympd_state) {
    struct mpd_status *status = mpd_run_status(mympd_state->mpd_state->conn);
    if (status == NULL) {
        check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
        return false;
    }
    long queue_length = (long)mpd_status_get_queue_length(status);
    mpd_status_free(status);

    time_t now = time(NULL);
    time_t add_time = mympd_state->mpd_state->song_end_time - (mympd_state->mpd_state->crossfade + 10);

    MYMPD_LOG_DEBUG("Queue length: %ld", queue_length);
    MYMPD_LOG_DEBUG("Min queue length: %ld", mympd_state->jukebox_queue_length);

    if (queue_length >= mympd_state->jukebox_queue_length && now < add_time) {
        MYMPD_LOG_DEBUG("Jukebox: Queue length >= %ld and add_time not reached", mympd_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    long add_songs = mympd_state->jukebox_queue_length > queue_length ? mympd_state->jukebox_queue_length - queue_length : 0;

    if (now > add_time &&
        add_time > 0 &&
        queue_length <= mympd_state->jukebox_queue_length)
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

    if (mympd_state->mpd_state->feat_mpd_playlists == false && strcmp(mympd_state->jukebox_playlist, "Database") != 0) {
        MYMPD_LOG_WARN("Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = mpd_client_jukebox_add_to_queue(mympd_state, add_songs, mympd_state->jukebox_mode, mympd_state->jukebox_playlist, false);

    //update playback state
    mympd_api_queue_status(mympd_state, NULL);
    if (mympd_state->mpd_state->state != MPD_STATE_PLAY) {
        MYMPD_LOG_DEBUG("Jukebox: start playback");
        rc = mpd_run_play(mympd_state->mpd_state->conn);
        check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_play");    
    }

    if (rc == true) {
        //notify clients
        send_jsonrpc_event("update_jukebox");
        return true;
    }

    MYMPD_LOG_DEBUG("Jukebox mode: %d", mympd_state->jukebox_mode);
    MYMPD_LOG_ERROR("Jukebox: Error adding song(s)");
    return false;
}

bool mpd_client_jukebox_add_to_queue(struct t_mympd_state *mympd_state, long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    if (manual == false) {
        MYMPD_LOG_DEBUG("Jukebox queue length: %ld", mympd_state->jukebox_queue.length);
    }
    if ((manual == false && add_songs > mympd_state->jukebox_queue.length) ||
        (manual == true))
    {
        bool rc = mpd_client_jukebox_fill_jukebox_queue(mympd_state, add_songs, jukebox_mode, playlist, manual);
        if (rc == false) {
            return false;
        }
    }
    long added = 0;
    struct t_list_node *current;
    if (manual == false) {
        current = mympd_state->jukebox_queue.head;
    }
    else {
        current = mympd_state->jukebox_queue_tmp.head;
    }
    while (current != NULL &&
        added < add_songs)
    {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	        bool rc = mpd_run_add(mympd_state->mpd_state->conn, current->key);
            if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_add") == true) {
	            MYMPD_LOG_NOTICE("Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("Jukebox adding song %s failed", current->key);
            }
        }
        else {
            bool rc = add_album_to_queue(mympd_state, (struct mpd_song *)current->user_data);
            if (rc == true) {
                MYMPD_LOG_NOTICE("Jukebox adding album: %s - %s", current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("Jukebox adding album %s - %s failed", current->value_p, current->key);
            }
        }
        if (manual == false) {
            mympd_state->jukebox_queue.head->user_data = NULL;
            mpd_client_rm_jukebox_entry(&mympd_state->jukebox_queue, 0);
            current = mympd_state->jukebox_queue.head;
        }
        else {
            mympd_state->jukebox_queue_tmp.head->user_data = NULL;
            mpd_client_rm_jukebox_entry(&mympd_state->jukebox_queue_tmp, 0);
            current = mympd_state->jukebox_queue_tmp.head;
        }
    }
    if (added == 0) {
        MYMPD_LOG_ERROR("Error adding song(s)");
        send_jsonrpc_notify("jukebox", "error", "Addings songs from jukebox to queue failed");
        return false;
    }
    if (manual == false) {
        if ((jukebox_mode == JUKEBOX_ADD_SONG && mympd_state->jukebox_queue.length < 25) ||
            (jukebox_mode == JUKEBOX_ADD_ALBUM && mympd_state->jukebox_queue.length < 5))
        {
            bool rc = mpd_client_jukebox_fill_jukebox_queue(mympd_state, add_songs, jukebox_mode, playlist, manual);
            if (rc == false) {
                return false;
            }
        }
        MYMPD_LOG_DEBUG("Jukebox queue length: %ld", mympd_state->jukebox_queue.length);
    }
    return true;
}

//private functions
static bool add_album_to_queue(struct t_mympd_state *mympd_state, struct mpd_song *album) {
    bool rc = mpd_search_add_db_songs(mympd_state->mpd_state->conn, true);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_db_songs") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        return false;
    }

    const char *value = NULL;
    unsigned i = 0;
    sds expression = sdsnewlen("(", 1);
    while ((value = mpd_song_get_tag(album, mympd_state->mpd_state->tag_albumartist, i)) != NULL) {
        expression = escape_mpd_search_expression(expression, mpd_tag_name(mympd_state->mpd_state->tag_albumartist), "==", value);
        expression = sdscat(expression, " AND ");
        i++;
    }
    expression = escape_mpd_search_expression(expression, "Album", "==", mpd_song_get_tag(album, MPD_TAG_ALBUM, 0));
    expression = sdscatlen(expression, ")", 1);

    rc = mpd_search_add_expression(mympd_state->mpd_state->conn, expression);
    FREE_SDS(expression);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_expression") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        return false;
    }
    rc = mpd_search_commit(mympd_state->mpd_state->conn);
    mpd_response_finish(mympd_state->mpd_state->conn);
    return check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit");
}

static struct t_list *mpd_client_jukebox_get_last_played(struct t_mympd_state *mympd_state, enum jukebox_modes jukebox_mode) {
    struct mpd_song *song;
    struct t_list *queue_list = list_new();

    bool rc = mpd_send_list_queue_meta(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_queue_meta") == false) {
        FREE_PTR(queue_list);
        return NULL;
    }
    while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            const char *tag_value = NULL;
            if (mympd_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                tag_value = mpd_song_get_tag(song, mympd_state->jukebox_unique_tag.tags[0], 0);
            }
            list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
        }
        else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
            sds album = mpd_shared_get_tag_values(song, MPD_TAG_ALBUM, sdsempty());
            sds albumartist = mpd_shared_get_tag_values(song, mympd_state->mpd_state->tag_albumartist, sdsempty());
            list_push(queue_list, album, 0, albumartist, NULL);
            FREE_SDS(album);
            FREE_SDS(albumartist);
        }
        mpd_song_free(song);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false);

    //append last_played to queue list
    struct t_list_node *current = mympd_state->last_played.head;
    while (current != NULL) {
        rc = mpd_send_list_meta(mympd_state->mpd_state->conn, current->key);
        if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == true) {
            song = mpd_recv_song(mympd_state->mpd_state->conn);
            if (song != NULL) {
                if (jukebox_mode == JUKEBOX_ADD_SONG) {
                    list_push(queue_list, current->key, 0, mpd_song_get_tag(song, mympd_state->jukebox_unique_tag.tags[0], 0), NULL);
                }
                else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                    sds album = mpd_shared_get_tag_values(song, MPD_TAG_ALBUM, sdsempty());
                    sds albumartist = mpd_shared_get_tag_values(song, mympd_state->mpd_state->tag_albumartist, sdsempty());
                    list_push(queue_list, album, 0, albumartist, NULL);
                    FREE_SDS(album);
                    FREE_SDS(albumartist);
                }
                mpd_song_free(song);
            }
        }
        mpd_response_finish(mympd_state->mpd_state->conn);
        check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false);
        current = current->next;
    }
    //get last_played from disc
    if (queue_list->length < 20) {
        sds line = sdsempty();
        char *data = NULL;
        sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", mympd_state->config->workdir);
        errno = 0;
        FILE *fp = fopen(lp_file, OPEN_FLAGS_READ);
        if (fp != NULL) {
            while (sds_getline(&line, fp, 1000) == 0 && queue_list->length < 20) {
                int value = (int)strtoimax(line, &data, 10);
                if (value > 0 && strlen(data) > 2) {
                    data = data + 2;
                    rc = mpd_send_list_meta(mympd_state->mpd_state->conn, data);
                    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == true) {
                        song = mpd_recv_song(mympd_state->mpd_state->conn);
                        if (song != NULL) {
                            if (jukebox_mode == JUKEBOX_ADD_SONG) {
                                list_push(queue_list, data, 0, mpd_song_get_tag(song, mympd_state->jukebox_unique_tag.tags[0], 0), NULL);
                            }
                            else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                                sds album = mpd_shared_get_tag_values(song, MPD_TAG_ALBUM, sdsempty());
                                sds albumartist = mpd_shared_get_tag_values(song, mympd_state->mpd_state->tag_albumartist, sdsempty());
                                list_push(queue_list, album, 0, albumartist, NULL);
                                FREE_SDS(album);
                                FREE_SDS(albumartist);
                            }
                            mpd_song_free(song);
                        }
                    }
                    mpd_response_finish(mympd_state->mpd_state->conn);
                    check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false);
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
    MYMPD_LOG_DEBUG("Jukebox last_played list length: %ld", queue_list->length);
    return queue_list;
}

static bool mpd_client_jukebox_fill_jukebox_queue(struct t_mympd_state *mympd_state,
    long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    send_jsonrpc_notify("jukebox", "info", "Filling jukebox queue");
    MYMPD_LOG_DEBUG("Jukebox queue to small, adding entities");
    if (mympd_state->mpd_state->feat_mpd_tags == true) {
        if (mympd_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            enable_mpd_tags(mympd_state->mpd_state, &mympd_state->jukebox_unique_tag);
        }
        else {
            disable_all_mpd_tags(mympd_state->mpd_state);
        }
    }
    bool rc = _mpd_client_jukebox_fill_jukebox_queue(mympd_state, add_songs, jukebox_mode, playlist, manual);
    if (mympd_state->mpd_state->feat_mpd_tags == true) {
        enable_mpd_tags(mympd_state->mpd_state, &mympd_state->mpd_state->tag_types_mympd);
    }

    if (rc == false) {
        MYMPD_LOG_ERROR("Filling jukebox queue failed, disabling jukebox");
        send_jsonrpc_notify("jukebox", "error", "Filling jukebox queue failed, disabling jukebox");
        mympd_state->jukebox_mode = JUKEBOX_OFF;
        return false;
    }
    return true;
}

static bool _mpd_client_jukebox_fill_jukebox_queue(struct t_mympd_state *mympd_state,
    long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    long added = 0;

    if (manual == true) {
        mpd_client_clear_jukebox(&mympd_state->jukebox_queue_tmp);
    }

    //get last_played and current queue
    struct t_list *queue_list = mpd_client_jukebox_get_last_played(mympd_state, jukebox_mode);
    if (queue_list == NULL) {
        return false;
    }

    struct t_list *add_list = manual == false ? &mympd_state->jukebox_queue : &mympd_state->jukebox_queue_tmp;

    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        added = _fill_jukebox_queue_songs(mympd_state, add_songs, playlist, manual, queue_list, add_list);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        added = _fill_jukebox_queue_albums(mympd_state, add_songs, manual, queue_list, add_list);
    }

    if (added < add_songs) {
        MYMPD_LOG_WARN("Jukebox queue didn't contain %ld entries", add_songs);
        if (mympd_state->jukebox_enforce_unique == true) {
            MYMPD_LOG_WARN("Disabling jukebox unique constraints temporarily");
            mympd_state->jukebox_enforce_unique = false;
            send_jsonrpc_notify("jukebox", "warn", "Playlist to small, disabling jukebox unique constraints temporarily");
        }
    }

    list_clear(queue_list);
    FREE_PTR(queue_list);
    return true;
}

static long _fill_jukebox_queue_albums(struct t_mympd_state *mympd_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list)
{
    if (mympd_state->album_cache == NULL) {
        MYMPD_LOG_WARN("Album cache is null, jukebox can not add albums");
        return -1;
    }

    long start_length = 0;
    if (manual == false) {
        start_length = mympd_state->jukebox_queue.length;
        add_albums = 10 - mympd_state->jukebox_queue.length;
        if (add_albums <= 0) {
            return 0;
        }
    }

    long skipno = 0;
    long nkeep = 0;
    long lineno = 1;
    raxIterator iter;
    raxStart(&iter, mympd_state->album_cache);
    raxSeek(&iter, "^", NULL, 0);
    sds album = sdsempty();
    sds albumartist = sdsempty();
    while (raxNext(&iter)) {
        struct mpd_song *song = (struct mpd_song *)iter.data;
        album = mpd_shared_get_tag_values(song, MPD_TAG_ALBUM, sdsempty());
        albumartist = mpd_shared_get_tag_values(song, mympd_state->mpd_state->tag_albumartist, sdsempty());
        long is_uniq = JUKEBOX_UNIQ_IS_UNIQ;
        if (mympd_state->jukebox_enforce_unique == true) {
            is_uniq = mpd_client_jukebox_unique_album(mympd_state, album, albumartist, manual, queue_list);
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

static long _fill_jukebox_queue_songs(struct t_mympd_state *mympd_state, long add_songs, const char *playlist,
        bool manual, struct t_list *queue_list, struct t_list *add_list)
{
    unsigned start = 0;
    unsigned end = start + MPD_RESULTS_MAX;
    long skipno = 0;
    long nkeep = 0;
    long lineno = 1;
    time_t since = time(NULL);
    since = since - (mympd_state->jukebox_last_played * 3600);

    if (mympd_state->sticker_cache == NULL) {
        MYMPD_LOG_WARN("Sticker cache is null, jukebox doesn't respect last played constraint");
    }

    long start_length = 0;
    if (manual == false) {
        start_length = mympd_state->jukebox_queue.length;
        add_songs = (long)50 - start_length;
        if (add_songs <= 0) {
            return 0;
        }
    }
    bool from_database = strcmp(playlist, "Database") == 0 ? true : false;
    do {
        MYMPD_LOG_DEBUG("Jukebox: iterating through source, start: %u", start);

        if (from_database == true) {
            if (mpd_search_db_songs(mympd_state->mpd_state->conn, false) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_db_songs");
            }
            else if (mpd_search_add_uri_constraint(mympd_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, "") == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_add_uri");
                mpd_search_cancel(mympd_state->mpd_state->conn);
            }
            else if (mpd_search_add_window(mympd_state->mpd_state->conn, start, end) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_add_window");
                mpd_search_cancel(mympd_state->mpd_state->conn);
            }
            else if (mpd_search_commit(mympd_state->mpd_state->conn) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_search_commit");
                mpd_search_cancel(mympd_state->mpd_state->conn);
            }
        }
        else {
            if (mpd_send_list_playlist_meta(mympd_state->mpd_state->conn, playlist) == false) {
                MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_playlist_meta");
            }
        }

        if (check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false) == false) {
            return -1;
        }
        struct mpd_song *song;
        while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
            const char *tag_value = mpd_song_get_tag(song, mympd_state->jukebox_unique_tag.tags[0], 0);
            const char *uri = mpd_song_get_uri(song);
            time_t last_played = 0;
            if (mympd_state->sticker_cache != NULL) {
                struct t_sticker *sticker = get_sticker_from_cache(mympd_state->sticker_cache, uri);
                if (sticker != NULL) {
                    last_played = sticker->lastPlayed;
                }
            }

            long is_uniq = JUKEBOX_UNIQ_IS_UNIQ;
            if (last_played > since) {
                //song was played too recently
                is_uniq = JUKEBOX_UNIQ_IN_QUEUE;
            }
            else if (mympd_state->jukebox_enforce_unique == true) {
                is_uniq = mpd_client_jukebox_unique_tag(mympd_state, uri, tag_value, manual, queue_list);
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
        mpd_response_finish(mympd_state->mpd_state->conn);
        if (check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false) == false) {
            return -1;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (from_database == true && lineno + skipno > (long)start);
    MYMPD_LOG_DEBUG("Jukebox iterated through %ld songs, skipped %ld", lineno, skipno);
    return (int)nkeep;
}

static long mpd_client_jukebox_unique_tag(struct t_mympd_state *mympd_state, const char *uri,
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

    current = manual == false ? mympd_state->jukebox_queue.head : mympd_state->jukebox_queue_tmp.head;
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

static long mpd_client_jukebox_unique_album(struct t_mympd_state *mympd_state, const char *album,
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

    current = manual == false ? mympd_state->jukebox_queue.head : mympd_state->jukebox_queue_tmp.head;
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
