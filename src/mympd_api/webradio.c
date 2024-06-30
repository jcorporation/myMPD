/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/webradio.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <string.h>

/**
 * Search webradio by uri in favorites and WebradioDB and print json response
 * @param mympd_state pointer to mympd_state
 * @param uri Uri to search for
 * @return newly allocated sds string, or empty string on error
 */
sds mympd_api_webradio_from_uri_tojson(struct t_mympd_state *mympd_state, const char *uri) {
    sds buffer = sdsempty();
    struct t_webradio_data *webradio = mympd_api_webradio_by_uri(mympd_state, uri);
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
    list_to_json_array(buffer, &webradio->genres);
    buffer = sdscat(buffer, ",\"Languages\":");
    list_to_json_array(buffer, &webradio->languages);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_char(buffer, "Type", webradio_type_name(webradio->type), false);
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}

/**
 * Search webradio by uri in favorites and WebradioDB
 * @param mympd_state pointer to mympd_state
 * @param uri Uri to search for
 * @return pointer to webradio data or NULL on error
 */
struct t_webradio_data *mympd_api_webradio_by_uri(struct t_mympd_state *mympd_state, const char *uri) {
    void *data = raxNotFound;
    if (mympd_state->webradio_favorites->idx_uris != NULL) {
        data = raxFind(mympd_state->webradio_favorites->idx_uris, (unsigned char *)uri, strlen(uri));
    }
    if (data == raxNotFound) {
        if (mympd_state->webradiodb->idx_uris != NULL) {
            data = raxFind(mympd_state->webradiodb->idx_uris, (unsigned char *)uri, strlen(uri));
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
    struct t_webradio_data *webradio = mympd_api_webradio_by_uri(mympd_state, uri);
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
    struct t_webradio_data *webradio = mympd_api_webradio_by_uri(mympd_state, uri);
    if (webradio != NULL) {
        return webradio_to_extm3u(webradio, buffer, uri);
    }
    return sdscat(buffer, "Webradio not found");
}
