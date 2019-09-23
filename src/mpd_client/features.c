/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../log.h"
#include "../list.h"
#include "../config_defs.h"
#include "mpd_client_utils.h"
#include "state.h"
#include "features.h"

//private definitions
static void mpd_client_feature_love(t_mpd_state *mpd_state);
static void mpd_client_feature_tags(t_mpd_state *mpd_state);
static void mpd_client_feature_music_directory(t_mpd_state *mpd_state);

//public functions
void mpd_client_mpd_features(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;

    mpd_state->protocol = mpd_connection_get_server_version(mpd_state->conn);
    LOG_INFO("MPD protocoll version: %u.%u.%u", mpd_state->protocol[0], mpd_state->protocol[1], mpd_state->protocol[2]);

    // Defaults
    mpd_state->feat_sticker = false;
    mpd_state->feat_playlists = false;
    mpd_state->feat_tags = false;
    mpd_state->feat_advsearch = false;
    mpd_state->feat_fingerprint = false;
    mpd_state->feat_smartpls = mpd_state->smartpls;
    mpd_state->feat_coverimage = true;
    
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
                LOG_DEBUG("MPD supports fingerprint command");
                if (LIBMPDCLIENT_CHECK_VERSION(2, 17, 0)) {
                    mpd_state->feat_fingerprint = true;
                }
                else {
                    LOG_DEBUG("libmpdclient don't support fingerprint command");
                }
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        check_error_and_recover(NULL, NULL, 0);
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

    mpd_client_feature_music_directory(mpd_state);
    mpd_client_feature_tags(mpd_state);
    mpd_client_feature_love(mpd_state);
    mpd_client_get_state(mpd_state, NULL);
    
    if (LIBMPDCLIENT_CHECK_VERSION(2, 17, 0) && mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        mpd_state->feat_advsearch = true;
        LOG_INFO("Enabling advanced search");
    } 
    else {
        LOG_WARN("Disabling advanced search, depends on mpd >= 0.21.0 and libmpdclient >= 2.17.0.");
    }
}

//private functions
static void mpd_client_feature_tags(t_mpd_state *mpd_state) {
    sds taglist = sdsnew(mpd_state->taglist);
    sds searchtaglist = sdsnew(mpd_state->searchtaglist);    
    sds browsetaglist = sdsnew(mpd_state->browsetaglist);
    sds *tokens;
    int tokens_count;
    struct mpd_pair *pair;

    reset_t_tags(mpd_state->mpd_tag_types);
    reset_t_tags(mpd_state->mympd_tag_types);
    reset_t_tags(mpd_state->search_tag_types);
    reset_t_tags(mpd_state->browse_tag_types);
    
    sds logline = sdsnew("MPD supported tags: ");
    if (mpd_send_list_tag_types(mpd_state->conn)) {
        while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                logline = sdscatprintf(logline, "%s ", pair->value);
                mpd_state->mpd_tag_types->tags[mpd_state->mpd_tag_types->len++] = tag;
            }
            else {
                LOG_WARN("Unknown tag %s (libmpdclient to old)", pair->value);
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        check_error_and_recover(NULL, NULL, 0);
    }

    if (mpd_state->mpd_tag_types_len == 0) {
        logline = sdscat(logline, "none");
        LOG_INFO(logline);
        LOG_INFO("Tags are disabled");
        mpd_state->feat_tags = false;
    }
    else {
        mpd_state->feat_tags = true;
        LOG_INFO(logline);
        logline = sdscat(sdsempty(), "myMPD enabled tags: ");
        tokens = sdssplitlen(taglist, sdslen(taglist), ",", 1, &tokens_count);
        for (int i = 0; i < tokens_count; i++) {
            sdstrim(tokens[i], " ");
            enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mpd_tag_types->tags, mpd_state->mpd_tag_types->len, tag) == true) {
                    logline = sdscatprintf(logline, "%s ", mpd_tag_name(tag));
                    mpd_state->mympd_tag_types->tags[mpd_state->mympd_tag_types->len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
        }
        sdsfreesplitres(tokens, tokens_count);
        LOG_INFO(logline);
        
        #if LIBMPDCLIENT_CHECK_VERSION(2,12,0)
        if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
            LOG_VERBOSE("Enabling mpd tag types");
            if (mpd_command_list_begin(mpd_state->conn, false)) {
                mpd_send_clear_tag_types(mpd_state->conn);
                mpd_send_enable_tag_types(mpd_state->conn, mpd_state->mympd_tag_types->tags, mpd_state->mympd_tag_types->len);
                if (mpd_command_list_end(mpd_state->conn)) {
                    mpd_response_finish(mpd_state->conn);
                }
            }
            check_error_and_recover(NULL);
        }
        #endif
        logline = sdscat(sdsempty(), "myMPD enabled searchtags: ");
        tokens = sdssplitlen(searchtaglist, sdslen(searchtaglist), ",", 1, &tokens_count);
        for (int i = 0; i < tokens_count; i++) {
            sdstrim(tokens[i], " ");
            enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mympd_tag_types->tags, mpd_state->mympd_tag_types->len, tag) == true) {
                    logline = sdscatprintf(logline, "%s ", mpd_tag_name(tag));
                    mpd_state->search_tag_types->tags[mpd_state->search_tag_types->len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
        }
        sdsfreesplitres(tokens, tokens_count);
        LOG_INFO(logline);

        logline = sdscat(sdsempty(), "myMPD enabled browsetags: ");
        tokens = sdssplitlen(browsetaglist, sdslen(browsetaglist), ",", 1, &tokens_count);
        for (int i = 0; i < tokens_count; i++) {
            sdstrim(tokens[i], " ");
            enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mympd_tag_types->tags, mpd_state->mympd_tag_types->len, tag) == true) {
                    logline = sdscatprintf(logline, "%s ", mpd_tag_name(tag));
                    mpd_state->browse_tag_types->tags[mpd_state->browse_tag_types->len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
        }
        sdsfreesplitres(tokens, tokens_count);
        LOG_INFO(logline);
    }
    sds_free(logline);
    sds_free(taglist);
    sds_free(searchtaglist);
    sds_free(browsetaglist);
}

static void mpd_client_feature_love(t_mpd_state *mpd_state) {
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

static void mpd_client_feature_music_directory(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    mpd_state->feat_library = false;
    mpd_state->feat_coverimage = mpd_state->coverimage;
    mpd_state->music_directory_value = sdscat(sdsempty(), "");

    if (strncmp(mpd_state->mpd_host, "/", 1) == 0 && strncmp(mpd_state->music_directory, "auto", 4) == 0) {
        //get musicdirectory from mpd
        if (mpd_send_command(mpd_state->conn, "config", NULL)) {
            while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0) {
                    if (strncmp(pair->value, "smb://", 6) != 0 && strncmp(pair->value, "nfs://", 6) != 0) {
                        mpd_state->music_directory_value = sdscat(sdsempty(), pair->value);
                    }
                }
                mpd_return_pair(mpd_state->conn, pair);
            }
            mpd_response_finish(mpd_state->conn);
        }
        else {
            check_error_and_recover(NULL, NULL, 0);
        }
    }
    else if (strncmp(mpd_state->music_directory, "/", 1) == 0) {
        mpd_state->music_directory_value = sdscat(sdsempty(), mpd_state->music_directory);
    }
    else {
        //none or garbage, empty music_directory_value
    }
    
    //set feat_library
    if (strlen(mpd_state->music_directory_value) == 0) {
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
        mpd_state->music_directory_value = sdscat(sdsempty, "");;
    }
    
    if (mpd_state->feat_library == false) {
        LOG_WARN("Disabling coverimage support");
        mpd_state->feat_coverimage = false;
    }

    //push music_directory setting to web_server_queue
    t_work_result *web_server_response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(web_server_response);
    web_server_response->conn_id = -1;
    web_server_response->data = sdscatprintf(sdsempty(), "{\"musicDirectory\":\"%s\", \"featLibrary\": %s}",
        mpd_state->music_directory_value,
        mpd_state->feat_library == true ? "true" : "false"
    );
    tiny_queue_push(web_server_queue, web_server_response);
}
