/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <libgen.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../utility.h"
#include "../mympd_state.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_stats.h"

//private definitions
static sds mpd_client_put_last_played_obj(struct t_mympd_state *mympd_state, sds buffer, 
                                          unsigned entity_count, long last_played, const char *uri, const struct t_tags *tagcols);

//public functions
bool mpd_client_last_played_list_save(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving last_played list to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/last_played.XXXXXX", mympd_state->config->workdir);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
        sdsfree(tmp_file);
        return false;
    }    
    
    FILE *fp = fdopen(fd, "w");
    //first write last_played list to tmp file
    unsigned i = 0;
    struct list_node *current = mympd_state->last_played.head;
    while (current != NULL && i < mympd_state->last_played_count) {
        fprintf(fp, "%ld::%s\n", current->value_i, current->key);
        current = current->next;
        i++;
    }
    //append current last_played file to tmp file
    char *line = NULL;
    size_t n = 0;
    sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", mympd_state->config->workdir);
    FILE *fi = fopen(lp_file, "r");
    if (fi != NULL) {
        while (getline(&line, &n, fi) > 0 && i < mympd_state->last_played_count) {
            fputs(line, fp);
            i++;
        }
        FREE_PTR(line);
        fclose(fi);
    }
    else {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\": %s", lp_file, strerror(errno));
    }
    fclose(fp);
    
    if (rename(tmp_file, lp_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from %s to %s failed: %s", tmp_file, lp_file, strerror(errno));
        sdsfree(tmp_file);
        sdsfree(lp_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(lp_file);
    //empt list after write to disc
    list_free(&mympd_state->last_played);    
    return true;
}

bool mpd_client_add_song_to_last_played_list(struct t_mympd_state *mympd_state, const int song_id) {
    if (song_id > -1) {
        struct mpd_song *song = mpd_run_get_queue_song_id(mympd_state->mpd_state->conn, song_id);
        if (song) {
            const char *uri = mpd_song_get_uri(song);
            if (is_streamuri(uri) == true) {
                //Don't add streams to last played list
                mpd_song_free(song);
                return true;
            }
            list_insert(&mympd_state->last_played, uri, time(NULL), NULL, NULL);
            mpd_song_free(song);
            //write last_played list to disc
            if (mympd_state->last_played.length > 9 || 
                mympd_state->last_played.length > mympd_state->last_played_count)
            {
                mpd_client_last_played_list_save(mympd_state);
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

sds mpd_client_put_last_played_songs(struct t_mympd_state *mympd_state, sds buffer, sds method, 
                                     long request_id, const unsigned int offset, 
                                     const unsigned int limit, const struct t_tags *tagcols)
{
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    
    if (mympd_state->last_played.length > 0) {
        struct list_node *current = mympd_state->last_played.head;
        while (current != NULL) {
            entity_count++;
            if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = mpd_client_put_last_played_obj(mympd_state, buffer, entity_count, current->value_i, current->key, tagcols);
            }
            current = current->next;
        }
    }

    char *line = NULL;
    char *data = NULL;
    char *crap = NULL;
    size_t n = 0;
    sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", mympd_state->config->workdir);
    FILE *fp = fopen(lp_file, "r");
    if (fp != NULL) {
        while (getline(&line, &n, fp) > 0) {
            entity_count++;
            if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
                int value = strtoimax(line, &data, 10);
                if (strlen(data) > 2) {
                    data = data + 2;
                    strtok_r(data, "\n", &crap);
                    if (entities_returned++) {
                        buffer = sdscat(buffer, ",");
                    }
                    buffer = mpd_client_put_last_played_obj(mympd_state, buffer, entity_count, value, data, tagcols);
                }
                else {
                    MYMPD_LOG_ERROR("Reading last_played line failed");
                    MYMPD_LOG_DEBUG("Errorneous line: %s", line);
                }
            }
        }
        fclose(fp);
        FREE_PTR(line);
    }
    else {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\": %s", lp_file, strerror(errno));
    }
    sdsfree(lp_file);
    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    
    return buffer;
}

sds mpd_client_put_stats(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    struct mpd_stats *stats = mpd_run_stats(mympd_state->mpd_state->conn);
    if (stats == NULL) {
        buffer = check_error_and_recover(mympd_state->mpd_state, buffer, method, request_id);
        return buffer;        
    }
    
    const unsigned *version = mpd_connection_get_server_version(mympd_state->mpd_state->conn);
    sds mpd_version = sdscatfmt(sdsempty(),"%u.%u.%u", version[0], version[1], version[2]);
    sds libmpdclient_version = sdscatfmt(sdsempty(), "%i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_long(buffer, "artists", mpd_stats_get_number_of_artists(stats), true);
    buffer = tojson_long(buffer, "albums", mpd_stats_get_number_of_albums(stats), true);
    buffer = tojson_long(buffer, "songs", mpd_stats_get_number_of_songs(stats), true);
    buffer = tojson_ulong(buffer, "playtime", mpd_stats_get_play_time(stats), true);
    buffer = tojson_ulong(buffer, "uptime", mpd_stats_get_uptime(stats), true);
    buffer = tojson_long(buffer, "myMPDuptime", time(NULL) - mympd_state->config->startup_time, true);
    buffer = tojson_ulong(buffer, "dbUpdated", mpd_stats_get_db_update_time(stats), true);
    buffer = tojson_ulong(buffer, "dbPlaytime", mpd_stats_get_db_play_time(stats), true);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_char(buffer, "mpdVersion", mpd_version, true);
    sds libmympdclient_version = sdscatfmt(sdsempty(), "%i.%i.%i", LIBMYMPDCLIENT_MAJOR_VERSION, LIBMYMPDCLIENT_MINOR_VERSION, LIBMYMPDCLIENT_PATCH_VERSION);
    buffer = tojson_char(buffer, "libmympdclientVersion", libmympdclient_version, true);
    sdsfree(libmympdclient_version);
    buffer = tojson_char(buffer, "libmpdclientVersion", libmpdclient_version, false);
    buffer = jsonrpc_result_end(buffer);

    sdsfree(mpd_version);
    sdsfree(libmpdclient_version);
    mpd_stats_free(stats);

    return buffer;
}


//private functions
static sds mpd_client_put_last_played_obj(struct t_mympd_state *mympd_state, sds buffer, 
                                          unsigned entity_count, long last_played, const char *uri, const struct t_tags *tagcols)
{
    buffer = sdscat(buffer, "{");
    buffer = tojson_long(buffer, "Pos", entity_count, true);
    buffer = tojson_long(buffer, "LastPlayed", last_played, true);
    bool rc = mpd_send_list_meta(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == false) {
        buffer = put_empty_song_tags(buffer, mympd_state->mpd_state, tagcols, uri);
        mpd_response_finish(mympd_state->mpd_state->conn);
    }
    else {
        struct mpd_entity *entity;
        if ((entity = mpd_recv_entity(mympd_state->mpd_state->conn)) != NULL) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            buffer = put_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
            if (mympd_state->mpd_state->feat_stickers == true && mympd_state->sticker_cache != NULL) {
                buffer = sdscatlen(buffer, ",", 1);
                buffer = mpd_shared_sticker_list(buffer, mympd_state->sticker_cache, mpd_song_get_uri(song));
            }
            mpd_entity_free(entity);
        }
        else {
            buffer = put_empty_song_tags(buffer, mympd_state->mpd_state, tagcols, uri);
        }
        check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
        mpd_response_finish(mympd_state->mpd_state->conn);
    }
    buffer = sdscat(buffer, "}");
    return buffer;
}
