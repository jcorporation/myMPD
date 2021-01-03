/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../lua_mympd_state.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared/mpd_shared_features.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_state.h"
#include "mpd_client_features.h"

//private definitions
static void mpd_client_feature_commands(t_mpd_client_state *mpd_client_state);
static void mpd_client_feature_tags(t_mpd_client_state *mpd_client_state);
static void mpd_client_feature_music_directory(t_mpd_client_state *mpd_client_state);

//public functions
void mpd_client_mpd_features(t_config *config, t_mpd_client_state *mpd_client_state) {
    mpd_client_state->protocol = mpd_connection_get_server_version(mpd_client_state->mpd_state->conn);
    LOG_INFO("MPD protocoll version: %u.%u.%u", mpd_client_state->protocol[0], mpd_client_state->protocol[1], mpd_client_state->protocol[2]);

    // Defaults
    mpd_client_state->feat_sticker = false;
    mpd_client_state->feat_playlists = false;
    mpd_client_state->mpd_state->feat_tags = false;
    mpd_client_state->mpd_state->feat_advsearch = false;
    mpd_client_state->feat_fingerprint = false;
    mpd_client_state->feat_smartpls = mpd_client_state->smartpls;;
    mpd_client_state->feat_coverimage = true;
    mpd_client_state->feat_mpd_albumart = false;
    mpd_client_state->feat_mpd_readpicture = false;
    mpd_client_state->mpd_state->feat_mpd_searchwindow = false;
    mpd_client_state->feat_single_oneshot = false;
    mpd_client_state->feat_mpd_mount = false;
    mpd_client_state->feat_mpd_neighbor = false;
    mpd_client_state->feat_mpd_partitions = false;
    
    //get features
    mpd_client_feature_commands(mpd_client_state);
    mpd_client_feature_music_directory(mpd_client_state);
    mpd_client_feature_love(mpd_client_state);
    mpd_client_feature_tags(mpd_client_state);
    
    //set state
    sds buffer = sdsempty();
    buffer = mpd_client_put_state(config, mpd_client_state, buffer, NULL, 0);
    sdsfree(buffer);

    mpd_client_state->mpd_state->feat_mpd_searchwindow = mpd_shared_feat_mpd_searchwindow(mpd_client_state->mpd_state);
    mpd_client_state->mpd_state->feat_advsearch = mpd_shared_feat_advsearch(mpd_client_state->mpd_state);

    if (mpd_connection_cmp_server_version(mpd_client_state->mpd_state->conn, 0, 21, 0) >= 0) {
        mpd_client_state->feat_single_oneshot = true;
        LOG_INFO("Enabling single oneshot feature");
    }
    else {
        LOG_WARN("Disabling single oneshot feature, depends on mpd >= 0.21.0");
    }
    
    if (mpd_connection_cmp_server_version(mpd_client_state->mpd_state->conn, 0, 22, 0) >= 0 &&
        config->partitions == true) {
        mpd_client_state->feat_mpd_partitions = true;
        LOG_INFO("Enabling partitions feature");
    }
    else if (config->partitions == true && mpd_connection_cmp_server_version(mpd_client_state->mpd_state->conn, 0, 22, 0) == -1) {
        LOG_WARN("Disabling partitions support, depends on mpd >= 0.22.0");
    }
    
    if (config->mounts == false) {
        mpd_client_state->feat_mpd_mount = false;
    }
    else if (config->mounts == true && mpd_client_state->feat_mpd_mount == false) {
        LOG_WARN("Disabling mount and neighbor support");
        mpd_client_state->feat_mpd_neighbor = false;
    }
    
    //push settings to web_server_queue
    t_work_result *web_server_response = create_result_new(-1, 0, 0, "");
    sds data = sdsnew("{");
    data = tojson_char(data, "musicDirectory", mpd_client_state->music_directory_value, true);
    data = tojson_char(data, "playlistDirectory", config->playlist_directory, true);
    data = tojson_char(data, "coverimageName", mpd_client_state->coverimage_name, true);
    data = tojson_bool(data, "featLibrary", mpd_client_state->feat_library, false);
    data = tojson_bool(data, "featMpdAlbumart", mpd_client_state->feat_mpd_albumart, false);
    data = sdscat(data, "}");
    web_server_response->data = sdsreplace(web_server_response->data, data);
    sdsfree(data);
    tiny_queue_push(web_server_queue, web_server_response, 0);
}

void mpd_client_feature_love(t_mpd_client_state *mpd_client_state) {
    mpd_client_state->feat_love = false;
    if (mpd_client_state->love == true) {
        if (mpd_send_channels(mpd_client_state->mpd_state->conn) == true) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_channel_pair(mpd_client_state->mpd_state->conn)) != NULL) {
                if (strcmp(pair->value, mpd_client_state->love_channel) == 0) {
                    mpd_client_state->feat_love = true;
                }
                mpd_return_pair(mpd_client_state->mpd_state->conn, pair);            
            }
        }
        else {
            LOG_ERROR("Error in response to command: mpd_send_channels");
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);

        if (mpd_client_state->feat_love == false) {
            LOG_WARN("Disabling featLove, channel %s not found", mpd_client_state->love_channel);
        }
        else {
            LOG_INFO("Enabling featLove, channel %s found", mpd_client_state->love_channel);
        }
    }
}

//private functions
static void mpd_client_feature_commands(t_mpd_client_state *mpd_client_state) {
    if (mpd_send_allowed_commands(mpd_client_state->mpd_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_command_pair(mpd_client_state->mpd_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                LOG_DEBUG("MPD supports stickers");
                mpd_client_state->feat_sticker = true;
            }
            else if (strcmp(pair->value, "listplaylists") == 0) {
                LOG_DEBUG("MPD supports playlists");
                mpd_client_state->feat_playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                LOG_DEBUG("MPD supports fingerprint");
                mpd_client_state->feat_fingerprint = true;
            }
            else if (strcmp(pair->value, "albumart") == 0) {
                LOG_DEBUG("MPD supports albumart");
                mpd_client_state->feat_mpd_albumart = true;

            }
            else if (strcmp(pair->value, "readpicture") == 0) {
                LOG_DEBUG("MPD supports readpicture");
                mpd_client_state->feat_mpd_readpicture = true;
            }
            else if (strcmp(pair->value, "mount") == 0) {
                LOG_DEBUG("MPD supports mounts");
                mpd_client_state->feat_mpd_mount = true;
            }
            else if (strcmp(pair->value, "listneighbors") == 0) {
                LOG_DEBUG("MPD supports neighbors");
                mpd_client_state->feat_mpd_neighbor = true;
            }
            mpd_return_pair(mpd_client_state->mpd_state->conn, pair);
        }
    }
    else {
        LOG_ERROR("Error in response to command: mpd_send_allowed_commands");
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
    
    if (mpd_client_state->feat_sticker == false && mpd_client_state->stickers == true) {
        LOG_WARN("MPD don't support stickers, disabling myMPD feature");
        mpd_client_state->feat_sticker = false;
    }
    if (mpd_client_state->feat_sticker == true && mpd_client_state->stickers == false) {
        mpd_client_state->feat_sticker = false;
    }
    if (mpd_client_state->feat_sticker == false && mpd_client_state->smartpls == true) {
        LOG_WARN("Stickers are disabled, disabling smart playlists");
        mpd_client_state->feat_smartpls = false;
    }
    if (mpd_client_state->feat_playlists == false && mpd_client_state->smartpls == true) {
        LOG_WARN("Playlists are disabled, disabling smart playlists");
        mpd_client_state->feat_smartpls = false;
    }
}

static void mpd_client_feature_tags(t_mpd_client_state *mpd_client_state) {
    reset_t_tags(&mpd_client_state->search_tag_types);
    reset_t_tags(&mpd_client_state->browse_tag_types);
    reset_t_tags(&mpd_client_state->generate_pls_tag_types);

    mpd_shared_feat_tags(mpd_client_state->mpd_state);

    if (mpd_client_state->mpd_state->feat_tags == true) {
        check_tags(mpd_client_state->searchtaglist, "searchtags", &mpd_client_state->search_tag_types, mpd_client_state->mpd_state->mympd_tag_types);
        check_tags(mpd_client_state->browsetaglist, "browsetags", &mpd_client_state->browse_tag_types, mpd_client_state->mpd_state->mympd_tag_types);
        check_tags(mpd_client_state->generate_pls_tags, "generate pls tags", &mpd_client_state->generate_pls_tag_types, mpd_client_state->mpd_state->mympd_tag_types);
    }
}

static void mpd_client_feature_music_directory(t_mpd_client_state *mpd_client_state) {
    mpd_client_state->feat_library = false;
    mpd_client_state->feat_coverimage = mpd_client_state->coverimage;
    mpd_client_state->music_directory_value = sdscrop(mpd_client_state->music_directory_value);

    if (strncmp(mpd_client_state->mpd_state->mpd_host, "/", 1) == 0 && strncmp(mpd_client_state->music_directory, "auto", 4) == 0) {
        //get musicdirectory from mpd
        if (mpd_send_command(mpd_client_state->mpd_state->conn, "config", NULL) == true) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair(mpd_client_state->mpd_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0) {
                    if (strncmp(pair->value, "smb://", 6) != 0 && strncmp(pair->value, "nfs://", 6) != 0) {
                        mpd_client_state->music_directory_value = sdsreplace(mpd_client_state->music_directory_value, pair->value);
                    }
                }
                mpd_return_pair(mpd_client_state->mpd_state->conn, pair);
            }
        }
        else {
            LOG_ERROR("Error in response to command: config");
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
            LOG_ERROR("Can't get music_directory value from mpd");
        }
    }
    else if (strncmp(mpd_client_state->music_directory, "/", 1) == 0) {
        mpd_client_state->music_directory_value = sdsreplace(mpd_client_state->music_directory_value, mpd_client_state->music_directory);
    }
    else {
        //none or garbage, empty music_directory_value
    }
    strip_slash(mpd_client_state->music_directory_value);
    
    //set feat_library
    if (sdslen(mpd_client_state->music_directory_value) == 0) {
        LOG_WARN("Disabling featLibrary support");
        mpd_client_state->feat_library = false;
    }
    else if (testdir("MPD music_directory", mpd_client_state->music_directory_value, false) == 0) {
        LOG_INFO("Enabling featLibrary support");
        mpd_client_state->feat_library = true;
    }
    else {
        LOG_WARN("Disabling featLibrary support");
        mpd_client_state->feat_library = false;
        mpd_client_state->music_directory_value = sdscrop(mpd_client_state->music_directory_value);
    }
    
    if (mpd_client_state->feat_library == false) {
        if (mpd_connection_cmp_server_version(mpd_client_state->mpd_state->conn, 0, 21, 0) < 0) {
            LOG_WARN("Disabling coverimage support");
            mpd_client_state->feat_coverimage = false;
        }
    }
}
