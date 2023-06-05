/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "dist/rax/rax.h"
#include "mpd/playlist.h"
#include "mpd/queue.h"
#include "mpd/response.h"
#include "src/lib/list.h"
#include "src/lib/utility.h"
#include "src/mpd_client/playlists.h"

#include "src/lib/log.h"
#include "src/lib/random.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mpd_client/tags.h"

#include <string.h>

/**
 * Private definitions
 */

static bool playlist_sort(struct t_partition_state *partition_state, const char *playlist, const char *tagstr);
static bool replace_playlist(struct t_partition_state *partition_state, const char *new_pl,
        const char *to_replace_pl);

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
 * Deduplicates the playlist content
 * @param partition_state pointer to partition state
 * @param playlist playlist to check
 * @return -1 on error, else number of duplicate songs
 */
long mpd_client_playlist_dedup(struct t_partition_state *partition_state, const char *playlist, bool remove) {
    //get the whole playlist
    struct t_list duplicates;
    list_init(&duplicates);
    struct mpd_song *song;
    long pos = 0;
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
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_playlist") == false) {
        list_clear(&duplicates);
        return -1;
    }

    long rc = duplicates.length;
    if (remove == true) {
        struct t_list_node *current = duplicates.head;
        while (current != NULL) {
            if (mpd_run_playlist_delete(partition_state->conn, playlist, (unsigned)current->value_i) == false ||
                mympd_check_error_and_recover(partition_state, NULL, "mpd_run_playlist_delete") == false)
            {
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
 * Validates the playlist entries
 * @param partition_state pointer to partition state
 * @param playlist playlist to check
 * @return -1 on error, else number of removed songs
 */
long mpd_client_playlist_validate(struct t_partition_state *partition_state, const char *playlist, bool remove) {
    //get the whole playlist
    struct t_list plist;
    list_init(&plist);
    struct mpd_song *song;
    long pos = 0;
    if (mpd_send_list_playlist(partition_state->conn, playlist)) {
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            //reverse the playlist
            list_insert(&plist, mpd_song_get_uri(song), pos, NULL, NULL);
            mpd_song_free(song);
            pos++;
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_playlist") == false) {
        list_clear(&plist);
        return -1;
    }

    disable_all_mpd_tags(partition_state);
    //check each entry
    struct t_list_node *current = plist.head;
    long rc = 0;
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

                    if (mpd_run_playlist_delete(partition_state->conn, playlist, (unsigned)current->value_i) == false ||
                        mympd_check_error_and_recover(partition_state, NULL, "mpd_run_playlist_delete") == false)
                    {
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
 * @return true on success else false
 */
bool mpd_client_playlist_shuffle(struct t_partition_state *partition_state, const char *playlist) {
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
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_playlist") == false ||
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
    bool rc = true;
    while (i < plist.length) {
        if (mpd_command_list_begin(partition_state->conn, false) == true) {
            long j = 0;
            struct t_list_node *current;
            while ((current = list_shift_first(&plist)) != NULL) {
                i++;
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
        if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_playlist_add") == false) {
            //error adding songs to tmp playlist - delete it
            mpd_run_rm(partition_state->conn, playlist_tmp);
            mympd_check_error_and_recover(partition_state, NULL, "mpd_run_rm");
            rc = false;
            break;
        }
    }
    list_clear(&plist);
    if (rc == true) {
        rc = replace_playlist(partition_state, playlist_tmp, playlist);
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
 * @return true on success else false
 */
bool mpd_client_playlist_sort(struct t_partition_state *partition_state, const char *playlist, const char *tagstr) {
    bool rc = playlist_sort(partition_state, playlist, tagstr);
    enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
    return rc;
}

/**
 * Counts the number of songs in the playlist
 * @param partition_state pointer to partition specific states
 * @param playlist playlist to enumerate
 * @param empty_check true: checks only if playlist is not empty
 *                    false: enumerates the complete playlist
 * @return number of songs or -1 on error
 */
long mpd_client_enum_playlist(struct t_partition_state *partition_state, const char *playlist, bool empty_check) {
    int entity_count = 0;
    if (mpd_send_list_playlist(partition_state->conn, playlist)) {
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            entity_count++;
            mpd_song_free(song);
            if (empty_check == true) {
                break;
            }
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_playlist") == false) {
        return -1;
    }
    return entity_count;
}

/**
 * Private functions
 */

/**
 * Sorts a playlist.
 * @param partition_state pointer to partition specific states
 * @param playlist playlist to shuffle
 * @param tagstr mpd tag to sort by
 * @return true on success else false
 */
static bool playlist_sort(struct t_partition_state *partition_state, const char *playlist, const char *tagstr) {
    struct t_tags sort_tags = {
        .len = 1,
        .tags[0] = mpd_tag_name_parse(tagstr)
    };

    bool rc = false;

    if (strcmp(tagstr, "filename") == 0) {
        MYMPD_LOG_INFO(partition_state->name, "Sorting playlist \"%s\" by filename", playlist);
        rc = mpd_send_list_playlist(partition_state->conn, playlist);
    }
    else if (sort_tags.tags[0] != MPD_TAG_UNKNOWN) {
        MYMPD_LOG_INFO(partition_state->name, "Sorting playlist \"%s\" by tag \"%s\"", playlist, tagstr);
        enable_mpd_tags(partition_state, &sort_tags);
        rc = mpd_send_list_playlist_meta(partition_state->conn, playlist);
    }
    else {
        return false;
    }

    rax *plist = raxNew();
    if (rc == true) {
        sds key = sdsempty();
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
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
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_playlist") == false) {
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
        if (mpd_command_list_begin(partition_state->conn, false)) {
            long j = 0;
            while (raxNext(&iter)) {
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
        if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_playlist_add") == false) {
            //error adding songs to tmp playlist - delete it
            mpd_run_rm(partition_state->conn, playlist_tmp);
            mympd_check_error_and_recover(partition_state, NULL, "mpd_run_rm");
            rc = false;
            break;
        }
    }
    raxStop(&iter);
    rax_free_sds_data(plist);
    if (rc == true) {
        rc = replace_playlist(partition_state, playlist_tmp, playlist);    
    }
    FREE_SDS(playlist_tmp);
    return rc;
}

/**
 * Safely replaces a playlist with a new one
 * @param partition_state pointer to partition specific states
 * @param new_pl name of the new playlist to bring in place
 * @param to_replace_pl name of the playlist to replace
 * @return true 
 * @return false 
 */
static bool replace_playlist(struct t_partition_state *partition_state, const char *new_pl,
    const char *to_replace_pl)
{
    sds backup_pl = sdscatfmt(sdsempty(), "%s.bak", new_pl);
    //rename original playlist to old playlist
    mpd_run_rename(partition_state->conn, to_replace_pl, backup_pl);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_rename") == false) {
        FREE_SDS(backup_pl);
        return false;
    }
    //rename new playlist to orginal playlist
    mpd_run_rename(partition_state->conn, new_pl, to_replace_pl);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_rename") == false) {
        //restore original playlist
        mpd_run_rename(partition_state->conn, backup_pl, to_replace_pl);
        mympd_check_error_and_recover(partition_state, NULL, "mpd_run_rename");
        FREE_SDS(backup_pl);
        return false;
    }
    //delete old playlist
    mpd_run_rm(partition_state->conn, backup_pl);
    FREE_SDS(backup_pl);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_rename");
}
