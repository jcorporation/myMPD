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
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <pthread.h>
#include <mpd/client.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "features.h"
#include "../dist/src/sds/sds.h"

void mpd_client_feature_tags(t_mpd_state *mpd_state) {
    size_t max_len = 1024;
    char logline[max_len];
    size_t len = 0;
    char s[] = ",";
    char *taglist = strdup(mpd_state->taglist);
    char *searchtaglist = strdup(mpd_state->searchtaglist);    
    char *browsetaglist = strdup(mpd_state->browsetaglist);
    char *token = NULL;
    char *rest = NULL;
    struct mpd_pair *pair;

    mpd_state->mpd_tag_types_len = 0;
    memset(mpd_state->mpd_tag_types, 0, sizeof(mpd_state->mpd_tag_types));
    mpd_state->mympd_tag_types_len = 0;
    memset(mpd_state->mympd_tag_types, 0, sizeof(mpd_state->mympd_tag_types));
    mpd_state->search_tag_types_len = 0;
    memset(mpd_state->search_tag_types, 0, sizeof(mpd_state->search_tag_types));
    mpd_state->browse_tag_types_len = 0;
    memset(mpd_state->browse_tag_types, 0, sizeof(mpd_state->browse_tag_types));
    
    len = snprintf(logline, max_len, "MPD supported tags: ");
    if (mpd_send_list_tag_types(mpd_state->conn)) {
        while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                len += snprintf(logline + len, max_len - len, "%s ", pair->value);
                mpd_state->mpd_tag_types[mpd_state->mpd_tag_types_len++] = tag;
            }
            else {
                LOG_WARN("Unknown tag %s (libmpdclient to old)", pair->value);
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_list_tag_types");
    }

    if (mpd_state->mpd_tag_types_len == 0) {
        len += snprintf(logline + len, max_len -len, "none");
        LOG_INFO(logline);
        LOG_INFO("Tags are disabled");
        mpd_state->feat_tags = false;
    }
    else {
        mpd_state->feat_tags = true;
        LOG_INFO(logline);
        len = snprintf(logline, max_len, "myMPD enabled tags: ");
        token = strtok_r(taglist, s, &rest);
        while (token != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(token);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mpd_tag_types, mpd_state->mpd_tag_types_len, tag) == true) {
                    len += snprintf(logline + len, max_len - len, "%s ", mpd_tag_name(tag));
                    mpd_state->mympd_tag_types[mpd_state->mympd_tag_types_len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO(logline);
        #if LIBMPDCLIENT_CHECK_VERSION(2,12,0)
        if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
            LOG_VERBOSE("Enabling mpd tag types");
            if (mpd_command_list_begin(mpd_state->conn, false)) {
                mpd_send_clear_tag_types(mpd_state->conn);
                mpd_send_enable_tag_types(mpd_state->conn, mpd_state->mympd_tag_types, mpd_state->mympd_tag_types_len);
                if (!mpd_command_list_end(mpd_state->conn)) {
                    LOG_ERROR_AND_RECOVER("mpd_command_list_end");
                }
            }
            else {
                LOG_ERROR_AND_RECOVER("mpd_command_list_begin");
            }
            mpd_response_finish(mpd_state->conn);
        }
        #endif
        len = snprintf(logline, max_len, "myMPD enabled searchtags: ");
        token = strtok_r(searchtaglist, s, &rest);
        while (token != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(token);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mympd_tag_types, mpd_state->mympd_tag_types_len, tag) == true) {
                    len += snprintf(logline + len, max_len - len, "%s ", mpd_tag_name(tag));
                    mpd_state->search_tag_types[mpd_state->search_tag_types_len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO(logline);
        len = snprintf(logline, max_len, "myMPD enabled browsetags: ");
        token = strtok_r(browsetaglist, s, &rest);
        while (token != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(token);
            if (tag == MPD_TAG_UNKNOWN) {
                LOG_WARN("Unknown tag %s", token);
            }
            else {
                if (mpd_client_tag_exists(mpd_state->mympd_tag_types, mpd_state->mympd_tag_types_len, tag) == true) {
                    len += snprintf(logline + len, max_len - len, "%s ", mpd_tag_name(tag));
                    mpd_state->browse_tag_types[mpd_state->browse_tag_types_len++] = tag;
                }
                else {
                    LOG_DEBUG("Disabling tag %s", mpd_tag_name(tag));
                }
            }
            token = strtok_r(NULL, s, &rest);
        }
        LOG_INFO(logline);
    }
    FREE_PTR(taglist);
    FREE_PTR(searchtaglist);
    FREE_PTR(browsetaglist);

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

void mpd_client_feature_music_directory(t_mpd_state *mpd_state) {
    struct mpd_pair *pair;
    mpd_state->feat_library = false;
    mpd_state->feat_coverimage = mpd_state->coverimage;
    FREE_PTR(mpd_state->music_directory_value);
    mpd_state->music_directory_value = strdup("");

    if (strncmp(mpd_state->mpd_host, "/", 1) == 0 && strncmp(mpd_state->music_directory, "auto", 4) == 0) {
        //get musicdirectory from mpd
        if (mpd_send_command(mpd_state->conn, "config", NULL)) {
            while ((pair = mpd_recv_pair(mpd_state->conn)) != NULL) {
                if (strcmp(pair->name, "music_directory") == 0) {
                    if (strncmp(pair->value, "smb://", 6) != 0 && strncmp(pair->value, "nfs://", 6) != 0) {
                        FREE_PTR(mpd_state->music_directory_value);
                        mpd_state->music_directory_value = strdup(pair->value);
                    }
                }
                mpd_return_pair(mpd_state->conn, pair);
            }
            mpd_response_finish(mpd_state->conn);
        }
        else {
            LOG_ERROR_AND_RECOVER("config");
        }
    }
    else if (strncmp(mpd_state->music_directory, "/", 1) == 0) {
        FREE_PTR(mpd_state->music_directory_value);
        mpd_state->music_directory_value = strdup(mpd_state->music_directory);
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
        FREE_PTR(mpd_state->music_directory_value);
        mpd_state->music_directory_value = strdup("");
    }
    
    if (mpd_state->feat_library == false) {
        LOG_WARN("Disabling coverimage support");
        mpd_state->feat_coverimage = false;
    }

    //push music_directory setting to web_server_queue
    t_work_result *web_server_response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(web_server_response);
    web_server_response->conn_id = -1;
    web_server_response->length = snprintf(web_server_response->data, MAX_SIZE, 
        "{\"musicDirectory\":\"%s\", \"featLibrary\": %s}",
        mpd_state->music_directory_value,
        mpd_state->feat_library == true ? "true" : "false"
    );
    tiny_queue_push(web_server_queue, web_server_response);
}

bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag) {
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
	    return true;
	}
   }
   return false;
}

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
                LOG_DEBUG("MPD supports fingerprints");
                mpd_state->feat_fingerprint = true;
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_state->conn);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_send_allowed_commands");
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
