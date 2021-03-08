/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../list.h"
#include "../log.h"
#include "mpd_shared_typedefs.h"
#include "mpd_shared_tags.h"
#include "../mpd_shared.h"

bool mpd_shared_feat_mpd_searchwindow(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 20, 0) >= 0) {
        return true;
    }

    MYMPD_LOG_WARN("Disabling search window support, depends on mpd >= 0.20.0");
    return false;
}

bool mpd_shared_feat_advsearch(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        MYMPD_LOG_INFO("Enabling advanced search");
        return true;
    }

    MYMPD_LOG_WARN("Disabling advanced search, depends on mpd >= 0.21.0");
    return false;
}

void mpd_shared_feat_tags(t_mpd_state *mpd_state) {
    reset_t_tags(&mpd_state->mpd_tag_types);
    reset_t_tags(&mpd_state->mympd_tag_types);

    enable_all_mpd_tags(mpd_state);

    sds logline = sdsnew("MPD supported tags: ");
    if (mpd_send_list_tag_types(mpd_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_parse(pair->value);
            if (tag != MPD_TAG_UNKNOWN) {
                logline = sdscatfmt(logline, "%s ", pair->value);
                mpd_state->mpd_tag_types.tags[mpd_state->mpd_tag_types.len++] = tag;
            }
            else {
                MYMPD_LOG_WARN("Unknown tag %s (libmpdclient too old)", pair->value);
            }
            mpd_return_pair(mpd_state->conn, pair);
        }
    }
    else {
        MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_tag_types");
    }
    mpd_response_finish(mpd_state->conn);
    check_error_and_recover2(mpd_state, NULL, NULL, 0, false);

    if (mpd_state->mpd_tag_types.len == 0) {
        logline = sdscat(logline, "none");
        MYMPD_LOG_NOTICE(logline);
        MYMPD_LOG_NOTICE("Tags are disabled");
        mpd_state->feat_tags = false;
    }
    else {
        mpd_state->feat_tags = true;
        MYMPD_LOG_NOTICE(logline);

        check_tags(mpd_state->taglist, "mympdtags", &mpd_state->mympd_tag_types, mpd_state->mpd_tag_types);
        enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
    }
    sdsfree(logline);
}
