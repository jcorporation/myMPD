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
#include <dirent.h>
#include <pthread.h>
#include <mpd/client.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "smarpls.h"
#include "../dist/src/frozen/frozen.h"
#include "../dist/src/sds/sds.h"

static int mpd_client_put_playlists(t_config *config, t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *filter) {
    struct mpd_playlist *pl;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *plpath;
    size_t len = 0;
    bool smartpls;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_send_list_playlists(mpd_state->conn) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_send_lists_playlists");
    }

    len = json_printf(&out, "{type: playlists, data: [");

    while ((pl = mpd_recv_playlist(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            plpath = mpd_playlist_get_path(pl);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plpath, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*plpath) == 0 )) 
            {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                size_t smartpls_file_len = config->varlibdir_len + strlen(plpath) + 11;
                char smartpls_file[smartpls_file_len];
                snprintf(smartpls_file, smartpls_file_len, "%s/smartpls/%s", config->varlibdir, plpath);
                if (validate_string(plpath) == true) {
                    if (access(smartpls_file, F_OK ) != -1) {
                        smartpls = true;
                    }
                    else {
                        smartpls = false;
                    }
                }
                else {
                    smartpls = false;
                }
                len += json_printf(&out, "{Type: %Q, uri: %Q, name: %Q, last_modified: %d}",
                    (smartpls == true ? "smartpls" : "plist"), 
                    plpath,
                    plpath,
                    mpd_playlist_get_last_modified(pl)
                );
            } else {
                entity_count--;
            }
        }
        mpd_playlist_free(pl);
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
        entity_count,
        offset,
        entities_returned
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_put_playlist_list(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri, const unsigned int offset, const char *filter, const t_tags *tagcols) {
    struct mpd_entity *entity;
    unsigned entities_returned = 0;
    unsigned entity_count = 0;
    const char *entityName;
    bool smartpls = false;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_send_list_playlist_meta(mpd_state->conn, uri) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");
    }

    len = json_printf(&out, "{type: playlist_detail, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        const struct mpd_song *song;
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            song = mpd_entity_get_song(entity);
            entityName = mpd_client_get_tag(song, MPD_TAG_TITLE);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0))
            {
                if (entities_returned++) {
                    len += json_printf(&out, ",");
                }
                len += json_printf(&out, "{Type: song, ");
                PUT_SONG_TAG_COLS(tagcols);
                len += json_printf(&out, ", Pos: %d", entity_count);
                len += json_printf(&out, "}");
            }
            else {
                entity_count--;
            }
        }
        mpd_entity_free(entity);
    }
    mpd_response_finish(mpd_state->conn);
    
    if (validate_string(uri) == true) {
        size_t smartpls_file_len = config->varlibdir_len + strlen(uri) + 11;
        char smartpls_file[smartpls_file_len];
        snprintf(smartpls_file, smartpls_file_len, "%s/smartpls/%s", config->varlibdir, uri);
        if (access(smartpls_file, F_OK ) != -1) {
            smartpls = true;
        }
    }
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q, uri: %Q, smartpls: %B}",
        entity_count,
        offset,
        entities_returned,
        filter,
        uri,
        smartpls
    );

    CHECK_RETURN_LEN();
}

static int mpd_client_rename_playlist(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *old_playlist, const char *new_playlist) {
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    size_t old_pl_file_len = config->varlibdir_len + strlen(old_playlist) + 11;
    char old_pl_file[old_pl_file_len];
    size_t new_pl_file_len = config->varlibdir_len + strlen(new_playlist) + 11;
    char new_pl_file[new_pl_file_len];
    size_t len = 0;

    if (validate_string(old_playlist) == false || validate_string(new_playlist) == false) {
        len = json_printf(&out, "{type: error, data: %Q}", "Invalid filename");
        return len;
    }

    snprintf(old_pl_file, old_pl_file_len, "%s/smartpls/%s", config->varlibdir, old_playlist);
    snprintf(new_pl_file, new_pl_file_len, "%s/smartpls/%s", config->varlibdir, new_playlist);

    if (access(old_pl_file, F_OK ) != -1) {
        //smart playlist
        if (access(new_pl_file, F_OK ) == -1) {
            //new playlist doesn't exist
            if (rename(old_pl_file, new_pl_file) == -1) {
                len = json_printf(&out, "{type: error, data: %Q}", "Renaming playlist failed");
                LOG_ERROR("Renaming smart playlist %s to %s failed", old_pl_file, new_pl_file);
                return len;
            }
        } 
    }

    if (mpd_run_rename(mpd_state->conn, old_playlist, new_playlist)) {
        len = json_printf(&out, "{type: result, data: %Q}", "Sucessfully renamed playlist");
    }
    else {
        RETURN_ERROR_AND_RECOVER("mpd_run_rename");
    }

    CHECK_RETURN_LEN();
}

int mpd_client_smartpls_put(t_config *config, char *buffer, const char *playlist) {
    size_t pl_file_len = config->varlibdir_len + 11 + strlen(playlist);
    char pl_file[pl_file_len];
    char *smartpltype = NULL;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    int je, int_buf1;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (validate_string(playlist) == false) {
        len = json_printf(&out, "{type: error, data: %Q}}", "Can not read smart playlist file");
        return len;
    }
    snprintf(pl_file, pl_file_len, "%s/smartpls/%s", config->varlibdir, playlist);
    char *content = json_fread(pl_file);
    if (content == NULL) {
        len = json_printf(&out, "{type: error, data: %Q}}", "Can not read smart playlist file");
        LOG_ERROR("Can't read smart playlist file: %s", pl_file);
        return len;
    }
    je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
    if (je == 1) {
        if (strcmp(smartpltype, "sticker") == 0) {
            je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, sticker: %Q, maxentries: %d}}",
                    playlist,
                    smartpltype,
                    p_charbuf1,
                    int_buf1);
                FREE_PTR(p_charbuf1);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can not parse smart playlist file");
                LOG_ERROR("Can't parse smart playlist file: %s", pl_file);
            }
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
            if (je == 1) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, timerange: %d}}",
                    playlist,
                    smartpltype,
                    int_buf1);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can not parse smart playlist file");
                LOG_ERROR("Can't parse smart playlist file: %s", pl_file);
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, tag: %Q, searchstr: %Q}}",
                    playlist,
                    smartpltype,
                    p_charbuf1,
                    p_charbuf2);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            } else {
                len = json_printf(&out, "{type: error, data: %Q]", "Can not parse smart playlist file");
                LOG_ERROR("Can't parse smart playlist file: %s", pl_file);
            }
        } else {
            len = json_printf(&out, "{type: error, data: %Q}}", "Unknown smart playlist type");
            LOG_ERROR("Unknown smart playlist type: %s", pl_file);
        }
        FREE_PTR(smartpltype);        
    } else {
        len = json_printf(&out, "{type: error, data: %Q}}", "Unknown smart playlist type");
        LOG_ERROR("Unknown smart playlist type: %s", pl_file);
    }
    FREE_PTR(content);
    return len;
}

bool mpd_client_smartpls_save(t_config *config, t_mpd_state *mpd_state, const char *smartpltype, const char *playlist, const char *tag, const char *searchstr, const int maxentries, const int timerange) {
    size_t tmp_file_len = config->varlibdir_len + strlen(playlist) + 18;
    char tmp_file[tmp_file_len];
    size_t pl_file_len = config->varlibdir_len + strlen(playlist) + 11;
    char pl_file[pl_file_len];
    int fd;
    
    if (validate_string(playlist) == false) {
        return false;
    }
    
    snprintf(tmp_file, tmp_file_len, "%s/smartpls/%s.XXXXXX", config->varlibdir, playlist);
    snprintf(pl_file, pl_file_len, "%s/smartpls/%s", config->varlibdir, playlist);
    
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct json_out out = JSON_OUT_FILE(fp);
    
    if (strcmp(smartpltype, "sticker") == 0) {
        if (json_printf(&out, "{type: %Q, sticker: %Q, maxentries: %d}", smartpltype, tag, maxentries) == -1) {
            LOG_ERROR("Can't write to file %s", tmp_file);
            fclose(fp);
            return false;
        }
        fclose(fp);
        if (rename(tmp_file, pl_file) == -1) {
            LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_sticker(mpd_state, playlist, tag, maxentries) == false) {
            LOG_ERROR("Update of smart playlist %s failed.", playlist);
            return false;
        }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        if (json_printf(&out, "{type: %Q, timerange: %d}", smartpltype, timerange) == -1) {
            LOG_ERROR("Can't write to file %s", tmp_file);
            fclose(fp);
            return false;
        }
        fclose(fp);
        if (rename(tmp_file, pl_file) == -1) {
            LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_newest(mpd_state, playlist, timerange) == false) {
            LOG_ERROR("Update of smart playlist %s failed", playlist);
            return false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        if (json_printf(&out, "{type: %Q, tag: %Q, searchstr: %Q}", smartpltype, tag, searchstr) == -1) {
            LOG_ERROR("Can't write to file %s", tmp_file);
            fclose(fp);
            return false;
        }
        fclose(fp);
        if (rename(tmp_file, pl_file) == -1) {
            LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pl_file);
            return false;
        }
        else if (mpd_client_smartpls_update_search(mpd_state, playlist, tag, searchstr) == false) {
            LOG_ERROR("Update of smart playlist %s failed", playlist);
            return false;
        }
    }
    return true;
}

bool mpd_client_smartpls_update_all(t_config *config, t_mpd_state *mpd_state) {
    DIR *dir;
    struct dirent *ent;

    if (mpd_state->feat_smartpls == false) {
        return true;
    }
    
    size_t dirname_len = config->varlibdir_len + 10;
    char dirname[dirname_len];
    snprintf(dirname, dirname_len, "%s/smartpls", config->varlibdir);
    if ((dir = opendir (dirname)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, ".", 1) == 0) {
                continue;
            }
            else {
                mpd_client_smartpls_update(config, mpd_state, ent->d_name);
            }
        }
        closedir (dir);
    } else {
        LOG_ERROR("Can't open smart playlist directory %s", dirname);
        return false;
    }
    return true;
}

bool mpd_client_smartpls_update(t_config *config, t_mpd_state *mpd_state, char *playlist) {
    char *smartpltype = NULL;
    int je;
    bool rc = true;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    int int_buf1;
    if (mpd_state->feat_smartpls == false) {
        return true;
    }
    
    size_t filename_len = config->varlibdir_len + strlen(playlist) + 11;
    char filename[filename_len];
    snprintf(filename, filename_len, "%s/smartpls/%s", config->varlibdir, playlist);
    char *content = json_fread(filename);
    if (content == NULL) {
        LOG_ERROR("Cant read smart playlist file %s", filename);
        return false;
    }
    je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
    if (je != 1) {
        LOG_ERROR("Cant read smart playlist type from %s", filename);
        return false;
    }
    if (strcmp(smartpltype, "sticker") == 0) {
        je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
        if (je == 2) {
            if (mpd_client_smartpls_update_sticker(mpd_state, playlist, p_charbuf1, int_buf1) == false) {
                LOG_ERROR("Update of smart playlist %s failed.", playlist);
                rc = false;
            }
            FREE_PTR(p_charbuf1);
         }
         else {
            LOG_ERROR("Can't parse smart playlist file %s", filename);
            rc = false;
         }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
        if (je == 1) {
            if (mpd_client_smartpls_update_newest(mpd_state, playlist, int_buf1) == false) {
                LOG_ERROR("Update of smart playlist %s failed", playlist);
                rc = false;
            }
        }
        else {
            LOG_ERROR("Can't parse smart playlist file %s", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
        if (je == 2) {
            if (mpd_client_smartpls_update_search(mpd_state, playlist, p_charbuf1, p_charbuf2) == false) {
                LOG_ERROR("Update of smart playlist %s failed", playlist);
                rc = false;
            }
            FREE_PTR(p_charbuf1);
            FREE_PTR(p_charbuf2);
        }
        else {
            LOG_ERROR("Can't parse smart playlist file %s", filename);
            rc = false;
        }
    }
    FREE_PTR(smartpltype);
    FREE_PTR(content);
    return rc;
}

bool mpd_client_smartpls_clear(t_mpd_state *mpd_state, const char *playlist) {
    struct mpd_playlist *pl;
    const char *plpath;
    bool exists = false;
    if (mpd_send_list_playlists(mpd_state->conn) == false) {
        LOG_ERROR_AND_RECOVER("mpd_send_list_playlists");
        return 1;
    }
    while ((pl = mpd_recv_playlist(mpd_state->conn)) != NULL) {
        plpath = mpd_playlist_get_path(pl);
        if (strcmp(playlist, plpath) == 0)
            exists = true;
        mpd_playlist_free(pl);
        if (exists == true) {
            break;
        }
    }
    mpd_response_finish(mpd_state->conn);
    
    if (exists) {
        if (mpd_run_rm(mpd_state->conn, playlist) == false) {
            LOG_ERROR_AND_RECOVER("mpd_run_rm");
            return false;
        }
    }
    return true;
}

bool mpd_client_smartpls_update_search(t_mpd_state *mpd_state, const char *playlist, const char *tag, const char *searchstr) {
    char buffer[MAX_SIZE];
    mpd_client_smartpls_clear(mpd_state, playlist);
    if (mpd_state->feat_advsearch == true && strcmp(tag, "expression") == 0) {
        mpd_client_search_adv(mpd_state, buffer, searchstr, NULL, true, NULL, playlist, 0, NULL);
    }
    else {
        mpd_client_search(mpd_state, buffer, searchstr, tag, playlist, 0, NULL);
    }
    LOG_INFO("Updated smart playlist %s", playlist);
    return true;
}

bool mpd_client_smartpls_update_sticker(t_mpd_state *mpd_state, const char *playlist, const char *sticker, const int maxentries) {
    struct mpd_pair *pair;
    char *uri = NULL;
    const char *p_value;
    char *crap = NULL;
    int value;
    int value_max = 0;
    int i = 0;
    size_t j;

    if (mpd_send_sticker_find(mpd_state->conn, "song", "", sticker) == false) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_find");
        return false;    
    }

    struct list add_list;
    list_init(&add_list);

    while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "file") == 0) {
            FREE_PTR(uri);
            uri = strdup(pair->value);
        } 
        else if (strcmp(pair->name, "sticker") == 0) {
            p_value = mpd_parse_sticker(pair->value, &j);
            if (p_value != NULL) {
                value = strtoimax(p_value, &crap, 10);
                if (value >= 1) {
                    list_push(&add_list, uri, value, NULL);
                }
                if (value > value_max) {
                    value_max = value;
                }
            }
        }
        mpd_return_pair(mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_state->conn);
    FREE_PTR(uri);

    mpd_client_smartpls_clear(mpd_state, playlist);
     
    if (value_max > 2) {
        value_max = value_max / 2;
    }

    list_sort_by_value(&add_list, false);

    struct node *current = add_list.list;
    while (current != NULL) {
        if (current->value >= value_max) {
            if (mpd_run_playlist_add(mpd_state->conn, playlist, current->data) == false) {
                LOG_ERROR_AND_RECOVER("mpd_run_playlist_add");
                list_free(&add_list);
                return 1;        
            }
            i++;
            if (i >= maxentries)
                break;
        }
        current = current->next;
    }
    list_free(&add_list);
    LOG_INFO("Updated smart playlist %s with %d songs, minValue: %d", playlist, i, value_max);
    return true;
}

bool mpd_client_smartpls_update_newest(t_mpd_state *mpd_state, const char *playlist, const int timerange) {
    int value_max = 0;
    char buffer[MAX_SIZE];
    char searchstr[50];
    
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_run_stats");
        return false;
    }

    mpd_client_smartpls_clear(mpd_state, playlist);
    value_max -= timerange;
    if (value_max > 0) {
        if (mpd_state->feat_advsearch == true) {
            snprintf(searchstr, 50, "(modified-since '%d')", value_max);
            mpd_client_search_adv(mpd_state, buffer, searchstr, NULL, true, NULL, playlist, 0, NULL);
        }
        else {
            snprintf(searchstr, 20, "%d", value_max);
            mpd_client_search(mpd_state, buffer, searchstr, "modified-since", playlist, 0, NULL);
        }
        LOG_INFO("Updated smart playlist %s", playlist);
    }
    return true;
}

