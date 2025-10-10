/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#include "compile_time.h"
#include "src/webserver/mg_user_data.h"

#include "src/lib/cert.h"
#include "src/lib/filehandler.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

// Private definitions

static bool read_certs(struct t_mg_user_data *mg_user_data, struct t_config *config);

// Public functions

/**
 * Mallocs and initializes the mg_user_data struct for the webserver
 * @param config pointer to myMPD config
 * @return struct t_mg_user_data* or NULL on error
 */
struct t_mg_user_data *webserver_init_mg_user_data(struct t_config *config) {
    struct t_mg_user_data *mg_user_data = malloc_assert(sizeof(struct t_mg_user_data));
    mg_user_data->cert_content = NULL;
    mg_user_data->cert = mg_str("");
    mg_user_data->key_content = NULL;
    mg_user_data->key = mg_str("");
    if (config->ssl == true &&
        read_certs(mg_user_data, config) == false)
    {
        FREE_PTR(mg_user_data);
        return NULL;
    }

    mg_user_data->config = config;
    mg_user_data->browse_directory = sdscatfmt(sdsempty(), "%S/%s", config->workdir, DIR_WORK_EMPTY);
    mg_user_data->music_directory = sdsempty();
    mg_user_data->placeholder_booklet = sdsempty();
    mg_user_data->placeholder_mympd = sdsempty();
    mg_user_data->placeholder_na = sdsempty();
    mg_user_data->placeholder_stream = sdsempty();
    mg_user_data->placeholder_playlist = sdsempty();
    mg_user_data->placeholder_smartpls = sdsempty();
    mg_user_data->placeholder_folder = sdsempty();
    mg_user_data->placeholder_transparent = sdsempty();
    sds default_coverimage_names = sdsnew(MYMPD_COVERIMAGE_NAMES);
    mg_user_data->coverimage_names= sds_split_comma_trim(default_coverimage_names, &mg_user_data->coverimage_names_len);
    FREE_SDS(default_coverimage_names);
    sds default_thumbnail_names = sdsnew(MYMPD_THUMBNAIL_NAMES);
    mg_user_data->thumbnail_names= sds_split_comma_trim(default_thumbnail_names, &mg_user_data->thumbnail_names_len);
    FREE_SDS(default_thumbnail_names);
    mg_user_data->publish_music = false;
    mg_user_data->publish_playlists = false;
    mg_user_data->connection_count = 2; // listening + wakeup
    list_init(&mg_user_data->stream_uris);
    list_init(&mg_user_data->session_list);
    mg_user_data->mympd_api_started = false;
    mg_user_data->webradiodb = NULL;
    mg_user_data->webradio_favorites = NULL;
    return mg_user_data;
}

/**
 * Frees the members of mg_user_data struct and the struct itself
 * @param mg_user_data pointer to mg_user_data struct
 */
void mg_user_data_free(struct t_mg_user_data *mg_user_data) {
    FREE_SDS(mg_user_data->browse_directory);
    FREE_SDS(mg_user_data->music_directory);
    sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
    sdsfreesplitres(mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len);
    list_clear(&mg_user_data->stream_uris);
    list_clear(&mg_user_data->session_list);
    FREE_SDS(mg_user_data->placeholder_booklet);
    FREE_SDS(mg_user_data->placeholder_mympd);
    FREE_SDS(mg_user_data->placeholder_na);
    FREE_SDS(mg_user_data->placeholder_stream);
    FREE_SDS(mg_user_data->placeholder_playlist);
    FREE_SDS(mg_user_data->placeholder_smartpls);
    FREE_SDS(mg_user_data->placeholder_folder);
    FREE_SDS(mg_user_data->placeholder_transparent);
    FREE_SDS(mg_user_data->cert_content);
    FREE_SDS(mg_user_data->key_content);
    FREE_PTR(mg_user_data);
}

/**
 * Frees the members of mg_user_data struct and the struct itself
 * @param mg_user_data pointer to mg_user_data struct
 */
void mg_user_data_free_void(void *mg_user_data) {
    mg_user_data_free((struct t_mg_user_data *)mg_user_data);
}

/**
 * Private functions
 */

/**
 * Reads the ssl key and certificate from disc
 * @param mg_user_data pointer to mongoose user data
 * @param config pointer to myMPD config
 * @return true on success, else false
 */
static bool read_certs(struct t_mg_user_data *mg_user_data, struct t_config *config) {
    int nread = 0;
    mg_user_data->cert_content = sds_getfile(sdsempty(), config->ssl_cert, SSL_FILE_MAX, false, true, &nread);
    if (nread <= 0) {
        MYMPD_LOG_ERROR(NULL, "Failure reading ssl certificate from disc");
        return false;
    }
    nread = 0;
    mg_user_data->key_content = sds_getfile(sdsempty(), config->ssl_key, SSL_FILE_MAX, false, true, &nread);
    if (nread <= 0) {
        MYMPD_LOG_ERROR(NULL, "Failure reading ssl key from disc");
        return false;
    }
    sds cert_details = certificate_get_detail(mg_user_data->cert_content);
    if (sdslen(cert_details) > 0) {
        MYMPD_LOG_INFO(NULL, "Certificate: %s", cert_details);
    }
    FREE_SDS(cert_details);
    mg_user_data->cert = mg_str(mg_user_data->cert_content);
    mg_user_data->key = mg_str(mg_user_data->key_content);
    return true;
}
