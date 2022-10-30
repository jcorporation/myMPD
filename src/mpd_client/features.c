/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/features.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/settings.h"
#include "src/mympd_api/status.h"

#include <mpd/client.h>

#include <string.h>

/**
 * Private definitions
 */

static void features_commands(struct t_partition_state *partition_state);
static void features_mpd_tags(struct t_partition_state *partition_state);
static void features_tags(struct t_partition_state *partition_state);
static void features_config(struct t_partition_state *partition_state);
static sds set_directory(const char *desc, sds directory, sds value);

/**
 * Public functions
 */

/**
 * Detects MPD features and disables/enables myMPD features accordingly
 * @param partition_state pointer to partition state
 */
void mpd_client_mpd_features(struct t_partition_state *partition_state) {
    partition_state->mpd_state->protocol = mpd_connection_get_server_version(partition_state->conn);
    MYMPD_LOG_NOTICE("MPD protocol version: %u.%u.%u",
        partition_state->mpd_state->protocol[0],
        partition_state->mpd_state->protocol[1],
        partition_state->mpd_state->protocol[2]
    );

    //first disable all features
    mpd_state_features_disable(partition_state->mpd_state);

    //get features
    features_commands(partition_state);
    features_config(partition_state);
    features_tags(partition_state);

    //set state
    sds buffer = sdsempty();
    buffer = mympd_api_status_get(partition_state, buffer, 0);
    FREE_SDS(buffer);

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 0) >= 0) {
        partition_state->mpd_state->feat_partitions = true;
        MYMPD_LOG_NOTICE("Enabling partitions feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling partitions feature, depends on mpd >= 0.22.0");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 4) >= 0 ) {
        partition_state->mpd_state->feat_binarylimit = true;
        MYMPD_LOG_NOTICE("Enabling binarylimit feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling binarylimit feature, depends on mpd >= 0.22.4");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 23, 3) >= 0 ) {
        partition_state->mpd_state->feat_playlist_rm_range = true;
        MYMPD_LOG_NOTICE("Enabling delete playlist range feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling delete playlist range feature, depends on mpd >= 0.23.3");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 23, 5) >= 0 ) {
        partition_state->mpd_state->feat_whence = true;
        MYMPD_LOG_NOTICE("Enabling position whence feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling position whence feature, depends on mpd >= 0.23.5");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 24, 0) >= 0 ) {
        partition_state->mpd_state->feat_advqueue = true;
        MYMPD_LOG_NOTICE("Enabling advanced queue feature");
        partition_state->mpd_state->feat_consume_oneshot = true;
        MYMPD_LOG_NOTICE("Enabling consume oneshot feature");
        partition_state->mpd_state->feat_playlist_dir_auto = true;
        MYMPD_LOG_NOTICE("Enabling playlist directory autoconfiguration feature");
        partition_state->mpd_state->feat_starts_with = true;
        MYMPD_LOG_NOTICE("Enabling starts_with filter expression feature");
    }
    else {
        MYMPD_LOG_WARN("Disabling advanced queue feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN("Disabling consume oneshot feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN("Disabling playlist directory autoconfiguration feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN("Disabling starts_with filter expression feature, depends on mpd >= 0.24.0");
    }
    settings_to_webserver(partition_state->mympd_state);
}

/**
 * Private functions
 */

/**
 * Looks for allowed MPD command
 * @param partition_state pointer to partition state
 */
static void features_commands(struct t_partition_state *partition_state) {
    if (mpd_send_allowed_commands(partition_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(partition_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                MYMPD_LOG_DEBUG("MPD supports stickers");
                partition_state->mpd_state->feat_stickers = true;
            }
            else if (strcmp(pair->value, "listplaylists") == 0) {
                MYMPD_LOG_DEBUG("MPD supports playlists");
                partition_state->mpd_state->feat_playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                MYMPD_LOG_DEBUG("MPD supports fingerprint");
                partition_state->mpd_state->feat_fingerprint = true;
            }
            else if (strcmp(pair->value, "albumart") == 0) {
                MYMPD_LOG_DEBUG("MPD supports albumart");
                partition_state->mpd_state->feat_albumart = true;
            }
            else if (strcmp(pair->value, "readpicture") == 0) {
                MYMPD_LOG_DEBUG("MPD supports readpicture");
                partition_state->mpd_state->feat_readpicture = true;
            }
            else if (strcmp(pair->value, "mount") == 0) {
                MYMPD_LOG_DEBUG("MPD supports mounts");
                partition_state->mpd_state->feat_mount = true;
            }
            else if (strcmp(pair->value, "listneighbors") == 0) {
                MYMPD_LOG_DEBUG("MPD supports neighbors");
                partition_state->mpd_state->feat_neighbor = true;
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_allowed_commands");
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);
}

/**
 * Sets enabled tags for myMPD
 * @param partition_state pointer to partition state
 */
static void features_tags(struct t_partition_state *partition_state) {
    reset_t_tags(&partition_state->mpd_state->tags_search);
    reset_t_tags(&partition_state->mpd_state->tags_browse);
    reset_t_tags(&partition_state->mympd_state->smartpls_generate_tag_types);

    features_mpd_tags(partition_state);

    if (partition_state->mpd_state->feat_tags == true) {
        check_tags(partition_state->mympd_state->tag_list_search, "tag_list_search",
            &partition_state->mpd_state->tags_search, &partition_state->mpd_state->tags_mympd);
        check_tags(partition_state->mympd_state->tag_list_browse, "tag_list_browse",
            &partition_state->mpd_state->tags_browse, &partition_state->mpd_state->tags_mympd);
        check_tags(partition_state->mympd_state->smartpls_generate_tag_list, "smartpls_generate_tag_list",
            &partition_state->mympd_state->smartpls_generate_tag_types, &partition_state->mpd_state->tags_mympd);
    }
}

/**
 * Checks enabled tags from MPD
 * @param partition_state pointer to partition state
 */
static void features_mpd_tags(struct t_partition_state *partition_state) {
    reset_t_tags(&partition_state->mpd_state->tags_mpd);
    reset_t_tags(&partition_state->mpd_state->tags_mympd);

    enable_all_mpd_tags(partition_state);

    sds logline = sdsnew("MPD supported tags: ");
    if (mpd_send_list_tag_types(partition_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_tag_type_pair(partition_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                logline = sdscatfmt(logline, "%s ", pair->value);
                partition_state->mpd_state->tags_mpd.tags[partition_state->mpd_state->tags_mpd.len++] = tag;
            }
            else {
                MYMPD_LOG_WARN("Unknown tag %s (libmpdclient too old)", pair->value);
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_tag_types");
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);

    if (partition_state->mpd_state->tags_mpd.len == 0) {
        logline = sdscatlen(logline, "none", 4);
        MYMPD_LOG_NOTICE("%s", logline);
        MYMPD_LOG_NOTICE("Tags are disabled");
        partition_state->mpd_state->feat_tags = false;
    }
    else {
        partition_state->mpd_state->feat_tags = true;
        MYMPD_LOG_NOTICE("%s", logline);
        check_tags(partition_state->mpd_state->tag_list, "tag_list",
            &partition_state->mpd_state->tags_mympd, &partition_state->mpd_state->tags_mpd);
    }

    bool has_albumartist = mpd_client_tag_exists(&partition_state->mpd_state->tags_mympd, MPD_TAG_ALBUM_ARTIST);
    if (has_albumartist == true) {
        partition_state->mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    }
    else {
        MYMPD_LOG_WARN("AlbumArtist tag not enabled");
        partition_state->mpd_state->tag_albumartist = MPD_TAG_ARTIST;
    }
    FREE_SDS(logline);
}

/**
 * Uses the config command to check for MPD features
 * @param partition_state pointer to partition state
 */
static void features_config(struct t_partition_state *partition_state) {
    partition_state->mpd_state->feat_library = false;
    sdsclear(partition_state->mpd_state->music_directory_value);
    sdsclear(partition_state->mpd_state->playlist_directory_value);

    //config command is only supported for socket connections
    if (partition_state->mpd_state->mpd_host[0] == '/') {
        if (mpd_connection_cmp_server_version(partition_state->conn, 0, 24, 0) == -1 ) {
            //assume true for older MPD versions
            partition_state->mpd_state->feat_pcre = true;
            MYMPD_LOG_NOTICE("Enabling pcre feature");
        }
        //get directories from mpd
        bool rc = mpd_send_command(partition_state->conn, "config", NULL);
        if (rc == true) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0 &&
                    is_streamuri(pair->value) == false &&
                    strncmp(partition_state->mympd_state->music_directory, "auto", 4) == 0)
                {
                    partition_state->mpd_state->music_directory_value = sds_replace(partition_state->mpd_state->music_directory_value, pair->value);
                }
                else if (strcmp(pair->name, "playlist_directory") == 0 &&
                    strncmp(partition_state->mympd_state->playlist_directory, "auto", 4) == 0)
                {
                    //supported since MPD 0.24
                    partition_state->mpd_state->playlist_directory_value = sds_replace(partition_state->mpd_state->playlist_directory_value, pair->value);
                }
                else if (strcmp(pair->name, "pcre") == 0) {
                    //supported since MPD 0.24
                    if (pair->value[0] == '1') {
                        partition_state->mpd_state->feat_pcre = true;
                        MYMPD_LOG_NOTICE("Enabling pcre feature");
                    }
                    else {
                        partition_state->mpd_state->feat_pcre = false;
                        MYMPD_LOG_WARN("Disabling pcre feature");
                    }
                }
                mpd_return_pair(partition_state->conn, pair);
            }
        }
        mpd_response_finish(partition_state->conn);
        mympd_check_rc_error_and_recover(partition_state, rc, "config");
    }

    partition_state->mpd_state->music_directory_value = set_directory("music", partition_state->mympd_state->music_directory,
        partition_state->mpd_state->music_directory_value);
    partition_state->mpd_state->playlist_directory_value = set_directory("playlist", partition_state->mympd_state->playlist_directory,
        partition_state->mpd_state->playlist_directory_value);

    //set feat_library
    if (sdslen(partition_state->mpd_state->music_directory_value) == 0) {
        MYMPD_LOG_WARN("Disabling library feature, music directory not defined");
        partition_state->mpd_state->feat_library = false;
    }
    else {
        partition_state->mpd_state->feat_library = true;
    }
}

/**
 * Checks and sets a mpd directory
 * @param desc descriptive name
 * @param directory directory setting (auto, none or a directory)
 * @param value already allocated sds string to set
 * @return pointer to value
 */
static sds set_directory(const char *desc, sds directory, sds value) {
    if (strncmp(directory, "auto", 4) == 0) {
        //valid
    }
    else if (directory[0] == '/') {
        value = sds_replace(value, directory);
    }
    else if (strncmp(directory, "none", 4) == 0) {
        //empty playlist_directory
        return value;
    }
    else {
        MYMPD_LOG_ERROR("Invalid %s directory value: \"%s\"", desc, directory);
        return value;
    }
    strip_slash(value);
    if (sdslen(value) > 0 &&
        testdir("Directory", value, false, true) != DIR_EXISTS)
    {
        sdsclear(value);
    }
    if (sdslen(value) == 0) {
        MYMPD_LOG_INFO("MPD %s directory is not set", desc);
    }
    else {
        MYMPD_LOG_INFO("MPD %s directory is \"%s\"", desc, value);
    }
    return value;
}
