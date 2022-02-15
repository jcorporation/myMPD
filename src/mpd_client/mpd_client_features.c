/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_features.h"

#include "../lib/api.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mympd_api/mympd_api_status.h"

#include <stdlib.h>
#include <string.h>

//private definitions
static void mpd_client_feature_commands(struct t_mympd_state *mympd_state);
static void mpd_client_feature_mpd_tags(struct t_mympd_state *mympd_state);
static void mpd_client_feature_tags(struct t_mympd_state *mympd_state);
static void mpd_client_feature_music_directory(struct t_mympd_state *mympd_state);

//public functions
void mpd_client_mpd_features(struct t_mympd_state *mympd_state) {
    mympd_state->mpd_state->protocol = mpd_connection_get_server_version(mympd_state->mpd_state->conn);
    MYMPD_LOG_NOTICE("MPD protocol version: %u.%u.%u",
        mympd_state->mpd_state->protocol[0],
        mympd_state->mpd_state->protocol[1],
        mympd_state->mpd_state->protocol[2]
    );

    // Defaults
    mympd_state->mpd_state->feat_mpd_stickers = false;
    mympd_state->mpd_state->feat_mpd_playlists = false;
    mympd_state->mpd_state->feat_mpd_tags = false;
    mympd_state->mpd_state->feat_mpd_advsearch = false;
    mympd_state->mpd_state->feat_mpd_fingerprint = false;
    mympd_state->mpd_state->feat_mpd_albumart = false;
    mympd_state->mpd_state->feat_mpd_readpicture = false;
    mympd_state->mpd_state->feat_mpd_single_oneshot = false;
    mympd_state->mpd_state->feat_mpd_mount = false;
    mympd_state->mpd_state->feat_mpd_neighbor = false;
    mympd_state->mpd_state->feat_mpd_partitions = false;
    mympd_state->mpd_state->feat_mpd_binarylimit = false;
    mympd_state->mpd_state->feat_mpd_smartpls = false;
    mympd_state->mpd_state->feat_mpd_playlist_rm_range = false;
    mympd_state->mpd_state->feat_mpd_whence = false;
    mympd_state->mpd_state->feat_mpd_advqueue = false;

    //get features
    mpd_client_feature_commands(mympd_state);
    mpd_client_feature_music_directory(mympd_state);
    mpd_client_feature_tags(mympd_state);

    //set state
    sds buffer = sdsempty();
    buffer = mympd_api_status_get(mympd_state, buffer, NULL, 0);
    FREE_SDS(buffer);

    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 21, 0) >= 0) {
        mympd_state->mpd_state->feat_mpd_single_oneshot = true;
        MYMPD_LOG_NOTICE("Enabling single oneshot feature");
        mympd_state->mpd_state->feat_mpd_advsearch = true;
        MYMPD_LOG_INFO("Enabling advanced search feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling single oneshot feature, depends on mpd >= 0.21.0");
        MYMPD_LOG_WARN("Disabling advanced search feature, depends on mpd >= 0.21.0");
    }

    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 22, 0) >= 0) {
        mympd_state->mpd_state->feat_mpd_partitions = true;
        MYMPD_LOG_NOTICE("Enabling partitions feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling partitions feature, depends on mpd >= 0.22.0");
    }

    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 22, 4) >= 0 ) {
        mympd_state->mpd_state->feat_mpd_binarylimit = true;
        MYMPD_LOG_NOTICE("Enabling binarylimit feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling binarylimit feature, depends on mpd >= 0.22.4");
    }

    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 23, 3) >= 0 ) {
        mympd_state->mpd_state->feat_mpd_playlist_rm_range = true;
        MYMPD_LOG_NOTICE("Enabling delete playlist range feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling delete playlist range feature, depends on mpd >= 0.23.3");
    }

    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 23, 5) >= 0 ) {
        mympd_state->mpd_state->feat_mpd_whence = true;
        MYMPD_LOG_NOTICE("Enabling position whence feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling position whence feature, depends on mpd >= 0.23.5");
    }
    
    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 24, 0) >= 0 ) {
        mympd_state->mpd_state->feat_mpd_advqueue = true;
        MYMPD_LOG_NOTICE("Enabling advanced queue feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling advancded queue feature, depends on mpd >= 0.24.0");
    }

    if (mympd_state->mpd_state->feat_mpd_advsearch == true && mympd_state->mpd_state->feat_mpd_playlists == true) {
        MYMPD_LOG_NOTICE("Enabling smart playlists feature");
        mympd_state->mpd_state->feat_mpd_smartpls = true;
    }
    else {
        MYMPD_LOG_WARN("Disabling smart playlists feature");
    }

    //push settings to web_server_queue
    struct set_mg_user_data_request *extra = malloc_assert(sizeof(struct set_mg_user_data_request));
    extra->music_directory = sdsdup(mympd_state->music_directory_value);
    extra->playlist_directory = sdsdup(mympd_state->playlist_directory);
    extra->coverimage_names = sdsdup(mympd_state->coverimage_names);
    extra->feat_mpd_albumart = mympd_state->mpd_state->feat_mpd_albumart;
    extra->mpd_stream_port = mympd_state->mpd_stream_port;
    extra->mpd_host = sdsdup(mympd_state->mpd_state->mpd_host);
    extra->covercache = mympd_state->covercache_keep_days > 0 ? true : false;

    struct t_work_result *web_server_response = create_result_new(-1, 0, INTERNAL_API_WEBSERVER_SETTINGS);
    web_server_response->extra = extra;
    mympd_queue_push(web_server_queue, web_server_response, 0);
}

//private functions
static void mpd_client_feature_commands(struct t_mympd_state *mympd_state) {
    if (mpd_send_allowed_commands(mympd_state->mpd_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(mympd_state->mpd_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                MYMPD_LOG_DEBUG("MPD supports stickers");
                mympd_state->mpd_state->feat_mpd_stickers = true;
            }
            else if (strcmp(pair->value, "listplaylists") == 0) {
                MYMPD_LOG_DEBUG("MPD supports playlists");
                mympd_state->mpd_state->feat_mpd_playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                MYMPD_LOG_DEBUG("MPD supports fingerprint");
                mympd_state->mpd_state->feat_mpd_fingerprint = true;
            }
            else if (strcmp(pair->value, "albumart") == 0) {
                MYMPD_LOG_DEBUG("MPD supports albumart");
                mympd_state->mpd_state->feat_mpd_albumart = true;
            }
            else if (strcmp(pair->value, "readpicture") == 0) {
                MYMPD_LOG_DEBUG("MPD supports readpicture");
                mympd_state->mpd_state->feat_mpd_readpicture = true;
            }
            else if (strcmp(pair->value, "mount") == 0) {
                MYMPD_LOG_DEBUG("MPD supports mounts");
                mympd_state->mpd_state->feat_mpd_mount = true;
            }
            else if (strcmp(pair->value, "listneighbors") == 0) {
                MYMPD_LOG_DEBUG("MPD supports neighbors");
                mympd_state->mpd_state->feat_mpd_neighbor = true;
            }
            mpd_return_pair(mympd_state->mpd_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_allowed_commands");
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false);
}

static void mpd_client_feature_tags(struct t_mympd_state *mympd_state) {
    reset_t_tags(&mympd_state->tag_types_search);
    reset_t_tags(&mympd_state->tag_types_browse);
    reset_t_tags(&mympd_state->smartpls_generate_tag_types);

    mpd_client_feature_mpd_tags(mympd_state);

    if (mympd_state->mpd_state->feat_mpd_tags == true) {
        check_tags(mympd_state->tag_list_search, "tag_list_search", &mympd_state->tag_types_search, mympd_state->mpd_state->tag_types_mympd);
        check_tags(mympd_state->tag_list_browse, "tag_list_browse", &mympd_state->tag_types_browse, mympd_state->mpd_state->tag_types_mympd);
        check_tags(mympd_state->smartpls_generate_tag_list, "smartpls_generate_tag_list", &mympd_state->smartpls_generate_tag_types, mympd_state->mpd_state->tag_types_mympd);
    }
}

static void mpd_client_feature_mpd_tags(struct t_mympd_state *mympd_state) {
    reset_t_tags(&mympd_state->mpd_state->tag_types_mpd);
    reset_t_tags(&mympd_state->mpd_state->tag_types_mympd);

    enable_all_mpd_tags(mympd_state->mpd_state);

    sds logline = sdsnew("MPD supported tags: ");
    if (mpd_send_list_tag_types(mympd_state->mpd_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_tag_type_pair(mympd_state->mpd_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                logline = sdscatfmt(logline, "%s ", pair->value);
                mympd_state->mpd_state->tag_types_mpd.tags[mympd_state->mpd_state->tag_types_mpd.len++] = tag;
            }
            else {
                MYMPD_LOG_WARN("Unknown tag %s (libmpdclient too old)", pair->value);
            }
            mpd_return_pair(mympd_state->mpd_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_tag_types");
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false);

    if (mympd_state->mpd_state->tag_types_mpd.len == 0) {
        logline = sdscatlen(logline, "none", 4);
        MYMPD_LOG_NOTICE("%s", logline);
        MYMPD_LOG_NOTICE("Tags are disabled");
        mympd_state->mpd_state->feat_mpd_tags = false;
    }
    else {
        mympd_state->mpd_state->feat_mpd_tags = true;
        MYMPD_LOG_NOTICE("%s", logline);
        check_tags(mympd_state->mpd_state->tag_list, "tag_list", &mympd_state->mpd_state->tag_types_mympd, mympd_state->mpd_state->tag_types_mpd);
        enable_mpd_tags(mympd_state->mpd_state, &mympd_state->mpd_state->tag_types_mympd);
    }

    bool has_albumartist = mpd_shared_tag_exists(mympd_state->mpd_state->tag_types_mympd.tags, mympd_state->mpd_state->tag_types_mympd.len, MPD_TAG_ALBUM_ARTIST);
    if (has_albumartist == true) {
        mympd_state->mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    }
    else {
        mympd_state->mpd_state->tag_albumartist = MPD_TAG_ARTIST;
    }
    FREE_SDS(logline);
}

static void mpd_client_feature_music_directory(struct t_mympd_state *mympd_state) {
    mympd_state->mpd_state->feat_mpd_library = false;
    sdsclear(mympd_state->music_directory_value);

    if (strncmp(mympd_state->mpd_state->mpd_host, "/", 1) == 0 && strncmp(mympd_state->music_directory, "auto", 4) == 0) {
        //get musicdirectory from mpd
        if (mpd_send_command(mympd_state->mpd_state->conn, "config", NULL) == true) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair(mympd_state->mpd_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0) {
                    if (strncmp(pair->value, "smb://", 6) != 0 && strncmp(pair->value, "nfs://", 6) != 0) {
                        mympd_state->music_directory_value = sds_replace(mympd_state->music_directory_value, pair->value);
                    }
                }
                mpd_return_pair(mympd_state->mpd_state->conn, pair);
            }
        }
        else {
            MYMPD_LOG_ERROR("Error in response to command: config");
        }
        mpd_response_finish(mympd_state->mpd_state->conn);
        if (check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false) == false) {
            MYMPD_LOG_ERROR("Can't get music_directory value from mpd");
        }
    }
    else if (strncmp(mympd_state->music_directory, "/", 1) == 0) {
        mympd_state->music_directory_value = sds_replace(mympd_state->music_directory_value, mympd_state->music_directory);
    }
    else if (strncmp(mympd_state->music_directory, "none", 4) == 0) {
        //empty music_directory
    }
    else {
        MYMPD_LOG_ERROR("Invalid music_directory value: \"%s\"", mympd_state->music_directory);
    }
    sds_strip_slash(mympd_state->music_directory_value);
    MYMPD_LOG_INFO("Music directory is \"%s\"", mympd_state->music_directory_value);

    //set feat_library
    if (sdslen(mympd_state->music_directory_value) == 0) {
        MYMPD_LOG_WARN("Disabling library feature, music directory not defined");
        mympd_state->mpd_state->feat_mpd_library = false;
    }
    else if (testdir("MPD music_directory", mympd_state->music_directory_value, false) == DIR_EXISTS) {
        MYMPD_LOG_NOTICE("Enabling library feature");
        mympd_state->mpd_state->feat_mpd_library = true;
    }
    else {
        MYMPD_LOG_WARN("Disabling library feature, music directory not accessible");
        mympd_state->mpd_state->feat_mpd_library = false;
        sdsclear(mympd_state->music_directory_value);
    }
}
