/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_extra_media.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_sticker.h"
#include "../mpd_client/mpd_client_tags.h"
#include "mympd_api_timer.h"

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//optional includes
#ifdef ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef ENABLE_FLAC
    #include <FLAC/metadata.h>
#endif

//private definitons
static void _get_extra_files(struct t_mympd_state *mympd_state, const char *uri, sds *booklet_path, struct t_list *images, bool is_dirname);
static int _get_embedded_covers_count(const char *media_file);
static int _get_embedded_covers_count_id3(const char *media_file);
static int _get_embedded_covers_count_flac(const char *media_file, bool is_ogg);

//public functions

sds get_extra_media(struct t_mympd_state *mympd_state, sds buffer, const char *uri, bool is_dirname) {
    struct t_list images;
    list_init(&images);
    sds booklet_path = sdsempty();
    if (is_streamuri(uri) == false &&
        mympd_state->mpd_state->feat_mpd_library == true)
    {
        _get_extra_files(mympd_state, uri, &booklet_path, &images, is_dirname);
    }
    buffer = tojson_sds(buffer, "bookletPath", booklet_path, true);
    buffer = sdscat(buffer, "\"images\": [");
    struct t_list_node *current = images.head;
    while (current != NULL) {
        if (current != images.head) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sds_catjson(buffer, current->key, sdslen(current->key));
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    int image_count = 0;
    if (is_dirname == false &&
        is_streamuri(uri) == false &&
        mympd_state->mpd_state->feat_mpd_library == true)
    {
        sds fullpath = sdscatfmt(sdsempty(), "%S/%s", mympd_state->music_directory_value, uri);
        image_count = _get_embedded_covers_count(fullpath);
        FREE_SDS(fullpath);
    }
    buffer = tojson_int(buffer, "embeddedImageCount", image_count, false);
    list_clear(&images);
    FREE_SDS(booklet_path);
    return buffer;
}

//private functions
static void _get_extra_files(struct t_mympd_state *mympd_state, const char *uri, sds *booklet_path, struct t_list *images, bool is_dirname) {
    sds path = sdsnew(uri);
    if (is_dirname == false) {
        dirname(path);
        sdsupdatelen(path);
    }

    if (is_virtual_cuedir(mympd_state->music_directory_value, path)) {
        //fix virtual cue sheet directories
        dirname(path);
        sdsupdatelen(path);
    }
    sds albumpath = sdscatfmt(sdsempty(), "%S/%S", mympd_state->music_directory_value, path);
    sds fullpath = sdsempty();
    MYMPD_LOG_DEBUG("Read extra files from albumpath: \"%s\"", albumpath);
    errno = 0;
    DIR *album_dir = opendir(albumpath);
    if (album_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(album_dir)) != NULL) {
            const char *ext = strrchr(next_file->d_name, '.');
            if (strcmp(next_file->d_name, mympd_state->booklet_name) == 0) {
                MYMPD_LOG_DEBUG("Found booklet for uri %s", uri);
                *booklet_path = sdscatfmt(*booklet_path, "/browse/music/%S/%S", path, mympd_state->booklet_name);
            }
            else if (ext != NULL) {
                if (strcasecmp(ext, ".webp") == 0 || strcasecmp(ext, ".jpg") == 0 ||
                    strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0 ||
                    strcasecmp(ext, ".avif") == 0 || strcasecmp(ext, ".svg") == 0)
                {
                    fullpath = sdscatfmt(fullpath, "/browse/music/%S/%s", path, next_file->d_name);
                    list_push(images, fullpath, 0, NULL, NULL);
                    sdsclear(fullpath);
                }
            }
        }
        closedir(album_dir);
    }
    else {
        MYMPD_LOG_ERROR("Can not open directory \"%s\" to get list of extra files", albumpath);
        MYMPD_LOG_ERRNO(errno);
    }
    FREE_SDS(fullpath);
    FREE_SDS(path);
    FREE_SDS(albumpath);
}

static int _get_embedded_covers_count(const char *media_file) {
    int count = 0;
    const char *mime_type_media_file = get_mime_type_by_ext(media_file);
    MYMPD_LOG_DEBUG("Mimetype of %s is %s", media_file, mime_type_media_file);
    if (strcmp(mime_type_media_file, "application/octet-stream") == 0) {
        MYMPD_LOG_DEBUG("Skip counting coverimages from %s", media_file);
        return count;
    }
    MYMPD_LOG_DEBUG("Counting coverimages from %s", media_file);
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        count = _get_embedded_covers_count_id3(media_file);
    }
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        count = _get_embedded_covers_count_flac(media_file, true);
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        count = _get_embedded_covers_count_flac(media_file, false);
    }
    MYMPD_LOG_DEBUG("Found %d embedded coverimages in %s", count, media_file);
    return count;
}

static int _get_embedded_covers_count_id3(const char *media_file) {
    int count = 0;
    #ifdef ENABLE_LIBID3TAG
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR("Can't parse id3_file: %s", media_file);
        return 0;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        return 0;
    }
    struct id3_frame *frame;
    do {
        frame = id3_tag_findframe(tags, "APIC", (unsigned)count);
        if (frame != NULL) {
            count++;
        }
    } while (frame != NULL);
    id3_file_close(file_struct);
    #else
    (void) media_file;
    #endif
    return count;
}

static int _get_embedded_covers_count_flac(const char *media_file, bool is_ogg) {
    int count = 0;
    #ifdef ENABLE_FLAC
    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();

    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        MYMPD_LOG_DEBUG("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        return 0;
    }
    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(iterator, chain);
    assert(iterator);
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            count++;
        }
    } while (FLAC__metadata_iterator_next(iterator));

    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) media_file;
    (void) is_ogg;
    #endif
    return count;
}
