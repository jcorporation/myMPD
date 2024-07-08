/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD feature detection
 */

#include "compile_time.h"
#include "src/mpd_client/features.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/settings.h"

#include <stdbool.h>
#include <string.h>

/**
 * Private definitions
 */

static void features_commands(struct t_partition_state *partition_state);
static void features_mpd_tags(struct t_partition_state *partition_state);
static void features_tags(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state);
static void set_simple_album_tags(struct t_partition_state *partition_state);
static void set_album_tags(struct t_partition_state *partition_state);
static void features_config(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state);
static sds set_directory(const char *desc, sds directory, sds value);

/**
 * Public functions
 */

/**
 * Detects MPD features and disables/enables myMPD features accordingly
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition state
 */
void mpd_client_mpd_features(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state) {
    partition_state->mpd_state->protocol = mpd_connection_get_server_version(partition_state->conn);
    MYMPD_LOG_NOTICE(partition_state->name, "MPD protocol version: %u.%u.%u",
        partition_state->mpd_state->protocol[0],
        partition_state->mpd_state->protocol[1],
        partition_state->mpd_state->protocol[2]
    );

    // first disable all features
    mpd_state_features_default(&partition_state->mpd_state->feat);

    // copy sticker feature flags from stickerdb connection
    partition_state->mpd_state->feat.stickers = mympd_state->stickerdb->mpd_state->feat.stickers;
    partition_state->mpd_state->feat.sticker_sort_window = mympd_state->stickerdb->mpd_state->feat.sticker_sort_window;
    partition_state->mpd_state->feat.sticker_int = mympd_state->stickerdb->mpd_state->feat.sticker_int;

    //get features
    features_commands(partition_state);
    features_config(mympd_state, partition_state);
    features_tags(mympd_state, partition_state);

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 0) >= 0) {
        partition_state->mpd_state->feat.partitions = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling partitions feature");
        partition_state->mpd_state->feat.search_add_sort_window = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling searchadd sort and window feature");
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Disabling partitions feature, depends on mpd >= 0.22.0");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 4) >= 0 ) {
        partition_state->mpd_state->feat.binarylimit = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling binarylimit feature");
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Disabling binarylimit feature, depends on mpd >= 0.22.4");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 23, 3) >= 0 ) {
        partition_state->mpd_state->feat.playlist_rm_range = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling delete playlist range feature");
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Disabling delete playlist range feature, depends on mpd >= 0.23.3");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 23, 5) >= 0 ) {
        partition_state->mpd_state->feat.whence = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling position whence feature");
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Disabling position whence feature, depends on mpd >= 0.23.5");
    }

    if (mpd_connection_cmp_server_version(partition_state->conn, 0, 24, 0) >= 0 ) {
        partition_state->mpd_state->feat.advqueue = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling advanced queue feature");
        partition_state->mpd_state->feat.consume_oneshot = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling consume oneshot feature");
        partition_state->mpd_state->feat.playlist_dir_auto = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling playlist directory autoconfiguration feature");
        partition_state->mpd_state->feat.starts_with = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling starts_with filter expression feature");
        partition_state->mpd_state->feat.db_added = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling db added feature");
        partition_state->mpd_state->feat.listplaylist_range = true;
        MYMPD_LOG_INFO(partition_state->name, "Enabling listplaylist range feature");
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Disabling advanced queue feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN(partition_state->name, "Disabling consume oneshot feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN(partition_state->name, "Disabling playlist directory autoconfiguration feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN(partition_state->name, "Disabling starts_with filter expression feature, depends on mpd >= 0.24.0");
        MYMPD_LOG_WARN(partition_state->name, "Disabling db added feature, depends on mpd >= 0.24.0");
    }
    settings_to_webserver(mympd_state);
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
            if (strcmp(pair->value, "listplaylists") == 0) {
                MYMPD_LOG_INFO(partition_state->name, "MPD supports playlists");
                partition_state->mpd_state->feat.playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                MYMPD_LOG_INFO(partition_state->name, "MPD supports fingerprint");
                partition_state->mpd_state->feat.fingerprint = true;
            }
            else if (strcmp(pair->value, "albumart") == 0) {
                MYMPD_LOG_INFO(partition_state->name, "MPD supports albumart");
                partition_state->mpd_state->feat.albumart = true;
            }
            else if (strcmp(pair->value, "readpicture") == 0) {
                MYMPD_LOG_INFO(partition_state->name, "MPD supports readpicture");
                partition_state->mpd_state->feat.readpicture = true;
            }
            else if (strcmp(pair->value, "mount") == 0) {
                MYMPD_LOG_INFO(partition_state->name, "MPD supports mounts");
                partition_state->mpd_state->feat.mount = true;
            }
            else if (strcmp(pair->value, "listneighbors") == 0) {
                MYMPD_LOG_INFO(partition_state->name, "MPD supports neighbors");
                partition_state->mpd_state->feat.neighbor = true;
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_allowed_commands");
}

/**
 * Sets enabled tags for myMPD
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition state
 */
static void features_tags(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state) {
    //reset all tags
    mpd_tags_reset(&partition_state->mpd_state->tags_mpd);
    mpd_tags_reset(&partition_state->mpd_state->tags_mympd);
    mpd_tags_reset(&partition_state->mpd_state->tags_search);
    mpd_tags_reset(&partition_state->mpd_state->tags_browse);
    mpd_tags_reset(&partition_state->mpd_state->tags_album);
    mpd_tags_reset(&mympd_state->smartpls_generate_tag_types);
    //check for all mpd tags
    enable_all_mpd_tags(partition_state);
    features_mpd_tags(partition_state);
    //parse the webui taglists and set the tag structs
    if (partition_state->mpd_state->feat.tags == true) {
        if (partition_state->config->albums.mode == ALBUM_MODE_ADV) {
            set_album_tags(partition_state);
        }
        else {
            set_simple_album_tags(partition_state);
        }
        check_tags(mympd_state->tag_list_search, "tag_list_search",
            &partition_state->mpd_state->tags_search, &partition_state->mpd_state->tags_mympd);
        check_tags(mympd_state->tag_list_browse, "tag_list_browse",
            &partition_state->mpd_state->tags_browse, &partition_state->mpd_state->tags_mympd);
        check_tags(mympd_state->smartpls_generate_tag_list, "smartpls_generate_tag_list",
            &mympd_state->smartpls_generate_tag_types, &partition_state->mpd_state->tags_mympd);
    }
    //enable only configures tags
    enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
}

/**
 * Sets the tags for albums
 * @param partition_state pointer to partition state
 */
static void set_album_tags(struct t_partition_state *partition_state) {
    sds logline = sdscatfmt(sdsempty(), "Enabled tag_list_album: ");
    for (size_t i = 0; i < partition_state->mpd_state->tags_mympd.len; i++) {
        switch(partition_state->mpd_state->tags_mympd.tags[i]) {
            case MPD_TAG_MUSICBRAINZ_RELEASETRACKID:
            case MPD_TAG_MUSICBRAINZ_TRACKID:
            case MPD_TAG_NAME:
            case MPD_TAG_TITLE:
            case MPD_TAG_DISC:
            case MPD_TAG_TITLE_SORT:
            case MPD_TAG_TRACK:
                //ignore this tags for albums
                break;
            default:
                partition_state->mpd_state->tags_album.tags[partition_state->mpd_state->tags_album.len++] = partition_state->mpd_state->tags_mympd.tags[i];
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(partition_state->mpd_state->tags_mympd.tags[i]));
        }
    }
    MYMPD_LOG_INFO(partition_state->name, "%s", logline);
    FREE_SDS(logline);
}

/**
 * Sets the tags for simple albums
 * @param partition_state pointer to partition state
 */
static void set_simple_album_tags(struct t_partition_state *partition_state) {
    sds logline = sdscatfmt(sdsempty(), "Enabled tag_list_album: ");
    for (size_t i = 0; i < partition_state->mpd_state->tags_mympd.len; i++) {
        switch(partition_state->mpd_state->tags_mympd.tags[i]) {
            case MPD_TAG_ALBUM:
            case MPD_TAG_ALBUM_ARTIST:
                partition_state->mpd_state->tags_album.tags[partition_state->mpd_state->tags_album.len++] = partition_state->mpd_state->tags_mympd.tags[i];
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(partition_state->mpd_state->tags_mympd.tags[i]));
                break;
            default:
                if (partition_state->mpd_state->tags_mympd.tags[i] == partition_state->config->albums.group_tag) {
                    // add album group tag
                    partition_state->mpd_state->tags_album.tags[partition_state->mpd_state->tags_album.len++] = partition_state->mpd_state->tags_mympd.tags[i];
                    logline = sdscatfmt(logline, "%s ", mpd_tag_name(partition_state->mpd_state->tags_mympd.tags[i]));
                }
                // ignore all other tags
        }
    }
    MYMPD_LOG_INFO(partition_state->name, "%s", logline);
    FREE_SDS(logline);
}

/**
 * Checks enabled tags from MPD and
 * populates tags_mpd and tags_mympd
 * @param partition_state pointer to partition state
 */
static void features_mpd_tags(struct t_partition_state *partition_state) {
    sds logline = sdsnew("MPD supported tags: ");
    if (mpd_send_list_tag_types(partition_state->conn)) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_tag_type_pair(partition_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                logline = sdscatfmt(logline, "%s ", pair->value);
                partition_state->mpd_state->tags_mpd.tags[partition_state->mpd_state->tags_mpd.len++] = tag;
            }
            else {
                MYMPD_LOG_WARN(partition_state->name, "Unknown tag %s (libmpdclient too old)", pair->value);
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_tag_types");

    if (partition_state->mpd_state->tags_mpd.len == 0) {
        logline = sdscatlen(logline, "none", 4);
        MYMPD_LOG_INFO(partition_state->name, "%s", logline);
        MYMPD_LOG_INFO(partition_state->name, "Tags are disabled");
        partition_state->mpd_state->feat.tags = false;
    }
    else {
        partition_state->mpd_state->feat.tags = true;
        MYMPD_LOG_INFO(partition_state->name, "%s", logline);
        //populate tags_mympd tags struct
        check_tags(partition_state->mpd_state->tag_list, "tag_list",
            &partition_state->mpd_state->tags_mympd, &partition_state->mpd_state->tags_mpd);
    }

    bool has_albumartist = mpd_client_tag_exists(&partition_state->mpd_state->tags_mympd, MPD_TAG_ALBUM_ARTIST);
    if (has_albumartist == true) {
        partition_state->mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "AlbumArtist tag not enabled");
        partition_state->mpd_state->tag_albumartist = MPD_TAG_ARTIST;
    }
    FREE_SDS(logline);
}

/**
 * Uses the config command to check for MPD features
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition state
 */
static void features_config(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state) {
    partition_state->mpd_state->feat.library = false;
    sdsclear(partition_state->mpd_state->music_directory_value);
    sdsclear(partition_state->mpd_state->playlist_directory_value);

    //config command is only supported for socket connections
    if (partition_state->mpd_state->mpd_host[0] == '/') {
        if (mpd_connection_cmp_server_version(partition_state->conn, 0, 24, 0) == -1 ) {
            //assume true for older MPD versions
            partition_state->mpd_state->feat.pcre = true;
            MYMPD_LOG_INFO(partition_state->name, "Enabling pcre feature");
        }
        //get directories from mpd
        if (mpd_send_command(partition_state->conn, "config", NULL)) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0 &&
                    is_streamuri(pair->value) == false &&
                    strncmp(mympd_state->music_directory, "auto", 4) == 0)
                {
                    partition_state->mpd_state->music_directory_value = sds_replace(partition_state->mpd_state->music_directory_value, pair->value);
                }
                else if (strcmp(pair->name, "playlist_directory") == 0 &&
                    strncmp(mympd_state->playlist_directory, "auto", 4) == 0)
                {
                    //supported since MPD 0.24
                    partition_state->mpd_state->playlist_directory_value = sds_replace(partition_state->mpd_state->playlist_directory_value, pair->value);
                }
                else if (strcmp(pair->name, "pcre") == 0) {
                    //supported since MPD 0.24
                    if (pair->value[0] == '1') {
                        partition_state->mpd_state->feat.pcre = true;
                        MYMPD_LOG_INFO(partition_state->name, "Enabling pcre feature");
                    }
                    else {
                        partition_state->mpd_state->feat.pcre = false;
                        MYMPD_LOG_WARN(partition_state->name, "Disabling pcre feature");
                    }
                }
                mpd_return_pair(partition_state->conn, pair);
            }
        }
        mpd_response_finish(partition_state->conn);
        mympd_check_error_and_recover(partition_state, NULL, "config");
    }

    partition_state->mpd_state->music_directory_value = set_directory("music", mympd_state->music_directory,
        partition_state->mpd_state->music_directory_value);
    partition_state->mpd_state->playlist_directory_value = set_directory("playlist", mympd_state->playlist_directory,
        partition_state->mpd_state->playlist_directory_value);

    //set feat_library
    if (sdslen(partition_state->mpd_state->music_directory_value) == 0) {
        MYMPD_LOG_WARN(partition_state->name, "Disabling library feature, music directory not defined");
        partition_state->mpd_state->feat.library = false;
    }
    else {
        partition_state->mpd_state->feat.library = true;
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
        MYMPD_LOG_ERROR(NULL, "Invalid %s directory value: \"%s\"", desc, directory);
        return value;
    }
    strip_slash(value);
    if (sdslen(value) > 0 &&
        testdir("Directory", value, false, false) != DIR_EXISTS)
    {
        MYMPD_LOG_WARN(NULL, "Directory %s not accessible", value);
        sdsclear(value);
    }
    if (sdslen(value) == 0) {
        MYMPD_LOG_WARN(NULL, "MPD %s directory is not set", desc);
    }
    else {
        MYMPD_LOG_INFO(NULL, "MPD %s directory is \"%s\"", desc, value);
    }
    return value;
}
