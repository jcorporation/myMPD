/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <libgen.h>
#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_stats.h"

//private definitions
static sds mpd_client_put_last_played_obj(t_mpd_state *mpd_state, sds buffer, 
                                          unsigned entity_count, int last_played, const char *uri, const t_tags *tagcols);

//public functions
bool mpd_client_last_played_list_save(t_config *config, t_mpd_state *mpd_state) {
    if (config->readonly == true) {
        LOG_VERBOSE("Skip saving last_played list to disc");
        return true;
    }
    LOG_VERBOSE("Saving last_played list to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/last_played.XXXXXX", config->varlibdir);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }    
    
    FILE *fp = fdopen(fd, "w");
    //first write last_played list to tmp file
    int i = 0;
    struct list_node *current = mpd_state->last_played.head;
    while (current != NULL && i < mpd_state->last_played_count) {
        fprintf(fp, "%ld::%s\n", current->value_i, current->key);
        current = current->next;
        i++;
    }
    //append current last_played file to tmp file
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    sds cfg_file = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
    FILE *fi = fopen(cfg_file, "r");
    if (fi != NULL) {
        while ((read = getline(&line, &n, fi)) > 0 && i < mpd_state->last_played_count) {
            fputs(line, fp);
            i++;
        }
        FREE_PTR(line);
        fclose(fi);
    }
    fclose(fp);
    
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    //empt list after write to disc
    list_free(&mpd_state->last_played);    
    return true;
}

bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id) {
    if (song_id > -1) {
        struct mpd_song *song = mpd_run_get_queue_song_id(mpd_state->conn, song_id);
        if (song) {
            const char *uri = mpd_song_get_uri(song);
            if (uri == NULL || strstr(uri, "://") != NULL) {
                //Don't add streams to last played list
                mpd_state->last_last_played_id = song_id;
                mpd_song_free(song);
                return true;
            }
            else {
                list_insert(&mpd_state->last_played, uri, time(NULL), NULL, NULL);
            }
            mpd_state->last_last_played_id = song_id;
            mpd_song_free(song);
            //write last_played list to disc
            if (config->readonly == false) {
                if (mpd_state->last_played.length > 9 || mpd_state->last_played.length > mpd_state->last_played_count) {
                    mpd_client_last_played_list_save(config, mpd_state);
                }
            }
            else if (mpd_state->last_played.length > mpd_state->last_played_count) {
                //remove last entry
                list_shift(&mpd_state->last_played, mpd_state->last_played.length - 1);
            }
            //notify clients
            sds buffer = jsonrpc_notify(sdsempty(), "update_lastplayed");
            mpd_client_notify(buffer);
            sdsfree(buffer);
        } else {
            LOG_ERROR("Can't get song from id %d", song_id);
            return false;
        }
    }
    return true;
}

sds mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                                     unsigned int offset, const t_tags *tagcols)
{
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    
    if (mpd_state->last_played.length > 0) {
        struct list_node *current = mpd_state->last_played.head;
        while (current != NULL) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = mpd_client_put_last_played_obj(mpd_state, buffer, entity_count, current->value_i, current->key, tagcols);
            }
            current = current->next;
        }
    }

    if (config->readonly == false) {
        char *line = NULL;
        char *data = NULL;
        char *crap = NULL;
        size_t n = 0;
        ssize_t read;
        sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
        FILE *fp = fopen(lp_file, "r");
        sdsfree(lp_file);
        if (fp != NULL) {
            while ((read = getline(&line, &n, fp)) > 0) {
                entity_count++;
                if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                    int value = strtoimax(line, &data, 10);
                    if (strlen(data) > 2) {
                        data = data + 2;
                        strtok_r(data, "\n", &crap);
                        if (entities_returned++) {
                            buffer = sdscat(buffer, ",");
                        }
                        buffer = mpd_client_put_last_played_obj(mpd_state, buffer, entity_count, value, data, tagcols);
                    }
                    else {
                        LOG_ERROR("Reading last_played line failed");
                    }
                }
            }
            fclose(fp);
            FREE_PTR(line);
        }
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end_result(buffer);
    
    return buffer;
}

sds mpd_client_put_stats(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    if (stats == NULL) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;        
    }
    
    const unsigned *version = mpd_connection_get_server_version(mpd_state->conn);
    sds mpd_version = sdscatfmt(sdsempty(),"%u.%u.%u", version[0], version[1], version[2]);
    sds libmpdclient_version = sdscatfmt(sdsempty(), "%i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    buffer = tojson_long(buffer, "artists", mpd_stats_get_number_of_artists(stats), true);
    buffer = tojson_long(buffer, "albums", mpd_stats_get_number_of_albums(stats), true);
    buffer = tojson_long(buffer, "songs", mpd_stats_get_number_of_songs(stats), true);
    buffer = tojson_long(buffer, "playtime", mpd_stats_get_play_time(stats), true);
    buffer = tojson_long(buffer, "uptime", mpd_stats_get_uptime(stats), true);
    buffer = tojson_long(buffer, "myMPDuptime", time(NULL) - config->startup_time, true);
    buffer = tojson_long(buffer, "dbUpdated", mpd_stats_get_db_update_time(stats), true);
    buffer = tojson_long(buffer, "dbPlaytime", mpd_stats_get_db_play_time(stats), true);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_char(buffer, "mpdVersion", mpd_version, true);
    #ifdef EMBEDDED_LIBMPDCLIENT
        sds libmympdclient_version = sdscatfmt(sdsempty(), "%i.%i.%i", LIBMYMPDCLIENT_MAJOR_VERSION, LIBMYMPDCLIENT_MINOR_VERSION, LIBMYMPDCLIENT_PATCH_VERSION);
        buffer = tojson_char(buffer, "libmympdclientVersion", libmympdclient_version, true);
        sdsfree(libmympdclient_version);
    #endif
    buffer = tojson_char(buffer, "libmpdclientVersion", libmpdclient_version, false);
    buffer = jsonrpc_end_result(buffer);

    sdsfree(mpd_version);
    sdsfree(libmpdclient_version);
    mpd_stats_free(stats);

    return buffer;
}


//private functions
static sds mpd_client_put_last_played_obj(t_mpd_state *mpd_state, sds buffer, 
                                          unsigned entity_count, int last_played, const char *uri, const t_tags *tagcols)
{
    buffer = sdscat(buffer, "{");
    buffer = tojson_long(buffer, "Pos", entity_count, true);
    buffer = tojson_long(buffer, "LastPlayed", last_played, true);
    if (!mpd_send_list_all_meta(mpd_state->conn, uri)) {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
        buffer = put_empty_song_tags(buffer, mpd_state, tagcols, uri);
    }
    else {
        struct mpd_entity *entity;
        if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            buffer = put_song_tags(buffer, mpd_state, tagcols, song);
            mpd_entity_free(entity);
            mpd_response_finish(mpd_state->conn);
        }
        else {
            buffer = put_empty_song_tags(buffer, mpd_state, tagcols, uri);
        }
    }
    buffer = sdscat(buffer, "}");
    return buffer;
}
