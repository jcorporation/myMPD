/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_playlists.h"

#include "../lib/filehandler.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/random.h"
#include "../lib/rax_extras.h"
#include "../lib/sds_extras.h"
#include "../lib/validate.h"
#include "mpd_client_errorhandler.h"
#include "mpd_client_tags.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//private definitions

static bool _mpd_client_playlist_sort(struct t_mpd_state *mpd_state, const char *playlist, const char *tagstr);
static bool _mpd_client_replace_playlist(struct t_mpd_state *mpd_state, const char *new_pl,
        const char *to_replace_pl);

//public functions

/**
 * Saves the smart playlist to disk.
 * @param workdir myMPD working directory
 * @param smartpltype type of the smart playlist: sticker, newest or search
 * @param playlist name of the smart playlist
 * @param expression mpd search expression
 * @param maxentries max entries for the playlist
 * @param timerange timerange for newest smart playlist type
 * @param sort mpd tag to sort or shuffle
 * @return true on success else false
 */
bool mpd_client_smartpls_save(sds workdir, const char *smartpltype, const char *playlist,
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

    sds pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
    bool rc = write_data_to_file(pl_file, line, sdslen(line));
    FREE_SDS(line);
    FREE_SDS(pl_file);
    return rc;
}

/**
 * Checks if playlist is a smart playlist
 * @param workdir myMPD working directory
 * @param playlist name of the playlist to check
 * @return true if it is a smart playlist else false
 */
bool is_smartpls(sds workdir, const char *playlist) {
    bool smartpls = false;
    if (strchr(playlist, '/') == NULL) {
        //filename only
        sds smartpls_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
        if (access(smartpls_file, F_OK ) != -1) { /* Flawfinder: ignore */
            smartpls = true;
        }
        FREE_SDS(smartpls_file);
    }
    return smartpls;
}

/**
 * Returns the mpd database last modification time
 * @param mpd_state pointer to struct mpd_state
 * @return last modification time
 */
time_t mpd_client_get_db_mtime(struct t_mpd_state *mpd_state) {
    struct mpd_stats *stats = mpd_run_stats(mpd_state->conn);
    if (stats == NULL) {
        mympd_check_error_and_recover(mpd_state);
        return 0;
    }
    time_t mtime = (time_t)mpd_stats_get_db_update_time(stats);
    mpd_stats_free(stats);
    return mtime;
}

/**
 * Returns the samrt playlist last modification time
 * @param workdir myMPD working directory
 * @param playlist name of the playlist to check
 * @return last modification time
 */
time_t mpd_client_get_smartpls_mtime(sds workdir, const char *playlist) {
    sds plpath = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
    struct stat attr;
    errno = 0;
    if (stat(plpath, &attr) != 0) {
        MYMPD_LOG_ERROR("Error getting mtime for \"%s\"", plpath);
        MYMPD_LOG_ERRNO(errno);
        attr.st_mtime = 0;
    }
    FREE_SDS(plpath);
    return attr.st_mtime;
}

/**
 * Returns the playlists last modification time
 * @param mpd_state pointer to struct mpd_state
 * @param playlist name of the playlist to check
 * @return last modification time
 */
time_t mpd_client_get_playlist_mtime(struct t_mpd_state *mpd_state, const char *playlist) {
    bool rc = mpd_send_list_playlists(mpd_state->conn);
    if (mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_send_list_playlists") == false) {
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
    if (mympd_check_error_and_recover(mpd_state) == false) {
        return 0;
    }

    return mtime;
}

/**
 * Shuffles a playlist
 * @param mpd_state pointer to struct mpd_state
 * @param playlist playlist to shuffle
 * @return true on success else false
 */
bool mpd_client_playlist_shuffle(struct t_mpd_state *mpd_state, const char *playlist) {
    MYMPD_LOG_INFO("Shuffling playlist %s", playlist);
    bool rc = mpd_send_list_playlist(mpd_state->conn, playlist);
    if (mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_send_list_playlist") == false) {
        return false;
    }

    struct t_list plist;
    list_init(&plist);
    struct mpd_song *song;
    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        list_push(&plist, mpd_song_get_uri(song), 0, NULL, NULL);
        mpd_song_free(song);
    }
    mpd_response_finish(mpd_state->conn);
    if (mympd_check_error_and_recover(mpd_state) == false ||
        list_shuffle(&plist) == false)
    {
        list_clear(&plist);
        return false;
    }

    long randnr = randrange(100000, 999999);
    sds playlist_tmp = sdscatfmt(sdsempty(), "%l-tmp-%s", randnr, playlist);

    //add shuffled songs to tmp playlist
    //uses command list to add MPD_COMMANDS_MAX songs at once
    long i = 0;
    while (i < plist.length) {
        if (mpd_command_list_begin(mpd_state->conn, false) == true) {
            long j = 0;
            struct t_list_node *current;
            while ((current = list_shift_first(&plist)) != NULL) {
                i++;
                j++;
                rc = mpd_send_playlist_add(mpd_state->conn, playlist_tmp, current->key);
                list_node_free(current);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_COMMANDS_MAX) {
                    break;
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        if (mympd_check_error_and_recover(mpd_state) == false) {
            //error adding songs to tmp playlist - delete it
            rc = mpd_run_rm(mpd_state->conn, playlist_tmp);
            mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_rm");
            rc = false;
            break;
        }
    }
    list_clear(&plist);
    if (rc == true) {
        rc = _mpd_client_replace_playlist(mpd_state, playlist_tmp, playlist);
    }
    FREE_SDS(playlist_tmp);
    return rc;
}

/**
 * Sorts a playlist.
 * Wrapper for _mpd_client_playlist_sort that enables the mympd tags afterwards
 * @param mpd_state pointer to struct mpd_state
 * @param playlist playlist to shuffle
 * @param tagstr mpd tag to sort by
 * @return true on success else false
 */
bool mpd_client_playlist_sort(struct t_mpd_state *mpd_state, const char *playlist, const char *tagstr) {
    bool rc = _mpd_client_playlist_sort(mpd_state, playlist, tagstr);
    enable_mpd_tags(mpd_state, &mpd_state->tag_types_mympd);
    return rc;
}

//private functions

/**
 * Sorts a playlist.
 * @param mpd_state pointer to struct mpd_state
 * @param playlist playlist to shuffle
 * @param tagstr mpd tag to sort by
 * @return true on success else false
 */
static bool _mpd_client_playlist_sort(struct t_mpd_state *mpd_state, const char *playlist, const char *tagstr) {
    struct t_tags sort_tags = {
        .len = 1,
        .tags[0] = mpd_tag_name_parse(tagstr)
    };

    bool rc = false;

    if (strcmp(tagstr, "filename") == 0) {
        MYMPD_LOG_INFO("Sorting playlist \"%s\" by filename", playlist);
        rc = mpd_send_list_playlist(mpd_state->conn, playlist);
    }
    else if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
        MYMPD_LOG_INFO("Sorting playlist \"%s\" by tag \"%s\"", playlist, tagstr);
        enable_mpd_tags(mpd_state, &sort_tags);
        rc = mpd_send_list_playlist_meta(mpd_state->conn, playlist);
    }
    else {
        return false;
    }
    if (mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_send_list_playlist") == false) {
        return false;
    }

    rax *plist = raxNew();
    sds key = sdsempty();
    struct mpd_song *song;
    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        const char *song_uri = mpd_song_get_uri(song);
        sdsclear(key);
        if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
            //sort by tag
            key = mpd_client_get_tag_value_string(song, sort_tags.tags[0], key);
            key = sdscatfmt(key, "::%s", song_uri);
        }
        else {
            //sort by filename
            key = sdscat(key, song_uri);
        }
        sds_utf8_tolower(key);
        sds data = sdsnew(song_uri);
        while (raxTryInsert(plist, (unsigned char *)key, sdslen(key), data, NULL) == 0) {
            //duplicate - add chars until it is uniq
            key = sdscatlen(key, ":", 1);
        }
        mpd_song_free(song);
    }
    FREE_SDS(key);
    mpd_response_finish(mpd_state->conn);
    if (mympd_check_error_and_recover(mpd_state) == false) {
        //free data
        rax_free_sds_data(plist);
        return false;
    }

    long randnr = randrange(100000, 999999);
    sds playlist_tmp = sdscatfmt(sdsempty(), "%l-tmp-%s", randnr, playlist);

    //add sorted songs to tmp playlist
    //uses command list to add MPD_COMMANDS_MAX songs at once
    unsigned i = 0;
    raxIterator iter;
    raxStart(&iter, plist);
    raxSeek(&iter, "^", NULL, 0);
    rc = true;
    while (i < plist->numele) {
        if (mpd_command_list_begin(mpd_state->conn, false) == true) {
            long j = 0;
            while (raxNext(&iter)) {
                i++;
                j++;
                rc = mpd_send_playlist_add(mpd_state->conn, playlist_tmp, iter.data);
                FREE_SDS(iter.data);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_COMMANDS_MAX) {
                    break;
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        if (mympd_check_error_and_recover(mpd_state) == false) {
            //error adding songs to tmp playlist - delete it
            rc = mpd_run_rm(mpd_state->conn, playlist_tmp);
            mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_rm");
            rc = false;
            break;
        }
    }
    raxStop(&iter);
    rax_free_sds_data(plist);
    if (rc == true) {
        rc = _mpd_client_replace_playlist(mpd_state, playlist_tmp, playlist);    
    }
    FREE_SDS(playlist_tmp);
    return rc;
}

/**
 * Safely replaces a playlist with a new one
 * @param mpd_state pointer to struct mpd_state
 * @param new_pl name of the new playlist to bring in place
 * @param to_replace_pl name of the playlist to replace
 * @param backup_pl name of
 * @return true 
 * @return false 
 */
static bool _mpd_client_replace_playlist(struct t_mpd_state *mpd_state, const char *new_pl,
    const char *to_replace_pl)
{
    sds backup_pl = sdscatfmt(sdsempty(), "%s.bak", new_pl);
    //rename original playlist to old playlist
    bool rc = mpd_run_rename(mpd_state->conn, to_replace_pl, backup_pl);
    if (mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_rename") == false) {
        FREE_SDS(backup_pl);
        return false;
    }
    //rename new playlist to orginal playlist
    rc = mpd_run_rename(mpd_state->conn, new_pl, to_replace_pl);
    if (mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_rename") == false) {
        //restore original playlist
        rc = mpd_run_rename(mpd_state->conn, backup_pl, to_replace_pl);
        mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_rename");
        FREE_SDS(backup_pl);
        return false;
    }
    //delete old playlist
    rc = mpd_run_rm(mpd_state->conn, backup_pl);
    FREE_SDS(backup_pl);
    return mympd_check_rc_error_and_recover(mpd_state, rc, "mpd_run_rename");
}
