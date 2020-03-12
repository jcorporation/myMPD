/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
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
#include "mpd_client_utility.h"
#include "mpd_client_state.h"
#include "mpd_client_features.h"

//private definitions
static void mpd_client_feature_commands(t_mpd_state *mpd_state);
static void check_tags(sds taglist, const char *taglistname, t_tags *tagtypes, t_tags allowed_tagtypes);
static void mpd_client_feature_tags(t_mpd_state *mpd_state);
static void mpd_client_feature_music_directory(t_mpd_state *mpd_state);

//public functions
void mpd_client_mpd_features(t_config *config, t_mpd_state *mpd_state) {
    mpd_state->protocol = mpd_connection_get_server_version(mpd_state->conn);
    LOG_INFO("MPD protocoll version: %u.%u.%u", mpd_state->protocol[0], mpd_state->protocol[1], mpd_state->protocol[2]);

    // Defaults
    mpd_state->feat_sticker = false;
    mpd_state->feat_playlists = false;
    mpd_state->feat_tags = false;
    mpd_state->feat_advsearch = false;
    mpd_state->feat_fingerprint = false;
    mpd_state->feat_smartpls = mpd_state->smartpls;;
    mpd_state->feat_coverimage = true;
    mpd_state->feat_mpd_albumart = false;
    mpd_state->feat_mpd_readpicture = false;
    mpd_state->feat_mpd_searchwindow = false;
    mpd_state->feat_single_oneshot = false;
    
    //get features
    mpd_client_feature_commands(mpd_state);
    mpd_client_feature_music_directory(mpd_state);
    mpd_client_feature_love(mpd_state);
    mpd_client_feature_tags(mpd_state);
    
    //set state
    sds buffer = sdsempty();
    buffer = mpd_client_put_state(config, mpd_state, buffer, NULL, 0);
    sdsfree(buffer);

    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 20, 0) >= 0) {
        mpd_state->feat_mpd_searchwindow = true;
    }
    else {
        LOG_WARN("Disabling searchwindow support, depends on mpd >= 0.20.0");
    }
    
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        mpd_state->feat_advsearch = true;
        LOG_INFO("Enabling advanced search");
    }
    else {
        LOG_WARN("Disabling advanced search, depends on mpd >= 0.21.0");
    }
    
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        mpd_state->feat_single_oneshot = true;
        LOG_INFO("Enabling single oneshot feature");
    } 
    else {
        LOG_WARN("Disabling single oneshot feature, depends on mpd >= 0.21.0");
    }
    
    //push settings to web_server_queue
    t_work_result *web_server_response = create_result_new(-1, 0, 0, "");
    sds data = sdsnew("{");
    data = tojson_char(data, "musicDirectory", mpd_state->music_directory_value, true);
    data = tojson_char(data, "playlistDirectory", config->playlist_directory, true);
    data = tojson_char(data, "coverimageName", mpd_state->coverimage_name, true);
    data = tojson_bool(data, "featLibrary", mpd_state->feat_library, false);
    data = tojson_bool(data, "featMpdAlbumart", mpd_state->feat_mpd_albumart, false);
    data = sdscat(data, "}");
    web_server_response->data = sdsreplace(web_server_response->data, data);
    sdsfree(data);
    tiny_queue_push(web_server_queue, web_server_response);
}

void mpd_client_feature_love(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    mpd_state->feat_love = false;
    if (mpd_state->love == true) {
        if (mpd_send_channels(mpd_state->conn)) {
            while ((pair = mpd_recv_channel_pair(mpd_state->conn)) != NULL) {
                if (strcmp(pair->value, mpd_state->love_channel) == 0) {
                    mpd_state->feat_love = true;
                }
                mpd_return_pair(mpd_state->conn, pair);            
            }
        }
        mpd_response_finish(mpd_state->conn);
        if (mpd_state->feat_love == false) {
            LOG_WARN("Disabling featLove, channel %s not found", mpd_state->love_channel);
        }
        else {
            LOG_INFO("Enabling featLove, channel %s found", mpd_state->love_channel);
        }
    }
}

//private functions
static void mpd_client_feature_commands(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    if (mpd_send_allowed_commands(mpd_state->conn)) {
        while ((pair = mpd_recv_command_pair(mpd_state->conn)) != NULL) {
            if (strcmp(pair->value, "sticker") == 0) {
                LOG_DEBUG("MPD supports stickers");
                mpd_state->feat_sticker = true;
            }
            else if (strcmp(pair->value, "listplaylists") == 0) {
                LOG_DEBUG("MPD supports playlists");
                mpd_state->feat_playlists = true;
            }
            else if (strcmp(pair->value, "getfingerprint") == 0) {
                LOG_DEBUG("MPD supports fingerprint");
                mpd_state->feat_fingerprint = true;
            }
            else if (strcmp(pair->value, "albumart") == 0) {
                LOG_DEBUG("MPD supports albumart");
                mpd_state->feat_mpd_albumart = true;

            }
            else if (strcmp(pair->value, "readpicture") == 0) {
                LOG_DEBUG("MPD supports readpicture");
                mpd_state->feat_mpd_readpicture = true;
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
    }
    if (mpd_state->feat_sticker == false && mpd_state->stickers == true) {
        LOG_WARN("MPD don't support stickers, disabling myMPD feature");
        mpd_state->feat_sticker = false;
    }
    if (mpd_state->feat_sticker == true && mpd_state->stickers == false) {
        mpd_state->feat_sticker = false;
    }
    if (mpd_state->feat_sticker == false && mpd_state->smartpls == true) {
        LOG_WARN("Stickers are disabled, disabling smart playlists");
        mpd_state->feat_smartpls = false;
    }
    if (mpd_state->feat_playlists == false && mpd_state->smartpls == true) {
        LOG_WARN("Playlists are disabled, disabling smart playlists");
        mpd_state->feat_smartpls = false;
    }
}

static void mpd_client_feature_tags(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;

    reset_t_tags(&mpd_state->mpd_tag_types);
    reset_t_tags(&mpd_state->mympd_tag_types);
    reset_t_tags(&mpd_state->search_tag_types);
    reset_t_tags(&mpd_state->browse_tag_types);
    reset_t_tags(&mpd_state->generate_pls_tag_types);

    enable_all_mpd_tags(mpd_state);
    
    sds logline = sdsnew("MPD supported tags: ");
    mpd_send_list_tag_types(mpd_state->conn);
    while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
        enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
        if (tag != MPD_TAG_UNKNOWN) {
            logline = sdscatfmt(logline, "%s ", pair->value);
            mpd_state->mpd_tag_types.tags[mpd_state->mpd_tag_types.len++] = tag;
        }
        else {
            LOG_WARN("Unknown tag %s (libmpdclient too old)", pair->value);
        }
        mpd_return_pair(mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_state->conn);
    check_error_and_recover2(mpd_state, NULL, NULL, 0, false);

    if (mpd_state->mpd_tag_types.len == 0) {
        logline = sdscat(logline, "none");
        LOG_INFO(logline);
        LOG_INFO("Tags are disabled");
        mpd_state->feat_tags = false;
    }
    else {
        mpd_state->feat_tags = true;
        LOG_INFO(logline);
        
        check_tags(mpd_state->taglist, "mympdtags", &mpd_state->mympd_tag_types, mpd_state->mpd_tag_types);
        
        enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
        
        check_tags(mpd_state->searchtaglist, "searchtags", &mpd_state->search_tag_types, mpd_state->mympd_tag_types);
        check_tags(mpd_state->browsetaglist, "browsetags", &mpd_state->browse_tag_types, mpd_state->mympd_tag_types);
        check_tags(mpd_state->generate_pls_tags, "generate pls tags", &mpd_state->generate_pls_tag_types, mpd_state->mympd_tag_types);
    }
    sdsfree(logline);
}

static void check_tags(sds taglist, const char *taglistname, t_tags *tagtypes, t_tags allowed_tag_types) {
    sds logline = sdscatfmt(sdsempty(), "Enabled %s: ", taglistname);
    int tokens_count;
    sds *tokens = sdssplitlen(taglist, sdslen(taglist), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            LOG_WARN("Unknown tag %s", tokens[i]);
        }
        else {
            if (mpd_client_tag_exists(allowed_tag_types.tags, allowed_tag_types.len, tag) == true) {
                logline = sdscatfmt(logline, "%s ", mpd_tag_name(tag));
                tagtypes->tags[tagtypes->len++] = tag;
            }
            else {
                LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
            }
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    LOG_INFO(logline);
    sdsfree(logline);
}

static void mpd_client_feature_music_directory(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    mpd_state->feat_library = false;
    mpd_state->feat_coverimage = mpd_state->coverimage;
    mpd_state->music_directory_value = sdscrop(mpd_state->music_directory_value);

    if (strncmp(mpd_state->mpd_host, "/", 1) == 0 && strncmp(mpd_state->music_directory, "auto", 4) == 0) {
        //get musicdirectory from mpd
        if (mpd_send_command(mpd_state->conn, "config", NULL)) {
            while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0) {
                    if (strncmp(pair->value, "smb://", 6) != 0 && strncmp(pair->value, "nfs://", 6) != 0) {
                        mpd_state->music_directory_value = sdsreplace(mpd_state->music_directory_value, pair->value);
                    }
                }
                mpd_return_pair(mpd_state->conn, pair);
            }
            mpd_response_finish(mpd_state->conn);
        }
        if (check_error_and_recover2(mpd_state, NULL, NULL, 0, false) == false) {
            LOG_ERROR("Can't get music_directory value from mpd");
        }
    }
    else if (strncmp(mpd_state->music_directory, "/", 1) == 0) {
        mpd_state->music_directory_value = sdsreplace(mpd_state->music_directory_value, mpd_state->music_directory);
    }
    else {
        //none or garbage, empty music_directory_value
    }
    
    //set feat_library
    if (sdslen(mpd_state->music_directory_value) == 0) {
        LOG_WARN("Disabling featLibrary support");
        mpd_state->feat_library = false;
    }
    else if (testdir("MPD music_directory", mpd_state->music_directory_value, false) == 0) {
        LOG_INFO("Enabling featLibrary support");
        mpd_state->feat_library = true;
    }
    else {
        LOG_WARN("Disabling featLibrary support");
        mpd_state->feat_library = false;
        mpd_state->music_directory_value = sdscrop(mpd_state->music_directory_value);
    }
    
    if (mpd_state->feat_library == false) {
        if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) < 0) {
            LOG_WARN("Disabling coverimage support");
            mpd_state->feat_coverimage = false;
        }
    }
}
