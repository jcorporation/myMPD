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
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_stats.h"

//private definitions
static sds mpd_client_put_last_played_obj(t_mpd_client_state *mpd_client_state, sds buffer, 
                                          unsigned entity_count, long last_played, const char *uri, const t_tags *tagcols);

//public functions
bool mpd_client_last_played_list_save(t_config *config, t_mpd_client_state *mpd_client_state) {
    if (config->readonly == true) {
        LOG_VERBOSE("Skip saving last_played list to disc");
        return true;
    }
    LOG_VERBOSE("Saving last_played list to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/last_played.XXXXXX", config->varlibdir);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
        sdsfree(tmp_file);
        return false;
    }    
    
    FILE *fp = fdopen(fd, "w");
    //first write last_played list to tmp file
    unsigned i = 0;
    struct list_node *current = mpd_client_state->last_played.head;
    while (current != NULL && i < mpd_client_state->last_played_count) {
        fprintf(fp, "%ld::%s\n", current->value_i, current->key);
        current = current->next;
        i++;
    }
    //append current last_played file to tmp file
    char *line = NULL;
    size_t n = 0;
    sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
    FILE *fi = fopen(lp_file, "r");
    if (fi != NULL) {
        while (getline(&line, &n, fi) > 0 && i < mpd_client_state->last_played_count) {
            fputs(line, fp);
            i++;
        }
        FREE_PTR(line);
        fclose(fi);
    }
    else {
        //ignore error
        LOG_DEBUG("Can not open file \"%s\": %s", lp_file, strerror(errno));
    }
    fclose(fp);
    
    if (rename(tmp_file, lp_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed: %s", tmp_file, lp_file, strerror(errno));
        sdsfree(tmp_file);
        sdsfree(lp_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(lp_file);
    //empt list after write to disc
    list_free(&mpd_client_state->last_played);    
    return true;
}

bool mpd_client_add_song_to_last_played_list(t_config *config, t_mpd_client_state *mpd_client_state, const int song_id) {
    if (song_id > -1) {
        struct mpd_song *song = mpd_run_get_queue_song_id(mpd_client_state->mpd_state->conn, song_id);
        if (song) {
            const char *uri = mpd_song_get_uri(song);
            if (is_streamuri(uri) == true) {
                //Don't add streams to last played list
                mpd_song_free(song);
                return true;
            }
            list_insert(&mpd_client_state->last_played, uri, time(NULL), NULL, NULL);
            mpd_song_free(song);
            //write last_played list to disc
            if (config->readonly == false) {
                if (mpd_client_state->last_played.length > 9 || mpd_client_state->last_played.length > mpd_client_state->last_played_count) {
                    mpd_client_last_played_list_save(config, mpd_client_state);
                }
            }
            else if (mpd_client_state->last_played.length > mpd_client_state->last_played_count) {
                //remove last entry
                list_shift(&mpd_client_state->last_played, mpd_client_state->last_played.length - 1);
            }
            //notify clients
            send_jsonrpc_event("update_lastplayed");
        }
        else {
            LOG_ERROR("Can't get song from id %d", song_id);
            return false;
        }
        if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
            return false;
        }
    }
    return true;
}

sds mpd_client_put_last_played_songs(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                                     const unsigned int offset, const unsigned int limit, const t_tags *tagcols)
{
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    
    if (mpd_client_state->last_played.length > 0) {
        struct list_node *current = mpd_client_state->last_played.head;
        while (current != NULL) {
            entity_count++;
            if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = mpd_client_put_last_played_obj(mpd_client_state, buffer, entity_count, current->value_i, current->key, tagcols);
            }
            current = current->next;
        }
    }

    if (config->readonly == false) {
        char *line = NULL;
        char *data = NULL;
        char *crap = NULL;
        size_t n = 0;
        sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
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
                        buffer = mpd_client_put_last_played_obj(mpd_client_state, buffer, entity_count, value, data, tagcols);
                    }
                    else {
                        LOG_ERROR("Reading last_played line failed");
                        LOG_DEBUG("Errorneous line: %s", line);
                    }
                }
            }
            fclose(fp);
            FREE_PTR(line);
        }
        else {
            //ignore error
            LOG_DEBUG("Can not open file \"%s\": %s", lp_file, strerror(errno));
        }
        sdsfree(lp_file);
    }
    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    
    return buffer;
}

sds mpd_client_put_stats(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
    struct mpd_stats *stats = mpd_run_stats(mpd_client_state->mpd_state->conn);
    if (stats == NULL) {
        buffer = check_error_and_recover(mpd_client_state->mpd_state, buffer, method, request_id);
        return buffer;        
    }
    
    const unsigned *version = mpd_connection_get_server_version(mpd_client_state->mpd_state->conn);
    sds mpd_version = sdscatfmt(sdsempty(),"%u.%u.%u", version[0], version[1], version[2]);
    sds libmpdclient_version = sdscatfmt(sdsempty(), "%i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_long(buffer, "artists", mpd_stats_get_number_of_artists(stats), true);
    buffer = tojson_long(buffer, "albums", mpd_stats_get_number_of_albums(stats), true);
    buffer = tojson_long(buffer, "songs", mpd_stats_get_number_of_songs(stats), true);
    buffer = tojson_ulong(buffer, "playtime", mpd_stats_get_play_time(stats), true);
    buffer = tojson_ulong(buffer, "uptime", mpd_stats_get_uptime(stats), true);
    buffer = tojson_long(buffer, "myMPDuptime", time(NULL) - config->startup_time, true);
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
static sds mpd_client_put_last_played_obj(t_mpd_client_state *mpd_client_state, sds buffer, 
                                          unsigned entity_count, long last_played, const char *uri, const t_tags *tagcols)
{
    buffer = sdscat(buffer, "{");
    buffer = tojson_long(buffer, "Pos", entity_count, true);
    buffer = tojson_long(buffer, "LastPlayed", last_played, true);
    bool rc = mpd_send_list_meta(mpd_client_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == false) {
        buffer = put_empty_song_tags(buffer, mpd_client_state->mpd_state, tagcols, uri);
    }
    else {
        struct mpd_entity *entity;
        if ((entity = mpd_recv_entity(mpd_client_state->mpd_state->conn)) != NULL) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            buffer = put_song_tags(buffer, mpd_client_state->mpd_state, tagcols, song);
            mpd_entity_free(entity);
            mpd_response_finish(mpd_client_state->mpd_state->conn);
        }
        else {
            buffer = put_empty_song_tags(buffer, mpd_client_state->mpd_state, tagcols, uri);
        }
    }
    buffer = sdscat(buffer, "}");
    return buffer;
}
