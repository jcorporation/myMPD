/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP client cache
 */

#include "compile_time.h"
#include "src/lib/http_client_cache.h"

#include "dist/mpack/mpack.h"
#include "src/lib/cache/cache_disk.h"
#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/http_client.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mpack.h"
#include "src/lib/sds_extras.h"

#include <inttypes.h>

/**
 * Checks the http client cache for uri
 * @param config Pointer to config
 * @param uri URI to check
 * @return struct mg_client_response_t* or NULL if not found
 */
struct mg_client_response_t *http_client_cache_check(struct t_config *config, const char *uri) {
    if (config->cache_http_keep_days == CACHE_DISK_DISABLED) {
        MYMPD_LOG_DEBUG(NULL, "HTTP client cache disabled");
        return NULL;
    }
    sds hash = sds_hash_sha256(uri);
    sds filepath = sdscatfmt(sdsempty(), "%s/%s/%s", config->cachedir, DIR_CACHE_HTTP, hash);
    FREE_SDS(hash);
    if (testfile_read(filepath) == false) {
        FREE_SDS(filepath);
        return NULL;
    }
    struct mg_client_response_t *response = http_client_cache_read(filepath);
    FREE_SDS(filepath);
    if (response != NULL) {
        MYMPD_LOG_INFO(NULL, "Found cached response for %s", uri);
        return response;
    }
    return NULL;
}

/**
 * Reads a response from the http client cache
 * @param filepath Cache filename
 * @return struct mg_client_response_t* or NULL on error
 */
struct mg_client_response_t *http_client_cache_read(const char *filepath) {
    struct mg_client_response_t *mg_client_response = malloc_assert(sizeof(struct mg_client_response_t));
    http_client_response_init(mg_client_response);
    mg_client_response->rc = 0;
    mpack_tree_t tree;
    mpack_tree_init_filename(&tree, filepath, 0);
    mpack_tree_set_error_handler(&tree, log_mpack_node_error);
    update_mtime(filepath);
    mpack_tree_parse(&tree);
    mpack_node_t root = mpack_tree_root(&tree);
    mg_client_response->response_code = mpack_node_int(mpack_node_map_cstr(root, "code"));
    // Read headers
    mpack_node_t header_node = mpack_node_map_cstr(root, "header");
    size_t len = mpack_node_array_length(header_node);
    for (size_t i = 0; i < len; i++) {
        mpack_node_t array_node = mpack_node_array_at(header_node, i);
        const char *key = mpack_node_str(array_node);
        size_t key_len = mpack_node_strlen(array_node);
        i++;
        array_node = mpack_node_array_at(header_node, i);
        const char *value = mpack_node_str(array_node);
        size_t value_len = mpack_node_strlen(array_node);

        list_push_len(&mg_client_response->header, key, key_len, 0, value, value_len, NULL);
    }
    // Read body
    mpack_node_t body_node = mpack_node_map_cstr(root, "body");
    const char *body = mpack_node_bin_data(body_node);
    size_t body_len = mpack_node_bin_size(body_node);
    mg_client_response->body = sdscatlen(mg_client_response->body, body, body_len);
    // Clean up and check for errors
    if (mpack_tree_destroy(&tree) == mpack_ok) {
        return mg_client_response;
    }
    // Return NULL on error
    rm_file(filepath);
    http_client_response_clear(mg_client_response);
    FREE_PTR(mg_client_response);
    return NULL;
}

/**
 * Writes a http client cache file
 * @param config Pointer to config
 * @param uri URI to write the cache for
 * @param mg_client_response The http response to cache
 * @return true on success, else false
 */
bool http_client_cache_write(struct t_config *config, const char *uri, struct mg_client_response_t *mg_client_response) {
    if (config->cache_http_keep_days == CACHE_DISK_DISABLED) {
        MYMPD_LOG_DEBUG(NULL, "HTTP client cache disabled");
        return true;
    }
    sds hash = sds_hash_sha256(uri);
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%s/%s.XXXXXX", config->cachedir, DIR_CACHE_HTTP, hash);
    FREE_SDS(hash);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    mpack_writer_t writer;
    mpack_writer_init_stdfile(&writer, fp, true);
    mpack_writer_set_error_handler(&writer, log_mpack_write_error);
    mpack_build_map(&writer);
    mpack_write_kv(&writer, "code", mg_client_response->response_code);
    //write headers
    mpack_write_cstr(&writer, "header");
    mpack_start_array(&writer, (uint32_t)mg_client_response->header.length * 2);
    struct t_list_node *current = mg_client_response->header.head;
    while (current != NULL) {
        mpack_write_cstr(&writer, current->key);
        mpack_write_cstr(&writer, current->value_p);
        current = current->next;
    }
    mpack_finish_array(&writer);
    //write body
    mpack_write_cstr(&writer, "body");
    mpack_write_bin(&writer, mg_client_response->body, (uint32_t)sdslen(mg_client_response->body));
    mpack_complete_map(&writer);
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
