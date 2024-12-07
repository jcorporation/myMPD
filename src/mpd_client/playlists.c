/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Playlist helper functions.
 */

#include "compile_time.h"
#include "src/mpd_client/playlists.h"

#include "dist/rax/rax.h"
#include "src/lib/convert.h"
#include "src/lib/fields.h"
#include "src/lib/log.h"
#include "src/lib/random.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/lib/smartpls.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mpd_client/tags.h"

#include <stdbool.h>
#include <string.h>

/**
 * Private definitions
 */

static bool playlist_sort(struct t_partition_state *partition_state, const char *playlist, const char *tagstr, bool sortdesc, sds *error);
static bool replace_playlist(struct t_partition_state *partition_state, const char *new_pl,
        const char *to_replace_pl, sds *error);
static bool mpd_worker_playlist_content_enumerate_mpd(struct t_partition_state *partition_state, const char *plist,
        unsigned *count, unsigned *duration, sds *error);
static bool mpd_worker_playlist_content_enumerate_manual(struct t_partition_state *partition_state, const char *plist,
        unsigned *count, unsigned *duration, sds *error);

/**
 * Public functions
 */

/**
 * Returns the playlists last modification time
 * @param partition_state pointer to partition specific states
 * @param playlist name of the playlist to check
 * @return last modification time
 */
time_t mpd_client_get_playlist_mtime(struct t_partition_state *partition_state, const char *playlist) {
    time_t mtime = 0;
    if (mpd_send_list_playlists(partition_state->conn)) {
        struct mpd_playlist *pl;
        while ((pl = mpd_recv_playlist(partition_state->conn)) != NULL) {
            const char *plpath = mpd_playlist_get_path(pl);
            if (strcmp(plpath, playlist) == 0) {
                mtime = mpd_playlist_get_last_modified(pl);
                mpd_playlist_free(pl);
                break;
            }
            mpd_playlist_free(pl);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_playlists") == false) {
        return 0;
    }
    return mtime;
}

/**
 * Deduplicates all static playlists
 * @param partition_state pointer to partition state
 * @param remove true = remove duplicate songs, else count duplicate songs
 * @param error pointer to an already allocated sds string for the error message
 * @return -1 on error, else number of removed songs
 */
int64_t mpd_client_playlist_dedup_all(struct t_partition_state *partition_state, bool remove, sds *error) {
    //get all playlists excluding smartplaylists
    struct t_list plists;
    list_init(&plists);
    if (mpd_client_get_all_playlists(partition_state, &plists, false, error) == false) {
        list_clear(&plists);
        return -1;
    }
    int64_t result = 0;
    struct t_list_node *current;
    while ((current = list_shift_first(&plists)) != NULL) {
        int64_t rc = mpd_client_playlist_dedup(partition_state, current->key, remove, error);
        if (rc > -1) {
            result += rc;
        }
        list_node_free(current);
    }
    list_clear(&plists);
    return result;
}

/**
 * Deduplicates the playlist content
 * @param partition_state pointer to partition state
 * @param playlist playlist to check
 * @param remove true = remove duplicate songs, else count duplicate songs
 * @param error pointer to an already allocated sds string for the error message
 * @return -1 on error, else number of duplicate songs
 */
int64_t mpd_client_playlist_dedup(struct t_partition_state *partition_state, const char *playlist, bool remove, sds *error) {
    //get the whole playlist
    struct t_list duplicates;
    list_init(&duplicates);
    struct mpd_song *song;
    unsigned pos = 0;
    rax *plist = raxNew();
    if (mpd_send_list_playlist(partition_state->conn, playlist)) {
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            const char *uri = mpd_song_get_uri(song);
            if (raxTryInsert(plist, (unsigned char *)uri, strlen(uri), NULL, NULL) == 0) {
                //duplicate
                list_insert(&duplicates, uri, pos, NULL, NULL);
            }
            mpd_song_free(song);
            pos++;
        }
    }
    raxFree(plist);
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_send_list_playlist") == false) {
        list_clear(&duplicates);
        return -1;
    }

    int64_t rc = duplicates.length;
    if (remove == true) {
        struct t_list_node *current = duplicates.head;
        while (current != NULL) {
            mpd_run_playlist_delete(partition_state->conn, playlist, (unsigned)current->value_i);
            if (mympd_check_error_and_recover(partition_state, error, "mpd_run_playlist_delete") == false) {
                rc = -1;
                break;
            }
            MYMPD_LOG_WARN(MPD_PARTITION_DEFAULT, "Playlist \"%s\": duplicate entry \"%s\" removed", playlist, current->key);
            current = current -> next;
        }
    }
    list_clear(&duplicates);
    return rc;
}

/**
 * Validates all entries from all static playlists
 * @param partition_state pointer to partition state
 * @param remove true = remove invalid songs, else count invalid songs
 * @param error pointer to an already allocated sds string for the error message
 * @return -1 on error, else number of removed songs
 */
int mpd_client_playlist_validate_all(struct t_partition_state *partition_state, bool remove, sds *error) {
    //get all playlists excluding smartplaylists
    struct t_list plists;
    list_init(&plists);
    if (mpd_client_get_all_playlists(partition_state, &plists, false, error) == false) {
        list_clear(&plists);
        return -1;
    }
    int result = 0;
    struct t_list_node *current;
    while ((current = list_shift_first(&plists)) != NULL) {
        int rc = mpd_client_playlist_validate(partition_state, current->key, remove, error);
        if (rc > -1) {
            result += rc;
        }
        list_node_free(current);
    }
    list_clear(&plists);
    return result;
}

/**
 * Validates the playlist entries
 * @param partition_state pointer to partition state
 * @param playlist playlist to check
 * @param remove true = remove invalid songs, else count invalid songs
 * @param error pointer to an already allocated sds string for the error message
 * @return -1 on error, else number of removed songs
 */
int mpd_client_playlist_validate(struct t_partition_state *partition_state, const char *playlist, bool remove, sds *error) {
    //get the whole playlist
    struct t_list plist;
    list_init(&plist);
    struct mpd_song *song;
    unsigned pos = 0;
    if (mpd_send_list_playlist(partition_state->conn, playlist)) {
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            //reverse the playlist
            list_insert(&plist, mpd_song_get_uri(song), pos, NULL, NULL);
            mpd_song_free(song);
            pos++;
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_send_list_playlist") == false) {
        list_clear(&plist);
        return -1;
    }

    disable_all_mpd_tags(partition_state);
    //check each entry
    struct t_list_node *current = plist.head;
    int rc = 0;
    while (current != NULL) {
        if (is_streamuri(current->key) == false) {
            if (mpd_send_list_meta(partition_state->conn, current->key) == false ||
                mpd_response_finish(partition_state->conn) == false)
            {
                //entry not found
                //silently clear the error if song is not found
                if (mpd_connection_clear_error(partition_state->conn) == false ||
                    mpd_response_finish(partition_state->conn) == false)
                {
                    rc = -1;
                    break;
                }
                if (remove == true) {
                    mpd_run_playlist_delete(partition_state->conn, playlist, (unsigned)current->value_i);
                    if (mympd_check_error_and_recover(partition_state, error, "mpd_run_playlist_delete") == false) {
                        rc = -1;
                        break;
                    }
                    MYMPD_LOG_WARN(MPD_PARTITION_DEFAULT, "Playlist \"%s\": %s removed", playlist, current->key);
                }
                else {
                    MYMPD_LOG_WARN(MPD_PARTITION_DEFAULT, "Playlist \"%s\": %s not found", playlist, current->key);
                }
                rc++;
            }
        }
        current = current -> next;
    }
    list_clear(&plist);
    enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
    return rc;
}

/**
 * Shuffles a playlist
 * @param partition_state pointer to partition specific states
 * @param playlist playlist to shuffle
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success else false
 */
bool mpd_client_playlist_shuffle(struct t_partition_state *partition_state, const char *playlist, sds *error) {
    MYMPD_LOG_INFO(partition_state->name, "Shuffling playlist %s", playlist);
    struct t_list plist;
    list_init(&plist);
    struct mpd_song *song;
    if (mpd_send_list_playlist(partition_state->conn, playlist)) {
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            list_push(&plist, mpd_song_get_uri(song), 0, NULL, NULL);
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_send_list_playlist") == false ||
        list_shuffle(&plist) == false)
    {
        list_clear(&plist);
        return false;
    }

    char rand_str[10];
    randstring(rand_str, 10);
    sds playlist_tmp = sdscatfmt(sdsempty(), "%s-tmp-%s", rand_str, playlist);

    //add shuffled songs to tmp playlist
    //uses command list to add MPD_COMMANDS_MAX songs at once
    bool rc = true;
    while (plist.length > 0) {
        if (mpd_command_list_begin(partition_state->conn, false) == true) {
            unsigned j = 0;
            struct t_list_node *current;
            while ((current = list_shift_first(&plist)) != NULL) {
                j++;
                rc = mpd_send_playlist_add(partition_state->conn, playlist_tmp, current->key);
                list_node_free(current);
                if (rc == false) {
                    mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_COMMANDS_MAX) {
                    break;
                }
            }
            mpd_client_command_list_end_check(partition_state);
        }
        mpd_response_finish(partition_state->conn);
        if (mympd_check_error_and_recover(partition_state, error, "mpd_send_playlist_add") == false) {
            //error adding songs to tmp playlist - delete it
            mpd_run_rm(partition_state->conn, playlist_tmp);
            mympd_check_error_and_recover(partition_state, error, "mpd_run_rm");
            rc = false;
            break;
        }
    }
    list_clear(&plist);
    if (rc == true) {
        rc = replace_playlist(partition_state, playlist_tmp, playlist, error);
    }
    FREE_SDS(playlist_tmp);
    return rc;
}

/**
 * Sorts a playlist.
 * Wrapper for playlist_sort that enables the mympd tags afterwards
 * @param partition_state pointer to partition specific states
 * @param playlist playlist to shuffle
 * @param tagstr mpd tag to sort by
 * @param sortdesc sort descending?
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success else false
 */
bool mpd_client_playlist_sort(struct t_partition_state *partition_state, const char *playlist, const char *tagstr, bool sortdesc, sds *error) {
    bool rc = playlist_sort(partition_state, playlist, tagstr, sortdesc, error);
    enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
    return rc;
}

/**
 * Counts the number of songs in the playlist
 * @param partition_state pointer to partition specific states
 * @param plist playlist to enumerate
 * @param count pointer to unsigned for entity count
 * @param duration pointer to unsigned for total playtime
 * @param error pointer to an already allocated sds string for the error message
 * @return number of songs or -1 on error
 */
bool mpd_client_enum_playlist(struct t_partition_state *partition_state, const char *plist,
        unsigned *count, unsigned *duration, sds *error)
{
    return partition_state->mpd_state->feat.mpd_0_24_0 == true
        ? mpd_worker_playlist_content_enumerate_mpd(partition_state, plist, count, duration, error)
        : mpd_worker_playlist_content_enumerate_manual(partition_state, plist, count, duration, error);
}

/**
 * Enumerates the playlist and returns the count and total length
 * This functions uses the playlistlength command of MPD 0.24
 * @param partition_state pointer to partition state
 * @param plist playlist name to enumerate
 * @param count pointer to unsigned for entity count
 * @param duration pointer to unsigned for total playtime
 * @param error pointer to an already allocated sds string for the error message
 * @return pointer to buffer 
 */
static bool mpd_worker_playlist_content_enumerate_mpd(struct t_partition_state *partition_state, const char *plist,
        unsigned *count, unsigned *duration, sds *error)
{
    *count = 0;
    *duration = 0;
    if (mpd_send_playlistlength(partition_state->conn, plist)) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
            if (strcmp(pair->name, "songs") == 0) {
                str2uint(count, pair->value);
            }
            else if (strcmp(pair->name, "playtime") == 0) {
                str2uint(duration, pair->value);
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_playlistlength");
}

/**
 * Enumerates the playlist and returns the count and total length.
 * This functions retrieves the complete playlist.
 * @param partition_state pointer to partition state
 * @param plist playlist name to enumerate
 * @param count pointer to unsigned for entity count
 * @param duration pointer to unsigned for total playtime
 * @param error pointer to an already allocated sds string for the error message
 * @return pointer to buffer 
 */
static bool mpd_worker_playlist_content_enumerate_manual(struct t_partition_state *partition_state, const char *plist,
        unsigned *count, unsigned *duration, sds *error)
{
    unsigned entity_count = 0;
    unsigned total_time = 0;
    disable_all_mpd_tags(partition_state);
    if (mpd_send_list_playlist_meta(partition_state->conn, plist)) {
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            total_time += mpd_song_get_duration(song);
            entity_count++;
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    *count = entity_count;
    *duration = total_time;
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_list_playlist_meta") &&
        enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
}

/**
 * Crops a playlist
 * @param partition_state Pointer to partition specific states
 * @param plist Playlist name
 * @param num_entries May number of songs
 * @return true on success, else false
 */
bool mpd_client_playlist_crop(struct t_partition_state *partition_state, const char *plist, unsigned num_entries) {
    unsigned duration;
    unsigned count;
    if (mpd_client_enum_playlist(partition_state, plist, &count, &duration, NULL) == true) {
        if (count > num_entries) {
            return mpd_run_playlist_delete_range(partition_state->conn, plist, num_entries, UINT_MAX);
        }
        return true;
    }
    return false;
}

/**
 * Clears a playlist
 * @param partition_state pointer to partition specific states
 * @param plist playlist name
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mpd_client_playlist_clear(struct t_partition_state *partition_state, const char *plist, sds *error) {
    mpd_run_playlist_clear(partition_state->conn, plist);
    return mympd_check_error_and_recover(partition_state, error, "mpd_run_playlist_clear");
}

/**
 * Gets all playlists.
 * @param partition_state pointer to partition state
 * @param l pointer to list to populate
 * @param smartpls true = integrate smart playlists, false = ignore smart playlists
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mpd_client_get_all_playlists(struct t_partition_state *partition_state, struct t_list *l, bool smartpls, sds *error) {
    if (mpd_send_list_playlists(partition_state->conn)) {
        struct mpd_playlist *pl;
        while ((pl = mpd_recv_playlist(partition_state->conn)) != NULL) {
            const char *plpath = mpd_playlist_get_path(pl);
            bool sp = is_smartpls(partition_state->config->workdir, plpath);
            if (!(smartpls == false && sp == true)) {
                list_push(l, plpath, sp, NULL, NULL);
            }
            mpd_playlist_free(pl);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_send_list_playlists") == false) {
        return false;
    }
    return true;
}

/**
 * Parses the provided string to the playlist_sort_type
 * @param str String to parse
 * @return enum playlist_sort_types or PLSORT_UNKNOWN on error
 */
enum playlist_sort_types playlist_parse_sort(const char *str) {
    if (strcmp(str, "Name") == 0) { return PLSORT_NAME; }
    if (strcmp(str, "Last-Modified") == 0) { return PLSORT_LAST_MODIFIED; }
    return PLSORT_UNKNOWN;
}

/**
 * Private functions
 */

/**
 * Sorts a playlist.
 * @param partition_state pointer to partition specific states
 * @param playlist playlist to shuffle
 * @param tagstr mpd tag to sort by
 * @param sortdesc sort descending?
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success else false
 */
static bool playlist_sort(struct t_partition_state *partition_state, const char *playlist, const char *tagstr, bool sortdesc, sds *error) {
    struct t_mpd_tags sort_tags = {
        .len = 1,
        .tags[0] = mpd_tag_name_parse(tagstr)
    };
    enum sort_by_type sort_by = SORT_BY_TAG;
    bool rc = false;
    if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
        enable_mpd_tags(partition_state, &sort_tags);
        rc = mpd_send_list_playlist_meta(partition_state->conn, playlist);
    }
    else if (strcmp(tagstr, "filename") == 0) {
        rc = mpd_send_list_playlist(partition_state->conn, playlist);
        sort_by = SORT_BY_FILENAME;
    }
    else if (strcmp(tagstr, "Last-Modified") == 0) {
        rc = mpd_send_list_playlist(partition_state->conn, playlist);
        sort_by = SORT_BY_LAST_MODIFIED;
        //swap sort direction
        sortdesc = sortdesc == true
            ? false
            : true;
    }
    else if (strcmp(tagstr, "Added") == 0) {
        rc = mpd_send_list_playlist(partition_state->conn, playlist);
        sort_by = SORT_BY_ADDED;
        //swap sort direction
        sortdesc = sortdesc == true
            ? false
            : true;
    }
    else {
        MYMPD_LOG_ERROR(partition_state->name, "Invalid sort tag: %s", tagstr);
        return false;
    }
    MYMPD_LOG_INFO(partition_state->name, "Sorting playlist \"%s\" by tag \"%s\"", playlist, tagstr);

    rax *plist = raxNew();
    if (rc == true) {
        sds key = sdsempty();
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            key = get_sort_key(key, sort_by, sort_tags.tags[0], song);
            sds data = sdsnew(mpd_song_get_uri(song));
            rax_insert_no_dup(plist, key, data);
            mpd_song_free(song);
            sdsclear(key);
        }
        FREE_SDS(key);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_send_list_playlist") == false) {
        //free data
        rax_free_sds_data(plist);
        return false;
    }

    char rand_str[10];
    randstring(rand_str, 10);
    sds playlist_tmp = sdscatfmt(sdsempty(), "%s-tmp-%s", rand_str, playlist);

    //add sorted songs to tmp playlist
    //uses command list to add MPD_COMMANDS_MAX songs at once
    unsigned i = 0;
    raxIterator iter;
    raxStart(&iter, plist);
    int (*iterator)(struct raxIterator *iter);
    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
        iterator = &raxNext;
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
        iterator = &raxPrev;
    }
    rc = true;
    while (i < plist->numele) {
        if (mpd_command_list_begin(partition_state->conn, false)) {
            unsigned j = 0;
            while (iterator(&iter)) {
                i++;
                j++;
                if (mpd_send_playlist_add(partition_state->conn, playlist_tmp, iter.data) == false) {
                    mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_COMMANDS_MAX) {
                    break;
                }
            }
            mpd_client_command_list_end_check(partition_state);
        }
        mpd_response_finish(partition_state->conn);
        if (mympd_check_error_and_recover(partition_state, error, "mpd_send_playlist_add") == false) {
            //error adding songs to tmp playlist - delete it
            mpd_run_rm(partition_state->conn, playlist_tmp);
            mympd_check_error_and_recover(partition_state, error, "mpd_run_rm");
            rc = false;
            break;
        }
    }
    raxStop(&iter);
    rax_free_sds_data(plist);
    if (rc == true) {
        rc = replace_playlist(partition_state, playlist_tmp, playlist, error);
    }
    FREE_SDS(playlist_tmp);
    return rc;
}

/**
 * Safely replaces a playlist with a new one
 * @param partition_state pointer to partition specific states
 * @param new_pl name of the new playlist to bring in place
 * @param to_replace_pl name of the playlist to replace
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
static bool replace_playlist(struct t_partition_state *partition_state, const char *new_pl,
    const char *to_replace_pl, sds *error)
{
    sds backup_pl = sdscatfmt(sdsempty(), "%s.bak", new_pl);
    //rename original playlist to old playlist
    mpd_run_rename(partition_state->conn, to_replace_pl, backup_pl);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_run_rename") == false) {
        FREE_SDS(backup_pl);
        return false;
    }
    //rename new playlist to orginal playlist
    mpd_run_rename(partition_state->conn, new_pl, to_replace_pl);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_run_rename") == false) {
        //restore original playlist
        mpd_run_rename(partition_state->conn, backup_pl, to_replace_pl);
        mympd_check_error_and_recover(partition_state, error, "mpd_run_rename");
        FREE_SDS(backup_pl);
        return false;
    }
    //delete old playlist
    mpd_run_rm(partition_state->conn, backup_pl);
    FREE_SDS(backup_pl);
    return mympd_check_error_and_recover(partition_state, error, "mpd_run_rename");
}
