/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Configuration handling
 */

#include "compile_time.h"
#include "src/lib/config.h"

#include "src/lib/cacertstore.h"
#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/config_def.h"
#include "src/lib/env.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

/**
 * Default config definition
 */
static const struct t_config_default config_default[] = {
    [CI_ACL]                    = {"acl",                    {.t = CIT_S, .s = ""},             0, 0, vcb_isname},
    [CI_ALBUM_MODE]             = {"album_mode",             {.t = CIT_S, .s = "adv"},          0, 0, vcb_isalnum},
    [CI_ALBUM_GROUP_TAG]        = {"album_group_tag",        {.t = CIT_S, .s = "Date"},         0, 0, vcb_isalnum},
    [CI_CA_CERT_STORE]          = { "ca_cert_store",         {.t = CIT_S, .s = ""},             0, 0, vcb_isfilepath},
    [CI_CACHE_COVER_KEEP_DAYS]  = {"cache_cover_keep_days",  {.t = CIT_I, .i = 31},             CACHE_AGE_MIN, CACHE_AGE_MAX, NULL},
    [CI_CACHE_HTTP_KEEP_DAYS]   = {"cache_http_keep_days",   {.t = CIT_I, .i = 31},             CACHE_AGE_MIN, CACHE_AGE_MAX, NULL},
    [CI_CACHE_LYRICS_KEEP_DAYS] = {"cache_lyrics_keep_days", {.t = CIT_I, .i = 31},             CACHE_AGE_MIN, CACHE_AGE_MAX, NULL},
    [CI_CACHE_MISC_KEEP_DAYS]   = {"cache_misc_keep_days",   {.t = CIT_I, .i = 1},              1, CACHE_AGE_MAX, NULL},
    [CI_CACHE_THUMBS_KEEP_DAYS] = {"cache_thumbs_keep_days", {.t = CIT_I, .i = 31},             CACHE_AGE_MIN, CACHE_AGE_MAX, NULL},
    [CI_CERT_CHECK]             = {"cert_check",             {.t = CIT_B, .b = true},           0, 0, NULL},
    [CI_CUSTOM_CERT]            = {"custom_cert",            {.t = CIT_B, .b = false},          0, 0, NULL},
    [CI_CUSTOM_CSS]             = {"custom_css",             {.t = CIT_S, .s = ""},             0, 0, vcb_istext},
    [CI_CUSTOM_JS]              = {"custom_js",              {.t = CIT_S, .s = ""},             0, 0, vcb_istext},
    [CI_HTTP]                   = {"http",                   {.t = CIT_B, .b = true},           0, 0, NULL},
    [CI_HTTP_HOST]              = {"http_host",              {.t = CIT_S, .s = ""},             0, 0, vcb_isname}, 
    [CI_HTTP_PORT]              = {"http_port",              {.t = CIT_I, .i = 8080},           0, MPD_PORT_MAX, NULL},
    [CI_LOGLEVEL]               = {"loglevel",               {.t = CIT_I, .i = CFG_MYMPD_LOGLEVEL},  LOGLEVEL_MIN, LOGLEVEL_MAX, NULL},
    [CI_MYMPD_URI]              = {"mympd_uri",              {.t = CIT_S, .s = "auto"},         0, 0, vcb_isname},
    [CI_PIN_HASH]               = {"pin_hash",               {.t = CIT_S, .s = ""},             0, 0, vcb_isalnum},
    [CI_SAVE_CACHES]            = {"save_caches",            {.t = CIT_B, .b = true},           0, 0, NULL},
    [CI_SCRIPTACL]              = {"scriptacl",              {.t = CIT_S, .s = "+127.0.0.0/8"}, 0, 0, vcb_isname},
    [CI_SCRIPTS_EXTERNAL]       = {"scripts_external",       {.t = CIT_B, .b = false},          0, 0, NULL},
    [CI_SSL]                    = {"ssl",                    {.t = CIT_B, .b = true},           0, 0, NULL},
    [CI_SSL_CERT]               = {"ssl_cert",               {.t = CIT_S, .s = ""},             0, 0, vcb_isfilepath},
    [CI_SSL_KEY]                = {"ssl_key",                {.t = CIT_S, .s = ""},             0, 0, vcb_isfilepath},
    [CI_SSL_PORT]               = {"ssl_port",               {.t = CIT_I, .i = 8443},           0, MPD_PORT_MAX, NULL},
    [CI_SSL_SAN]                = {"ssl_san",                {.t = CIT_S, .s = ""},             0, 0, vcb_isname},
    [CI_STICKERS]               = {"stickers",               {.t = CIT_B, .b = true},           0, 0, NULL},
    [CI_STICKERS_PAD_INT]       = {"stickers_pad_int",       {.t = CIT_B, .b = false},          0, 0, NULL},
    [CI_WEBRADIODB]             = {"webradiodb",             {.t = CIT_B, .b = true},           0, 0, NULL}
};

/**
 * Compile time initialization check
 */
#define IFV_N (sizeof config_default/sizeof config_default[0])
_Static_assert(IFV_N == CI_COUNT, "Unexpected size");

/**
 * Frees the config struct
 * @param config pointer to config struct
 */
void mympd_config_free(struct t_config *config) {
    // Command line options
    FREE_SDS(config->cachedir);
    FREE_SDS(config->workdir);
    // Configuration
    FREE_SDS(config->acl);
    FREE_SDS(config->album_mode);
    FREE_SDS(config->album_group_tag);
    FREE_SDS(config->ca_certs);
    FREE_SDS(config->ca_cert_store);
    FREE_SDS(config->custom_css);
    FREE_SDS(config->custom_js);
    FREE_SDS(config->http_host);
    FREE_SDS(config->mympd_uri);
    FREE_SDS(config->pin_hash);
    FREE_SDS(config->scriptacl);
    FREE_SDS(config->ssl_cert);
    FREE_SDS(config->ssl_key);
    FREE_SDS(config->ssl_san);
    // Struct itself
    FREE_PTR(config);
}

/**
 * Sets the initial default values for config struct
 * This function is used before reading command line arguments
 * @param config pointer to config struct
 */
void mympd_config_defaults_initial(struct t_config *config) {
    //command line options
    config->log_to_syslog = CFG_MYMPD_LOG_TO_SYSLOG;
    config->cachedir = sdsnew(MYMPD_CACHE_DIR);
    config->workdir = sdsnew(MYMPD_WORK_DIR);
    //not configurable
    config->bootstrap = false;
    config->startup_time = time(NULL);
    //set all other sds strings to NULL
    config->acl = NULL;
    config->album_mode = NULL;
    config->album_group_tag = NULL;
    config->ca_certs = NULL;
    config->ca_cert_store = NULL;
    config->custom_css = NULL;
    config->custom_js = NULL;
    config->http_host = NULL;
    config->mympd_uri = NULL;
    config->pin_hash = NULL;
    config->scriptacl = NULL;
    config->ssl_cert = NULL;
    config->ssl_key = NULL;
    config->ssl_san = NULL;
}

/**
 * Sets config values
 * @param config Pointer to config
 * @param ci Config item enum
 * @param value Value to set
 */
static void set_config(struct t_config *config, enum config_item ci, struct t_config_value *value) {
    switch (ci) {
        case CI_CUSTOM_CERT:
            assert(value->t == CIT_B);
            config->custom_cert = value->b;
            break;
        case CI_HTTP:
            assert(value->t == CIT_B);
            config->http = value->b;
            break;
        case CI_SAVE_CACHES:
            assert(value->t == CIT_B);
            config->save_caches = value->b;
            break;
        case CI_SSL:
            assert(value->t == CIT_B);
            config->ssl = value->b;
            break;
        case CI_STICKERS:
            assert(value->t == CIT_B);
            config->stickers = value->b;
            break;
        case CI_STICKERS_PAD_INT:
            assert(value->t == CIT_B);
            config->stickers_pad_int = value->b;
            break;
        case CI_WEBRADIODB:
            assert(value->t == CIT_B);
            config->webradiodb = value->b;
            break;
        case CI_CERT_CHECK:
            assert(value->t == CIT_B);
            config->cert_check = value->b;
            break;
        case CI_CACHE_COVER_KEEP_DAYS:
            assert(value->t == CIT_I);
            config->cache_cover_keep_days = value->i;
            break;
        case CI_CACHE_LYRICS_KEEP_DAYS:
            assert(value->t == CIT_I);
            config->cache_lyrics_keep_days = value->i;
            break;
        case CI_CACHE_THUMBS_KEEP_DAYS:
            assert(value->t == CIT_I);
            config->cache_thumbs_keep_days = value->i;
            break;
        case CI_CACHE_MISC_KEEP_DAYS:
            assert(value->t == CIT_I);
            config->cache_misc_keep_days = value->i;
            break;
        case CI_CACHE_HTTP_KEEP_DAYS:
            assert(value->t == CIT_I);
            config->cache_http_keep_days = value->i;
            break;
        case CI_HTTP_PORT:
            assert(value->t == CIT_I);
            config->http_port = value->i;
            break;
        case CI_LOGLEVEL:
            assert(value->t == CIT_I);
            config->loglevel = value->i;
            break;
        case CI_SSL_PORT:
            assert(value->t == CIT_I);
            config->ssl_port = value->i;
            break;
        case CI_ACL:
            assert(value->t == CIT_S);
            config->acl = value->s;
            break;
        case CI_ALBUM_MODE:
            assert(value->t == CIT_S);
            config->album_mode = value->s;
            break;
        case CI_ALBUM_GROUP_TAG:
            assert(value->t == CIT_S);
            config->album_group_tag = value->s;
            break;
        case CI_HTTP_HOST:
            assert(value->t == CIT_S);
            config->http_host = value->s;
            break;
        case CI_MYMPD_URI:
            assert(value->t == CIT_S);
            config->mympd_uri = value->s;
            break;
        case CI_PIN_HASH:
            assert(value->t == CIT_S);
            config->pin_hash = value->s;
            break;
        case CI_SCRIPTACL:
            assert(value->t == CIT_S);
            config->scriptacl = value->s;
            break;
        case CI_SCRIPTS_EXTERNAL:
            assert(value->t == CIT_B);
            config->scripts_external = value->b;
            break;
        case CI_SSL_CERT:
            assert(value->t == CIT_S);
            config->ssl_cert = value->s;
            break;
        case CI_SSL_KEY:
            assert(value->t == CIT_S);
            config->ssl_key = value->s;
            break;
        case CI_SSL_SAN:
            assert(value->t == CIT_S);
            config->ssl_san = value->s;
            break;
        case CI_CA_CERT_STORE:
            assert(value->t == CIT_S);
            config->ca_cert_store = value->s;
            break;
        case CI_CUSTOM_CSS:
            assert(value->t == CIT_S);
            config->custom_css = value->s;
            break;
        case CI_CUSTOM_JS:
            assert(value->t == CIT_S);
            config->custom_js = value->s;
            break;
        case CI_COUNT:
            assert(NULL);
            // This should not occur
    }
}

/**
 * Reads the myMPD configuration from environment or files
 * This function is used after reading command line arguments.
 * @param config pointer to config struct
 * @return true on success, else false
 */
bool mympd_config_read(struct t_config *config) {
    const char *http_host;
    #ifdef MYMPD_ENABLE_IPV6
        if (get_ipv6_support() == true) {
            http_host = CFG_MYMPD_HTTP_HOST_IPV6;
        }
        else {
            http_host = CFG_MYMPD_HTTP_HOST_IPV4;
        }
    #else
        http_host = CFG_MYMPD_HTTP_HOST_IPV4;
    #endif

    sds env_var = sdsempty();
    sds cfg_file = sdsempty();
    for (enum config_item i = 0; i < CI_COUNT; i++) {
        env_var = sdscatfmt(env_var, "MYMPD_%s", config_default[i].file);
        sdstoupper(env_var);
        cfg_file = sdscatfmt(cfg_file, "%s/%s/%s", config->workdir, DIR_WORK_CONFIG, config_default[i].file);
        struct t_config_value value;
        value.t = config_default[i].value.t;
        bool rc;
        switch (config_default[i].value.t) {
            case CIT_B:
                value.b = getenv_bool(env_var, config_default[i].value.b, &rc);
                if (rc == true) {
                    // Overwrite config file
                    try_rm_file(cfg_file);
                    value.b = state_file_rw_bool(config->workdir, DIR_WORK_CONFIG, config_default[i].file, value.b, true);
                }
                else {
                    value.b = state_file_rw_bool(config->workdir, DIR_WORK_CONFIG, config_default[i].file, config_default[i].value.b, true);
                }
                break;
            case CIT_I:
                value.i = getenv_int(env_var, config_default[i].value.i, config_default[i].min, config_default[i].max, &rc);
                if (rc == true) {
                    // Overwrite config file
                    try_rm_file(cfg_file);
                    value.i = state_file_rw_int(config->workdir, DIR_WORK_CONFIG, config_default[i].file, value.i, config_default[i].min, config_default[i].max, true);
                }
                else {
                    value.i = state_file_rw_int(config->workdir, DIR_WORK_CONFIG, config_default[i].file, config_default[i].value.i, config_default[i].min, config_default[i].max, true);
                }
                break;
            case CIT_S: {
                const char *def;
                switch (i) {
                    case CI_HTTP_HOST: def = http_host; break;
                    case CI_CA_CERT_STORE: def = find_ca_cert_store(false); break;
                    default: def = config_default[i].value.s;
                }
                value.s = getenv_string(env_var, def, config_default[i].vcb, &rc);
                if (rc == true) {
                    // Overwrite config file
                    try_rm_file(cfg_file);
                    value.s = state_file_rw_string_sds(config->workdir, DIR_WORK_CONFIG, config_default[i].file, value.s, config_default[i].vcb, true);
                }
                else {
                    FREE_SDS(value.s);
                    value.s = state_file_rw_string(config->workdir, DIR_WORK_CONFIG, config_default[i].file, def, config_default[i].vcb, true);
                }
                break;
            }
        }
        set_config(config, i, &value);
        sdsclear(env_var);
        sdsclear(cfg_file);
    }
    FREE_SDS(env_var);
    FREE_SDS(cfg_file);

    // Handle custom certificates
    if (config->custom_cert == false) {
        FREE_SDS(config->ssl_cert);
        FREE_SDS(config->ssl_key);
        config->ssl_cert = sdscatfmt(sdsempty(), "%S/%s/server.pem", config->workdir, DIR_WORK_SSL);
        config->ssl_key = sdscatfmt(sdsempty(), "%S/%s/server.key", config->workdir, DIR_WORK_SSL);
    }

    // Parse album configuration
    config->albums.mode = parse_album_mode(config->album_mode);
    config->albums.group_tag = mpd_tag_name_iparse(config->album_group_tag);
    FREE_SDS(config->album_mode);
    FREE_SDS(config->album_group_tag);

    return true;
}

/**
 * Removes all files from the config directory
 * @param config pointer to config struct
 * @return bool true on success, else false
 */
bool mympd_config_rm(struct t_config *config) {
    errno = 0;
    sds filepath = sdscatfmt(sdsempty(), "%S/%s", config->workdir, DIR_WORK_CONFIG);
    DIR *config_dir = opendir(filepath);
    if (config_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Error opening directory \"%s\"", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(filepath);
        return false;
    }

    struct dirent *next_file;
    while ((next_file = readdir(config_dir)) != NULL ) {
        if (next_file->d_type != DT_REG) {
            continue;
        }
        sdsclear(filepath);
        filepath = sdscatfmt(filepath, "%S/%s/%s", config->workdir, DIR_WORK_CONFIG, next_file->d_name);
        rm_file(filepath);
    }
    closedir(config_dir);
    FREE_SDS(filepath);
    return true;
}

/**
 * Reads the ca certificates
 * @param config Pointer to central config
 * @return true on success or disabled certificate checking, else false
 */
bool mympd_read_ca_certificates(struct t_config *config) {
    if (config->cert_check == false) {
        return true;
    }
    if (sdslen(config->ca_cert_store) == 0) {
        MYMPD_LOG_EMERG(NULL, "System certificate store not found.");
        return false;
    }
    MYMPD_LOG_INFO(NULL, "Reading ca certificates from %s", config->ca_cert_store);
    config->ca_certs = sdsempty();
    int nread;
    config->ca_certs = sds_getfile(config->ca_certs, config->ca_cert_store, CACERT_STORE_SIZE_MAX, false, true, &nread);
    if (nread == FILE_TO_BIG) {
        MYMPD_LOG_EMERG(NULL, "System certificate store too big.");
        return false;
    }
    if (nread <= FILE_IS_EMPTY) {
        MYMPD_LOG_EMERG(NULL, "System certificate store not found or empty.");
        return false;
    }
    return true;
}

/**
 * Dumps the default myMPD configuration
 */
void mympd_config_dump_default(void) {
    printf("Default configuration:\n");
    for (int i = 0; i < CI_COUNT; i++) {
        switch (config_default[i].value.t) {
            case CIT_B:
                printf("    %s: %s\n", config_default[i].file, (config_default[i].value.b == true ? "true" : "false"));
                break;
            case CIT_I:
                printf("    %s: %d\n", config_default[i].file, config_default[i].value.i);
                break;
            case CIT_S:
                if (i == CI_CA_CERT_STORE) {
                    const char *ca_cert_store = find_ca_cert_store(true);
                    if (ca_cert_store == NULL) {
                        ca_cert_store = "";
                    }
                    printf("    %s: %s\n", config_default[i].file, ca_cert_store);
                }
                else {
                    printf("    %s: %s\n", config_default[i].file, config_default[i].value.s);
                }
                break;
        }
    }
}
