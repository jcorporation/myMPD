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
#include "mpd_client_utility.h"

sds put_extra_files(t_mpd_state *mpd_state, sds buffer, const char *uri) {
    bool lyrics = false;
    bool booklet = false;
    struct list images;
    list_init(&images);
    detect_extra_files(mpd_state, uri, &booklet, &lyrics, &images);
    buffer = tojson_bool(buffer, "booklet", booklet, true);
    buffer = tojson_bool(buffer, "lyricsfile", lyrics, true);
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

void detect_extra_files(t_mpd_state *mpd_state, const char *uri, bool *booklet, bool *lyrics, struct list *images) {
    *booklet = false;
    *lyrics = false;
  
    char *uricpy = strdup(uri);
    
    char *filename = basename(uricpy);
    strip_extension(filename);
    sds lyricsfile = sdscatfmt(sdsempty(), "%s.txt", filename);
    
    char *path = dirname(uricpy);
    sds albumpath = sdscatfmt(sdsempty(), "%s/%s", mpd_state->music_directory_value, path);
    
    DIR *album_dir = opendir(albumpath);
    if (album_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(album_dir)) != NULL) {
            const char *ext = strrchr(next_file->d_name, '.');
            if (strcmp(next_file->d_name, mpd_state->booklet_name) == 0) {
                LOG_DEBUG("Found booklet for uri %s", uri);
                *booklet = true;
            }
            else if (strcmp(next_file->d_name, lyricsfile) == 0) {
                LOG_DEBUG("Found lyrics %s", next_file->d_name);
                *lyrics = true;
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
    sdsfree(lyricsfile);
}

void disable_all_mpd_tags(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Disabling all mpd tag types");
        mpd_run_clear_tag_types(mpd_state->conn);
        check_error_and_recover2(mpd_state, NULL, NULL, 0, false);
    }
}

void enable_all_mpd_tags(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Enabling all mpd tag types");
        mpd_run_all_tag_types(mpd_state->conn);
        check_error_and_recover2(mpd_state, NULL, NULL, 0, false);
    }
}

void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Setting interesting mpd tag types");
        if (mpd_command_list_begin(mpd_state->conn, false)) {
            mpd_send_clear_tag_types(mpd_state->conn);
            if (enable_tags.len > 0) {
                mpd_send_enable_tag_types(mpd_state->conn, enable_tags.tags, enable_tags.len);
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        check_error_and_recover(mpd_state, NULL, NULL, 0);
    }
}

sds put_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const struct mpd_song *song) {
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            char *tag_value = mpd_client_get_tag(song, tagcols->tags[tagnr]);
            buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), tag_value == NULL ? "-" : tag_value, true);
        }
    }
    else {
        char *tag_value = mpd_client_get_tag(song, MPD_TAG_TITLE);
        buffer = tojson_char(buffer, "Title", tag_value == NULL ? "-" : tag_value, true);
    }
    buffer = tojson_long(buffer, "Duration", mpd_song_get_duration(song), true);
    buffer = tojson_long(buffer, "LastModified", mpd_song_get_last_modified(song), true);
    buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), false);
    return buffer;
}

sds put_empty_song_tags(sds buffer, t_mpd_state *mpd_state, const t_tags *tagcols, const char *uri) {
    if (mpd_state->feat_tags == true) {
        for (size_t tagnr = 0; tagnr < tagcols->len; ++tagnr) {
            if (tagcols->tags[tagnr] == MPD_TAG_TITLE) {
                buffer = tojson_char(buffer, "Title", basename((char *)uri), true);
            }
            else {
                buffer = tojson_char(buffer, mpd_tag_name(tagcols->tags[tagnr]), "-", true);
            }
        }
    }
    else {
        buffer = tojson_char(buffer, "Title", basename((char *)uri), true);
    }
    buffer = tojson_long(buffer, "Duration", 0, true);
    buffer = tojson_char(buffer, "uri", uri, false);
    return buffer;
}


bool check_error_and_recover2(t_mpd_state *mpd_state, sds *buffer, sds method, int request_id, bool notify) {
    enum mpd_error error = mpd_connection_get_error(mpd_state->conn);
    if (error  != MPD_ERROR_SUCCESS) {
        const char *error_msg = mpd_connection_get_error_message(mpd_state->conn);
        LOG_ERROR("MPD error: %s (%d)", error_msg , error);
        if (buffer != NULL) {
            if (*buffer != NULL) {
                if (notify == false) {
                    *buffer = jsonrpc_respond_message(*buffer, method, request_id, mpd_connection_get_error_message(mpd_state->conn), true);
                }
                else {
                    *buffer = jsonrpc_start_notify(*buffer, "error");
                    *buffer = tojson_char(*buffer, "message", mpd_connection_get_error_message(mpd_state->conn), false);
                    *buffer = jsonrpc_end_notify(*buffer);
                }
            }
        }

        if (error == 8 || //Connection closed by the server
            error == 5) { //Broken pipe
            mpd_state->conn_state = MPD_FAILURE;
        }
        mpd_connection_clear_error(mpd_state->conn);
        if (mpd_state->conn_state != MPD_FAILURE) {
            mpd_response_finish(mpd_state->conn);
            //enable default mpd tags after cleaning error
            enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
        }
        return false;
    }
    return true;
}

sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    check_error_and_recover2(mpd_state, &buffer, method, request_id, false);
    return buffer;
}

sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer) {
    check_error_and_recover2(mpd_state, &buffer, NULL, 0, true);
    return buffer;
}

sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    buffer = sdscrop(buffer);
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == true) {
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    }
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

char *mpd_client_get_tag(struct mpd_song const *song, const enum mpd_tag_type tag) {
    char *str = (char *)mpd_song_get_tag(song, tag, 0);
    if (str == NULL) {
        if (tag == MPD_TAG_TITLE) {
            str = basename((char *)mpd_song_get_uri(song));
        }
        else if (tag == MPD_TAG_ALBUM_ARTIST) {
            str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        }
    }
    return str;
}

bool is_smartpls(t_config *config, t_mpd_state *mpd_state, const char *plpath) {
    bool smartpls = false;
    if (mpd_state->feat_smartpls == true) {
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

bool mpd_client_tag_exists(const enum mpd_tag_type tag_types[64], const size_t tag_types_len, const enum mpd_tag_type tag) {
    for (size_t i = 0; i < tag_types_len; i++) {
        if (tag_types[i] == tag) {
            return true;
        }
    }
    return false;
}

void reset_t_tags(t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}

void default_mpd_state(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_interval = 0;
    mpd_state->timeout = 10000;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->song_id = -1;
    mpd_state->song_uri = sdsempty();
    mpd_state->next_song_id = -1;
    mpd_state->last_song_id = -1;
    mpd_state->last_song_uri = sdsempty();
    mpd_state->queue_version = 0;
    mpd_state->queue_length = 0;
    mpd_state->last_last_played_id = -1;
    mpd_state->song_end_time = 0;
    mpd_state->song_start_time = 0;
    mpd_state->last_song_end_time = 0;
    mpd_state->last_song_start_time = 0;
    mpd_state->last_skipped_id = 0;
    mpd_state->crossfade = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->music_directory = sdsempty();
    mpd_state->music_directory_value = sdsempty();
    mpd_state->jukebox_mode = JUKEBOX_OFF;
    mpd_state->jukebox_playlist = sdsempty();
    mpd_state->jukebox_unique_tag.len = 1;
    mpd_state->jukebox_unique_tag.tags[0] = MPD_TAG_ARTIST;
    mpd_state->jukebox_last_played = 24;
    mpd_state->jukebox_queue_length = 1;
    mpd_state->coverimage_name = sdsempty();
    mpd_state->love_channel = sdsempty();
    mpd_state->love_message = sdsempty();
    mpd_state->taglist = sdsempty();
    mpd_state->searchtaglist = sdsempty();
    mpd_state->browsetaglist = sdsempty();
    mpd_state->generate_pls_tags = sdsempty();
    mpd_state->mpd_host = sdsempty();
    mpd_state->mpd_port = 0;
    mpd_state->mpd_pass = sdsempty();
    mpd_state->smartpls_sort = sdsempty();
    mpd_state->smartpls_prefix = sdsempty();
    mpd_state->smartpls_interval = 14400;
    mpd_state->booklet_name = sdsnew("booklet.pdf");
    reset_t_tags(&mpd_state->mpd_tag_types);
    reset_t_tags(&mpd_state->mympd_tag_types);
    reset_t_tags(&mpd_state->search_tag_types);
    reset_t_tags(&mpd_state->browse_tag_types);
    reset_t_tags(&mpd_state->generate_pls_tag_types);
    //init last played songs list
    list_init(&mpd_state->last_played);
    //jukebox queue
    list_init(&mpd_state->jukebox_queue);
    list_init(&mpd_state->jukebox_queue_tmp);
    mpd_state->sticker_cache = NULL;
}

void free_mpd_state(t_mpd_state *mpd_state) {
    sdsfree(mpd_state->music_directory);
    sdsfree(mpd_state->music_directory_value);
    sdsfree(mpd_state->jukebox_playlist);
    sdsfree(mpd_state->song_uri);
    sdsfree(mpd_state->last_song_uri);
    sdsfree(mpd_state->coverimage_name);
    sdsfree(mpd_state->love_channel);
    sdsfree(mpd_state->love_message);
    sdsfree(mpd_state->taglist);
    sdsfree(mpd_state->searchtaglist);
    sdsfree(mpd_state->browsetaglist);
    sdsfree(mpd_state->generate_pls_tags);
    sdsfree(mpd_state->mpd_host);
    sdsfree(mpd_state->mpd_pass);
    sdsfree(mpd_state->smartpls_sort);
    sdsfree(mpd_state->smartpls_prefix);
    sdsfree(mpd_state->booklet_name);
    list_free(&mpd_state->jukebox_queue);
    list_free(&mpd_state->jukebox_queue_tmp);
    free(mpd_state);
}
