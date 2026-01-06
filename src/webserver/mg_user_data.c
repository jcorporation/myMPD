/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#include "compile_time.h"
#include "src/webserver/mg_user_data.h"

#include "src/lib/config/cert.h"
#include "src/lib/filehandler.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

#ifdef MYMPD_EMBEDDED_ASSETS
    //embedded files for release build
    #include "embedded_files.c"
#endif

// Private definitions

static void add_file(struct t_mg_user_data *mg_user_data, const char *uri, const char *mimetype,
        bool compressed, bool cache, const unsigned char *data, unsigned size);
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
    mg_user_data->image_names_sm = sds_split_comma_trim(MYMPD_IMAGE_NAMES_SM, &mg_user_data->image_names_sm_len);
    mg_user_data->image_names_md = sds_split_comma_trim(MYMPD_IMAGE_NAMES_MD, &mg_user_data->image_names_md_len);
    mg_user_data->image_names_lg = sds_split_comma_trim(MYMPD_IMAGE_NAMES_LG, &mg_user_data->image_names_lg_len);
    mg_user_data->lyrics.uslt_ext = sdsempty();
    mg_user_data->lyrics.sylt_ext = sdsempty();
    mg_user_data->lyrics.vorbis_uslt = sdsempty();
    mg_user_data->lyrics.vorbis_sylt = sdsempty();
    mg_user_data->publish_music = false;
    mg_user_data->publish_playlists = false;
    mg_user_data->connection_count = 2; // listening + wakeup
    list_init(&mg_user_data->stream_uris);
    list_init(&mg_user_data->session_list);
    mg_user_data->mympd_api_started = false;
    mg_user_data->webradiodb = NULL;
    mg_user_data->webradio_favorites = NULL;
    mg_user_data->embedded_file_index = 0;
    #ifdef MYMPD_EMBEDDED_ASSETS
        add_file(mg_user_data, "/", "text/html; charset=utf-8", true, false, index_html_data, index_html_size);
        add_file(mg_user_data, "/css/combined.css", "text/css; charset=utf-8", true, false, combined_css_data, combined_css_size);
        add_file(mg_user_data, "/js/combined.js", "application/javascript; charset=utf-8", true, false, combined_js_data, combined_js_size);
        add_file(mg_user_data, "/sw.js", "application/javascript; charset=utf-8", true, false, sw_js_data, sw_js_size);
        add_file(mg_user_data, "/mympd.webmanifest", "application/manifest+json", true, false, mympd_webmanifest_data, mympd_webmanifest_size);
        add_file(mg_user_data, "/assets/coverimage-notavailable.svg", "image/svg+xml", true, true, coverimage_notavailable_svg_data, coverimage_notavailable_svg_size);
        add_file(mg_user_data, "/assets/MaterialIcons-Regular.woff2", "font/woff2", false, true, MaterialIcons_Regular_woff2_data, MaterialIcons_Regular_woff2_size);
        add_file(mg_user_data, "/assets/coverimage-stream.svg", "image/svg+xml", true, true, coverimage_stream_svg_data, coverimage_stream_svg_size);
        add_file(mg_user_data, "/assets/coverimage-booklet.svg", "image/svg+xml", true, true, coverimage_booklet_svg_data, coverimage_booklet_svg_size);
        add_file(mg_user_data, "/assets/coverimage-mympd.svg", "image/svg+xml", true, true, coverimage_mympd_svg_data, coverimage_mympd_svg_size);
        add_file(mg_user_data, "/assets/coverimage-playlist.svg", "image/svg+xml", true, true, coverimage_playlist_svg_data, coverimage_playlist_svg_size);
        add_file(mg_user_data, "/assets/coverimage-smartpls.svg", "image/svg+xml", true, true, coverimage_smartpls_svg_data, coverimage_smartpls_svg_size);
        add_file(mg_user_data, "/assets/coverimage-transparent.svg", "image/svg+xml", true, true, coverimage_transparent_svg_data, coverimage_transparent_svg_size);
        add_file(mg_user_data, "/assets/coverimage-folder.svg", "image/svg+xml", true, true, coverimage_folder_svg_data, coverimage_folder_svg_size);
        add_file(mg_user_data, "/assets/mympd-background-dark.svg", "image/svg+xml", true, true, mympd_background_dark_svg_data, mympd_background_dark_svg_size);
        add_file(mg_user_data, "/assets/mympd-background-light.svg", "image/svg+xml", true, true, mympd_background_light_svg_data, mympd_background_light_svg_size);
        add_file(mg_user_data, "/assets/appicon-192.png", "image/png", false, true, appicon_192_png_data, appicon_192_png_size);
        add_file(mg_user_data, "/assets/appicon-512.png", "image/png", false, true, appicon_512_png_data, appicon_512_png_size);
        add_file(mg_user_data, "/assets/ligatures.json", "application/json", true, true, ligatures_json_data, ligatures_json_size);
        #ifdef I18N_bg_BG
            add_file(mg_user_data, "/assets/i18n/bg-BG.json", "application/json", true, true, i18n_bg_BG_json_data, i18n_bg_BG_json_size);
        #endif
        #ifdef I18N_de_DE
            add_file(mg_user_data, "/assets/i18n/de-DE.json", "application/json", true, true, i18n_de_DE_json_data, i18n_de_DE_json_size);
        #endif
        #ifdef I18N_en_US
            add_file(mg_user_data, "/assets/i18n/en-US.json", "application/json", true, true, i18n_en_US_json_data, i18n_en_US_json_size);
        #endif
        #ifdef I18N_es_AR
            add_file(mg_user_data, "/assets/i18n/es-AR.json", "application/json", true, true, i18n_es_AR_json_data, i18n_es_AR_json_size);
        #endif
        #ifdef I18N_es_ES
            add_file(mg_user_data, "/assets/i18n/es-ES.json", "application/json", true, true, i18n_es_ES_json_data, i18n_es_ES_json_size);
        #endif
        #ifdef I18N_es_VE
            add_file(mg_user_data, "/assets/i18n/es-VE.json", "application/json", true, true, i18n_es_VE_json_data, i18n_es_VE_json_size);
        #endif
        #ifdef I18N_fi_FI
            add_file(mg_user_data, "/assets/i18n/fi-FI.json", "application/json", true, true, i18n_fi_FI_json_data, i18n_fi_FI_json_size);
        #endif
        #ifdef I18N_fr_FR
            add_file(mg_user_data, "/assets/i18n/fr-FR.json", "application/json", true, true, i18n_fr_FR_json_data, i18n_fr_FR_json_size);
        #endif
        #ifdef I18N_it_IT
            add_file(mg_user_data, "/assets/i18n/it-IT.json", "application/json", true, true, i18n_it_IT_json_data, i18n_it_IT_json_size);
        #endif
        #ifdef I18N_ja_JP
            add_file(mg_user_data, "/assets/i18n/ja-JP.json", "application/json", true, true, i18n_ja_JP_json_data, i18n_ja_JP_json_size);
        #endif
        #ifdef I18N_ko_KR
            add_file(mg_user_data, "/assets/i18n/ko-KR.json", "application/json", true, true, i18n_ko_KR_json_data, i18n_ko_KR_json_size);
        #endif
        #ifdef I18N_nl_NL
            add_file(mg_user_data, "/assets/i18n/nl-NL.json", "application/json", true, true, i18n_nl_NL_json_data, i18n_nl_NL_json_size);
        #endif
        #ifdef I18N_pl_PL
            add_file(mg_user_data, "/assets/i18n/pl-PL.json", "application/json", true, true, i18n_pl_PL_json_data, i18n_pl_PL_json_size);
        #endif
        #ifdef I18N_ru_RU
            add_file(mg_user_data, "/assets/i18n/ru-RU.json", "application/json", true, true, i18n_ru_RU_json_data, i18n_ru_RU_json_size);
        #endif
        #ifdef I18N_zh_Hans
            add_file(mg_user_data, "/assets/i18n/zh-Hans.json", "application/json", true, true, i18n_zh_Hans_json_data, i18n_zh_Hans_json_size);
        #endif
        #ifdef I18N_zh_Hant
            add_file(mg_user_data, "/assets/i18n/zh-Hant.json", "application/json", true, true, i18n_zh_Hant_json_data, i18n_zh_Hant_json_size);
        #endif
    #endif
    add_file(mg_user_data, NULL, NULL, false, false, NULL, 0);
    // Enforce last entry to be NULL
    mg_user_data->embedded_files[MAX_EMBEDDED_FILES - 1].uri = NULL;
    mg_user_data->embedded_files[MAX_EMBEDDED_FILES - 1].mimetype = NULL;
    mg_user_data->embedded_files[MAX_EMBEDDED_FILES - 1].compressed = false;
    mg_user_data->embedded_files[MAX_EMBEDDED_FILES - 1].cache = false;
    mg_user_data->embedded_files[MAX_EMBEDDED_FILES - 1].data = NULL;
    mg_user_data->embedded_files[MAX_EMBEDDED_FILES - 1].size = 0;
    return mg_user_data;
}

/**
 * Frees the members of mg_user_data struct and the struct itself
 * @param mg_user_data pointer to mg_user_data struct
 */
void mg_user_data_free(struct t_mg_user_data *mg_user_data) {
    FREE_SDS(mg_user_data->browse_directory);
    FREE_SDS(mg_user_data->music_directory);
    sdsfreesplitres(mg_user_data->image_names_sm, mg_user_data->image_names_sm_len);
    sdsfreesplitres(mg_user_data->image_names_md, mg_user_data->image_names_md_len);
    sdsfreesplitres(mg_user_data->image_names_lg, mg_user_data->image_names_lg_len);
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
    FREE_SDS(mg_user_data->lyrics.uslt_ext);
    FREE_SDS(mg_user_data->lyrics.sylt_ext);
    FREE_SDS(mg_user_data->lyrics.vorbis_uslt);
    FREE_SDS(mg_user_data->lyrics.vorbis_sylt);
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
 * Adds a file to the 
 * @param mg_user_data 
 * @param uri 
 * @param mimetype 
 * @param compressed 
 * @param cache 
 * @param data 
 * @param size 
 */
static void add_file(struct t_mg_user_data *mg_user_data, const char *uri, const char *mimetype,
        bool compressed, bool cache, const unsigned char *data, unsigned size)
{
    if (mg_user_data->embedded_file_index == MAX_EMBEDDED_FILES - 1) {
        MYMPD_LOG_EMERG(NULL, "Too many embedded files");
        abort();
    }
    mg_user_data->embedded_files[mg_user_data->embedded_file_index].uri = uri;
    mg_user_data->embedded_files[mg_user_data->embedded_file_index].mimetype = mimetype;
    mg_user_data->embedded_files[mg_user_data->embedded_file_index].compressed = compressed;
    mg_user_data->embedded_files[mg_user_data->embedded_file_index].cache = cache;
    mg_user_data->embedded_files[mg_user_data->embedded_file_index].data = data;
    mg_user_data->embedded_files[mg_user_data->embedded_file_index].size = size;
    mg_user_data->embedded_file_index++;
}

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
