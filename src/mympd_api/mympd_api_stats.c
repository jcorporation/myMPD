/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_stats.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared/mpd_shared_tags.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private definitions
static sds mympd_api_get_last_played_obj(struct t_mympd_state *mympd_state, sds buffer, long entity_count,
                                         long long last_played, const char *uri, sds searchstr, const struct t_tags *tagcols);

//public functions
bool mympd_api_stats_last_played_file_save(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving last_played list to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/last_played.XXXXXX", mympd_state->config->workdir);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }

    int i = 0;
    struct t_list_node *current;
    bool write_rc = true;
    while ((current = list_shift_first(&mympd_state->last_played)) != NULL) {
        if (fprintf(fp, "%lld::%s\n", current->value_i, current->key) < 0) {
            MYMPD_LOG_ERROR("Could not write last played songs to disc");
            write_rc = false;
            list_node_free(current);
            break;
        }
        i++;
        list_node_free(current);
    }
    //append current last_played file to tmp file
    sds filepath = sdscatfmt(sdsempty(), "%s/state/last_played", mympd_state->config->workdir);
    if (write_rc == true) {
        errno = 0;
        FILE *fi = fopen(filepath, OPEN_FLAGS_READ);
        if (fi != NULL) {
            sds line = sdsempty();
            while (sds_getline_n(&line, fi, 1000) == 0 &&
                i < mympd_state->last_played_count)
            {
                if (fputs(line, fp) == EOF) {
                    MYMPD_LOG_ERROR("Could not write last played songs to disc");
                    write_rc = false;
                    break;
                }
                i++;
            }
            (void) fclose(fi);
            FREE_SDS(line);
        }
        else {
            //ignore error
            MYMPD_LOG_DEBUG("Can not open file \"%s\"", filepath);
            if (errno != ENOENT) {
                MYMPD_LOG_ERRNO(errno);
            }
        }
    }

    bool rc = rename_tmp_file(fp, tmp_file, filepath, write_rc);
    FREE_SDS(tmp_file);
    FREE_SDS(filepath);
    return rc;
}

bool mympd_api_stats_last_played_add_song(struct t_mympd_state *mympd_state, const int song_id) {
    if (song_id > -1) {
        struct mpd_song *song = mpd_run_get_queue_song_id(mympd_state->mpd_state->conn, (unsigned)song_id);
        if (song) {
            const char *uri = mpd_song_get_uri(song);
            if (is_streamuri(uri) == true) {
                //Don't add streams to last played list
                mpd_song_free(song);
                return true;
            }
            list_insert(&mympd_state->last_played, uri, (long)time(NULL), NULL, NULL);
            mpd_song_free(song);
            //write last_played list to disc
            if (mympd_state->last_played.length > 9 ||
                mympd_state->last_played.length > mympd_state->last_played_count)
            {
                mympd_api_stats_last_played_file_save(mympd_state);
            }
            //notify clients
            send_jsonrpc_event("update_lastplayed");
        }
        else {
            MYMPD_LOG_ERROR("Can't get song from id %d", song_id);
            return false;
        }
        if (check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false) == false) {
            return false;
        }
    }
    return true;
}

sds mympd_api_stats_last_played_list(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                     long request_id, const long offset, const long limit,
                                     sds searchstr, const struct t_tags *tagcols)
{
    long entity_count = 0;
    long entities_returned = 0;
    sds_utf8_tolower(searchstr);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds obj = sdsempty();

    long real_limit = offset + limit;

    if (mympd_state->last_played.length > 0) {
        struct t_list_node *current = mympd_state->last_played.head;
        while (current != NULL) {
            obj = mympd_api_get_last_played_obj(mympd_state, obj, entity_count, current->value_i, current->key, searchstr, tagcols);
            if (sdslen(obj) > 0) {
                entity_count++;
                if (entity_count > offset && entity_count <= real_limit) {
                    if (entities_returned++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    buffer = sdscatsds(buffer, obj);
                    sdsclear(obj);
                }
            }
            current = current->next;
        }
    }

    sds line = sdsempty();
    sdsclear(obj);
    char *data = NULL;
    sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", mympd_state->config->workdir);
    errno = 0;
    FILE *fp = fopen(lp_file, OPEN_FLAGS_READ);
    if (fp != NULL) {
        while (sds_getline(&line, fp, 1000) == 0) {
            int value = (int)strtoimax(line, &data, 10);
            if (strlen(data) > 2) {
                data = data + 2;
                obj = mympd_api_get_last_played_obj(mympd_state, obj, entity_count, value, data, searchstr, tagcols);
                if (sdslen(obj) > 0) {
                    entity_count++;
                    if (entity_count > offset && entity_count <= real_limit) {
                        if (entities_returned++) {
                            buffer = sdscatlen(buffer, ",", 1);
                        }
                        buffer = sdscatsds(buffer, obj);
                        sdsclear(obj);
                    }
                }
            }
            else {
                MYMPD_LOG_ERROR("Reading last_played line failed");
                MYMPD_LOG_DEBUG("Errorneous line: %s", line);
            }
        }
        (void) fclose(fp);
        FREE_SDS(line);
    }
    else {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", lp_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
    }
    FREE_SDS(lp_file);
    FREE_SDS(obj);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);

    return buffer;
}

sds mympd_api_stats_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    struct mpd_stats *stats = mpd_run_stats(mympd_state->mpd_state->conn);
    if (stats == NULL) {
        buffer = check_error_and_recover(mympd_state->mpd_state, buffer, method, request_id);
        return buffer;
    }

    const unsigned *version = mpd_connection_get_server_version(mympd_state->mpd_state->conn);
    sds mpd_protocol_version = sdscatfmt(sdsempty(),"%u.%u.%u", version[0], version[1], version[2]);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_uint(buffer, "artists", mpd_stats_get_number_of_artists(stats), true);
    buffer = tojson_uint(buffer, "albums", mpd_stats_get_number_of_albums(stats), true);
    buffer = tojson_uint(buffer, "songs", mpd_stats_get_number_of_songs(stats), true);
    buffer = tojson_ulong(buffer, "playtime", mpd_stats_get_play_time(stats), true);
    buffer = tojson_ulong(buffer, "uptime", mpd_stats_get_uptime(stats), true);
    buffer = tojson_llong(buffer, "myMPDuptime", (long long)(time(NULL) - mympd_state->config->startup_time), true);
    buffer = tojson_ulong(buffer, "dbUpdated", mpd_stats_get_db_update_time(stats), true);
    buffer = tojson_ulong(buffer, "dbPlaytime", mpd_stats_get_db_play_time(stats), true);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_char(buffer, "mpdProtocolVersion", mpd_protocol_version, false);
    buffer = jsonrpc_result_end(buffer);

    FREE_SDS(mpd_protocol_version);
    mpd_stats_free(stats);

    return buffer;
}

//private functions
static sds mympd_api_get_last_played_obj(struct t_mympd_state *mympd_state, sds buffer, long entity_count,
                                         long long last_played, const char *uri, sds searchstr, const struct t_tags *tagcols)
{
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_long(buffer, "Pos", entity_count, true);
    buffer = tojson_llong(buffer, "LastPlayed", last_played, true);
    bool rc = mpd_send_list_meta(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == false) {
        mpd_response_finish(mympd_state->mpd_state->conn);
        rc = false;
    }
    else {
        struct mpd_entity *entity;
        if ((entity = mpd_recv_entity(mympd_state->mpd_state->conn)) != NULL) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            if (filter_mpd_song(song, searchstr, tagcols) == true) {
                buffer = get_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
                if (mympd_state->mpd_state->feat_mpd_stickers == true &&
                    mympd_state->sticker_cache != NULL)
                {
                    buffer = sdscatlen(buffer, ",", 1);
                    buffer = mpd_shared_sticker_list(buffer, mympd_state->sticker_cache, mpd_song_get_uri(song));
                }
                rc = true;
            }
            else {
                rc = false;
            }
            mpd_entity_free(entity);
        }
        else {
            rc = false;
        }
        check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
        mpd_response_finish(mympd_state->mpd_state->conn);
    }
    buffer = sdscatlen(buffer, "}", 1);
    if (rc == false) {
        sdsclear(buffer);
    }
    return buffer;
}
