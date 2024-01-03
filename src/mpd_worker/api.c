/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_worker/api.h"

#include "src/lib/covercache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/playlists.h"
#include "src/mpd_worker/add_random.h"
#include "src/mpd_worker/album_cache.h"
#include "src/mpd_worker/jukebox.h"
#include "src/mpd_worker/smartpls.h"
#include "src/mpd_worker/song.h"

/**
 * Handler for mpd worker api requests
 * @param mpd_worker_state pointer to mpd_worker_state struct
 */
void mpd_worker_api(struct t_mpd_worker_state *mpd_worker_state) {
    struct t_work_request *request = mpd_worker_state->request;
    bool rc;
    bool bool_buf1;
    bool async = false;
    int int_buf1;
    unsigned uint_buf1;
    unsigned uint_buf2;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    sds error = sdsempty();

    struct t_jsonrpc_parse_error parse_error;
    jsonrpc_parse_error_init(&parse_error);

    const char *method = get_cmd_id_method_name(request->cmd_id);
    MYMPD_LOG_INFO(NULL, "MPD WORKER API request (%lu)(%u) %s: %s", request->conn_id, request->id, method, request->data);
    //create response struct
    struct t_work_response *response = create_response(request);
    //some shortcuts
    struct t_partition_state *partition_state = mpd_worker_state->partition_state;

    switch(request->cmd_id) {
        case INTERNAL_API_JUKEBOX_REFILL: {
            free_response(response);
            struct t_list *queue_list = (struct t_list *)request->extra;
            rc = mpd_worker_jukebox_queue_fill(mpd_worker_state, queue_list, 0, &error);
            if (rc == true) {
                mpd_worker_jukebox_push(mpd_worker_state);
            }
            else {
                mpd_worker_jukebox_error(mpd_worker_state, error);
            }
            list_free(queue_list);
            request->extra = NULL;
            async = true;
            break;
        }
        case INTERNAL_API_JUKEBOX_REFILL_ADD: {
            free_response(response);
            struct t_list *queue_list = (struct t_list *)request->extra;
            if (json_get_uint(request->data, "$.params.addSongs", 1, JUKEBOX_ADD_SONG_MAX, &uint_buf1, &parse_error) == true ) {
                rc = mpd_worker_jukebox_queue_fill_add(mpd_worker_state, queue_list, uint_buf1, &error);
                if (rc == true) {
                    mpd_worker_jukebox_push(mpd_worker_state);
                }
                else {
                    mpd_worker_jukebox_error(mpd_worker_state, error);
                }
            }
            list_free(queue_list);
            request->extra = NULL;
            async = true;
            break;
        }
        case MYMPD_API_QUEUE_ADD_RANDOM:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 2, &uint_buf1, &parse_error) == true &&
                json_get_uint(request->data, "$.params.quantity", 1, 1000, &uint_buf2, &parse_error) == true)
            {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Task to add random songs to queue has started");
                push_response(response);
                rc = mpd_worker_add_random_to_queue(mpd_worker_state, uint_buf2, uint_buf1, sds_buf1, request->partition);
                sds_buf2 = jsonrpc_respond_with_message_or_error(sdsempty(), request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Successfully added random songs to queue", "Adding random songs to queue failed");
                ws_notify_client(sds_buf2, request->id);
                async = true;
            }
            break;
        case MYMPD_API_COVERCACHE_CROP:
        case MYMPD_API_COVERCACHE_CLEAR:
            int_buf1 = request->cmd_id == MYMPD_API_COVERCACHE_CLEAR
                ? 0
                : mpd_worker_state->config->covercache_keep_days;
            int_buf1 = covercache_clear(mpd_worker_state->config->cachedir, int_buf1);
            sds_buf1 = sdsfromlonglong((long long) int_buf1);
            response->data = int_buf1 < 0
                ? jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Failed to clean up the covercache")
                : jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_INFO, "Removed %{smart_count} cover |||| Removed %{smart_count} covers",
                    2, "smartCount", sds_buf1);
            break;
        case MYMPD_API_SONG_FINGERPRINT:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_ispathfilename, &parse_error) == true) {
                response->data = mpd_worker_song_fingerprint(partition_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_CACHES_CREATE:
            if (json_get_bool(request->data, "$.params.force", &bool_buf1, &parse_error) == true) {
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_DATABASE);
                push_response(response);
                mpd_worker_album_cache_create(mpd_worker_state, bool_buf1);
                async = true;
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_SHUFFLE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true) {
                rc = mpd_client_playlist_shuffle(partition_state, sds_buf1, &error);
                response->data = rc == true
                    ? jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Shuffled playlist %{plist} successfully", 2, "plist", sds_buf1)
                    : jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Shuffling playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_SORT:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_string(request->data, "$.params.tag", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdtag, &parse_error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, NULL) == true)
            {
                rc = mpd_client_playlist_sort(partition_state, sds_buf1, sds_buf2, bool_buf1, &error);
                response->data = rc == true
                    ? jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                          JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Sorted playlist %{plist} successfully", 2, "plist", sds_buf1)
                    : jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                           JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Sorting playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_bool(request->data, "$.params.remove", &bool_buf1, &parse_error) == true)
            {
                int result = mpd_client_playlist_validate(partition_state, sds_buf1, bool_buf1, &error);
                if (result == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_ERROR, "Validation of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                }
                else if (result == 0) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_INFO, "Content of playlist %{plist} is valid", 2, "plist", sds_buf1);
                }
                else {
                    sds_buf2 = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlist %{plist}", 4, "count", sds_buf2, "plist", sds_buf1);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "%{count} invalid entries in playlist %{plist}", 4, "count", sds_buf2, "plist", sds_buf1);
                    }
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL:
            if (json_get_bool(request->data, "$.params.remove", &bool_buf1, &parse_error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists validation started");
                push_response(response);
                int result = mpd_client_playlist_validate_all(partition_state, bool_buf1, &error);
                if (result == -1) {
                    sds_buf1 = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_ERROR, "Validation of all playlists failed: %{error}", 2, "error", error);
                }
                else if (result == 0) {
                    sds_buf1 = jsonrpc_notify(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                        JSONRPC_SEVERITY_INFO, "Content of all playlists are valid");
                }
                else {
                    sds_buf2 = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        sds_buf1 = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlist %{plist}", 2, "count", sds_buf2);
                    }
                    else {
                        sds_buf1 = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_PLAYLIST,
                            JSONRPC_SEVERITY_WARN, "%{count} invalid entries in playlists", 2, "count", sds_buf2);
                    }
                }
                ws_notify(sds_buf1, MPD_PARTITION_ALL);
                async = true;
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_DEDUP:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_bool(request->data, "$.params.remove", &bool_buf1, &parse_error) == true)
            {
                int64_t result = mpd_client_playlist_dedup(partition_state, sds_buf1, bool_buf1, &error);
                if (result == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                }
                else if (result == 0) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of playlist %{plist} is uniq", 2, "plist", sds_buf1);
                }
                else {
                    sds_buf2 = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlist %{plist}", 4, "count", sds_buf2, "plist", sds_buf1);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count} duplicate entries in playlist %{plist}", 4, "count", sds_buf2, "plist", sds_buf1);
                    }
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL:
            if (json_get_bool(request->data, "$.params.remove", &bool_buf1, &parse_error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists deduplication started");
                push_response(response);
                sds buffer;
                int64_t result = mpd_client_playlist_dedup_all(partition_state, bool_buf1, &error);
                if (result == -1) {
                    buffer = jsonrpc_notify_phrase(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of all playlists failed: %{error}", 2, "error", error);
                }
                else if (result == 0) {
                    buffer = jsonrpc_notify(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of all playlists are uniq");
                }
                else {
                    sds_buf2 = sdsfromlonglong((long long)result);
                    if (bool_buf1 == true) {
                        buffer = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count} entries from playlists", 2, "count", sds_buf2);
                    }
                    else {
                        buffer = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count} duplicate entries in playlists", 2, "count", sds_buf2);
                    }
                }
                ws_notify(buffer, MPD_PARTITION_ALL);
                FREE_SDS(buffer);
                async = true;
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_bool(request->data, "$.params.remove", &bool_buf1, &parse_error) == true)
            {
                int result1 = mpd_client_playlist_validate(partition_state, sds_buf1, bool_buf1, &error);
                if (result1 == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Validation of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    break;
                }
                int64_t result2 = mpd_client_playlist_dedup(partition_state, sds_buf1, bool_buf1, &error);
                if (result2 == -1) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of playlist %{plist} failed: %{error}", 4, "plist", sds_buf1, "error", error);
                    break;
                }
                
                if (result1 + result2 == 0) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of playlist %{plist} is valid and uniq", 2, "plist", sds_buf1);
                }
                else {
                    sds_buf2 = sdsfromlonglong((long long)result1);
                    sds_buf3 = sdsfromlonglong((long long)result2);
                    if (bool_buf1 == true) {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count1} invalid and %{count2} duplicate entries from playlist %{plist}",
                            6, "count1", sds_buf2, "count2", sds_buf3, "plist", sds_buf1);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count1} invalid and %{count2} duplicate entries in playlist %{plist}",
                            6, "count1", sds_buf2, "count2", sds_buf3, "plist", sds_buf1);
                    }
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP_ALL:
            if (json_get_bool(request->data, "$.params.remove", &bool_buf1, &parse_error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists validation and deduplication started");
                push_response(response);
                int result1 = mpd_client_playlist_validate_all(partition_state, bool_buf1, &error);
                int64_t result2 = -1;
                if (result1 > -1) {
                    result2 = mpd_client_playlist_dedup_all(partition_state, bool_buf1, &error);
                }
                if (result1 == -1) {
                    sds_buf1 = jsonrpc_notify_phrase(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Validation of all playlists failed: %{error}", 2, "error", error);
                }
                else if (result2 == -1) {
                    sds_buf1 = jsonrpc_notify_phrase(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deduplication of all playlists failed: %{error}", 2, "error", error);
                }
                else if (result1 + result2 == 0) {
                    sds_buf1 = jsonrpc_notify(sdsempty(),
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Content of all playlists are valid and uniq");
                }
                else {
                    sds_buf2 = sdsfromlonglong((long long)result1);
                    sds_buf3 = sdsfromlonglong((long long)result2);
                    if (bool_buf1 == true) {
                        sds_buf1 = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "Removed %{count1} invalid and %{count2} duplicate entries from playlists",
                            4, "count1", sds_buf2, "count2", sds_buf3);
                    }
                    else {
                        sds_buf1 = jsonrpc_notify_phrase(sdsempty(),
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_WARN, "%{count1} invalid and %{count2} duplicate entries in playlists",
                            4, "count1", sds_buf2, "count2", sds_buf3);
                    }
                }
                ws_notify(sds_buf1, MPD_PARTITION_ALL);
                async = true;
            }
            break;
        case MYMPD_API_SMARTPLS_UPDATE:
            if (mpd_worker_state->smartpls == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Smart playlists are disabled");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &parse_error) == true) {
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
            if (json_get_bool(request->data, "$.params.force", &bool_buf1, &parse_error) == true) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Smart playlists update started");
                push_response(response);
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
    FREE_SDS(sds_buf3);

    if (async == true) {
        //already responded
        free_request(request);
        FREE_SDS(error);
        jsonrpc_parse_error_clear(&parse_error);
        return;
    }

    //sync request handling
    if (sdslen(response->data) == 0) {
        // error handling
        if (parse_error.message != NULL) {
            // jsonrpc parsing error
            response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Parsing error: %{message}", 4, "message", parse_error.message, "path", parse_error.path);
        }
        else if (sdslen(error) > 0) {
            // error from api function
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
            MYMPD_LOG_ERROR(MPD_PARTITION_DEFAULT, "Error processing method \"%s\"", method);
        }
        else {
            // no response and no error - this should not occur
            response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "No response for method %{method}", 2, "method", method);
            MYMPD_LOG_ERROR(MPD_PARTITION_DEFAULT, "No response for method \"%s\"", method);
        }
    }
    push_response(response);
    free_request(request);
    FREE_SDS(error);
    jsonrpc_parse_error_clear(&parse_error);
}
