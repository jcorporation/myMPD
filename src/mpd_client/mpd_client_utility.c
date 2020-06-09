/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
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

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../list.h"
#include "config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"

//private definitons
static void detect_extra_files(t_mpd_client_state *mpd_client_state, const char *uri, bool *booklet, struct list *images);

//public functions

sds put_extra_files(t_mpd_client_state *mpd_client_state, sds buffer, const char *uri) {
    bool booklet = false;
    struct list images;
    list_init(&images);
    detect_extra_files(mpd_client_state, uri, &booklet, &images);
    buffer = tojson_bool(buffer, "booklet", booklet, true);
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
    return buffer;
}

void json_to_tags(const char *str, int len, void *user_data) {
    struct json_token t;
    int i;
    t_tags *tags = (t_tags *) user_data;
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

bool is_smartpls(t_config *config, t_mpd_client_state *mpd_client_state, const char *plpath) {
    bool smartpls = false;
    if (mpd_client_state->feat_smartpls == true) {
        sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->varlibdir, plpath);
        if (validate_string(plpath) == true) {
            if (access(smartpls_file, F_OK ) != -1) { /* Flawfinder: ignore */
                smartpls = true;
            }
        }
        sdsfree(smartpls_file);
    }
    return smartpls;
}

void default_mpd_client_state(t_mpd_client_state *mpd_client_state) {
    mpd_client_state->song_id = -1;
    mpd_client_state->song_uri = sdsempty();
    mpd_client_state->next_song_id = -1;
    mpd_client_state->last_song_id = -1;
    mpd_client_state->last_song_uri = sdsempty();
    mpd_client_state->queue_version = 0;
    mpd_client_state->queue_length = 0;
    mpd_client_state->last_last_played_id = -1;
    mpd_client_state->song_end_time = 0;
    mpd_client_state->song_start_time = 0;
    mpd_client_state->last_song_end_time = 0;
    mpd_client_state->last_song_start_time = 0;
    mpd_client_state->last_skipped_id = 0;
    mpd_client_state->crossfade = 0;
    mpd_client_state->set_song_played_time = 0;
    mpd_client_state->music_directory = sdsempty();
    mpd_client_state->music_directory_value = sdsempty();
    mpd_client_state->jukebox_mode = JUKEBOX_OFF;
    mpd_client_state->jukebox_playlist = sdsempty();
    mpd_client_state->jukebox_unique_tag.len = 1;
    mpd_client_state->jukebox_unique_tag.tags[0] = MPD_TAG_ARTIST;
    mpd_client_state->jukebox_last_played = 24;
    mpd_client_state->jukebox_queue_length = 1;
    mpd_client_state->coverimage_name = sdsempty();
    mpd_client_state->love_channel = sdsempty();
    mpd_client_state->love_message = sdsempty();
    mpd_client_state->searchtaglist = sdsempty();
    mpd_client_state->browsetaglist = sdsempty();
    mpd_client_state->generate_pls_tags = sdsempty();
    mpd_client_state->smartpls_sort = sdsempty();
    mpd_client_state->smartpls_prefix = sdsempty();
    mpd_client_state->smartpls_interval = 14400;
    mpd_client_state->booklet_name = sdsnew("booklet.pdf");
    reset_t_tags(&mpd_client_state->search_tag_types);
    reset_t_tags(&mpd_client_state->browse_tag_types);
    reset_t_tags(&mpd_client_state->generate_pls_tag_types);
    //init last played songs list
    list_init(&mpd_client_state->last_played);
    //jukebox queue
    list_init(&mpd_client_state->jukebox_queue);
    list_init(&mpd_client_state->jukebox_queue_tmp);
    mpd_client_state->sticker_cache = NULL;
    //mpd state
    mpd_client_state->mpd_state = (t_mpd_state *)malloc(sizeof(t_mpd_state));
    mpd_shared_default_mpd_state(mpd_client_state->mpd_state);
}

void free_mpd_client_state(t_mpd_client_state *mpd_client_state) {
    sdsfree(mpd_client_state->music_directory);
    sdsfree(mpd_client_state->music_directory_value);
    sdsfree(mpd_client_state->jukebox_playlist);
    sdsfree(mpd_client_state->song_uri);
    sdsfree(mpd_client_state->last_song_uri);
    sdsfree(mpd_client_state->coverimage_name);
    sdsfree(mpd_client_state->love_channel);
    sdsfree(mpd_client_state->love_message);
    sdsfree(mpd_client_state->searchtaglist);
    sdsfree(mpd_client_state->browsetaglist);
    sdsfree(mpd_client_state->generate_pls_tags);
    sdsfree(mpd_client_state->smartpls_sort);
    sdsfree(mpd_client_state->smartpls_prefix);
    sdsfree(mpd_client_state->booklet_name);
    list_free(&mpd_client_state->jukebox_queue);
    list_free(&mpd_client_state->jukebox_queue_tmp);
    //mpd state
    mpd_shared_free_mpd_state(mpd_client_state->mpd_state);
    free(mpd_client_state);
    mpd_client_state = NULL;
}


//private functions

static void detect_extra_files(t_mpd_client_state *mpd_client_state, const char *uri, bool *booklet, struct list *images) {
    *booklet = false;
  
    char *uricpy = strdup(uri);
    
    char *filename = basename(uricpy);
    strip_extension(filename);
    
    char *path = dirname(uricpy);
    sds albumpath = sdscatfmt(sdsempty(), "%s/%s", mpd_client_state->music_directory_value, path);
    
    DIR *album_dir = opendir(albumpath);
    if (album_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(album_dir)) != NULL) {
            const char *ext = strrchr(next_file->d_name, '.');
            if (strcmp(next_file->d_name, mpd_client_state->booklet_name) == 0) {
                LOG_DEBUG("Found booklet for uri %s", uri);
                *booklet = true;
            }
            else if (ext != NULL) {
                if (strcmp(ext, ".webp") == 0 || strcmp(ext, ".jpg") == 0 ||
                    strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".png") == 0 ||
                    strcmp(ext, ".tiff") == 0 || strcmp(ext, ".svg") == 0 ||
                    strcmp(ext, ".bmp") == 0) 
                {
                    sds fullpath = sdscatfmt(sdsempty(), "%s/%s", path, next_file->d_name);
                    list_push(images, fullpath, 0, NULL, NULL);
                    sdsfree(fullpath);
                }
            }
        }
        closedir(album_dir);
    }
    FREE_PTR(uricpy);
    sdsfree(albumpath);
}
