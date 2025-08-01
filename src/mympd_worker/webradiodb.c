/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief WebradioDB update functions
 */

#include "compile_time.h"
#include "src/mympd_worker/webradiodb.h"

#include "dist/mongoose/mongoose.h"
#include "dist/rax/rax.h"
#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/http_client.h"
#include "src/lib/json/json_query.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/webradio.h"

// private definitions

static bool parse_webradiodb(sds str, struct t_webradios *webradiodb);
static bool icb_webradio_alternate(const char *path, sds key, sds value, enum json_vtype vtype,
        validate_callback vcb, void *userdata, struct t_json_parse_error *error);
static struct t_webradio_data *parse_webradiodb_data(sds str);

// public functions

/**
 * Updates the WebradioDB from the cloud
 * @param mympd_worker_state mpd worker state
 * @param force true = force update, false = update only if database is outdated
 * @return true on success, else false
 */
bool mympd_worker_webradiodb_update(struct t_mympd_worker_state *mympd_worker_state, bool force) {
    if (force == false) {
        sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", mympd_worker_state->config->workdir, DIR_WORK_TAGS, FILENAME_WEBRADIODB);
        time_t mtime = get_mtime(filepath);
        FREE_SDS(filepath);
        time_t now = time(NULL);
        if (mtime > now - 86400) {
            MYMPD_LOG_INFO(NULL, "WebradioDB is already up-to-date.");
            return true;
        }
    }

    struct mg_client_request_t http_request = {
        .method = "GET",
        .uri = WEBRADIODB_URI,
        .extra_headers = NULL,
        .post_data = NULL,
        .cert_check = mympd_worker_state->config->cert_check,
        .ca_certs = mympd_worker_state->config->ca_certs
    };
    struct mg_client_response_t http_response;
    http_client_response_init(&http_response);
    http_client_request(&http_request, &http_response);
    if (http_response.rc != 0) {
        http_client_response_clear(&http_response);
        return false;
    }
    struct t_webradios *webradiodb = webradios_new();
    bool rc = parse_webradiodb(http_response.body, webradiodb);
    http_client_response_clear(&http_response);
    if (rc == false) {
        webradios_free(webradiodb);
        return false;
    }

    webradios_save_to_disk(mympd_worker_state->config, webradiodb, FILENAME_WEBRADIODB);
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_WEBRADIODB_CREATED, "", "default");
    request->extra = (void *) webradiodb;
    request->extra_free = webradios_free_void;
    mympd_queue_push(mympd_api_queue, request, 0);
    return true;
}

// private functions

/**
 * Parses the downloaded webradios.min.json
 * @param str string to parse
 * @param webradiodb webradios struct to populate
 * @return true on success, else false
 */
static bool parse_webradiodb(sds str, struct t_webradios *webradiodb) {
    struct mg_str j_key;
    struct mg_str j_value;
    size_t off = 0;
    sds data_str = sdsempty();
    while ((off = mg_json_next(mg_str_n(str, sdslen(str)), off, &j_key, &j_value)) != 0) {
        data_str = sdscatlen(data_str, j_value.buf, j_value.len);
        struct t_webradio_data *data = parse_webradiodb_data(data_str);
        if (data != NULL) {
            if (raxTryInsert(webradiodb->db, (unsigned char *)j_key.buf, j_key.len, data, NULL) == 1) {
                // write uri index
                struct t_list_node *current = data->uris.head;
                while (current != NULL) {
                    raxTryInsert(webradiodb->idx_uris, (unsigned char *)current->key, sdslen(current->key), data, NULL);
                    current = current->next;
                }
            }
            else {
                // insert error
                MYMPD_LOG_ERROR(NULL, "Duplicate WebradioDB key found: %.*s", (int)j_key.len, j_key.buf);
                webradio_data_free(data);
            }
        }
        sdsclear(data_str);
    }
    FREE_SDS(data_str);
    MYMPD_LOG_INFO(NULL, "Added %" PRIu64 " webradios", webradiodb->db->numele);
    return true;
}

/**
 * Iteration callback function to parse the alternate webradio streams
 * @param path json path
 * @param key not used
 * @param value value to parse as mpd tag
 * @param vtype mjson value type
 * @param vcb not used - we validate directly
 * @param userdata void pointer to t_tags struct
 * @param error pointer to t_json_parse_error
 * @return true on success else false
 */
static bool icb_webradio_alternate(const char *path, sds key, sds value, enum json_vtype vtype,
        validate_callback vcb, void *userdata, struct t_json_parse_error *error)
{
    (void)vcb;
    (void)key;
    struct t_webradio_data *data = (struct t_webradio_data *)userdata;
    sds uri = NULL;
    sds codec = NULL;
    uint bitrate;
    if (vtype != JSON_TOK_OBJECT) {
        MYMPD_LOG_ERROR(NULL, "Invalid value for path %s", path);
        return false;
    }
    if (json_get_string(value, "$.StreamUri", 1, URI_LENGTH_MAX, &uri, vcb_isuri, error) == true &&
        json_get_string(value, "$.Codec", 1, URI_LENGTH_MAX, &codec, vcb_isname, error) == true &&
        json_get_uint_max(value, "$.Bitrate", &bitrate, error) == true)
    {
        list_push(&data->uris, uri, bitrate, codec, NULL);
    }
    FREE_SDS(uri);
    FREE_SDS(codec);
    return true;
}

/**
 * Parses a webradioDB entry
 * @param str string to parse
 * @return struct t_webradio_data* 
 */
static struct t_webradio_data *parse_webradiodb_data(sds str) {
    struct t_webradio_data *data = webradio_data_new(WEBRADIO_WEBRADIODB);
    struct t_json_parse_error parse_error;
    json_parse_error_init(&parse_error);
    sds uri = NULL;
    sds codec = NULL;
    uint bitrate;
    if (json_get_string(str, "$.Name", 1, URI_LENGTH_MAX, &data->name, vcb_isname, &parse_error) == false ||
        json_get_string(str, "$.Image", 1, URI_LENGTH_MAX, &data->image, vcb_isname, &parse_error) == false ||
        json_get_string(str, "$.Homepage", 0, URI_LENGTH_MAX, &data->homepage, vcb_isuri, &parse_error) == false ||
        json_get_string(str, "$.Country", 0, URI_LENGTH_MAX, &data->country, vcb_isname, &parse_error) == false ||
        json_get_string(str, "$.Region", 0, URI_LENGTH_MAX, &data->region, vcb_isname, &parse_error) == false ||
        json_get_string(str, "$.Description", 0, URI_LENGTH_MAX, &data->description, vcb_istext, &parse_error) == false ||
        json_get_array_string(str, "$.Genre", &data->genres, vcb_isname, 64, &parse_error) == false ||
        json_get_array_string(str, "$.Languages", &data->languages, vcb_isname, 64, &parse_error) == false ||
        json_get_string(str, "$.StreamUri", 1, URI_LENGTH_MAX, &uri, vcb_isuri, &parse_error) == false ||
        json_get_string(str, "$.Codec", 1, URI_LENGTH_MAX, &codec, vcb_isname, &parse_error) == false ||
        json_get_uint_max(str, "$.Bitrate", &bitrate, &parse_error) == false ||
        json_get_time_max(str, "$.Added", &data->added, &parse_error) == false ||
        json_get_time_max(str, "$.Last-Modified", &data->last_modified, &parse_error) == false)
    {
        webradio_data_free(data);
        json_parse_error_clear(&parse_error);
        return NULL;
    }
    list_push(&data->uris, uri, bitrate, codec, NULL);
    json_iterate_object(str, "$.alternativeStreams", icb_webradio_alternate, data, NULL, NULL, 64, &parse_error);
    FREE_SDS(uri);
    FREE_SDS(codec);
    json_parse_error_clear(&parse_error);
    return data;
}
