/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/webradio_favorites.h"

#include "dist/rax/rax.h"
#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/utility.h"
#include "src/mympd_api/webradio.h"

#include <dirent.h>
#include <string.h>

/**
 * Prints a webradio favorite as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param name webradio name
 * @return pointer to buffer
 */
sds mympd_api_webradio_favorite_get_by_name(struct t_webradios *webradio_favorites, sds buffer, unsigned request_id, sds name) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIO_FAVORITE_GET_BY_NAME;
    void *data = raxNotFound;
    if (webradio_favorites->db != NULL) {
        data = raxFind(webradio_favorites->db, (unsigned char *)name, strlen(name));
    }
    if (data == raxNotFound) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio favorite not found");
        return buffer;
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":");
    buffer = mympd_api_webradio_print(webradio, buffer);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Prints a webradio favorite as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri webradio stream uri
 * @return pointer to buffer
 */
sds mympd_api_webradio_favorite_get_by_uri(struct t_webradios *webradio_favorites, sds buffer, unsigned request_id, sds uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIO_FAVORITE_GET_BY_URI;
    void *data = raxNotFound;
    if (webradio_favorites->idx_uris != NULL) {
        data = raxFind(webradio_favorites->idx_uris, (unsigned char *)uri, strlen(uri));
    }
    if (data == raxNotFound) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio favorite not found");
        return buffer;
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":");
    buffer = mympd_api_webradio_print(webradio, buffer);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Prints the webradio list as a jsonrpc response
 * @param webradio_favorites Webradio favorites struct
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param expression string to search
 * @param offset offset for the list
 * @param limit maximum entries to print
 * @return pointer to buffer
 */
sds mympd_api_webradio_favorites_search(struct t_webradios *webradio_favorites, sds buffer, unsigned request_id,
        sds expression, unsigned offset, unsigned limit)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIO_FAVORITE_SEARCH;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    //TODO
    (void) webradio_favorites;
    (void) expression;
    (void) offset;
    (void) limit;
    buffer = sdscatlen(buffer, "],", 2);
    //buffer = tojson_uint64(buffer, "totalEntities", webradios->numele, true);
    //buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Saves a webradio favorite
 * @param webradio_favorites Webradio favorites struct
 * @param webradio webradio struct to save
 * @param old_name old webradio name
 * @return true on success, else false
 */
bool mympd_api_webradio_favorite_save(struct t_webradios *webradio_favorites, struct t_webradio_data *webradio, sds old_name) {
    if (webradio->uris.head == NULL) {
        return false;
    }
    if (sdslen(old_name) > 0) {
        struct t_list old_names;
        list_push(&old_names, old_name, 0, NULL, NULL);
        if (mympd_api_webradio_favorite_delete(webradio_favorites, &old_names) == false) {
            return false;
        }
        list_clear(&old_names);
    }

    sds id = sdsdup(webradio->uris.head->key);
    sanitize_filename(id);
    if (raxTryInsert(webradio_favorites->db, (unsigned char *)id, sdslen(id), webradio, NULL) == 1) {
        // write uri index
        raxTryInsert(webradio_favorites->idx_uris, (unsigned char *)webradio->uris.head->key, sdslen(webradio->uris.head->key), webradio, NULL);
        return true;
    }
    return false;
}

/**
 * Deletes webradio favorite
 * @param webradio_favorites Webradio favorites struct
 * @param names webradio ids to delete
 * @return true on success, else false
 */
bool mympd_api_webradio_favorite_delete(struct t_webradios *webradio_favorites, struct t_list *names) {
    bool rc = true;
    struct t_list_node *current = names->head;
    while (current != NULL) {
        void *data = NULL;
        if (raxRemove(webradio_favorites->db, (unsigned char *)current->key, sdslen(current->key), &data) == 1) {
            struct t_webradio_data *webradio = (struct t_webradio_data *)data;
            // remove uri index
            raxRemove(webradio_favorites->idx_uris, (unsigned char *)webradio->uris.head->key, sdslen(webradio->uris.head->key), NULL);
            webradio_data_free(webradio);
        }
        else {
            rc = false;
        }
        current = current->next;
    }
    return rc;
}
