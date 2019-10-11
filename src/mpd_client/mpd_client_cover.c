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
#include <stdbool.h>
#include <unistd.h>
#include <libgen.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
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
        cover = sdscat(sdsempty(), "/assets/coverimage-notavailable.svg");
    }
    else if (strstr(path, "://") != NULL) {
        char *name = strstr(path, "://");
        name += 3;
        replacechar(name, '/', '_');
        replacechar(name, '.', '_');
        replacechar(name, ':', '_');
        cover = sdscatfmt(sdsempty(), "%s/pics/%s.png", config->varlibdir, name);
        LOG_DEBUG("Check for cover %s", cover);
        if (access(cover, F_OK ) == -1 ) { /* Flawfinder: ignore */
            cover = sdscatfmt(sdsempty(), "/assets/coverimage-stream.svg");
        }
        else {
            cover = sdscatfmt(sdsempty(), "/pics/%s.png", name);
        }
    }
    else if (mpd_state->feat_library == true && sdslen(mpd_state->music_directory_value) > 0) {
        dirname(path);
        cover = sdscatfmt(sdsempty(), "%s/%s/%s", mpd_state->music_directory_value, path, mpd_state->coverimage_name);
        if (access(cover, F_OK ) == -1 ) { /* Flawfinder: ignore */
            if (config->plugins_coverextract == true) {
                sds media_file = sdscatfmt(sdsempty(), "%s/%s", mpd_state->music_directory_value, uri);
                size_t image_file_len = 1500;
                char image_file[image_file_len];
                size_t image_mime_type_len = 100;
                char image_mime_type[image_mime_type_len];
                bool rc = plugin_coverextract(media_file, "", image_file, image_file_len, image_mime_type, image_mime_type_len, false);
                if (rc == true) {
                    cover = sdscatfmt(sdsempty(), "/albumart/%s", uri);
                }
                else {
                    cover = sdscatfmt(sdsempty(), "/assets/coverimage-notavailable.svg");
                }
            }
            else {
                cover = sdscatfmt(sdsempty(), "/assets/coverimage-notavailable.svg");
            }
        }
        else {
            cover = sdscatfmt(sdsempty(), "/library/%s/%s", path, mpd_state->coverimage_name);
        }
    }
    else {
        cover = sdscatfmt(sdsempty(), "/assets/coverimage-notavailable.svg");
    }

    FREE_PTR(orgpath);
    return cover;
}
