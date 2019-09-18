/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <pthread.h>
#include <mpd/client.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "log.h"
#include "config_defs.h"
#include "global.h"
#include "stats.h"
#include "../dist/src/sds/sds.h"

bool mpd_client_get_sticker(t_mpd_state *mpd_state, const char *uri, t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap = NULL;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->lastSkipped = 0;
    sticker->like = 1;
    
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }

    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, "playCount") == 0) {
                sticker->playCount = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "skipCount") == 0) {
                sticker->skipCount = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "lastPlayed") == 0) {
                sticker->lastPlayed = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "lastSkipped") == 0) {
                sticker->lastSkipped = strtoimax(pair->value, &crap, 10);
            }
            else if (strcmp(pair->name, "like") == 0) {
                sticker->like = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_state->conn, pair);
        }
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
        return false;
    }
    return true;
}

bool mpd_client_count_song_uri(t_mpd_state *mpd_state, const char *uri, const char *name, const int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    struct mpd_pair *pair;
    char *crap = NULL;
    int old_value = 0;
    char v[10];
    
    if (mpd_send_sticker_list(mpd_state->conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd_state->conn)) != NULL) {
            if (strcmp(pair->name, name) == 0) {
                old_value = strtoimax(pair->value, &crap, 10);
            }
            mpd_return_sticker(mpd_state->conn, pair);
        }
    } else {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
        return false;
    }
    old_value += value;
    if (old_value > 999999999) {
        old_value = 999999999;
    }
    else if (old_value < 0) {
        old_value = 0;
    }
    snprintf(v, 10, "%d", old_value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> %s: %s", uri, name, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, name, v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;
}

bool mpd_client_like_song_uri(t_mpd_state *mpd_state, const char *uri, int value) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    char v[10];
    if (value > 2) {
        value = 2;
    }
    else if (value < 0) {
        value = 0;
    }
    snprintf(v, 10, "%d", value);
    LOG_VERBOSE("Setting sticker: \"%s\" -> like: %s", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "like", v)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
        return false;
    }
    return true;        
}

bool mpd_client_last_played_list_save(t_config *config, t_mpd_state *mpd_state) {
    size_t tmp_file_len = config->varlibdir_len + 26;
    char tmp_file[tmp_file_len];
    size_t cfg_file_len = config->varlibdir_len + 19;
    char cfg_file[cfg_file_len];
    snprintf(cfg_file, cfg_file_len, "%s/state/last_played", config->varlibdir);
    snprintf(tmp_file, tmp_file_len, "%s/state/last_played.XXXXXX", config->varlibdir);
    int fd;
    int i = 0;

    LOG_VERBOSE("Saving last_played list to disc");

    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }    
    
    FILE *fp = fdopen(fd, "w");
    //first write last_played list to tmp file
    struct node *current = mpd_state->last_played.list;
    while (current != NULL && i < mpd_state->last_played_count) {
        fprintf(fp, "%d::%s\n", current->value, current->data);
        current = current->next;
        i++;
    }
    //append current last_played file to tmp file
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    FILE *fi = fopen(cfg_file, "r");
    if (fi != NULL) {
        while ((read = getline(&line, &n, fi)) > 0 && i < mpd_state->last_played_count) {
            fprintf(fp, "%s", line);
            i++;
        }
        FREE_PTR(line);
        fclose(fi);
    }
    fclose(fp);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        return false;
    }
    //empt list after write to disc
    list_free(&mpd_state->last_played);    
    return true;
}

bool mpd_client_last_played_list(t_config *config, t_mpd_state *mpd_state, const int song_id) {
    struct mpd_song *song;

    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd_state->conn, song_id);
        if (song) {
            const char *uri = mpd_song_get_uri(song);
            if (uri == NULL || strstr(uri, "://") != NULL) {
                //Don't add streams to last played list
                mpd_state->last_last_played_id = song_id;
                mpd_song_free(song);
                return true;
            }
            else {
                list_insert(&mpd_state->last_played, uri, time(NULL), NULL);
            }
            mpd_state->last_last_played_id = song_id;
            mpd_song_free(song);
            //write last_played list to disc
            if (mpd_state->last_played.length > 9 || mpd_state->last_played.length > mpd_state->last_played_count) {
                mpd_client_last_played_list_save(config, mpd_state);
            }
            //notify clients
            char buffer[MAX_SIZE];
            size_t len = snprintf(buffer, MAX_SIZE, "{\"type\": \"update_lastplayed\"}");
            mpd_client_notify(buffer, len);
        } else {
            LOG_ERROR("Can't get song from id %d", song_id);
            return false;
        }
    }
    return true;
}

bool mpd_client_last_played_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    char v[20];
    snprintf(v, 20, "%ld", mpd_state->song_start_time);
    LOG_VERBOSE("Setting sticker: \"%s\" -> lastPlayed: %s", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastPlayed", v)) {
        LOG_ERROR_AND_RECOVER("mpd_run_sticker_set");
        return false;
    }
    return true;
}

bool mpd_client_last_skipped_song_uri(t_mpd_state *mpd_state, const char *uri) {
    if (uri == NULL || strstr(uri, "://") != NULL) {
        return false;
    }
    char v[20];
    time_t now = time(NULL);
    snprintf(v, 20, "%ld", now);
    LOG_VERBOSE("Setting sticker: \"%s\" -> lastSkipped: %s", uri, v);
    if (!mpd_run_sticker_set(mpd_state->conn, "song", uri, "lastSkipped", v)) {
        LOG_ERROR_AND_RECOVER("mpd_run_sticker_set");
        return false;
    }
    return true;
}

int mpd_client_put_last_played_songs(t_config *config, t_mpd_state *mpd_state, char *buffer, unsigned int offset, const t_tags *tagcols) {
    const struct mpd_song *song;
    struct mpd_entity *entity;
    size_t len = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: last_played_songs, data: [");
    
    if (mpd_state->last_played.length > 0) {
        struct node *current = mpd_state->last_played.list;
        while (current != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ",");
                len += json_printf(&out, "{Pos: %d, LastPlayed: %d, ", entity_count, current->value);
                if (!mpd_send_list_all_meta(mpd_state->conn, current->data)) {
                    LOG_ERROR_AND_RECOVER("mpd_send_list_all_meta");
                }
                else {
                    if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
                        song = mpd_entity_get_song(entity);
                        PUT_SONG_TAG_COLS(tagcols);
                        mpd_entity_free(entity);
                        mpd_response_finish(mpd_state->conn);
                    }
                }
                len += json_printf(&out, "}");
            }
            current = current->next;
        }
    }

    char *line = NULL;
    char *data = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    
    size_t lp_file_len = config->varlibdir_len + 19;
    char lp_file[lp_file_len];
    snprintf(lp_file, lp_file_len, "%s/state/last_played", config->varlibdir);
    FILE *fp = fopen(lp_file, "r");
    if (fp != NULL) {
        while ((read = getline(&line, &n, fp)) > 0 && entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            int value = strtoimax(line, &data, 10);
            if (strlen(data) > 2) {
                data = data + 2;
                strtok_r(data, "\n", &crap);
                if (entities_returned++) 
                    len += json_printf(&out, ",");
                len += json_printf(&out, "{Pos: %d, LastPlayed: %d, ", entity_count, value);
                if (!mpd_send_list_all_meta(mpd_state->conn, data)) {
                    LOG_ERROR_AND_RECOVER("mpd_send_list_all_meta");
                }
                else {
                    if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
                        song = mpd_entity_get_song(entity);
                        PUT_SONG_TAG_COLS(tagcols);
                        mpd_entity_free(entity);
                        mpd_response_finish(mpd_state->conn);
                    }
                }
                len += json_printf(&out, "}");
            }
            else {
                LOG_ERROR("Reading last_played line failed");
            }
        }
        fclose(fp);
        FREE_PTR(line);
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
        entity_count,
        offset,
        entities_returned
    );

    CHECK_RETURN_LEN();
}

int mpd_client_put_stats(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    const unsigned *version = mpd_connection_get_server_version(mpd_state->conn);
    size_t len = 0;
    char mpd_version[20];
    char libmpdclient_version[20];
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    snprintf(mpd_version, 20, "%u.%u.%u", version[0], version[1], version[2]);
    snprintf(libmpdclient_version, 20, "%i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);

    if (stats == NULL) {
        RETURN_ERROR_AND_RECOVER("mpd_run_stats");
    }
    len = json_printf(&out, "{type: mpdstats, data: {artists: %d, albums: %d, songs: %d, "
        "playtime: %d, uptime: %d, dbUpdated: %d, dbPlaytime: %d, mympdVersion: %Q, mpdVersion: %Q, "
        "libmpdclientVersion: %Q}}",
        mpd_stats_get_number_of_artists(stats),
        mpd_stats_get_number_of_albums(stats),
        mpd_stats_get_number_of_songs(stats),
        mpd_stats_get_play_time(stats),
        mpd_stats_get_uptime(stats),
        mpd_stats_get_db_update_time(stats),
        mpd_stats_get_db_play_time(stats),
        MYMPD_VERSION,
        mpd_version,
        libmpdclient_version
    );
    mpd_stats_free(stats);

    CHECK_RETURN_LEN();
}
