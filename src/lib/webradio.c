/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/webradio.h"

#include "dist/mpack/mpack.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"

/**
 * Creates a new webradio data struct
 * @return struct t_webradio_data* 
 */
struct t_webradio_data *webradio_data_new(void) {
    struct t_webradio_data *data = malloc_assert(sizeof(struct t_webradio_data));
    list_init(&data->uris);
    list_init(&data->genres);
    list_init(&data->languages);
    data->name = NULL;
    data->image = NULL;
    data->homepage = NULL;
    data->country = NULL;
    data->state = NULL;
    data->description = NULL;
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
    FREE_SDS(data->state);
    FREE_SDS(data->description);
    // pointer data itself
    FREE_PTR(data);
}

/**
 * Frees the webradios rax
 * @param webradios rax tree to free
 */
void webradio_free(rax *webradios) {
    raxIterator iter;
    raxStart(&iter, webradios);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        webradio_data_free((struct t_webradio_data *)iter.data);
        iter.data = NULL;
    }
    raxStop(&iter);
    raxFree(webradios);
}

/**
 * Saves the webradios to disk
 * @param config pointer to config
 * @param webradios webradios rax to write
 * @param filename file to write
 * @return true on success, else false
 */
bool webradio_save_to_disk(struct t_config *config, rax *webradios, const char *filename) {
    if (webradios == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Webradio is NULL not saving anything");
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
    mpack_start_array(&writer, (uint32_t)webradios->numele);
    raxIterator iter;
    raxStart(&iter, webradios);
    raxSeek(&iter, "^", NULL, 0);
    struct t_list_node *current;
    while (raxNext(&iter)) {
        mpack_build_map(&writer);
        struct t_webradio_data *data = (struct t_webradio_data *)iter.data;
        mpack_write_cstr(&writer, "Key");
        mpack_write_str(&writer, (char *)iter.key, (uint32_t)iter.key_len);
        mpack_write_kv(&writer, "Name", data->name);
        mpack_write_kv(&writer, "Image", data->image);
        mpack_write_kv(&writer, "Homepage", data->homepage);
        mpack_write_kv(&writer, "Country", data->country);
        mpack_write_kv(&writer, "State", data->state);
        mpack_write_kv(&writer, "Description", data->description);
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
 * @param filename file to read
 * @return newly allocated rax with webradios
 */
rax *webradio_read_from_disk(struct t_config *config, const char *filename) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", config->workdir, DIR_WORK_TAGS, filename);
    if (testfile_read(filepath) == false) {
        FREE_SDS(filepath);
        return false;
    }
    rax *webradios = raxNew();

    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    FREE_SDS(filepath);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);
    size_t len = mpack_node_array_length(root);
    sds key = sdsempty();
    sds uri = sdsempty();
    sds codec = sdsempty();
    for (size_t i = 0; i < len; i++) {
        mpack_node_t entry = mpack_node_array_at(root, i);
        struct t_webradio_data *data = webradio_data_new();
        key = mpackstr_sdscat(key, entry, "Key");
        data->name = mpackstr_sds(entry, "Name");
        data->image = mpackstr_sds(entry, "Image");
        data->homepage = mpackstr_sds(entry, "Homepage");
        data->country = mpackstr_sds(entry, "Country");
        data->state = mpackstr_sds(entry, "State");
        data->description = mpackstr_sds(entry, "Description");
        mpack_node_t genre_node = mpack_node_map_cstr(entry, "Genres");
        size_t genre_len = mpack_node_array_length(genre_node);
        for (size_t j = 0; j < genre_len; j++) {
            mpack_node_t array_node = mpack_node_array_at(genre_node, j);
            list_push_len(&data->languages, mpack_node_str(array_node), mpack_node_data_len(array_node),0, NULL, 0, NULL);
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
        if (raxTryInsert(webradios, (unsigned char *)key, strlen(key), data, NULL) == 0) {
            // insert error
            MYMPD_LOG_ERROR(NULL, "Duplicate WebradioDB key found: %s", key);
            webradio_data_free(data);
        }
        sdsclear(key);
    }
    FREE_SDS(key);
    FREE_SDS(uri);
    FREE_SDS(codec);
    // clean up and check for errors
    bool rc = mpack_tree_destroy(&tree) != mpack_ok
        ? false
        : true;
    if (rc == false) {
        MYMPD_LOG_ERROR("default", "Reading webradios %s failed.", filename);
        webradio_free(webradios);
        return NULL;
    }

    MYMPD_LOG_INFO(NULL, "Read %" PRIu64 " webradios %s from disc", webradios->numele, filename);
    return webradios;
}
