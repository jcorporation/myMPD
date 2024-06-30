/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/webradiodb.h"

#include "dist/rax/rax.h"
#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/mympd_api/webradio.h"

#include <dirent.h>
#include <string.h>

/**
 * Prints a WebradioDB entry as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param name webradio name
 * @return pointer to buffer
 */
sds mympd_api_webradiodb_radio_get_by_name(struct t_webradios *webradiodb, sds buffer, unsigned request_id, sds name) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIODB_RADIO_GET_BY_NAME;
    void *data = raxNotFound;
    if (webradiodb->db != NULL) {
        data = raxFind(webradiodb->db, (unsigned char *)name, strlen(name));
    }
    if (data == raxNotFound) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "WebradioDB entry not found");
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
 * Prints a WebradioDB entry as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri webradio stream uri
 * @return pointer to buffer
 */
sds mympd_api_webradiodb_radio_get_by_uri(struct t_webradios *webradiodb, sds buffer, unsigned request_id, sds uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI;
    void *data = raxNotFound;
    if (webradiodb->db != NULL) {
        data = raxFind(webradiodb->idx_uris, (unsigned char *)uri, strlen(uri));
    }
    if (data == raxNotFound) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "WebradioDB entry not found");
        return buffer;
    }
    struct t_webradio_data *webradio = (struct t_webradio_data *)data;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":");
    buffer = mympd_api_webradio_print(webradio, buffer);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
