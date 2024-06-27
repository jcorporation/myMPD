/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/webradio.h"

#include "src/lib/jsonrpc.h"

#include <string.h>

/**
 * Search webradio by uri in favorites and WebradioDB and print json response
 * @param mympd_state pointer to mympd_state
 * @param uri Uri to search for
 * @return newly allocated sds string, or empty string on error
 */
sds webradio_from_uri_tojson(struct t_mympd_state *mympd_state, const char *uri) {
    sds buffer = sdsempty();
    struct t_webradio_data *webradio = webradio_by_uri(mympd_state, uri);
    if (webradio != NULL) {
        buffer = webradio_print(webradio, buffer);
    }
    return buffer;
}

/**
 * Prints a webradio entry
 * @param webradio webradio data struct to print
 * @param buffer already allocated buffer to append the data
 * @return pointer to buffer
 */
sds webradio_print(struct t_webradio_data *webradio, sds buffer) {
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_sds(buffer, "Name", webradio->name, true);
    buffer = tojson_sds(buffer, "Image", webradio->homepage, true);
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
        buffer = tojson_sds(buffer, "StreamUri", current->key, true);
        buffer = tojson_sds(buffer, "Codec", current->value_p, true);
        buffer = tojson_int64(buffer, "Bitrate", current->value_i, true);
    }
    buffer = sdscat(buffer, "},\"Genres\":[");
    list_to_json_array(buffer, &webradio->genres);
    buffer = sdscat(buffer, "],\"Languages\":[");
    list_to_json_array(buffer, &webradio->languages);
    buffer = sdscatlen(buffer, "]}", 2);
    return buffer;
}

/**
 * Search webradio by uri in favorites and WebradioDB
 * @param mympd_state pointer to mympd_state
 * @param uri Uri to search for
 * @return pointer to webradio data or NULL on error
 */
struct t_webradio_data *webradio_by_uri(struct t_mympd_state *mympd_state, const char *uri) {
    void *data = raxFind(mympd_state->webradio_favorites->idx_uris, (unsigned char *)uri, strlen(uri));
    if (data == raxNotFound) {
        raxFind(mympd_state->webradio_favorites->idx_uris, (unsigned char *)uri, strlen(uri));
    }
    if (data == raxNotFound) {
        return NULL;
    }
    return (struct t_webradio_data *)data;
}
