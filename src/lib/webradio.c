/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webradio functions
 */

#include "compile_time.h"
#include "src/lib/webradio.h"

#include "dist/mpack/mpack.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include <pthread.h>

/**
 * Search webradio by uri in favorites and WebradioDB
 * @param webradio_favorites Pointer to webradio favorites
 * @param webradiodb Pointer to WebradioDB
 * @param uri Uri to search for
 * @return pointer to webradio data or NULL on error
 */
struct t_webradio_data *webradio_by_uri(struct t_webradios *webradio_favorites, struct t_webradios *webradiodb,
        const char *uri)
{
    void *data = raxNotFound;
    if (webradio_favorites != NULL) {
        if (webradio_favorites->idx_uris != NULL) {
            data = raxFind(webradio_favorites->idx_uris, (unsigned char *)uri, strlen(uri));
        }
    }
    if (webradiodb != NULL) {
        if (data == raxNotFound) {
            if (webradiodb->idx_uris != NULL) {
                data = raxFind(webradiodb->idx_uris, (unsigned char *)uri, strlen(uri));
            }
        }
    }
    if (data == raxNotFound) {
        return NULL;
    }
    return (struct t_webradio_data *)data;
}

/**
 * Returns an extm3u for a webradio
 * @param webradio_favorites Pointer to webradio favorites
 * @param webradiodb Pointer to WebradioDB
 * @param buffer Buffer to append the uri
 * @param uri Webradio uri
 * @return Pointer to buffer
 */
sds webradio_get_extm3u(struct t_webradios *webradio_favorites, struct t_webradios *webradiodb, sds buffer, sds uri) {
    struct t_webradio_data *webradio = webradio_by_uri(webradio_favorites, webradiodb, uri);
    if (webradio != NULL) {
        return webradio_to_extm3u(webradio, buffer, uri);
    }
    return sdscat(buffer, "Webradio not found");
}

/**
 * Creates a new webradio data struct
 * @param type Webradio type
 * @return struct t_webradio_data* 
 */
struct t_webradio_data *webradio_data_new(enum webradio_type type) {
    struct t_webradio_data *data = malloc_assert(sizeof(struct t_webradio_data));
    list_init(&data->uris);
    list_init(&data->genres);
    list_init(&data->languages);
    data->name = NULL;
    data->image = NULL;
    data->homepage = NULL;
    data->country = NULL;
    data->region = NULL;
    data->description = NULL;
    data->type = type;
    data->added = -1;
    data->last_modified = -1;
    return data;
}

/**
 * Frees a webradios data struct
 * @param data struct to free
 */
void webradio_data_free(struct t_webradio_data *data) {
    if (data == NULL) {
        return;
    }
    list_clear(&data->uris);
    list_clear(&data->genres);
    list_clear(&data->languages);
    FREE_SDS(data->name);
    FREE_SDS(data->image);
    FREE_SDS(data->homepage);
    FREE_SDS(data->country);
    FREE_SDS(data->region);
    FREE_SDS(data->description);
    // pointer data itself
    FREE_PTR(data);
}

/**
 * Returns the uri for the webradio image
 * @param webradio Webradio struct
 * @param buffer buffer to append the uri
 * @return pointer to buffer
 */
sds webradio_get_cover_uri(struct t_webradio_data *webradio, sds buffer) {
    if (webradio->type == WEBRADIO_WEBRADIODB) {
        buffer = sdscat(buffer, "/proxy-covercache?uri=");
        buffer = sds_urlencode(buffer, WEBRADIODB_URI_PICS, strlen(WEBRADIODB_URI_PICS));
        buffer = sds_urlencode(buffer, webradio->image, sdslen(webradio->image));
    }
    else if (is_streamuri(webradio->image) == true) {
        buffer = sdscat(buffer, "/proxy-covercache?uri=");
        buffer = sds_urlencode(buffer, webradio->image, sdslen(webradio->image));
    }
    else {
        buffer = sdscat(buffer, "/browse/pics/thumbs/");
        buffer = sds_urlencode(buffer, webradio->image, sdslen(webradio->image));
    }
    return buffer;
}

/**
 * Sets the webradio tags for search
 * @param tags Struct to set
 */
void webradio_tags_search(struct t_webradio_tags *tags) {
    tags->len = 0;
    tags->tags[tags->len++] = WEBRADIO_TAG_NAME;
    tags->tags[tags->len++] = WEBRADIO_TAG_COUNTRY;
    tags->tags[tags->len++] = WEBRADIO_TAG_REGION;
    tags->tags[tags->len++] = WEBRADIO_TAG_DESCRIPTION;
    tags->tags[tags->len++] = WEBRADIO_TAG_GENRES;
    tags->tags[tags->len++] = WEBRADIO_TAG_LANGUAGES;
}

/**
 * Mapping for webradio tags to strings
 */
static const char *webradio_tag_types_names[WEBRADIO_TAG_COUNT] = {
    [WEBRADIO_TAG_NAME] = "Name",
    [WEBRADIO_TAG_IMAGE] = "Image",
    [WEBRADIO_TAG_HOMEPAGE] = "Homepage",
    [WEBRADIO_TAG_COUNTRY] = "Country",
    [WEBRADIO_TAG_REGION] = "Region",
    [WEBRADIO_TAG_DESCRIPTION] = "Description",
    [WEBRADIO_TAG_URIS] = "Uri",
    [WEBRADIO_TAG_BITRATE] = "Bitrate",
    [WEBRADIO_TAG_CODEC] = "Codec",
    [WEBRADIO_TAG_GENRES] = "Genres",
    [WEBRADIO_TAG_LANGUAGES] = "Languages",
    [WEBRADIO_TAG_ADDED] = "Added",
    [WEBRADIO_TAG_LASTMODIFIED] = "Last-Modified"
};

/**
 * Parses a string to a webradio tag type
 * @param name string to parse
 * @return enum webradio_tag_type 
 */
enum webradio_tag_type webradio_tag_name_parse(const char *name) {
    for (unsigned i = 0; i < WEBRADIO_TAG_COUNT; ++i) {
        if (strcmp(name, webradio_tag_types_names[i]) == 0) {
            return (enum webradio_tag_type)i;
        }
    }
    return WEBRADIO_TAG_UNKNOWN;
}

/**
 * Returns the webradio type as string literal
 * @param type webradio type
 * @return Name of the webradio type
 */
const char *webradio_type_name(enum webradio_type type) {
    return type == WEBRADIO_WEBRADIODB 
        ? "webradiodb"
        : "favorite";
}

/**
 * Returns the value of a webradio tag
 * @param webradio Webdadio
 * @param tag_type Webradio tag
 * @param idx Index of tag
 * @return Tag value or NULL if not exists
 */
const char *webradio_get_tag(const struct t_webradio_data *webradio, enum webradio_tag_type tag_type, unsigned int idx) {
    switch(tag_type) {
        case WEBRADIO_TAG_NAME:
            return idx == 0 ? webradio->name : NULL;
        case WEBRADIO_TAG_IMAGE:
            return idx == 0 ? webradio->image : NULL;
        case WEBRADIO_TAG_HOMEPAGE:
            return idx == 0 ? webradio->homepage : NULL;
        case WEBRADIO_TAG_COUNTRY:
            return idx == 0 ? webradio->country : NULL;
        case WEBRADIO_TAG_REGION:
            return idx == 0 ? webradio->region : NULL;
        case WEBRADIO_TAG_DESCRIPTION:
            return idx == 0 ? webradio->description : NULL;
        case WEBRADIO_TAG_URIS:
        case WEBRADIO_TAG_CODEC: {
            struct t_list_node *node = list_node_at(&webradio->uris, idx);
            if (node == NULL) {
                return NULL;
            }
            if (tag_type == WEBRADIO_TAG_URIS) { return node->key; }
            if (tag_type == WEBRADIO_TAG_CODEC) { return node->value_p; }
            return NULL;
        }
        case WEBRADIO_TAG_GENRES: {
            struct t_list_node *node = list_node_at(&webradio->genres, idx);
            if (node == NULL) {
                return NULL;
            }
            return node->key;
        }
        case WEBRADIO_TAG_LANGUAGES: {
            struct t_list_node *node = list_node_at(&webradio->languages, idx);
            if (node == NULL) {
                return NULL;
            }
            return node->key;
        }
        case WEBRADIO_TAG_BITRATE:
        case WEBRADIO_TAG_ADDED:
        case WEBRADIO_TAG_LASTMODIFIED:
        case WEBRADIO_TAG_UNKNOWN:
        case WEBRADIO_TAG_COUNT:
            return NULL;
    }
    return NULL;
}

/**
 * Returns an extm3u for a webradio
 * @param webradio Pointer to webradio struct
 * @param buffer Already allocated buffer to append the extm3u
 * @param uri Optional uri. If NULL use the first uri from the webradio entry.
 * @return Pointer to buffer
 */
sds webradio_to_extm3u(const struct t_webradio_data *webradio, sds buffer, const char *uri) {
    return sdscatfmt(buffer, "#EXTM3U\n"
        "#EXTINF:-1,%s\n"
        "#PLAYLIST:%s\n"
        "%s\n",
        webradio->name,
        webradio->name,
        (uri == NULL ? webradio->uris.head->key : uri));
}

/**
 * Initializes the webradios struct
 * @return struct t_webradios* 
 */
struct t_webradios *webradios_new(void) {
    struct t_webradios *webradios = malloc_assert(sizeof(struct t_webradios));
    int rc = pthread_rwlock_init(&webradios->rwlock, NULL);
    if (rc == 0) {
        webradios->db = raxNew();
        webradios->idx_uris = raxNew();
        return webradios;
    }
    MYMPD_LOG_ERROR(NULL, "Can not init lock");
    MYMPD_LOG_ERRNO(NULL, rc);
    FREE_PTR(webradios);
    return NULL;
}

/**
 * Clears the webradios struct
 * @param webradios struct to free
 * @param init_rax re-init the rax trees?
 */
void webradios_clear(struct t_webradios *webradios, bool init_rax) {
    raxIterator iter;
    if (webradios->db != NULL) {
        raxStart(&iter, webradios->db);
        raxSeek(&iter, "^", NULL, 0);
        while (raxNext(&iter)) {
            webradio_data_free((struct t_webradio_data *)iter.data);
            iter.data = NULL;
        }
        raxStop(&iter);
        raxFree(webradios->db);
        if (init_rax == true) {
            webradios->db = raxNew();
        }
        else {
            webradios->db = NULL;
        }
    }
    if (webradios->idx_uris != NULL) {
        raxStart(&iter, webradios->idx_uris);
        raxSeek(&iter, "^", NULL, 0);
        while (raxNext(&iter)) {
            iter.data = NULL;
        }
        raxStop(&iter);
        raxFree(webradios->idx_uris);
        if (init_rax == true) {
            webradios->idx_uris = raxNew();
        }
        else {
            webradios->idx_uris = NULL;
        }
    }
}

/**
 * Frees the webradios struct
 * @param webradios struct to free
 */
void webradios_free(struct t_webradios *webradios) {
    webradios_clear(webradios, false);
    int rc = pthread_rwlock_destroy(&webradios->rwlock);
    if (rc != 0) {
        MYMPD_LOG_ERROR(NULL, "Can not destroy lock");
        MYMPD_LOG_ERRNO(NULL, rc);
    }
    FREE_PTR(webradios);
}

/**
 * Acquires a read lock
 * @param webradios pointer to webradios struct
 * @return true on success, else false
 */
bool webradios_get_read_lock(struct t_webradios *webradios) {
    MYMPD_LOG_DEBUG(NULL, "Waiting for read lock");
    int rc = pthread_rwlock_rdlock(&webradios->rwlock);
    if (rc == 0) {
        return true;
    }
    MYMPD_LOG_ERROR(NULL, "Can not get read lock");
    MYMPD_LOG_ERRNO(NULL, rc);
    return false;
}

/**
 * Acquires a write lock
 * @param webradios pointer to webradios struct
 * @return true on success, else false
 */
bool webradios_get_write_lock(struct t_webradios *webradios) {
    MYMPD_LOG_DEBUG(NULL, "Waiting for rw lock");
    int rc = pthread_rwlock_wrlock(&webradios->rwlock);
    if (rc == 0) {
        return true;
    }
    MYMPD_LOG_ERROR(NULL, "Can not get write lock");
    MYMPD_LOG_ERRNO(NULL, rc);
    return false;
}

/**
 * Frees the lock
 * @param webradios pointer to webradios struct
 * @return true on success, else false
 */
bool webradios_release_lock(struct t_webradios *webradios) {
    MYMPD_LOG_DEBUG(NULL, "Releasing lock");
    int rc = pthread_rwlock_unlock(&webradios->rwlock);
    if (rc == 0) {
        return true;
    }
    MYMPD_LOG_ERROR(NULL, "Can not free the lock");
    MYMPD_LOG_ERRNO(NULL, rc);
    return false;
}

/**
 * Saves the webradios to disk
 * @param config pointer to config
 * @param webradios webradios struct to write
 * @param filename file to write
 * @return true on success, else false
 */
bool webradios_save_to_disk(struct t_config *config, struct t_webradios *webradios, const char *filename) {
    if (webradios == NULL || webradios->db == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Webradios is NULL not saving anything");
        return true;
    }
    MYMPD_LOG_INFO(NULL, "Saving webradios to disc (%s)", filename);
    mpack_writer_t writer;
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s/%s.XXXXXX", config->workdir, DIR_WORK_TAGS, filename);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    // init mpack
    mpack_writer_init_stdfile(&writer, fp, true);
    mpack_writer_set_error_handler(&writer, log_mpack_write_error);
    mpack_start_array(&writer, (uint32_t)webradios->db->numele);
    raxIterator iter;
    raxStart(&iter, webradios->db);
    raxSeek(&iter, "^", NULL, 0);
    struct t_list_node *current;
    while (raxNext(&iter)) {
        mpack_build_map(&writer);
        struct t_webradio_data *data = (struct t_webradio_data *)iter.data;
        mpack_write_kv(&writer, "Name", data->name);
        mpack_write_kv(&writer, "Image", data->image);
        mpack_write_kv(&writer, "Homepage", data->homepage);
        mpack_write_kv(&writer, "Country", data->country);
        mpack_write_kv(&writer, "Region", data->region);
        mpack_write_kv(&writer, "Description", data->description);
        mpack_write_kv(&writer, "Added", (int64_t)data->added);
        mpack_write_kv(&writer, "Last-Modified", (int64_t)data->added);
        mpack_write_cstr(&writer, "Genres");
        mpack_build_array(&writer);
        current = data->genres.head;
        while (current != NULL) {
            mpack_write_cstr(&writer, current->key);
            current = current->next;
        }
        mpack_complete_array(&writer);
        mpack_write_cstr(&writer, "Languages");
        mpack_build_array(&writer);
        current = data->languages.head;
        while (current != NULL) {
            mpack_write_cstr(&writer, current->key);
            current = current->next;
        }
        mpack_complete_array(&writer);
        mpack_write_cstr(&writer, "Streams");
        mpack_build_array(&writer);
        current = data->uris.head;
        while (current != NULL) {
            mpack_build_map(&writer);
            mpack_write_kv(&writer, "Uri", current->key);
            mpack_write_kv(&writer, "Codec", current->value_p);
            mpack_write_kv(&writer, "Bitrate", current->value_i);
            mpack_complete_map(&writer);
            current = current->next;
        }
        mpack_complete_array(&writer);
        mpack_complete_map(&writer);
    }
    raxStop(&iter);
    mpack_finish_array(&writer);
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
 * Reads the webradios file from disk
 * @param config pointer to config
 * @param webradios webradios struct to populate
 * @param filename file to read
 * @param type Webradio type
 * @return true on success, else false
 */
bool webradios_read_from_disk(struct t_config *config, struct t_webradios *webradios, const char *filename, enum webradio_type type) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", config->workdir, DIR_WORK_TAGS, filename);
    if (testfile_read(filepath) == false) {
        FREE_SDS(filepath);
        webradios_clear(webradios, true);
        return false;
    }

    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    FREE_SDS(filepath);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);
    size_t len = mpack_node_array_length(root);
    sds uri = sdsempty();
    sds codec = sdsempty();
    for (size_t i = 0; i < len; i++) {
        mpack_node_t entry = mpack_node_array_at(root, i);
        struct t_webradio_data *data = webradio_data_new(type);
        data->name = mpackstr_sds(entry, "Name");
        data->image = mpackstr_sds(entry, "Image");
        data->homepage = mpackstr_sds(entry, "Homepage");
        data->country = mpackstr_sds(entry, "Country");
        data->region = mpackstr_sds(entry, "Region");
        data->description = mpackstr_sds(entry, "Description");
        data->added = (time_t)mpack_node_int(mpack_node_map_cstr(entry, "Added"));
        data->last_modified = (time_t)mpack_node_int(mpack_node_map_cstr(entry, "Last-Modified"));
        mpack_node_t genre_node = mpack_node_map_cstr(entry, "Genres");
        size_t genre_len = mpack_node_array_length(genre_node);
        for (size_t j = 0; j < genre_len; j++) {
            mpack_node_t array_node = mpack_node_array_at(genre_node, j);
            list_push_len(&data->genres, mpack_node_str(array_node), mpack_node_data_len(array_node),0, NULL, 0, NULL);
        }
        mpack_node_t lang_node = mpack_node_map_cstr(entry, "Languages");
        size_t lang_len = mpack_node_array_length(lang_node);
        for (size_t j = 0; j < lang_len; j++) {
            mpack_node_t array_node = mpack_node_array_at(lang_node, j);
            list_push_len(&data->languages, mpack_node_str(array_node), mpack_node_data_len(array_node),0, NULL, 0, NULL);
        }
        mpack_node_t streams_node = mpack_node_map_cstr(entry, "Streams");
        size_t streams_len = mpack_node_array_length(streams_node);
        for (size_t j = 0; j < streams_len; j++) {
            mpack_node_t array_node = mpack_node_array_at(streams_node, j);
            uri = mpackstr_sdscat(uri, array_node, "Uri");
            codec = mpackstr_sdscat(codec, array_node, "Codec");
            int64_t bitrate = mpack_node_int(mpack_node_map_cstr(array_node, "Bitrate"));
            list_push(&data->uris, uri, bitrate, codec, NULL);
            sdsclear(uri);
            sdsclear(codec);
        }
        if (sdslen(data->name) == 0) {
            MYMPD_LOG_ERROR(NULL, "Failure reading webradio entry");
            webradio_data_free(data);
        }
        else if (raxTryInsert(webradios->db, (unsigned char *)data->name, strlen(data->name), data, NULL) == 1) {
            // write uri index
            struct t_list_node *current = data->uris.head;
            while (current != NULL) {
                raxTryInsert(webradios->idx_uris, (unsigned char *)current->key, sdslen(current->key), data, NULL);
                current = current->next;
            }
        }
        else {
            // insert error
            MYMPD_LOG_ERROR(NULL, "Duplicate WebradioDB key found: %s", data->name);
            webradio_data_free(data);
        }
    }
    FREE_SDS(uri);
    FREE_SDS(codec);
    // clean up and check for errors
    bool rc = mpack_tree_destroy(&tree) != mpack_ok
        ? false
        : true;
    if (rc == false) {
        MYMPD_LOG_ERROR("default", "Reading webradios %s failed.", filename);
        webradios_clear(webradios, true);
        return rc;
    }

    MYMPD_LOG_INFO(NULL, "Read %" PRIu64 " webradios from %s", webradios->db->numele, filename);
    return rc;
}
