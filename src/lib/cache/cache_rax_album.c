/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Album cache
 */

#include "compile_time.h"
#include "src/lib/cache/cache_rax_album.h"

#include "dist/mpack/mpack.h"
#include "dist/rax/rax.h"
#include "src/lib/album.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_client/tags.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/**
 * Private definitions
 */

static struct t_album *album_from_mpack_node(mpack_node_t album_node, const struct t_mympd_mpd_tags *tags, sds *key);

/**
 * Public functions
 */

/**
 * Parses the string to the album mode enum
 * @param mode_str string to parse
 * @return the album mode as enum
 */
enum album_modes parse_album_mode(const char *mode_str) {
    if (strcmp(mode_str, "simple") == 0) {
        return ALBUM_MODE_SIMPLE;
    }
    // default is advanced album mode
    return ALBUM_MODE_ADV;
}

/**
 * Lookups the album mode
 * @param mode the album mode
 * @return album mode as string
 */
const char *lookup_album_mode(enum album_modes mode) {
    switch (mode) {
        case ALBUM_MODE_SIMPLE:
            return "simple";
        case ALBUM_MODE_ADV:
            return "adv";
    }
    // default is adv
    return "adv";
}

/**
 * Removes the album cache file
 * @param workdir myMPD working directory
 * @return bool true on success, else false
 */
bool album_cache_remove(sds workdir) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_TAGS, FILENAME_ALBUMCACHE);
    int rc = try_rm_file(filepath);
    FREE_SDS(filepath);
    return rc == RM_FILE_ERROR
        ? false
        : true;
}

/**
 * Reads the album cache from disc
 * @param album_cache pointer to t_cache struct
 * @param workdir myMPD working directory
 * @param album_config album configuration
 * @return bool true on success, else false
 */
bool album_cache_read(struct t_cache *album_cache, sds workdir, const struct t_albums_config *album_config) {
    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_TAGS, FILENAME_ALBUMCACHE);
    if (testfile_read(filepath) == false) {
        FREE_SDS(filepath);
        return false;
    }

    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    FREE_SDS(filepath);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);

    // check for expected album_mode
    enum album_modes album_mode = (enum album_modes)mpack_node_int(mpack_node_map_cstr(root, "albumMode"));
    if (album_mode != album_config->mode) {
        mpack_tree_destroy(&tree);
        MYMPD_LOG_WARN(NULL, "Unexpected album mode, discarding cache");
        album_cache_remove(workdir);
        return NULL;
    }

    // check for expected album_group_tag
    enum mpd_tag_type group_tag = (enum mpd_tag_type)mpack_node_int(mpack_node_map_cstr(root, "albumGroupTag"));
    if (group_tag != album_config->group_tag) {
        mpack_tree_destroy(&tree);
        MYMPD_LOG_WARN(NULL, "Unexpected album group tag, discarding cache");
        album_cache_remove(workdir);
        return NULL;
    }

    // read tags array
    struct t_mympd_mpd_tags *album_tags = malloc_assert(sizeof(struct t_mympd_mpd_tags));
    mympd_mpd_tags_reset(album_tags);

    mpack_node_t tags_node = mpack_node_map_cstr(root, "tags");
    size_t len = mpack_node_array_length(tags_node);
    for (size_t i = 0; i < len; i++) {
        mpack_node_t value_node = mpack_node_array_at(tags_node, i);
        char *value = mpack_node_cstr_alloc(value_node, JSONRPC_STR_MAX);
        if (value == NULL) {
            break;
        }
        enum mpd_tag_type tag = mpd_tag_name_parse(value);
        if (tag != MPD_TAG_UNKNOWN) {
            album_tags->tags[album_tags->len++] = tag;
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Unkown MPD tag type: \"%s\"", value);
        }
        MPACK_FREE(value);
    }

    // read albums array
    mpack_node_t albums_node = mpack_node_map_cstr(root, "albums");
    len = mpack_node_array_length(albums_node);
    sds key = sdsempty();
    album_cache->building = true;
    album_cache->cache = raxNew();

    for (size_t i = 0; i < len; i++) {
        mpack_node_t album_node = mpack_node_array_at(albums_node, i);
        struct t_album *album = album_from_mpack_node(album_node, album_tags, &key);
        if (album != NULL) {
            if (raxTryInsert(album_cache->cache, (unsigned char *)key, sdslen(key), album, NULL) == 0) {
                MYMPD_LOG_ERROR(NULL, "Duplicate key in album cache file found: %s", key);
                album_free(album);
            }
        }
    }
    FREE_SDS(key);
    // clean up and check for errors
    bool rc = mpack_tree_destroy(&tree) != mpack_ok
        ? false
        : true;
    if (rc == false) {
        MYMPD_LOG_ERROR("default", "Reading album cache failed, discarding cache");
        album_cache_remove(workdir);
        album_cache_free(album_cache);
    }
    else {
        MYMPD_LOG_INFO(NULL, "Read %" PRIu64 " album(s) from disc", album_cache->cache->numele);
    }
    FREE_PTR(album_tags);
    album_cache->building = false;
    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT(NULL, "Album cache read");
    #endif
    return rc;
}

/**
 * Saves the album cache to disc in mpack format
 * @param album_cache pointer to t_cache struct
 * @param workdir myMPD working directory
 * @param album_tags album tags to write
 * @param album_config album configuration
 * @param free_data true=free the album cache, else not
 * @return bool true on success, else false
 */
bool album_cache_write(struct t_cache *album_cache, sds workdir, const struct t_mympd_mpd_tags *album_tags, const struct t_albums_config *album_config, bool free_data) {
    if (album_cache->cache == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Album cache is NULL not saving anything");
        return true;
    }
    MYMPD_LOG_INFO(NULL, "Saving album cache to disc");
    mpack_writer_t writer;
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s/%s.XXXXXX", workdir, DIR_WORK_TAGS, FILENAME_ALBUMCACHE);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    // init mpack
    mpack_writer_init_stdfile(&writer, fp, true);
    mpack_writer_set_error_handler(&writer, log_mpack_write_error);
    mpack_build_map(&writer);
    mpack_write_kv(&writer, "albumMode", album_config->mode);
    mpack_write_kv(&writer, "albumGroupTag", album_config->group_tag);
    mpack_write_cstr(&writer, "tags");
    mpack_start_array(&writer, (uint32_t)album_tags->len);
    for (unsigned tagnr = 0; tagnr < album_tags->len; ++tagnr) {
        mpack_write_cstr(&writer, mpd_tag_name(album_tags->tags[tagnr]));
    }
    mpack_finish_array(&writer);
    mpack_write_cstr(&writer, "albums");
    mpack_start_array(&writer, (uint32_t)album_cache->cache->numele);
    raxIterator iter;
    raxStart(&iter, album_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        const struct t_album *album = (struct t_album *)iter.data;
        mpack_build_map(&writer);
        mpack_write_kv(&writer, "uri", album_get_uri(album));
        mpack_write_kv(&writer, "Discs", album_get_disc_count(album));
        mpack_write_kv(&writer, "Songs", album_get_song_count(album));
        mpack_write_kv(&writer, "Duration", album_get_total_time(album));
        mpack_write_kv(&writer, "Last-Modified", (uint64_t)album_get_last_modified(album));
        mpack_write_kv(&writer, "Added", (uint64_t)album_get_added(album));
        mpack_write_cstr(&writer, "AlbumId");
        mpack_write_str(&writer, (char *)iter.key, (uint32_t)iter.key_len);
        for (unsigned tagnr = 0; tagnr < album_tags->len; ++tagnr) {
            enum mpd_tag_type tag = album_tags->tags[tagnr];
            if (album_get_tag(album, tag, 0) == NULL) {
                // do not write empty tags
                continue;
            }
            if (is_multivalue_tag(tag) == true) {
                const char *value;
                unsigned count = 0;
                mpack_write_cstr(&writer, mpd_tag_name(tag));
                mpack_build_array(&writer);
                while ((value = album_get_tag(album, tag, count)) != NULL) {
                    mpack_write_cstr(&writer, value);
                    count++;
                }
                mpack_complete_array(&writer);
            }
            else {
                mpack_write_kv(&writer, mpd_tag_name(tag), album_get_tag(album, tag, 0));
            }
        }
        mpack_complete_map(&writer);
        if (free_data == true) {
            album_free((struct t_album *)iter.data);
        }
    }
    raxStop(&iter);
    mpack_finish_array(&writer);
    mpack_complete_map(&writer);
    if (free_data == true) {
        raxFree(album_cache->cache);
        album_cache->cache = NULL;
    }
    // finish writing
    bool rc = mpack_writer_destroy(&writer) != mpack_ok
        ? false
        : true;
    if (rc == false) {
        rm_file(tmp_file);
        MYMPD_LOG_ERROR("default", "An error occurred encoding the data");
        FREE_SDS(tmp_file);
        return false;
    }
    // rename tmp file
    sds filepath = sdscatlen(sdsempty(), tmp_file, sdslen(tmp_file) - 7);
    errno = 0;
    if (rename(tmp_file, filepath) == -1) {
        MYMPD_LOG_ERROR(NULL, "Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        rm_file(tmp_file);
        rc = false;
    }
    FREE_SDS(filepath);
    FREE_SDS(tmp_file);
    return rc;
}

/**
 * Constructs the albumkey from song info
 * @param albumkey already allocated sds string to set the key
 * @param song mpd song struct
 * @param album_config album configuration
 * @return pointer to changed albumkey
 */
sds album_cache_get_key_from_song(sds albumkey, const struct mpd_song *song, const struct t_albums_config *album_config) {
    sdsclear(albumkey);
    if (album_config->mode == ALBUM_MODE_ADV) {
        // use MusicBrainz album id
        const char *mb_album_id = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0);
        if (mb_album_id != NULL &&
            strlen(mb_album_id) == MBID_LENGTH) // MBID must be exactly 36 characters long
        {
            return sdscatlen(albumkey, mb_album_id, MBID_LENGTH);
        }
    }

    // fallback to hashed AlbumArtist::Album::<group tag>
    // first try AlbumArtist tag
    albumkey = mympd_client_get_tag_value_string(song, MPD_TAG_ALBUM_ARTIST, albumkey);
    if (sdslen(albumkey) == 0) {
        // AlbumArtist tag is empty, fallback to Artist tag
        #ifdef MYMPD_DEBUG
            MYMPD_LOG_DEBUG(NULL, "AlbumArtist for uri \"%s\" is empty, falling back to Artist", mpd_song_get_uri(song));
        #endif
        albumkey = mympd_client_get_tag_value_string(song, MPD_TAG_ARTIST, albumkey);
    }
    if (sdslen(albumkey) == 0) {
        MYMPD_LOG_WARN(NULL, "Can not create albumkey for uri \"%s\", tags AlbumArtist and Artist are empty", mpd_song_get_uri(song));
        return albumkey;
    }

    const char *album_name = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (album_name == NULL) {
        // album tag is empty
        MYMPD_LOG_WARN(NULL, "Can not create albumkey for uri \"%s\", tag Album is empty", mpd_song_get_uri(song));
        sdsclear(albumkey);
        return albumkey;
    }
    // append album
    albumkey = sdscatfmt(albumkey, "::%s", album_name);
    // optionally append group tag
    if (album_config->group_tag != MPD_TAG_UNKNOWN) {
        const char *group_tag_value = mpd_song_get_tag(song, album_config->group_tag, 0);
        if (group_tag_value != NULL) {
            albumkey = sdscatfmt(albumkey, "::%s", group_tag_value);
        }
    }
    // return the hash
    return sds_hash_sha1_sds(albumkey);
}

/**
 * Constructs the albumkey from album struct
 * @param albumkey already allocated sds string to set the key
 * @param album t_album struct
 * @param album_config album configuration
 * @return pointer to changed albumkey
 */
sds album_cache_get_key_from_album(sds albumkey, const struct t_album *album, const struct t_albums_config *album_config) {
    sdsclear(albumkey);
    if (album_config->mode == ALBUM_MODE_ADV) {
        // use MusicBrainz album id
        const char *mb_album_id = album_get_tag(album, MPD_TAG_MUSICBRAINZ_ALBUMID, 0);
        if (mb_album_id != NULL &&
            strlen(mb_album_id) == MBID_LENGTH) // MBID must be exactly 36 characters long
        {
            return sdscatlen(albumkey, mb_album_id, MBID_LENGTH);
        }
    }

    // fallback to hashed AlbumArtist::Album::<group tag>
    // first try AlbumArtist tag
    albumkey = album_get_tag_value_string(album, MPD_TAG_ALBUM_ARTIST, albumkey);
    if (sdslen(albumkey) == 0) {
        // AlbumArtist tag is empty, fallback to Artist tag
        #ifdef MYMPD_DEBUG
            MYMPD_LOG_DEBUG(NULL, "AlbumArtist for uri \"%s\" is empty, falling back to Artist", album_get_uri(album));
        #endif
        albumkey = album_get_tag_value_string(album, MPD_TAG_ARTIST, albumkey);
    }
    if (sdslen(albumkey) == 0) {
        MYMPD_LOG_WARN(NULL, "Can not create albumkey for uri \"%s\", tags AlbumArtist and Artist are empty", album_get_uri(album));
        return albumkey;
    }

    const char *album_name = album_get_tag(album, MPD_TAG_ALBUM, 0);
    if (album_name == NULL) {
        // album tag is empty
        MYMPD_LOG_WARN(NULL, "Can not create albumkey for uri \"%s\", tag Album is empty", album_get_uri(album));
        sdsclear(albumkey);
        return albumkey;
    }
    // append album
    albumkey = sdscatfmt(albumkey, "::%s", album_name);
    // optionally append group tag
    if (album_config->group_tag != MPD_TAG_UNKNOWN) {
        const char *group_tag_value = album_get_tag(album, album_config->group_tag, 0);
        if (group_tag_value != NULL) {
            albumkey = sdscatfmt(albumkey, "::%s", group_tag_value);
        }
    }
    // return the hash
    return sds_hash_sha1_sds(albumkey);
}

/**
 * Gets the album from the album cache
 * @param album_cache pointer to t_cache struct
 * @param key the album
 * @return mpd_song struct representing the album or NULL on error
 */
struct t_album *album_cache_get_album(struct t_cache *album_cache, sds key) {
    if (album_cache->cache == NULL) {
        return NULL;
    }
    //try to get album
    void *data;
    if (raxFind(album_cache->cache, (unsigned char*)key, sdslen(key), &data) == 0) {
        MYMPD_LOG_ERROR(NULL, "Album for key \"%s\" not found in cache", key);
        return NULL;
    }
    return (struct t_album *) data;
}

/**
 * Frees the album cache
 * @param album_cache pointer to t_cache struct
 */
void album_cache_free(struct t_cache *album_cache) {
    if (album_cache->cache == NULL) {
        return;
    }
    album_cache_free_rt(album_cache->cache);
    album_cache->cache = NULL;
}

/**
 * Frees the album cache radix tree
 * @param album_cache_rt Pointer to album cache radix tree
 */
void album_cache_free_rt(rax *album_cache_rt) {
    MYMPD_LOG_DEBUG(NULL, "Freeing album cache");
    raxIterator iter;
    raxStart(&iter, album_cache_rt);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        album_free((struct t_album *)iter.data);
    }
    raxStop(&iter);
    raxFree(album_cache_rt);
}

/**
 * Frees the album cache radix tree
 * @param album_cache_rt Pointer to album cache radix tree
 */
void album_cache_free_rt_void(void *album_cache_rt) {
    album_cache_free_rt((rax *)album_cache_rt);
}

/**
 * Private functions
 */

/**
 * Creates a mpd_song struct from cache
 * @param album_node mpack node to parse
 * @param tags tags to read
 * @param key already allocated sds string to set the album key
 * @return struct mpd_song* allocated mpd_song struct
 */
static struct t_album *album_from_mpack_node(mpack_node_t album_node, const struct t_mympd_mpd_tags *tags, sds *key) {
    struct t_album *album = NULL;
    sdsclear(*key);
    char *uri = mpack_node_cstr_alloc(mpack_node_map_cstr(album_node, "uri"), JSONRPC_STR_MAX);
    if (uri != NULL) {
        album = album_new_uri(uri);
        *key = mpackstr_sdscat(*key, album_node, "AlbumId");

        album_set_disc_count(album, mpack_node_uint(mpack_node_map_cstr(album_node, "Discs")));
        album_set_song_count(album, mpack_node_uint(mpack_node_map_cstr(album_node, "Songs")));
        album_set_total_time(album, mpack_node_uint(mpack_node_map_cstr(album_node, "Duration")));
        album_set_last_modified(album, mpack_node_int(mpack_node_map_cstr(album_node, "Last-Modified")));
        album_set_added(album, mpack_node_int(mpack_node_map_cstr(album_node, "Added")));
        for (size_t i = 0; i < tags->len; i++) {
            enum mpd_tag_type tag = tags->tags[i];
            const char *tag_name = mpd_tag_name(tag);
            mpack_node_t value_node = mpack_node_map_cstr_optional(album_node, tag_name);
            if (mpack_node_is_missing(value_node) == false) {
                if (is_multivalue_tag(tag) == true) {
                    size_t len = mpack_node_array_length(value_node);
                    for (size_t j = 0; j < len; j++) {
                        char *value = mpack_node_cstr_alloc(mpack_node_array_at(value_node, j), JSONRPC_STR_MAX);
                        if (value != NULL) {
                            album_append_tag(album, tags->tags[i], value);
                            MPACK_FREE(value);
                        }
                    }
                }
                else {
                    char *value = mpack_node_cstr_alloc(value_node, JSONRPC_STR_MAX);
                    if (value != NULL) {
                        album_append_tag(album, tags->tags[i], value);
                        MPACK_FREE(value);
                    }
                }
            }
        }
        MPACK_FREE(uri);
    }
    return album;
}
