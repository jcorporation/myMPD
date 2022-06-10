/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_shared_playlists.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/random.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_tags.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

time_t mpd_shared_get_db_mtime(struct t_mpd_state *mpd_state) {
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    if (stats == NULL) {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
        return 0;
    }
    time_t mtime = (time_t)mpd_stats_get_db_update_time(stats);
    mpd_stats_free(stats);
    return mtime;
}

time_t mpd_shared_get_smartpls_mtime(struct t_config *config, const char *playlist) {
    sds plpath = sdscatfmt(sdsempty(), "%S/smartpls/%s", config->workdir, playlist);
    struct stat attr;
    errno = 0;
    if (stat(plpath, &attr) != 0) {
        MYMPD_LOG_ERROR("Error getting mtime for \"%s\"", plpath);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(plpath);
        return 0;
    }
    FREE_SDS(plpath);
    return attr.st_mtime;
}

time_t mpd_shared_get_playlist_mtime(struct t_mpd_state *mpd_state, const char *playlist) {
    bool rc = mpd_send_list_playlists(mpd_state->conn);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_playlists") == false) {
        return 0;
    }
    time_t mtime = 0;
    struct mpd_playlist *pl;
    while ((pl = mpd_recv_playlist(mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        if (strcmp(plpath, playlist) == 0) {
            mtime = mpd_playlist_get_last_modified(pl);
            mpd_playlist_free(pl);
            break;
        }
        mpd_playlist_free(pl);
    }
    mpd_response_finish(mpd_state->conn);
    if (check_error_and_recover2(mpd_state, NULL, NULL, 0, false) == false) {
        return 0;
    }

    return mtime;
}

sds mpd_shared_playlist_shuffle(struct t_mpd_state *mpd_state, sds buffer, sds method,
        long request_id, const char *uri)
{
    MYMPD_LOG_INFO("Shuffling playlist %s", uri);
    bool rc = mpd_send_list_playlist(mpd_state->conn, uri);
    if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlist") == false) {
        return buffer;
    }

    struct t_list plist;
    list_init(&plist);
    struct mpd_song *song;
    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        list_push(&plist, mpd_song_get_uri(song), 0, NULL, NULL);
        mpd_song_free(song);
    }
    mpd_response_finish(mpd_state->conn);
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        list_clear(&plist);
        return buffer;
    }

    if (list_shuffle(&plist) == false) {
        if (buffer != NULL) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Playlist is too small to shuffle");
        }
        list_clear(&plist);
        enable_mpd_tags(mpd_state, &mpd_state->tag_types_mympd);
        return buffer;
    }

    long randnr = randrange(100000, 999999);
    sds uri_tmp = sdscatfmt(sdsempty(), "%l-tmp-%s", randnr, uri);
    sds uri_old = sdscatfmt(sdsempty(), "%l-old-%s", randnr, uri);

    //add shuffled songs to tmp playlist
    //uses command list to add MPD_RESULTS_MAX songs at once
    long i = 0;
    while (i < plist.length) {
        if (mpd_command_list_begin(mpd_state->conn, false) == true) {
            long j = 0;
            struct t_list_node *current;
            while ((current = list_shift_first(&plist)) != NULL) {
                i++;
                j++;
                rc = mpd_send_playlist_add(mpd_state->conn, uri_tmp, current->key);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_RESULTS_MAX) {
                    break;
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
            //error adding songs to tmp playlist - delete it
            rc = mpd_run_rm(mpd_state->conn, uri_tmp);
            check_rc_error_and_recover(mpd_state, NULL, method, request_id, false, rc, "mpd_run_rm");
            FREE_SDS(uri_tmp);
            FREE_SDS(uri_old);
            list_clear(&plist);
            return buffer;
        }
    }
    list_clear(&plist);

    rc = mpd_shared_replace_playlist(mpd_state, uri_tmp, uri, uri_old);

    FREE_SDS(uri_tmp);
    FREE_SDS(uri_old);

    if (buffer != NULL) {
        if (rc == true) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Shuffled playlist succesfully");
        }
        else {
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Shuffling playlist failed");
        }
    }
    return buffer;
}

sds mpd_shared_playlist_sort(struct t_mpd_state *mpd_state, sds buffer, sds method,
        long request_id, const char *uri, const char *tagstr)
{
    struct t_tags sort_tags = {
        .len = 1,
        .tags[0] = mpd_tag_name_parse(tagstr)
    };

    bool rc = false;

    if (strcmp(tagstr, "filename") == 0) {
        MYMPD_LOG_INFO("Sorting playlist %s by filename", uri);
        rc = mpd_send_list_playlist(mpd_state->conn, uri);
    }
    else if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
        MYMPD_LOG_INFO("Sorting playlist %s by tag %s", uri, tagstr);
        enable_mpd_tags(mpd_state, &sort_tags);
        rc = mpd_send_list_playlist_meta(mpd_state->conn, uri);
    }
    else {
        if (buffer != NULL) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Leaving playlist as it is");
        }
        return buffer;
    }
    if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlist") == false) {
        return buffer;
    }

    rax *plist = raxNew();
    sds key = sdsempty();
    struct mpd_song *song;
    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        const char *tag_value = NULL;
        const char *song_uri = mpd_song_get_uri(song);
        sdsclear(key);
        if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
            //sort by tag
            tag_value = mpd_song_get_tag(song, sort_tags.tags[0], 0);
            key = sdscatfmt(key, "%s::%s", tag_value, song_uri);
        }
        else {
            //sort by filename
            key = sdscat(key, song_uri);
        }
        sds_utf8_tolower(key);
        raxInsert(plist, (unsigned char *)key, sdslen(key), sdsnew(song_uri), NULL);
        mpd_song_free(song);
    }
    FREE_SDS(key);
    mpd_response_finish(mpd_state->conn);
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        //free data
        raxIterator iter;
        raxStart(&iter, plist);
        raxSeek(&iter, "^", NULL, 0);
        while (raxNext(&iter)) {
            FREE_SDS(iter.data);
        }
        raxStop(&iter);
        raxFree(plist);
        return buffer;
    }

    long randnr = randrange(100000, 999999);
    sds uri_tmp = sdscatfmt(sdsempty(), "%l-tmp-%s", randnr, uri);
    sds uri_old = sdscatfmt(sdsempty(), "%l-old-%s", randnr, uri);

    //add sorted songs to tmp playlist
    //uses command list to add MPD_RESULTS_MAX songs at once
    unsigned i = 0;
    raxIterator iter;
    raxStart(&iter, plist);
    raxSeek(&iter, "^", NULL, 0);
    while (i < plist->numele) {
        if (mpd_command_list_begin(mpd_state->conn, false) == true) {
            long j = 0;
            while (raxNext(&iter)) {
                i++;
                j++;
                rc = mpd_send_playlist_add(mpd_state->conn, uri_tmp, iter.data);
                FREE_SDS(iter.data);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_RESULTS_MAX) {
                    break;
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
            //error adding songs to tmp playlist - delete it
            rc = mpd_run_rm(mpd_state->conn, uri_tmp);
            check_rc_error_and_recover(mpd_state, NULL, method, request_id, false, rc, "mpd_run_rm");
            FREE_SDS(uri_tmp);
            FREE_SDS(uri_old);
            while (raxNext(&iter)) {
                FREE_SDS(iter.data);
            }
            raxStop(&iter);
            raxFree(plist);
            return buffer;
        }
    }
    raxStop(&iter);
    raxFree(plist);

    rc = mpd_shared_replace_playlist(mpd_state, uri_tmp, uri, uri_old);
    FREE_SDS(uri_tmp);
    FREE_SDS(uri_old);

    if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
        enable_mpd_tags(mpd_state, &mpd_state->tag_types_mympd);
    }
    if (buffer != NULL) {
        if (rc == true) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Sorted playlist succesfully");
        }
        else {
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Sorting playlist failed");
        }
    }
    return buffer;
}

bool mpd_shared_replace_playlist(struct t_mpd_state *mpd_state, const char *new_pl, const char *to_replace_pl, const char *backup_pl) {
    //rename original playlist to old playlist
    bool rc = mpd_run_rename(mpd_state->conn, to_replace_pl, backup_pl);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_rename") == false) {
        return false;
    }
    //rename new playlist to orginal playlist
    rc = mpd_run_rename(mpd_state->conn, new_pl, to_replace_pl);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_rename") == false) {
        //restore original playlist
        rc = mpd_run_rename(mpd_state->conn, backup_pl, to_replace_pl);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_rename");
        return false;
    }
    //delete old playlist
    rc = mpd_run_rm(mpd_state->conn, backup_pl);
    if (check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_rm") == false) {
        return false;
    }
    return true;
}

bool mpd_shared_smartpls_save(const char *workdir, const char *smartpltype, const char *playlist,
                              const char *expression, const int maxentries,
                              const int timerange, const char *sort)
{
    sds line = sdscatlen(sdsempty(), "{", 1);
    line = tojson_char(line, "type", smartpltype, true);
    if (strcmp(smartpltype, "sticker") == 0) {
        line = tojson_char(line, "sticker", expression, true);
        line = tojson_long(line, "maxentries", maxentries, true);
        line = tojson_long(line, "minvalue", timerange, true);
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        line = tojson_long(line, "timerange", timerange, true);
    }
    else if (strcmp(smartpltype, "search") == 0) {
        line = tojson_char(line, "expression", expression, true);
    }
    line = tojson_char(line, "sort", sort, false);
    line = sdscatlen(line, "}", 1);

    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", workdir, playlist);
    bool rc = write_data_to_file(pl_file, line, sdslen(line));

    FREE_SDS(line);
    FREE_SDS(pl_file);
    return rc;
}
