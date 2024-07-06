/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/webradio.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/lib/search.h"
#include "src/lib/utility.h"

#include <string.h>

/**
 * Searches the webradio list
 * @param webradio_favorites Webradio favorites struct
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param expression string to search
 * @param offset offset for the list
 * @param limit maximum entries to print
 * @return pointer to buffer
 */
sds mympd_api_webradio_search(struct t_webradios *webradios, sds buffer, unsigned request_id,
    enum mympd_cmd_ids cmd_id, unsigned offset, unsigned limit, sds expression)
{
    if (webradios->db == NULL) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio database is empty");
    }
    unsigned entities_returned = 0;
    unsigned entities_found = 0;
    unsigned real_limit = offset + limit;
    struct t_webradio_tags webradio_tags;
    webradio_tags_search(&webradio_tags);
    struct t_list *expr_list = parse_search_expression_to_list(expression, SEARCH_TYPE_WEBRADIO);
    raxIterator iter;
    raxStart(&iter, webradios->db);
    raxSeek(&iter, "^", NULL, 0);
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
            buffer = sdscat(buffer,"\"data\":[");
    while (raxNext(&iter)) {
        struct t_webradio_data *webradio_data = (struct t_webradio_data *)iter.data;
        if (search_expression_webradio(webradio_data, expr_list, &webradio_tags) == true) {
            if (entities_found >= offset) {
                if (entities_returned++) {
                    buffer= sdscatlen(buffer, ",", 1);
                }
                buffer = mympd_api_webradio_print(webradio_data, buffer);
            }
            entities_found++;
            if (entities_found == real_limit) {
                break;
            }
        }
    }
    raxStop(&iter);
    free_search_expression_list(expr_list);

    buffer = sdscatlen(buffer, "],", 2);
    if (entities_found == real_limit) {
        buffer = tojson_int(buffer, "totalEntities", -1, true);
    }
    else {
        buffer = tojson_uint(buffer, "totalEntities", entities_found, true);
    }
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "expression", expression, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}

/**
 * Search webradio by uri in favorites and WebradioDB and print json response
 * @param mympd_state pointer to mympd_state
 * @param uri Uri to search for
 * @return newly allocated sds string, or empty string on error
 */
sds mympd_api_webradio_from_uri_tojson(struct t_mympd_state *mympd_state, const char *uri) {
    sds buffer = sdsempty();
    struct t_webradio_data *webradio = mympd_api_webradio_by_uri(mympd_state, uri, WEBRADIO_ALL);
    if (webradio != NULL) {
        buffer = mympd_api_webradio_print(webradio, buffer);
    }
    return buffer;
}

/**
 * Prints a webradio entry
 * @param webradio webradio data struct to print
 * @param buffer already allocated buffer to append the data
 * @return pointer to buffer
 */
sds mympd_api_webradio_print(struct t_webradio_data *webradio, sds buffer) {
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_sds(buffer, "Name", webradio->name, true);
    sds image = webradio_get_cover_uri(webradio, sdsempty());
    buffer = tojson_sds(buffer, "Image", image, true);
    FREE_SDS(image);
    buffer = tojson_sds(buffer, "Homepage", webradio->homepage, true);
    buffer = tojson_sds(buffer, "Country", webradio->country, true);
    buffer = tojson_sds(buffer, "State", webradio->state, true);
    buffer = tojson_sds(buffer, "Description", webradio->description, true);
    struct t_list_node *current = webradio->uris.head;
    buffer = tojson_sds(buffer, "StreamUri", current->key, true);
    buffer = tojson_sds(buffer, "Codec", current->value_p, true);
    buffer = tojson_int64(buffer, "Bitrate", current->value_i, true);
    buffer = sdscat(buffer, "\"alternativeStreams\":{");
    current = current->next;
    unsigned i = 0;
    while (current != NULL) {
        if (i++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        sds key = sdsdup(current->key);
        sanitize_filename(key);
        buffer = sds_catjson(buffer, key, sdslen(key));
        buffer = sdscatlen(buffer, ":{", 2);
        buffer = tojson_sds(buffer, "StreamUri", current->key, true);
        buffer = tojson_sds(buffer, "Codec", current->value_p, true);
        buffer = tojson_int64(buffer, "Bitrate", current->value_i, false);
        buffer = sdscatlen(buffer, "}", 1);
        current = current->next;
    }
    buffer = sdscat(buffer, "},\"Genres\":");
    buffer = list_to_json_array(buffer, &webradio->genres);
    buffer = sdscat(buffer, ",\"Languages\":");
    buffer = list_to_json_array(buffer, &webradio->languages);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_char(buffer, "Type", webradio_type_name(webradio->type), false);
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}

/**
 * Prints a Webradio entry as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param name webradio name
 * @return pointer to buffer
 */
sds mympd_api_webradio_radio_get_by_name(struct t_webradios *webradios, sds buffer, unsigned request_id, sds name) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIODB_RADIO_GET_BY_NAME;
    void *data = raxNotFound;
    if (webradios->db != NULL) {
        data = raxFind(webradios->db, (unsigned char *)name, strlen(name));
    }
    if (data == raxNotFound) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio entry not found");
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":");
    buffer = mympd_api_webradio_print(webradio, buffer);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Prints a Webradio entry as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri webradio stream uri
 * @return pointer to buffer
 */
sds mympd_api_webradio_radio_get_by_uri(struct t_webradios *webradios, sds buffer, unsigned request_id, sds uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI;
    void *data = raxNotFound;
    if (webradios->db != NULL) {
        data = raxFind(webradios->idx_uris, (unsigned char *)uri, strlen(uri));
    }
    if (data == raxNotFound) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio entry not found");
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":");
    buffer = mympd_api_webradio_print(webradio, buffer);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Search webradio by uri in favorites and WebradioDB
 * @param mympd_state pointer to mympd_state
 * @param uri Uri to search for
 * @param type Webradio type to lookup
 * @return pointer to webradio data or NULL on error
 */
struct t_webradio_data *mympd_api_webradio_by_uri(struct t_mympd_state *mympd_state, const char *uri, enum webradio_type type) {
    void *data = raxNotFound;
    if (type == WEBRADIO_ALL || WEBRADIO_FAVORITE) {
        if (mympd_state->webradio_favorites->idx_uris != NULL) {
            data = raxFind(mympd_state->webradio_favorites->idx_uris, (unsigned char *)uri, strlen(uri));
        }
    }
    if (type == WEBRADIO_ALL || WEBRADIO_WEBRADIODB) {
        if (data == raxNotFound) {
            if (mympd_state->webradiodb->idx_uris != NULL) {
                data = raxFind(mympd_state->webradiodb->idx_uris, (unsigned char *)uri, strlen(uri));
            }
        }
    }
    if (data == raxNotFound) {
        return NULL;
    }
    return (struct t_webradio_data *)data;
}

/**
 * Returns the uri for the webradio image
 * @param mympd_state pointer to mympd_state
 * @param buffer Buffer to append the uri
 * @param uri Webradio uri
 * @return Pointer to buffer
 */
sds mympd_api_webradio_get_cover_by_uri(struct t_mympd_state *mympd_state, sds buffer, sds uri) {
    struct t_webradio_data *webradio = mympd_api_webradio_by_uri(mympd_state, uri, WEBRADIO_ALL);
    if (webradio != NULL) {
        return webradio_get_cover_uri(webradio, buffer);
    }
    // Not found
    return sdscat(buffer, "/assets/coverimage-stream");
}

/**
 * Returns an extm3u for a webradio
 * @param mympd_state pointer to mympd_state
 * @param buffer Buffer to append the uri
 * @param uri Webradio uri
 * @return Pointer to buffer
 */
sds mympd_api_webradio_get_extm3u(struct t_mympd_state *mympd_state, sds buffer, sds uri) {
    struct t_webradio_data *webradio = mympd_api_webradio_by_uri(mympd_state, uri, WEBRADIO_ALL);
    if (webradio != NULL) {
        return webradio_to_extm3u(webradio, buffer, uri);
    }
    return sdscat(buffer, "Webradio not found");
}
