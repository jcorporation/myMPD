/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <libgen.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../plugins.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"

sds mpd_client_get_cover(t_config *config, t_mpd_state *mpd_state, const char *uri, sds cover) {
    char *orgpath = strdup(uri);
    char *path = orgpath;

    if (mpd_state->feat_coverimage == false) {
        cover = sdsreplace(cover, "/assets/coverimage-notavailable.svg");
    }
    else if (strstr(path, "://") != NULL) {
        char *name = strstr(path, "://");
        name += 3;
        replacechar(name, '/', '_');
        replacechar(name, '.', '_');
        replacechar(name, ':', '_');
        cover = sdscatfmt(cover, "%s/pics/%s.png", config->varlibdir, name);
        LOG_DEBUG("Check for cover %s", cover);
        if (access(cover, F_OK ) == -1 ) { /* Flawfinder: ignore */
            cover = sdscrop(cover);
            cover = sdscatfmt(cover, "/assets/coverimage-stream.svg");
        }
        else {
            cover = sdscrop(cover);
            cover = sdscatfmt(cover, "/pics/%s.png", name);
        }
    }
    else if (mpd_state->feat_library == true && sdslen(mpd_state->music_directory_value) > 0) {
        dirname(path);
        cover = sdscrop(cover);
        cover = sdscatfmt(cover, "%s/%s/%s", mpd_state->music_directory_value, path, mpd_state->coverimage_name);
        if (access(cover, F_OK ) == -1 ) { /* Flawfinder: ignore */
            if (config->plugins_coverextract == true) {
                sds media_file = sdscatfmt(sdsempty(), "%s/%s", mpd_state->music_directory_value, uri);
                size_t image_file_len = 1500;
                char image_file[image_file_len];
                size_t image_mime_type_len = 100;
                char image_mime_type[image_mime_type_len];
                bool rc = plugin_coverextract(media_file, "", image_file, image_file_len, image_mime_type, image_mime_type_len, false);
                sdsfree(media_file);
                if (rc == true) {
                    cover = sdscrop(cover);
                    cover = sdscatfmt(cover, "/albumart/%s", uri);
                }
                else {
                    cover = sdsreplace(cover, "/assets/coverimage-notavailable.svg");
                }
            }
            else {
                cover = sdsreplace(cover, "/assets/coverimage-notavailable.svg");
            }
        }
        else {
            cover = sdscrop(cover);
            cover = sdscatfmt(cover, "/library/%s/%s", path, mpd_state->coverimage_name);
        }
    }
    else {
        cover = sdsreplace(cover, "/assets/coverimage-notavailable.svg");
    }

    FREE_PTR(orgpath);
    return cover;
}
