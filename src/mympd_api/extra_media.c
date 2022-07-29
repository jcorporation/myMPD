/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "extra_media.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/errorhandler.h"
#include "../mpd_client/tags.h"
#include "timer.h"

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

/**
 * Private definitons
 */

static void _get_extra_files(struct t_mpd_shared_state *mpd_shared_state, const char *uri, sds *booklet_path, struct t_list *images, bool is_dirname);
static int _get_embedded_covers_count(const char *media_file);
static int _get_embedded_covers_count_id3(const char *media_file);
static int _get_embedded_covers_count_flac(const char *media_file, bool is_ogg);

/**
 * Public functions
 */

/**
 * Looks for images and the booklet in the songs directory and counts the number of embedded images
 * @param mpd_shared_state pointer to the shared mpd state
 * @param buffer buffer to append the jsonrpc result
 * @param uri song uri to get extra media for
 * @param is_dirname true if uri is a directory, else false
 * @return pointer to buffer
 */
sds get_extra_media(struct t_mpd_shared_state *mpd_shared_state, sds buffer, const char *uri, bool is_dirname) {
    struct t_list images;
    list_init(&images);
    sds booklet_path = sdsempty();
    if (is_streamuri(uri) == false &&
        mpd_shared_state->feat_mpd_library == true)
    {
        _get_extra_files(mpd_shared_state, uri, &booklet_path, &images, is_dirname);
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
        mpd_shared_state->feat_mpd_library == true)
    {
        sds fullpath = sdscatfmt(sdsempty(), "%S/%s", mpd_shared_state->music_directory_value, uri);
        image_count = _get_embedded_covers_count(fullpath);
        FREE_SDS(fullpath);
    }
    buffer = tojson_int(buffer, "embeddedImageCount", image_count, false);
    list_clear(&images);
    FREE_SDS(booklet_path);
    return buffer;
}

/**
 * Private functions
 */

/**
 * Looks for images and the booklet in the songs directory
 * @param mpd_shared_state pointer to the shared mpd state
 * @param uri song uri to get extra media for
 * @param booklet_path pointer to sds to populate with the booklet path
 * @param images pointer to already alocated list
 * @param is_dirname true if uri is a directory, else false
 */
static void _get_extra_files(struct t_mpd_shared_state *mpd_shared_state, const char *uri, sds *booklet_path, struct t_list *images, bool is_dirname) {
    sds path = sdsnew(uri);
    if (is_dirname == false) {
        dirname(path);
        sdsupdatelen(path);
    }

    if (is_virtual_cuedir(mpd_shared_state->music_directory_value, path)) {
        //fix virtual cue sheet directories
        dirname(path);
        sdsupdatelen(path);
    }
    sds albumpath = sdscatfmt(sdsempty(), "%S/%S", mpd_shared_state->music_directory_value, path);
    sds fullpath = sdsempty();
    MYMPD_LOG_DEBUG("Read extra files from albumpath: \"%s\"", albumpath);
    errno = 0;
    DIR *album_dir = opendir(albumpath);
    if (album_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(album_dir)) != NULL) {
            if (strcmp(next_file->d_name, mpd_shared_state->booklet_name) == 0) {
                MYMPD_LOG_DEBUG("Found booklet for uri %s", uri);
                *booklet_path = sdscatfmt(*booklet_path, "/browse/music/%S/%S", path, mpd_shared_state->booklet_name);
            }
            else if (is_image(next_file->d_name) == true) {
                fullpath = sdscatfmt(fullpath, "/browse/music/%S/%s", path, next_file->d_name);
                list_push(images, fullpath, 0, NULL, NULL);
                sdsclear(fullpath);
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

/**
 * Counts embedded images
 * @param media_file pointer to the shared mpd state
 * @return image count
 */
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

/**
 * Counts embedded images for id3v2 tagged files
 * @param media_file pointer to the shared mpd state
 * @return image count
 */
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

/**
 * Counts embedded images for vorbis und flac files
 * @param media_file pointer to the shared mpd state
 * @return image count
 */
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
