/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "../mympd_state.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"

//private definitons
static void detect_extra_files(struct t_mympd_state *mympd_state, const char *uri, sds *booklet_path, struct list *images, bool is_dirname);

//public functions
bool caches_init(struct t_mympd_state *mympd_state) {
    if (mympd_state->mpd_state->feat_stickers == true || mympd_state->mpd_state->feat_tags == true) {
        //push cache building request to mpd_worker thread
        if (mympd_state->mpd_state->feat_stickers == true) {
            mympd_state->sticker_cache_building = true;
        }
        if (mympd_state->mpd_state->feat_tags == true) {
            mympd_state->album_cache_building = true;
        }
        t_work_request *request = create_request(-1, 0, MPDWORKER_API_CACHES_CREATE, "MPDWORKER_API_CACHES_CREATE", "");
        request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPDWORKER_API_CACHES_CREATE\",\"params\":{}}");
        tiny_queue_push(mpd_worker_queue, request, 0);
    }
    else {
        MYMPD_LOG_INFO("Caches creation skipped, sticker_cache and tags are disabled");
    }
    return true;
}

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

void json_to_tags(const char *str, int len, void *user_data) {
    struct json_token t;
    int i;
    struct t_tags *tags = (struct t_tags *) user_data;
    tags->len = 0;
    for (i = 0; json_scanf_array_elem(str, len, "", i, &t) > 0; i++) {
        sds token = sdscatlen(sdsempty(), t.ptr, t.len);
        enum mpd_tag_type tag = mpd_tag_name_iparse(token);
        sdsfree(token);
        if (tag != MPD_TAG_UNKNOWN) {
            tags->tags[tags->len++] = tag;
        }
    }
}

bool is_smartpls(struct t_mympd_state *mympd_state, const char *plpath) {
    bool smartpls = false;
    sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, plpath);
    if (validate_string(plpath) == true) {
        if (access(smartpls_file, F_OK ) != -1) { /* Flawfinder: ignore */
            smartpls = true;
        }
    }
    sdsfree(smartpls_file);
    return smartpls;
}

bool mpd_client_set_binarylimit(struct t_mympd_state *mympd_state) {
    bool rc = false;
    if (mpd_connection_cmp_server_version(mympd_state->mpd_state->conn, 0, 22, 4) >= 0 ) {
        rc = mpd_run_binarylimit(mympd_state->mpd_state->conn, mympd_state->mpd_state->binarylimit);
        check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_binarylimit");
    }
    MYMPD_LOG_DEBUG("binarylimit command not supported, depends on mpd >= 0.22.4");
    return rc;
}

//private functions
static void detect_extra_files(struct t_mympd_state *mympd_state, const char *uri, sds *booklet_path, struct list *images, bool is_dirname) {
    char *uricpy = strdup(uri);
    
    const char *path = is_dirname == false ? dirname(uricpy) : uri;
    sds albumpath = sdscatfmt(sdsempty(), "%s/%s", mympd_state->music_directory_value, path);
    MYMPD_LOG_DEBUG("Read extra files from albumpath: %s", albumpath);
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
        MYMPD_LOG_ERROR("Can not open directory \"%s\" to get list of extra files: %s", albumpath, strerror(errno));
    }
    FREE_PTR(uricpy);
    sdsfree(albumpath);
}
