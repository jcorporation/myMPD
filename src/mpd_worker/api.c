/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_worker/api.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/playlists.h"
#include "src/mpd_worker/cache.h"
#include "src/mpd_worker/smartpls.h"

/**
 * Handler for mpd worker api requests
 * @param mpd_worker_state pointer to mpd_worker_state struct
 */
void mpd_worker_api(struct t_mpd_worker_state *mpd_worker_state) {
    struct t_work_request *request = mpd_worker_state->request;
    bool rc;
    bool bool_buf1;
    bool async = false;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds error = sdsempty();

    const char *method = get_cmd_id_method_name(request->cmd_id);
    MYMPD_LOG_INFO(NULL, "MPD WORKER API request (%lld)(%ld) %s: %s", request->conn_id, request->id, method, request->data);
    //create response struct
    struct t_work_response *response = create_response(request);

    switch(request->cmd_id) {
        case MYMPD_API_CACHES_CREATE:
            if (json_get_bool(request->data, "$.params.force", &bool_buf1, &error) == true) {
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_DATABASE);
                push_response(response, request->id, request->conn_id);
                mpd_worker_cache_init(mpd_worker_state, bool_buf1);
                async = true;
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_SHUFFLE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                if (mpd_client_playlist_shuffle(mpd_worker_state->partition_state, sds_buf1, &error)) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Shuffled playlist %{plist} successfully", 2, "plist", sds_buf1);
                }
                else {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Shuffling playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    sdsclear(error);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_SORT:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.tag", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdtag, &error) == true)
            {
                rc = mpd_client_playlist_sort(mpd_worker_state->partition_state, sds_buf1, sds_buf2, &error);
                if (rc == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Sorted playlist %{plist} successfully", 2, "plist", sds_buf1);
                }
                else {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Sorting playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    sdsclear(error);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_bool(request->data, "$.params.remove", &bool_buf1, &error) == true)
            {
                long result = mpd_client_playlist_validate(mpd_worker_state->partition_state, sds_buf1, bool_buf1, &error);
                if (result == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_ERROR, "Validation of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    sdsclear(error);
                }
                else if (result == 0) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_INFO, "Content of playlist %{plist} is valid", 2, "plist", sds_buf1);
                }
                else {
                    sds result_str = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlist %{plist}", 4, "count", result_str, "plist", sds_buf1);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "%{count} invalid entries in playlist %{plist}", 4, "count", result_str, "plist", sds_buf1);
                    }
                    FREE_SDS(result_str);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL:
            if (json_get_bool(request->data, "$.params.remove", &bool_buf1, &error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists validation started");
                push_response(response, request->id, request->conn_id);
                long result = mpd_client_playlist_validate_all(mpd_worker_state->partition_state, bool_buf1, &error);
                sds buffer;
                if (result == -1) {
                    buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_ERROR, "Validation of all playlists failed: %{error}", 2, "error", error);
                    sdsclear(error);
                }
                else if (result == 0) {
                    buffer = jsonrpc_notify(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_INFO, "Content of all playlists are valid");
                }
                else {
                    sds result_str = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlist %{plist}", 2, "count", result_str);
                    }
                    else {
                        buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "%{count} invalid entries in playlists", 2, "count", result_str);
                    }
                    FREE_SDS(result_str);
                }
                ws_notify(buffer, MPD_PARTITION_ALL);
                FREE_SDS(buffer);
                async = true;
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_DEDUP:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_bool(request->data, "$.params.remove", &bool_buf1, &error) == true)
            {
                long result = mpd_client_playlist_dedup(mpd_worker_state->partition_state, sds_buf1, bool_buf1, &error);
                if (result == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    sdsclear(error);
                }
                else if (result == 0) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of playlist %{plist} is uniq", 2, "plist", sds_buf1);
                }
                else {
                    sds result_str = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlist %{plist}", 4, "count", result_str, "plist", sds_buf1);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count} duplicate entries in playlist %{plist}", 4, "count", result_str, "plist", sds_buf1);
                    }
                    FREE_SDS(result_str);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL:
            if (json_get_bool(request->data, "$.params.remove", &bool_buf1, &error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists deduplication started");
                push_response(response, request->id, request->conn_id);
                sds buffer;
                long result = mpd_client_playlist_dedup_all(mpd_worker_state->partition_state, bool_buf1, &error);
                if (result == -1) {
                    buffer = jsonrpc_notify_phrase(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of all playlists failed: %{error}", 2, "error", error);
                    sdsclear(error);
                }
                else if (result == 0) {
                    buffer = jsonrpc_notify(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of all playlists are uniq");
                }
                else {
                    sds result_str = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        buffer = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlists", 2, "count", result_str);
                    }
                    else {
                        buffer = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count} duplicate entries in playlists", 4, "count", result_str);
                    }
                    FREE_SDS(result_str);
                }
                ws_notify(buffer, MPD_PARTITION_ALL);
                FREE_SDS(buffer);
                async = true;
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_bool(request->data, "$.params.remove", &bool_buf1, &error) == true)
            {
                long result1 = mpd_client_playlist_validate(mpd_worker_state->partition_state, sds_buf1, bool_buf1, &error);
                if (result1 == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Validation of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    sdsclear(error);
                    break;
                }
                long result2 = mpd_client_playlist_dedup(mpd_worker_state->partition_state, sds_buf1, bool_buf1, &error);
                if (result2 == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    sdsclear(error);
                    break;
                }
                
                if (result1 + result2 == 0) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of playlist %{plist} is valid and uniq", 2, "plist", sds_buf1);
                }
                else {
                    sds result_str1 = sdsfromlonglong((long long)result1);
                    sds result_str2 = sdsfromlonglong((long long)result2);
                    if (bool_buf1 == true) {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count1} invalid and %{count2} duplicate entries from playlist %{plist}",
                            6, "count1", result_str1, "count2", result_str2, "plist", sds_buf1);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count1} invalid and %{count2} duplicate entries in playlist %{plist}",
                            6, "count1", result_str1, "count2", result_str2, "plist", sds_buf1);
                    }
                    FREE_SDS(result_str1);
                    FREE_SDS(result_str2);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP_ALL:
            if (json_get_bool(request->data, "$.params.remove", &bool_buf1, &error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists validation and deduplication started");
                push_response(response, request->id, request->conn_id);
                sds buffer;
                long result1 = mpd_client_playlist_validate_all(mpd_worker_state->partition_state, bool_buf1, &error);
                long result2 = -1;
                if (result1 > -1) {
                    result2 = mpd_client_playlist_dedup_all(mpd_worker_state->partition_state, bool_buf1, &error);
                }
                if (result1 == -1) {
                    buffer = jsonrpc_notify_phrase(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Validation of all playlists failed: %{error}", 2, "error", error);
                }
                else if (result2 == -1) {
                    buffer = jsonrpc_notify_phrase(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of all playlists failed: %{error}", 2, "error", error);
                }
                else if (result1 + result2 == 0) {
                    buffer = jsonrpc_notify(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of all playlists are valid and uniq");
                }
                else {
                    sds result_str1 = sdsfromlonglong((long long)result1);
                    sds result_str2 = sdsfromlonglong((long long)result2);
                    if (bool_buf1 == true) {
                        buffer = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count1} invalid and %{count2} duplicate entries from playlists",
                            4, "count1", result_str1, "count2", result_str2);
                    }
                    else {
                        buffer = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count1} invalid and %{count2} duplicate entries in playlists",
                            4, "count1", result_str1, "count2", result_str2);
                    }
                    FREE_SDS(result_str1);
                    FREE_SDS(result_str2);
                }
                ws_notify(buffer, MPD_PARTITION_ALL);
                FREE_SDS(buffer);
                sdsclear(error);
                async = true;
            }
            break;
        case MYMPD_API_SMARTPLS_UPDATE:
            if (mpd_worker_state->smartpls == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Smart playlists are disabled");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_worker_smartpls_update(mpd_worker_state, sds_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Smart playlist %{playlist} updated", 2, "playlist", sds_buf1);
                    //notify client
                    //send mpd event manually as fallback if mpd playlist is not created (no songs are found)
                    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_STORED_PLAYLIST, MPD_PARTITION_ALL);
                }
                else {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Updating smart playlist %{playlist} failed", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_SMARTPLS_UPDATE_ALL:
            if (mpd_worker_state->smartpls == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Smart playlists are disabled");
                break;
            }
            if (json_get_bool(request->data, "$.params.force", &bool_buf1, &error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Smart playlists update started");
                push_response(response, request->id, request->conn_id);
                rc = mpd_worker_smartpls_update_all(mpd_worker_state, bool_buf1);
                if (rc == true) {
                    send_jsonrpc_notify(JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, MPD_PARTITION_ALL, "Smart playlists updated");
                }
                else {
                    send_jsonrpc_notify(JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, MPD_PARTITION_ALL, "Smart playlists update failed");
                }
                async = true;
            }
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Unknown request");
            MYMPD_LOG_ERROR(MPD_PARTITION_DEFAULT, "Unknown API request: %.*s", (int)sdslen(request->data), request->data);
    }
    FREE_SDS(sds_buf1);
    FREE_SDS(sds_buf2);

    //error handling
    if (sdslen(error) > 0) {
        response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
        MYMPD_LOG_ERROR(MPD_PARTITION_DEFAULT, "Error processing method \"%s\"", method);
    }
    FREE_SDS(error);

    if (async == true) {
        //already responded
        free_request(request);
        return;
    }

    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "No response for method %{method}", 2, "method", method);
        MYMPD_LOG_ERROR(MPD_PARTITION_DEFAULT, "No response for method \"%s\"", method);
    }
    push_response(response, request->id, request->conn_id);
    free_request(request);
}
