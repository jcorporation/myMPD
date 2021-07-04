/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <inttypes.h>
#include <ctype.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../api.h"
#include "../list.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "mympd_config_defs.h"
#include "../utility.h"
#include "../log.h"
#include "../mympd_state.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_playlists.h"
#include "mpd_worker_utility.h"
#include "mpd_worker_smartpls.h"

//private definitions
static bool mpd_worker_smartpls_per_tag(struct t_mpd_worker_state *mpd_worker_state);
static bool mpd_worker_smartpls_clear(struct t_mpd_worker_state *mpd_worker_state, const char *playlist);
static bool mpd_worker_smartpls_update_search(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *expression);
static bool mpd_worker_smartpls_update_sticker(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *sticker, const int maxentries, const int minvalue);
static bool mpd_worker_smartpls_update_newest(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const int timerange);

//public functions
bool mpd_worker_smartpls_update_all(struct t_mpd_worker_state *mpd_worker_state, bool force) {
    if (mpd_worker_state->mpd_state->feat_playlists == false) {
        MYMPD_LOG_DEBUG("Playlists are disabled");
        return true;
    }
    
    mpd_worker_smartpls_per_tag(mpd_worker_state);

    unsigned long db_mtime = mpd_shared_get_db_mtime(mpd_worker_state->mpd_state);
    MYMPD_LOG_DEBUG("Database mtime: %d", db_mtime);
    
    sds dirname = sdscatfmt(sdsempty(), "%s/smartpls", mpd_worker_state->config->workdir);
    errno = 0;
    DIR *dir = opendir (dirname);
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, ".", 1) == 0) {
                continue;
            }
            unsigned long playlist_mtime = mpd_shared_get_playlist_mtime(mpd_worker_state->mpd_state, ent->d_name);
            unsigned long smartpls_mtime = mpd_shared_get_smartpls_mtime(mpd_worker_state->config, ent->d_name);
            MYMPD_LOG_DEBUG("Playlist %s: playlist mtime %d, smartpls mtime %d", ent->d_name, playlist_mtime, smartpls_mtime);
            if (force == true || db_mtime > playlist_mtime || smartpls_mtime > playlist_mtime) {
                mpd_worker_smartpls_update(mpd_worker_state, ent->d_name);
            }
            else {
                MYMPD_LOG_INFO("Update of smart playlist %s skipped, already up to date", ent->d_name);
            }
        }
        closedir (dir);
    }
    else {
        MYMPD_LOG_ERROR("Can't open smart playlist directory \"%s\"", dirname);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(dirname);
        return false;
    }
    sdsfree(dirname);
    return true;
}

bool mpd_worker_smartpls_update(struct t_mpd_worker_state *mpd_worker_state, const char *playlist) {
    char *smartpltype = NULL;
    int je;
    bool rc = true;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    int int_buf1;
    int int_buf2;
    
    if (mpd_worker_state->mpd_state->feat_playlists == false) {
        MYMPD_LOG_WARN("Playlists are disabled");
        return true;
    }
    if (validate_string_not_dir(playlist) == false) {
        MYMPD_LOG_ERROR("Invalid smart playlist name");
        return false;
    }
    
    sds filename = sdscatfmt(sdsempty(), "%s/smartpls/%s", mpd_worker_state->config->workdir, playlist);
    char *content = json_fread(filename);
    if (content == NULL) {
        MYMPD_LOG_ERROR("Cant read smart playlist \"%s\"", playlist);
        sdsfree(filename);
        return false;
    }
    je = json_scanf(content, (int)strlen(content), "{type: %Q }", &smartpltype);
    if (je != 1) {
        MYMPD_LOG_ERROR("Cant read smart playlist type from \"%s\"", filename);
        sdsfree(filename);
        return false;
    }
    if (strcmp(smartpltype, "sticker") == 0) {
        je = json_scanf(content, (int)strlen(content), "{sticker: %Q, maxentries: %d, minvalue: %d}", &p_charbuf1, &int_buf1, &int_buf2);
        if (je == 3) {
            rc = mpd_worker_smartpls_update_sticker(mpd_worker_state, playlist, p_charbuf1, int_buf1, int_buf2);
            if (rc == false) {
                MYMPD_LOG_ERROR("Update of smart playlist \"%s\" failed.", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR("Can't parse smart playlist file \"%s\"", filename);
            rc = false;
        }
        FREE_PTR(p_charbuf1);
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        je = json_scanf(content, (int)strlen(content), "{timerange: %d}", &int_buf1);
        if (je == 1) {
            rc = mpd_worker_smartpls_update_newest(mpd_worker_state, playlist, int_buf1);
            if (rc == false) {
                MYMPD_LOG_ERROR("Update of smart playlist \"%s\" failed", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR("Can't parse smart playlist file \"%s\"", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        je = json_scanf(content, (int)strlen(content), "{expression: %Q}", &p_charbuf1);
        if (je == 1) {
            rc = mpd_worker_smartpls_update_search(mpd_worker_state, playlist, p_charbuf1);
            if (rc == false) {
                MYMPD_LOG_ERROR("Update of smart playlist \"%s\" failed", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR("Can't parse smart playlist file \"%s\"", filename);
            rc = false;
        }
        FREE_PTR(p_charbuf1);
        FREE_PTR(p_charbuf2);
    }
    if (rc == true) {
        je = json_scanf(content, (int)strlen(content), "{sort: %Q}", &p_charbuf1);
        if (je == 1 && strlen(p_charbuf1) > 0) {
            mpd_shared_playlist_shuffle_sort(mpd_worker_state->mpd_state, NULL, NULL, 0, playlist, p_charbuf1);
        }
        FREE_PTR(p_charbuf1);
    }
    FREE_PTR(smartpltype);
    FREE_PTR(content);
    sdsfree(filename);
    return rc;
}

//private functions
static bool mpd_worker_smartpls_per_tag(struct t_mpd_worker_state *mpd_worker_state) {
    for (size_t i = 0; i < mpd_worker_state->smartpls_generate_tag_types.len; i++) {
        enum mpd_tag_type tag = mpd_worker_state->smartpls_generate_tag_types.tags[i];
        bool rc = mpd_search_db_tags(mpd_worker_state->mpd_state->conn, tag);

        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_db_tags") == false) {
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_worker_state->mpd_state->conn);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit") == false) {
            return false;
        }
        struct mpd_pair *pair;
        struct list tag_list;
        list_init(&tag_list);
        while ((pair = mpd_recv_pair_tag(mpd_worker_state->mpd_state->conn, tag)) != NULL) {
            if (strlen(pair->value) > 0) {
                list_push(&tag_list, pair->value, 0, NULL, NULL);
            }
            mpd_return_pair(mpd_worker_state->mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_worker_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
            list_free(&tag_list);
            return false;
        }
        struct list_node *current = tag_list.head;
        while (current != NULL) {
            const char *tagstr = mpd_tag_name(tag);
            sds playlist = sdscatfmt(sdsempty(), "%s%s%s-%s", mpd_worker_state->smartpls_prefix, (sdslen(mpd_worker_state->smartpls_prefix) > 0 ? "-" : ""), tagstr, current->key);
            sds plpath = sdscatfmt(sdsempty(), "%s/smartpls/%s", mpd_worker_state->config->workdir, playlist);
            if (access(plpath, F_OK) == -1) { /* Flawfinder: ignore */
                MYMPD_LOG_INFO("Created smart playlist %s", playlist);
                sds expression = sdsnew("(");
                expression = escape_mpd_search_expression(expression, tagstr, "==", current->key);
                expression = sdscat(expression, ")");
                mpd_shared_smartpls_save(mpd_worker_state->config->workdir, "search", playlist, expression, 0, 0, mpd_worker_state->smartpls_sort);
                sdsfree(expression);
            }
            sdsfree(playlist);
            sdsfree(plpath);
            current = current->next;
        }
        list_free(&tag_list);
    }
    return true;
}

static bool mpd_worker_smartpls_clear(struct t_mpd_worker_state *mpd_worker_state, const char *playlist) {
    struct mpd_playlist *pl;
    bool exists = false;
    
    //first check if playlist exists
    bool rc = mpd_send_list_playlists(mpd_worker_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_playlists") == false) {
        return false;
    }
    while ((pl = mpd_recv_playlist(mpd_worker_state->mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        if (strcmp(playlist, plpath) == 0) {
            exists = true;
        }
        mpd_playlist_free(pl);
        if (exists == true) {
            break;
        }
    }
    mpd_response_finish(mpd_worker_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }

    //delete playlist if exists
    if (exists) {
        rc = mpd_run_rm(mpd_worker_state->mpd_state->conn, playlist);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_rm") == false) {
            return false;
        }
    }
    return true;
}

static bool mpd_worker_smartpls_update_search(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *expression) {
    mpd_worker_smartpls_clear(mpd_worker_state, playlist);
    sds buffer = sdsempty();
    buffer = mpd_shared_search_adv(mpd_worker_state->mpd_state, buffer, NULL, 0, expression, NULL, true, NULL, playlist, 0, 0, NULL, NULL);
    sdsfree(buffer);
    MYMPD_LOG_INFO("Updated smart playlist %s", playlist);
    return true;
}

static bool mpd_worker_smartpls_update_sticker(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *sticker, const int maxentries, const int minvalue)
{
    bool rc = mpd_send_sticker_find(mpd_worker_state->mpd_state->conn, "song", "", sticker);
    if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_find") == false) {
        return false;    
    }

    struct list add_list;
    list_init(&add_list);

    struct mpd_pair *pair;
    char *uri = NULL;
    int value_max = 0;

    while ((pair = mpd_recv_pair(mpd_worker_state->mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "file") == 0) {
            FREE_PTR(uri);
            uri = strdup(pair->value);
        } 
        else if (strcmp(pair->name, "sticker") == 0) {
            size_t j;
            const char *p_value = mpd_parse_sticker(pair->value, &j);
            if (p_value != NULL) {
                char *crap;
                int value = strtoimax(p_value, &crap, 10);
                if (value >= 1) {
                    list_push(&add_list, uri, value, NULL, NULL);
                }
                if (value > value_max) {
                    value_max = value;
                }
            }
        }
        mpd_return_pair(mpd_worker_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_worker_state->mpd_state->conn);
    FREE_PTR(uri);
    if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }

    mpd_worker_smartpls_clear(mpd_worker_state, playlist);
     
    if (minvalue > 0) {
        value_max = minvalue;
    }
    else if (value_max > 2) {
        value_max = value_max / 2;
    }

    list_sort_by_value_i(&add_list, false);

    int i = 0;
    if (mpd_command_list_begin(mpd_worker_state->mpd_state->conn, false)) {
        struct list_node *current = add_list.head;

        while (current != NULL) {
            if (current->value_i >= value_max) {
                rc = mpd_send_playlist_add(mpd_worker_state->mpd_state->conn, playlist, current->key);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                i++;
                if (i >= maxentries) {
                    break;
                }
            }
            current = current->next;
        }
        if (mpd_command_list_end(mpd_worker_state->mpd_state->conn)) {
            mpd_response_finish(mpd_worker_state->mpd_state->conn);
        }
        if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
            list_free(&add_list);
            return false;
        }
    }
    list_free(&add_list);
    MYMPD_LOG_INFO("Updated smart playlist %s with %d songs, minValue: %d", playlist, i, value_max);
    return true;
}

static bool mpd_worker_smartpls_update_newest(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const int timerange) {
    unsigned long value_max = 0;
    
    struct mpd_stats *stats = mpd_run_stats(mpd_worker_state->mpd_state->conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    else {
        check_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0);
        return false;
    }

    mpd_worker_smartpls_clear(mpd_worker_state, playlist);
    value_max -= timerange;
    sds buffer = sdsempty();
    sds method = sdsempty();
    if (value_max > 0) {
        sds searchstr = sdscatprintf(sdsempty(), "(modified-since '%lu')", value_max);
        buffer = mpd_shared_search_adv(mpd_worker_state->mpd_state, buffer, method, 0, searchstr, NULL, true, NULL, playlist, 0, 0, NULL, NULL);
        sdsfree(searchstr);
        MYMPD_LOG_INFO("Updated smart playlist %s", playlist);
    }
    sdsfree(buffer);
    sdsfree(method);
    return true;
}
