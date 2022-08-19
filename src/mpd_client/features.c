/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "features.h"

#include "../lib/api.h"
#include "../lib/filehandler.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_client/tags.h"
#include "../mympd_api/status.h"
#include "errorhandler.h"

#include <stdlib.h>
#include <string.h>

/**
 * Private definitions
 */

static void mpd_client_feature_commands(struct t_mympd_state *mympd_state);
static void mpd_client_feature_mpd_tags(struct t_mympd_state *mympd_state);
static void mpd_client_feature_tags(struct t_mympd_state *mympd_state);
static void mpd_client_feature_music_directory(struct t_mympd_state *mympd_state);

/**
 * Public functions
 */

/**
 * Detects MPD features and disables/enables myMPD features accordingly
 * @param mympd_state pointer to mympd_state struct
 */
void mpd_client_mpd_features(struct t_mympd_state *mympd_state) {
    mympd_state->mpd_state->protocol = mpd_connection_get_server_version(mympd_state->partition_state->conn);
    MYMPD_LOG_NOTICE("MPD protocol version: %u.%u.%u",
        mympd_state->mpd_state->protocol[0],
        mympd_state->mpd_state->protocol[1],
        mympd_state->mpd_state->protocol[2]
    );

    //first disable all features
    mpd_state_features_disable(mympd_state->mpd_state);

    //get features
    mpd_client_feature_commands(mympd_state);
    mpd_client_feature_music_directory(mympd_state);
    mpd_client_feature_tags(mympd_state);

    //set state
    sds buffer = sdsempty();
    buffer = mympd_api_status_get(mympd_state->partition_state, buffer, 0);
    FREE_SDS(buffer);

    if (mpd_connection_cmp_server_version(mympd_state->partition_state->conn, 0, 23, 8) >= 0) {
        mympd_state->mpd_state->feat_partitions = true;
        MYMPD_LOG_NOTICE("Enabling partitions feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling partitions feature, depends on mpd >= 0.23.8");
    }

    if (mpd_connection_cmp_server_version(mympd_state->partition_state->conn, 0, 22, 4) >= 0 ) {
        mympd_state->mpd_state->feat_binarylimit = true;
        MYMPD_LOG_NOTICE("Enabling binarylimit feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling binarylimit feature, depends on mpd >= 0.22.4");
    }

    if (mpd_connection_cmp_server_version(mympd_state->partition_state->conn, 0, 23, 3) >= 0 ) {
        mympd_state->mpd_state->feat_playlist_rm_range = true;
        MYMPD_LOG_NOTICE("Enabling delete playlist range feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling delete playlist range feature, depends on mpd >= 0.23.3");
    }

    if (mpd_connection_cmp_server_version(mympd_state->partition_state->conn, 0, 23, 5) >= 0 ) {
        mympd_state->mpd_state->feat_whence = true;
        MYMPD_LOG_NOTICE("Enabling position whence feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling position whence feature, depends on mpd >= 0.23.5");
    }

    if (mpd_connection_cmp_server_version(mympd_state->partition_state->conn, 0, 24, 0) >= 0 ) {
        mympd_state->mpd_state->feat_advqueue = true;
        MYMPD_LOG_NOTICE("Enabling advanced queue feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling advanced queue feature, depends on mpd >= 0.24.0");
    }

    //push settings to web_server_queue
    struct set_mg_user_data_request *extra = malloc_assert(sizeof(struct set_mg_user_data_request));
    extra->music_directory = sdsdup(mympd_state->mpd_state->music_directory_value);
    extra->playlist_directory = sdsdup(mympd_state->playlist_directory);
    extra->coverimage_names = sdsdup(mympd_state->coverimage_names);
    extra->thumbnail_names = sdsdup(mympd_state->thumbnail_names);
    extra->feat_albumart = mympd_state->mpd_state->feat_albumart;
    extra->mpd_stream_port = mympd_state->mpd_stream_port;
    extra->mpd_host = sdsdup(mympd_state->mpd_state->mpd_host);

    struct t_work_response *web_server_response = create_response_new(-1, 0, INTERNAL_API_WEBSERVER_SETTINGS, MPD_PARTITION_DEFAULT);
    web_server_response->extra = extra;
    mympd_queue_push(web_server_queue, web_server_response, 0);
}

/**
 * Private functions
 */

/**
 * Looks for allowed MPD command
 * @param mympd_state pointer to mympd_state struct
 */
static void mpd_client_feature_commands(struct t_mympd_state *mympd_state) {
    if (mpd_send_allowed_commands(mympd_state->partition_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(mympd_state->partition_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                MYMPD_LOG_DEBUG("MPD supports stickers");
                mympd_state->mpd_state->feat_stickers = true;
            }
            else if (strcmp(pair->value, "listplaylists") == 0) {
                MYMPD_LOG_DEBUG("MPD supports playlists");
                mympd_state->mpd_state->feat_playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                MYMPD_LOG_DEBUG("MPD supports fingerprint");
                mympd_state->mpd_state->feat_fingerprint = true;
            }
            else if (strcmp(pair->value, "albumart") == 0) {
                MYMPD_LOG_DEBUG("MPD supports albumart");
                mympd_state->mpd_state->feat_albumart = true;
            }
            else if (strcmp(pair->value, "readpicture") == 0) {
                MYMPD_LOG_DEBUG("MPD supports readpicture");
                mympd_state->mpd_state->feat_readpicture = true;
            }
            else if (strcmp(pair->value, "mount") == 0) {
                MYMPD_LOG_DEBUG("MPD supports mounts");
                mympd_state->mpd_state->feat_mount = true;
            }
            else if (strcmp(pair->value, "listneighbors") == 0) {
                MYMPD_LOG_DEBUG("MPD supports neighbors");
                mympd_state->mpd_state->feat_neighbor = true;
            }
            mpd_return_pair(mympd_state->partition_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_allowed_commands");
    }
    mpd_response_finish(mympd_state->partition_state->conn);
    mympd_check_error_and_recover(mympd_state->partition_state);
}

/**
 * Sets enabled tags for myMPD
 * @param mympd_state pointer to mympd_state struct
 */
static void mpd_client_feature_tags(struct t_mympd_state *mympd_state) {
    reset_t_tags(&mympd_state->mpd_state->tags_search);
    reset_t_tags(&mympd_state->mpd_state->tags_browse);
    reset_t_tags(&mympd_state->smartpls_generate_tag_types);

    mpd_client_feature_mpd_tags(mympd_state);

    if (mympd_state->mpd_state->feat_tags == true) {
        check_tags(mympd_state->tag_list_search, "tag_list_search",
            &mympd_state->mpd_state->tags_search, &mympd_state->mpd_state->tags_mympd);
        check_tags(mympd_state->tag_list_browse, "tag_list_browse",
            &mympd_state->mpd_state->tags_browse, &mympd_state->mpd_state->tags_mympd);
        check_tags(mympd_state->smartpls_generate_tag_list, "smartpls_generate_tag_list",
            &mympd_state->smartpls_generate_tag_types, &mympd_state->mpd_state->tags_mympd);
    }
}

/**
 * Checks enabled tags from MPD
 * @param mympd_state pointer to mympd_state struct
 */
static void mpd_client_feature_mpd_tags(struct t_mympd_state *mympd_state) {
    reset_t_tags(&mympd_state->mpd_state->tags_mpd);
    reset_t_tags(&mympd_state->mpd_state->tags_mympd);

    enable_all_mpd_tags(mympd_state->partition_state);

    sds logline = sdsnew("MPD supported tags: ");
    if (mpd_send_list_tag_types(mympd_state->partition_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_tag_type_pair(mympd_state->partition_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                logline = sdscatfmt(logline, "%s ", pair->value);
                mympd_state->mpd_state->tags_mpd.tags[mympd_state->mpd_state->tags_mpd.len++] = tag;
            }
            else {
                MYMPD_LOG_WARN("Unknown tag %s (libmpdclient too old)", pair->value);
            }
            mpd_return_pair(mympd_state->partition_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_tag_types");
    }
    mpd_response_finish(mympd_state->partition_state->conn);
    mympd_check_error_and_recover(mympd_state->partition_state);

    if (mympd_state->mpd_state->tags_mpd.len == 0) {
        logline = sdscatlen(logline, "none", 4);
        MYMPD_LOG_NOTICE("%s", logline);
        MYMPD_LOG_NOTICE("Tags are disabled");
        mympd_state->mpd_state->feat_tags = false;
    }
    else {
        mympd_state->mpd_state->feat_tags = true;
        MYMPD_LOG_NOTICE("%s", logline);
        check_tags(mympd_state->mpd_state->tag_list, "tag_list", &mympd_state->mpd_state->tags_mympd, &mympd_state->mpd_state->tags_mpd);
        enable_mpd_tags(mympd_state->partition_state, &mympd_state->mpd_state->tags_mympd);
    }

    bool has_albumartist = mpd_client_tag_exists(&mympd_state->mpd_state->tags_mympd, MPD_TAG_ALBUM_ARTIST);
    if (has_albumartist == true) {
        mympd_state->mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    }
    else {
        MYMPD_LOG_WARN("AlbumArtist tag not enabled");
        mympd_state->mpd_state->tag_albumartist = MPD_TAG_ARTIST;
    }
    FREE_SDS(logline);
}

/**
 * Checks for available MPD music directory
 * @param mympd_state pointer to mympd_state struct
 */
static void mpd_client_feature_music_directory(struct t_mympd_state *mympd_state) {
    mympd_state->mpd_state->feat_library = false;
    sdsclear(mympd_state->mpd_state->music_directory_value);

    if (strncmp(mympd_state->mpd_state->mpd_host, "/", 1) == 0 &&
        strncmp(mympd_state->music_directory, "auto", 4) == 0)
    {
        //get musicdirectory from mpd
        if (mpd_send_command(mympd_state->partition_state->conn, "config", NULL) == true) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair(mympd_state->partition_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0 &&
                    is_streamuri(pair->value) == false)
                {
                    mympd_state->mpd_state->music_directory_value = sds_replace(mympd_state->mpd_state->music_directory_value, pair->value);
                }
                mpd_return_pair(mympd_state->partition_state->conn, pair);
            }
        }
        else {
            MYMPD_LOG_ERROR("Error in response to command: config");
        }
        mpd_response_finish(mympd_state->partition_state->conn);
        if (mympd_check_error_and_recover(mympd_state->partition_state) == false) {
            MYMPD_LOG_ERROR("Can't get music_directory value from mpd");
        }
    }
    else if (strncmp(mympd_state->music_directory, "/", 1) == 0) {
        mympd_state->mpd_state->music_directory_value = sds_replace(mympd_state->mpd_state->music_directory_value, mympd_state->music_directory);
    }
    else if (strncmp(mympd_state->music_directory, "none", 4) == 0) {
        //empty music_directory
    }
    else {
        MYMPD_LOG_ERROR("Invalid music_directory value: \"%s\"", mympd_state->music_directory);
    }
    strip_slash(mympd_state->mpd_state->music_directory_value);
    MYMPD_LOG_INFO("Music directory is \"%s\"", mympd_state->mpd_state->music_directory_value);

    //set feat_library
    if (sdslen(mympd_state->mpd_state->music_directory_value) == 0) {
        MYMPD_LOG_WARN("Disabling library feature, music directory not defined");
        mympd_state->mpd_state->feat_library = false;
    }
    else if (testdir("MPD music_directory", mympd_state->mpd_state->music_directory_value, false, false) == DIR_EXISTS) {
        MYMPD_LOG_NOTICE("Enabling library feature");
        mympd_state->mpd_state->feat_library = true;
    }
    else {
        MYMPD_LOG_WARN("Disabling library feature, music directory not accessible");
        mympd_state->mpd_state->feat_library = false;
        sdsclear(mympd_state->mpd_state->music_directory_value);
    }
}
