/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "mpd/tag.h"
#include "src/lib/album_cache.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/libmympdclient/src/isong.h"
#include "dist/mjson/mjson.h"
#include "dist/rax/rax.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mpd_client/tags.h"

#include <errno.h>
#include <inttypes.h>

/**
 * myMPD saves album information in the album cache as a mpd_song struct.
 * Used fields:
 *   tags: tags from all songs of the album
 *   last_modified: last_modified from newest song
 *   duration: the album total time in seconds
 *   duration_ms: the album total time in milliseconds
 *   pos: number of discs
 *   prio: number of songs
 */

/**
 * Private definitions
 */

static sds album_to_cache_line(sds buffer, struct mpd_song *album, const struct t_tags *tagcols);
static struct mpd_song *album_from_cache_line(sds line, const struct t_tags *tagcols);

/**
 * Public functions
 */

/**
 * Removes the album cache file
 * @param cachedir myMPD cache directory
 * @return bool true on success, else false
 */
bool album_cache_remove(sds cachedir) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", cachedir, FILENAME_ALBUMCACHE);
    int rc = try_rm_file(filepath);
    FREE_SDS(filepath);
    return rc == RM_FILE_ERROR ? false : true;
}

/**
 * Reads the album cache from disc
 * @param album_cache pointer to t_cache struct
 * @param cachedir myMPD cache directory
 * @return bool true on success, else false
 */
bool album_cache_read(struct t_cache *album_cache, sds cachedir) {
    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif
    album_cache->building = true;
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", cachedir, FILENAME_ALBUMCACHE);
    errno = 0;
    FILE *fp = fopen(filepath, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", filepath);
        if (errno != ENOENT) {
            //ignore missing last_played file
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(filepath);
        album_cache->building = false;
        return false;
    }
    const struct t_tags album_tags = {
        .tags = {
            MPD_TAG_ALBUM,
            MPD_TAG_ALBUM_ARTIST,
            MPD_TAG_ARTIST,
            MPD_TAG_DATE,
            MPD_TAG_GENRE,
            MPD_TAG_MUSICBRAINZ_ALBUMARTISTID,
            MPD_TAG_MUSICBRAINZ_ALBUMID
        },
        .len = 7
    };
    sds line = sdsempty();
    sds key = sdsempty();
    if (album_cache->cache == NULL) {
        album_cache->cache = raxNew();
    }
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0) {
        if (validate_json_object(line) == true) {
            struct mpd_song *album = album_from_cache_line(line, &album_tags);
            if (album != NULL) {
                key = album_cache_get_key(album, key);
                if (raxTryInsert(album_cache->cache, (unsigned char *)key, sdslen(key), album, NULL) == 0) {
                    MYMPD_LOG_ERROR("Duplicate key in album cache file found");
                    mpd_song_free(album);
                }
            }
            else {
                MYMPD_LOG_ERROR("Reading album cache line failed");
                MYMPD_LOG_DEBUG("Erroneous line: %s", line);
            }
        }
        else {
            MYMPD_LOG_ERROR("Reading album cache line failed");
            MYMPD_LOG_DEBUG("Erroneous line: %s", line);
        }
    }
    FREE_SDS(line);
    FREE_SDS(key);
    (void) fclose(fp);
    FREE_SDS(filepath);
    album_cache->building = false;
    MYMPD_LOG_INFO("Read %lld album(s) from disc", (long long)album_cache->cache->numele);
    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT("Album cache read");
    #endif
    return true;
}

/**
 * Saves the album cache to disc as a njson file
 * @param album_cache pointer to t_cache struct
 * @param cachedir myMPD cache directory
 * @param free_data true=free the album cache, else not
 * @return bool true on success, else false
 */
bool album_cache_write(struct t_cache *album_cache, sds cachedir, bool free_data) {
    if (album_cache->cache == NULL) {
        MYMPD_LOG_DEBUG("Album cache is NULL not saving anything");
        return true;
    }
    MYMPD_LOG_INFO("Saving album cache");
    raxIterator iter;
    raxStart(&iter, album_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s.XXXXXX", cachedir, FILENAME_ALBUMCACHE);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    bool write_rc = true;
    sds line = sdsempty();
    const struct t_tags album_tags = {
        .tags = {
            MPD_TAG_ALBUM,
            MPD_TAG_ALBUM_ARTIST,
            MPD_TAG_ARTIST,
            MPD_TAG_DATE,
            MPD_TAG_GENRE,
            MPD_TAG_MUSICBRAINZ_ALBUMARTISTID,
            MPD_TAG_MUSICBRAINZ_ALBUMID
        },
        .len = 7
    };
    while (raxNext(&iter)) {
        sdsclear(line);
        line = album_to_cache_line(line, (struct mpd_song *)iter.data, &album_tags);
        if (fputs(line, fp) == EOF) {
            write_rc = false;
        }
        if (free_data == true) {
            mpd_song_free((struct mpd_song *)iter.data);
        }
    }
    FREE_SDS(line);
    raxStop(&iter);
    if (free_data == true) {
        raxFree(album_cache->cache);
        album_cache->cache = NULL;
    }
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", cachedir, FILENAME_ALBUMCACHE);
    bool rc = rename_tmp_file(fp, tmp_file, filepath, write_rc);
    FREE_SDS(tmp_file);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Constructs the albumkey from song info
 * @param song mpd song struct
 * @param albumkey sds string replaced by the key
 * @return pointer to albumkey
 */
sds album_cache_get_key(struct mpd_song *song, sds albumkey) {
    sdsclear(albumkey);
    albumkey = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, albumkey);
    if (sdslen(albumkey) == 0) {
        //album tag is empty
        MYMPD_LOG_WARN("Can not create albumkey for uri \"%s\", tag Album is empty", mpd_song_get_uri(song));
        return albumkey;
    }
    albumkey = sdscatlen(albumkey, "::", 2);
    size_t old_len = sdslen(albumkey);
    //first try AlbumArtist tag
    albumkey = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM_ARTIST, albumkey);
    if (old_len == sdslen(albumkey)) {
        //AlbumArtist tag is empty, fallback to Artist tag
        MYMPD_LOG_DEBUG("AlbumArtist for uri \"%s\" is empty, falling back to Artist", mpd_song_get_uri(song));
        albumkey = mpd_client_get_tag_value_string(song, MPD_TAG_ARTIST, albumkey);
    }
    if (old_len == sdslen(albumkey)) {
        MYMPD_LOG_WARN("Can not create albumkey for uri \"%s\", tags AlbumArtist and Artist are empty", mpd_song_get_uri(song));
        sdsclear(albumkey);
    }
    sds_utf8_tolower(albumkey);
    return albumkey;
}

/**
 * Gets the album from the album cache
 * @param album_cache pointer to t_cache struct
 * @param key the album
 * @return mpd_song struct representing the album
 */
struct mpd_song *album_cache_get_album(struct t_cache *album_cache, sds key) {
    if (album_cache->cache == NULL) {
        return NULL;
    }
    //try to get album
    void *data = raxFind(album_cache->cache, (unsigned char*)key, sdslen(key));
    if (data == raxNotFound) {
        MYMPD_LOG_ERROR("Album for key \"%s\" not found in cache", key);
        return NULL;
    }
    return (struct mpd_song *) data;
}

/**
 * Frees the album cache
 * @param album_cache pointer to t_cache struct
 */
void album_cache_free(struct t_cache *album_cache) {
    if (album_cache->cache == NULL) {
        MYMPD_LOG_DEBUG("Album cache is NULL not freeing anything");
        return;
    }
    MYMPD_LOG_DEBUG("Freeing album cache");
    raxIterator iter;
    raxStart(&iter, album_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        mpd_song_free((struct mpd_song *)iter.data);
    }
    raxStop(&iter);
    raxFree(album_cache->cache);
    album_cache->cache = NULL;
}

/**
 * Gets the number of songs
 * @param album mpd_song struct representing the album
 * @return number of songs
 */
unsigned album_get_song_count(struct mpd_song *album) {
    return album->prio;
}

/**
 * Gets the number of discs
 * @param album mpd_song struct representing the album
 * @return number of discs
 */
unsigned album_get_discs(struct mpd_song *album) {
    return album->pos;
}

/**
 * Gets the total play time
 * @param album mpd_song struct representing the album
 * @return total play time
 */
unsigned album_get_total_time(struct mpd_song *album) {
    return album->duration;
}

/**
 * Sets the albums disc count
 * @param album mpd_song struct representing the album
 * @param song mpd song to set discs from
 */
void album_cache_set_discs(struct mpd_song *album, struct mpd_song *song) {
    const char *disc;
    if ((disc = mpd_song_get_tag(song, MPD_TAG_DISC, 0)) != NULL) {
        unsigned d = (unsigned)strtoumax(disc, NULL, 10);
        if (d > album->pos) {
            album->pos = d;
        }
    }
}

/**
 * Sets the albums last modified date
 * @param album mpd_song struct representing the album
 * @param song mpd song to set last_modified from
 */
void album_cache_set_last_modified(struct mpd_song *album, struct mpd_song *song) {
    if (album->last_modified < song->last_modified) {
        album->last_modified = song->last_modified;
    }
}

/**
 * Increments the albums duration
 * @param album mpd_song struct representing the album
 * @param song pointer to a mpd_song struct
 */
void album_cache_inc_total_time(struct mpd_song *album, struct mpd_song *song) {
    album->duration += song->duration;
    album->duration_ms += song->duration_ms;
}

/**
 * Set the song count
 * @param album mpd_song struct representing the album
 * @param count song count
 */
void album_cache_set_song_count(struct mpd_song *album, unsigned count) {
    album->prio = count;
}

/**
 * Increments the song count
 * @param album pointer to a mpd_song struct
 */
void album_cache_inc_song_count(struct mpd_song *album) {
    album->prio++;
}

/**
 * Appends tag values to the album
 * @param album pointer to a mpd_song struct representing the album
 * @param song song to add tag values from
 * @param tags tags to append
 * @return true on success else false
 */
bool album_cache_append_tags(struct mpd_song *album,
        struct mpd_song *song, struct t_tags *tags)
{
    for (unsigned tagnr = 0; tagnr < tags->len; ++tagnr) {
        const char *value;
        enum mpd_tag_type tag = tags->tags[tagnr];
        //append only multivalue tags
        if (is_multivalue_tag(tag) == true) {
            unsigned value_nr = 0;
            while ((value = mpd_song_get_tag(song, tag, value_nr)) != NULL) {
                if (mympd_mpd_song_add_tag_dedup(album, tag, value) == false) {
                    return false;
                }
                value_nr++;
            }
        }
    }
    return true;
}

/**
 * Copies all values from a tag to another tag
 * @param song pointer to a mpd_song struct
 * @param src source tag
 * @param dst destination tag
 * @return true on success, else false
 */
bool album_cache_copy_tags(struct mpd_song *song, enum mpd_tag_type src, enum mpd_tag_type dst) {
    const char *value;
    unsigned value_nr = 0;
    while ((value = mpd_song_get_tag(song, src, value_nr)) != NULL) {
        if (mympd_mpd_song_add_tag_dedup(song, dst, value) == false) {
            return false;
        }
        value_nr++;
    }
    return true;
}

/**
 * Private functions
 */

/**
 * Prints a song struct for the album cache
 * @param buffer already allocated sds string to append
 * @param album mpd song struct
 * @param tagcols tags to print
 * @return sds pointer to buffer
 */
static sds album_to_cache_line(sds buffer, struct mpd_song *album, const struct t_tags *tagcols) {
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_uint(buffer, "Discs", album_get_discs(album), true);
    buffer = tojson_uint(buffer, "Songs", album_get_song_count(album), true);
    buffer = get_song_tags(buffer, true, tagcols, album);
    buffer = sdscatlen(buffer, "}\n", 2);
    return buffer;
}

/**
 * Creates a mpd_song struct from cache
 * @param line json line to parse
 * @param tagcols tags to read
 * @return struct mpd_song* allocated mpd_song struct
 */
static struct mpd_song *album_from_cache_line(sds line, const struct t_tags *tagcols) {
    sds uri = NULL;
    sds error = sdsempty();
    struct mpd_song *album = NULL;
    if (json_get_string(line, "$.uri", 1, FILEPATH_LEN_MAX, &uri, vcb_isfilepath, &error) == true) {
        album = mpd_song_new(uri);
        if (json_get_uint_max(line, "$.Discs", &album->pos, &error) == true &&
            json_get_uint_max(line, "$.Songs", &album->prio, &error) == true &&
            json_get_uint_max(line, "$.Duration", &album->duration, &error) == true &&
            json_get_time_max(line, "$.LastModified", &album->last_modified, &error) == true)
        {
            album->duration_ms = album->duration * 1000;
            sds path = sdsempty();
            for (size_t i = 0; i < tagcols->len; i++) {
                sdsclear(path);
                sdsclear(error);
                path = sdscatfmt(path, "$.%s", mpd_tag_name(tagcols->tags[i]));
                if (json_get_tag_values(line, path, album, vcb_isname, 64, &error) == false) {
                    mpd_song_free(album);
                    album = NULL;
                    break;
                }
            }
            FREE_SDS(path);
        }
        else {
            mpd_song_free(album);
            album = NULL;
        }
    }
    FREE_SDS(uri);
    FREE_SDS(error);
    return album;
}
