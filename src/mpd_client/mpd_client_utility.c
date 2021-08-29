/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_utility.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_shared.h"

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private definitons
static void detect_extra_files(struct t_mympd_state *mympd_state, const char *uri, sds *booklet_path, struct list *images, bool is_dirname);

//public functions
sds put_extra_files(struct t_mympd_state *mympd_state, sds buffer, const char *uri, bool is_dirname) {
    struct list images;
    list_init(&images);
    sds booklet_path = sdsempty();
    if (is_streamuri(uri) == false && mympd_state->mpd_state->feat_library == true) {
        detect_extra_files(mympd_state, uri, &booklet_path, &images, is_dirname);
    }
    buffer = tojson_char(buffer, "bookletPath", booklet_path, true);
    buffer = sdscat(buffer, "\"images\": [");
    struct list_node *current = images.head;
    while (current != NULL) {
        if (current != images.head) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatjson(buffer, current->key, sdslen(current->key));
        current = current->next;
    }
    buffer = sdscat(buffer, "]");
    list_free(&images);
    sdsfree(booklet_path);
    return buffer;
}

bool is_smartpls(struct t_mympd_state *mympd_state, sds playlist) {
    bool smartpls = false;
    if (vcb_isfilename(playlist) == true) {
        sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, playlist);
        if (access(smartpls_file, F_OK ) != -1) { /* Flawfinder: ignore */
            smartpls = true;
        }
        sdsfree(smartpls_file);
    }
    return smartpls;
}

bool is_streamuri(const char *uri) {
    if (uri != NULL && strstr(uri, "://") != NULL) {
        return true;
    }
    return false;
}

bool mpd_client_set_binarylimit(struct t_mympd_state *mympd_state) {
    bool rc = true;
    if (mympd_state->mpd_state->feat_mpd_binarylimit == true) {
        MYMPD_LOG_INFO("Setting binarylimit to %u", mympd_state->mpd_state->mpd_binarylimit);
        rc = mpd_run_binarylimit(mympd_state->mpd_state->conn, mympd_state->mpd_state->mpd_binarylimit);
        sds message = sdsempty();
        rc = check_rc_error_and_recover(mympd_state->mpd_state, &message, NULL, 0, true, rc, "mpd_run_binarylimit");
        if (sdslen(message) > 0) {
            ws_notify(message);
            rc = false;
        }
        sdsfree(message);
    }
    return rc;
}

//replacement for deprecated mpd_status_get_elapsed_time
unsigned mpd_client_get_elapsed_seconds(struct mpd_status *status) {
    return mpd_status_get_elapsed_ms(status) / 1000;
}

//private functions
static void detect_extra_files(struct t_mympd_state *mympd_state, const char *uri, sds *booklet_path, struct list *images, bool is_dirname) {
    char *uricpy = strdup(uri);
    
    const char *path = is_dirname == false ? dirname(uricpy) : uri;
    sds albumpath = sdscatfmt(sdsempty(), "%s/%s", mympd_state->music_directory_value, path);
    MYMPD_LOG_DEBUG("Read extra files from albumpath: %s", albumpath);
    errno = 0;
    DIR *album_dir = opendir(albumpath);
    if (album_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(album_dir)) != NULL) {
            const char *ext = strrchr(next_file->d_name, '.');
            if (strcmp(next_file->d_name, mympd_state->booklet_name) == 0) {
                MYMPD_LOG_DEBUG("Found booklet for uri %s", uri);
                *booklet_path = sdscatfmt(*booklet_path, "%s/%s", path, mympd_state->booklet_name);
            }
            else if (ext != NULL) {
                if (strcasecmp(ext, ".webp") == 0 || strcasecmp(ext, ".jpg") == 0 ||
                    strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0 ||
                    strcasecmp(ext, ".tiff") == 0 || strcasecmp(ext, ".svg") == 0 ||
                    strcasecmp(ext, ".bmp") == 0) 
                {
                    sds fullpath = sdscatfmt(sdsempty(), "%s/%s", path, next_file->d_name);
                    list_push(images, fullpath, 0, NULL, NULL);
                    sdsfree(fullpath);
                }
            }
        }
        closedir(album_dir);
    }
    else {
        MYMPD_LOG_ERROR("Can not open directory \"%s\" to get list of extra files", albumpath);
        MYMPD_LOG_ERRNO(errno);
    }
    FREE_PTR(uricpy);
    sdsfree(albumpath);
}
