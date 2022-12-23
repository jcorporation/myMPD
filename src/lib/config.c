/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/config.h"

#include "src/lib/env.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"
#include "src/lib/validate.h"

#include <inttypes.h>
#include <string.h>
#include <time.h>

/**
 * Private declarations
 */

static sds startup_getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool first_startup);
static int startup_getenv_int(const char *env_var, int default_value, int min, int max, bool first_startup);
static bool startup_getenv_bool(const char *env_var, bool default_value, bool first_startup);

/**
 * Public functions
 */

/**
 * Frees the config struct
 * @param config pointer to config struct
 */
void *mympd_config_free(struct t_config *config) {
    FREE_SDS(config->http_host);
    FREE_SDS(config->ssl_cert);
    FREE_SDS(config->ssl_key);
    FREE_SDS(config->ssl_san);
    FREE_SDS(config->acl);
    FREE_SDS(config->scriptacl);
    FREE_SDS(config->lualibs);
    FREE_SDS(config->pin_hash);
    FREE_SDS(config->user);
    FREE_SDS(config->workdir);
    FREE_SDS(config->cachedir);
    FREE_PTR(config);
    return NULL;
}

/**
 * Sets the initial default values for config struct
 * This function is used before reading command line arguments
 * @param config pointer to config struct
 */
void mympd_config_defaults_initial(struct t_config *config) {
    //command line options
    config->user = sdsnew(CFG_MYMPD_USER);
    config->workdir = sdsnew(MYMPD_WORK_DIR);
    config->cachedir = sdsnew(MYMPD_CACHE_DIR);
    config->log_to_syslog = CFG_LOG_TO_SYSLOG;
    //not configurable
    config->startup_time = time(NULL);
    config->first_startup = false;
    config->bootstrap = false;
    //set all other sds strings to NULL
    config->http_host = NULL;
    config->http_port = CFG_MYMPD_HTTP_PORT;
    config->ssl = CFG_MYMPD_SSL;
    config->ssl_port = CFG_MYMPD_SSL_PORT;
    config->ssl_san = NULL;
    config->ssl_cert = NULL;
    config->ssl_key = NULL;
    config->acl = NULL;
    config->scriptacl = NULL;
    config->lualibs = NULL;
    config->pin_hash = NULL;
    config->covercache_keep_days = CFG_COVERCACHE_KEEP_DAYS;
    config->save_caches = true;
}

/**
 * Sets the default values for config struct
 * This function is used after reading command line arguments and
 * reads the environment variables.
 * Environment variables are only respected at first startup.
 * @param config pointer to config struct
 */
void mympd_config_defaults(struct t_config *config) {
    if (config->first_startup == true) {
        MYMPD_LOG_INFO("Reading environment variables");
    }
    //configurable with environment variables at first startup
    config->http_host = startup_getenv_string("MYMPD_HTTP_HOST", CFG_MYMPD_HTTP_HOST, vcb_isname, config->first_startup);
    config->http_port = startup_getenv_int("MYMPD_HTTP_PORT", CFG_MYMPD_HTTP_PORT, 0, MPD_PORT_MAX, config->first_startup);
    #ifdef MYMPD_ENABLE_SSL
        config->ssl = startup_getenv_bool("MYMPD_SSL", CFG_MYMPD_SSL, config->first_startup);
        config->ssl_port = startup_getenv_int("MYMPD_SSL_PORT", CFG_MYMPD_SSL_PORT, 0, MPD_PORT_MAX, config->first_startup);
        config->ssl_san = startup_getenv_string("MYMPD_SSL_SAN", CFG_MYMPD_SSL_SAN, vcb_isname, config->first_startup);
        config->custom_cert = startup_getenv_bool("MYMPD_CUSTOM_CERT", CFG_MYMPD_CUSTOM_CERT, config->first_startup);
        sds default_cert = sdscatfmt(sdsempty(), "%S/ssl/server.pem", config->workdir);
        sds default_key = sdscatfmt(sdsempty(), "%S/ssl/server.key", config->workdir);
        if (config->custom_cert == true) {
            config->ssl_cert = startup_getenv_string("MYMPD_SSL_CERT", default_cert, vcb_isfilepath, config->first_startup);
            config->ssl_key = startup_getenv_string("MYMPD_SSL_KEY", default_key, vcb_isfilepath, config->first_startup);
            FREE_SDS(default_cert);
            FREE_SDS(default_key);
        }
        else {
            config->ssl_cert = default_cert;
            config->ssl_key = default_key;
        }
    #else
        config->ssl = false;
        config->ssl_port = 0;
        config->ssl_san = sdsempty();
        config->custom_cert = sdsempty();
        config->ssl_cert = sdsempty();
        config->ssl_key = sdsempty();
    #endif
    config->acl = startup_getenv_string("MYMPD_ACL", CFG_MYMPD_ACL, vcb_isname, config->first_startup);
    config->scriptacl = startup_getenv_string("MYMPD_SCRIPTACL", CFG_MYMPD_SCRIPTACL, vcb_isname, config->first_startup);
    #ifdef MYMPD_ENABLE_LUA
        config->lualibs = startup_getenv_string("MYMPD_LUALIBS", CFG_MYMPD_LUALIBS, vcb_isalnum, config->first_startup);
    #else
        config->lualibs = sdsempty();
    #endif
    config->loglevel = getenv_int("MYMPD_LOGLEVEL", CFG_MYMPD_LOGLEVEL, LOGLEVEL_MIN, LOGLEVEL_MAX);
    config->pin_hash = sdsnew(CFG_MYMPD_PIN_HASH);
    config->covercache_keep_days = startup_getenv_int("MYMPD_COVERCACHE_KEEP_DAYS", CFG_COVERCACHE_KEEP_DAYS, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, config->first_startup);
    config->save_caches = startup_getenv_bool("MYMPD_SAVE_CACHES", CFG_MYMPD_SSL, config->save_caches);
}

/**
 * Reads or writes the config from the /var/lib/mympd/config directory
 * @param config pointer to config struct
 * @param write if true create the file if not exists
 */
bool mympd_config_rw(struct t_config *config, bool write) {
    config->http_host = state_file_rw_string_sds(config->workdir, "config", "http_host", config->http_host, vcb_isname, write);
    config->http_port = state_file_rw_int(config->workdir, "config", "http_port", config->http_port, 0, MPD_PORT_MAX, write);

    #ifdef MYMPD_ENABLE_SSL
        config->ssl = state_file_rw_bool(config->workdir, "config", "ssl", config->ssl, write);
        config->ssl_port = state_file_rw_int(config->workdir, "config", "ssl_port", config->ssl_port, 0, MPD_PORT_MAX, write);
        config->ssl_san = state_file_rw_string_sds(config->workdir, "config", "ssl_san", config->ssl_san, vcb_isname, write);
        config->custom_cert = state_file_rw_bool(config->workdir, "config", "custom_cert", config->custom_cert, write);
        if (config->custom_cert == true) {
            config->ssl_cert = state_file_rw_string_sds(config->workdir, "config", "ssl_cert", config->ssl_cert, vcb_isname, write);
            config->ssl_key = state_file_rw_string_sds(config->workdir, "config", "ssl_key", config->ssl_key, vcb_isname, write);
        }
        config->pin_hash = state_file_rw_string_sds(config->workdir, "config", "pin_hash", config->pin_hash, vcb_isname, write);
    #else
        MYMPD_LOG_NOTICE("OpenSSL is disabled, ignoring ssl and pin settings");
    #endif
    config->acl = state_file_rw_string_sds(config->workdir, "config", "acl", config->acl, vcb_isname, write);
    config->scriptacl = state_file_rw_string_sds(config->workdir, "config", "scriptacl", config->scriptacl, vcb_isname, write);
    #ifdef MYMPD_ENABLE_LUA
        config->lualibs = state_file_rw_string_sds(config->workdir, "config", "lualibs", config->lualibs, vcb_isname, write);
    #else
        MYMPD_LOG_NOTICE("Lua is disabled, ignoring lua settings");
    #endif
    config->covercache_keep_days = state_file_rw_int(config->workdir, "config", "covercache_keep_days", config->covercache_keep_days, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, write);
    config->loglevel = state_file_rw_int(config->workdir, "config", "loglevel", config->loglevel, LOGLEVEL_MIN, LOGLEVEL_MAX, write);
    config->save_caches = state_file_rw_bool(config->workdir, "config", "save_caches", config->save_caches, write);
    //overwrite configured loglevel
    config->loglevel = getenv_int("MYMPD_LOGLEVEL", config->loglevel, LOGLEVEL_MIN, LOGLEVEL_MAX);
    return true;
}

/**
 * Private functions
 */

/**
 * Gets an environment variable as sds string
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param vcb validation callback
 * @param first_startup true for first startup else false
 * @return environment variable as sds string
 */
static sds startup_getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool first_startup) {
    return first_startup == true 
        ? getenv_string(env_var, default_value, vcb)
        : sdsnew(default_value);
}

/**
 * Gets an environment variable as int
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param first_startup true for first startup else false
 * @return environment variable as integer
 */
static int startup_getenv_int(const char *env_var, int default_value, int min, int max, bool first_startup) {
    return first_startup == true
        ? getenv_int(env_var, default_value, min, max)
        : default_value;
}

/**
 * Gets an environment variable as bool
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param first_startup true for first startup else false
 * @return environment variable as bool
 */
static bool startup_getenv_bool(const char *env_var, bool default_value, bool first_startup) {
    return first_startup == true
        ? getenv_bool(env_var, default_value)
        : default_value;
}
