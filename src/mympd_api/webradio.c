/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD API webradio functions
 */

#include "compile_time.h"
#include "src/mympd_api/webradio.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/lib/search.h"
#include "src/lib/utility.h"

#include <string.h>

/**
 * Searches the webradio list
 * @param webradios Pointer to webradios struct
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param cmd_id API ID
 * @param offset offset for the list
 * @param limit maximum entries to print
 * @param expression string to search
 * @param sort Sort tag
 * @param sortdesc Sort descending?
 * @return pointer to buffer
 */
sds mympd_api_webradio_search(struct t_webradios *webradios, sds buffer, unsigned request_id,
    enum mympd_cmd_ids cmd_id, unsigned offset, unsigned limit, sds expression, sds sort, bool sortdesc)
{
    unsigned entities_returned = 0;
    unsigned entities_found = 0;
    unsigned real_limit = offset + limit;
    struct t_webradio_tags webradio_tags;
    webradio_tags_search(&webradio_tags);
    struct t_list *expr_list = parse_search_expression_to_list(expression, SEARCH_TYPE_WEBRADIO);

    enum webradio_tag_type sort_tag = webradio_tag_name_parse(sort);
    switch(sort_tag) {
        case WEBRADIO_TAG_UNKNOWN:
            sort_tag = WEBRADIO_TAG_NAME;
            MYMPD_LOG_WARN(NULL, "Invalid sort tag: %s", sort);
            break;
        case WEBRADIO_TAG_BITRATE:
        case WEBRADIO_TAG_ADDED:
        case WEBRADIO_TAG_LASTMODIFIED:
            sortdesc = sortdesc == true ? false : true;
            break;
        default:
            break;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
            buffer = sdscat(buffer,"\"data\":[");

    sds key = sdsempty();
    raxIterator iter;
    rax *sorted = raxNew();
    raxStart(&iter, webradios->db);
    raxSeek(&iter, "^", NULL, 0);
    // Search and sort
    while (raxNext(&iter)) {
        struct t_webradio_data *webradio_data = (struct t_webradio_data *)iter.data;
        if (search_expression_webradio(webradio_data, expr_list, &webradio_tags) == true) {
            switch(sort_tag) {
                case WEBRADIO_TAG_BITRATE:
                    key = sds_pad_int(webradio_data->uris.head->value_i, key);
                    break;
                case WEBRADIO_TAG_ADDED:
                    key = sds_pad_int(webradio_data->added, key);
                    break;
                case WEBRADIO_TAG_LASTMODIFIED:
                    key = sds_pad_int(webradio_data->last_modified, key);
                    break;
                default: {
                    const char *sort_value = webradio_get_tag(webradio_data, sort_tag, 0);
                    key = sdscat(key, sort_value);
                }
            }
            if (sort_tag != WEBRADIO_TAG_NAME) {
                key = sdscat(key, webradio_data->name);
            }
            rax_insert_no_dup(sorted, key, iter.data);
            sdsclear(key);
        }
    }
    FREE_SDS(key);
    raxStop(&iter);
    // Print the result
    raxStart(&iter, sorted);
    int (*iterator)(struct raxIterator *iter);
    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
        iterator = &raxNext;
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
        iterator = &raxPrev;
    }
    while (iterator(&iter)) {
        struct t_webradio_data *webradio_data = (struct t_webradio_data *)iter.data;
        if (entities_found >= offset) {
            if (entities_returned++) {
                buffer= sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = mympd_api_webradio_print(webradio_data, buffer, NULL);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entities_found++;
        if (entities_found == real_limit) {
            break;
        }
    }
    raxStop(&iter);
    raxFree(sorted);
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
    buffer = tojson_uint(buffer, "limit", limit, true);
    buffer = tojson_sds(buffer, "expression", expression, true);
    buffer = tojson_sds(buffer, "sort", sort, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, false);
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
    struct t_webradio_data *webradio = webradio_by_uri(mympd_state->webradio_favorites, mympd_state->webradiodb, uri);
    if (webradio != NULL) {
        buffer = sdscatlen(buffer, "{", 1);
        buffer = mympd_api_webradio_print(webradio, buffer, NULL);
        buffer = sdscatlen(buffer, "}", 1);
    }
    return buffer;
}

/**
 * Prints a webradio entry
 * @param webradio webradio data struct to print
 * @param buffer already allocated buffer to append the data
 * @param uri Main uri for the entry
 * @return pointer to buffer
 */
sds mympd_api_webradio_print(struct t_webradio_data *webradio, sds buffer, const char *uri) {
    buffer = tojson_sds(buffer, "Name", webradio->name, true);
    if (webradio->type == WEBRADIO_WEBRADIODB) {
        sds image = sdscatfmt(sdsempty(), "%s%s", WEBRADIODB_URI_PICS, webradio->image);
        buffer = tojson_sds(buffer, "Image", image, true);
        FREE_SDS(image);
    }
    else {
        buffer = tojson_sds(buffer, "Image", webradio->image, true);
    }
    buffer = tojson_sds(buffer, "Homepage", webradio->homepage, true);
    buffer = tojson_sds(buffer, "Country", webradio->country, true);
    buffer = tojson_sds(buffer, "Region", webradio->region, true);
    buffer = tojson_sds(buffer, "Description", webradio->description, true);
    buffer = tojson_time(buffer, "Added", webradio->added, true);
    buffer = tojson_time(buffer, "Last-Modified", webradio->last_modified, true);
    struct t_list_node *main_uri = NULL;
    if (uri != NULL) {
        main_uri= list_get_node(&webradio->uris, uri);
    }
    if (main_uri == NULL) {
        main_uri = webradio->uris.head;
    }
    buffer = tojson_sds(buffer, "StreamUri", main_uri->key, true);
    buffer = tojson_sds(buffer, "Codec", main_uri->value_p, true);
    buffer = tojson_int64(buffer, "Bitrate", main_uri->value_i, true);
    buffer = sdscat(buffer, "\"alternativeStreams\":{");
    struct t_list_node *current = webradio->uris.head;
    unsigned i = 0;
    sds key = sdsempty();
    while (current != NULL) {
        if (current->key != main_uri->key) {
            if (i++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            key = sdscatsds(key, current->key);
            sanitize_filename(key);
            buffer = sds_catjson(buffer, key, sdslen(key));
            sdsclear(key);
            buffer = sdscatlen(buffer, ":{", 2);
            buffer = tojson_sds(buffer, "StreamUri", current->key, true);
            buffer = tojson_sds(buffer, "Codec", current->value_p, true);
            buffer = tojson_int64(buffer, "Bitrate", current->value_i, false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        current = current->next;
    }
    FREE_SDS(key);
    buffer = sdscat(buffer, "},\"Genres\":");
    buffer = list_to_json_array(buffer, &webradio->genres);
    buffer = sdscat(buffer, ",\"Languages\":");
    buffer = list_to_json_array(buffer, &webradio->languages);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_char(buffer, "Type", webradio_type_name(webradio->type), false);
    return buffer;
}

/**
 * Gets a Webradio entry by name and print it as jsonrpc response
 * @param webradios Pointer to webradios struct
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param cmd_id API method
 * @param name webradio name
 * @return pointer to buffer
 */
sds mympd_api_webradio_radio_get_by_name(struct t_webradios *webradios, sds buffer, unsigned request_id,
        enum mympd_cmd_ids cmd_id, sds name)
{
    void *data = raxFind(webradios->db, (unsigned char *)name, strlen(name));
    if (data == raxNotFound) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio entry not found");
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = mympd_api_webradio_print(webradio, buffer, NULL);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Gets a Webradio entry by uri and print it as jsonrpc response
 * @param webradios Pointer to webradios struct
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param cmd_id API method
 * @param uri webradio stream uri
 * @return pointer to buffer
 */
sds mympd_api_webradio_radio_get_by_uri(struct t_webradios *webradios, sds buffer, unsigned request_id,
        enum mympd_cmd_ids cmd_id, sds uri)
{
    void *data = raxFind(webradios->idx_uris, (unsigned char *)uri, strlen(uri));
    if (data == raxNotFound) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Webradio entry not found");
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = mympd_api_webradio_print(webradio, buffer, uri);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
