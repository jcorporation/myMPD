/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "config.h"

#include "log.h"
#include "mem.h"
#include "sds_extras.h"
#include "state_files.h"
#include "utility.h"
#include "validate.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//private functions
static const char *mympd_getenv(const char *env_var, bool first_startup);
static sds mympd_getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool first_startup);
static int mympd_getenv_int(const char *env_var, int default_value, int min, int max, bool first_startup);

#ifdef ENABLE_SSL
    static bool mympd_getenv_bool(const char *env_var, bool default_value, bool first_startup);
#endif

//public functions

/**
 * Frees the config struct
 * @param config pointer to config struct
 */
void *mympd_free_config(struct t_config *config) {
    FREE_SDS(config->http_host);
    FREE_SDS(config->http_port);
    #ifdef ENABLE_SSL
        FREE_SDS(config->ssl_port);
        FREE_SDS(config->ssl_cert);
        FREE_SDS(config->ssl_key);
        FREE_SDS(config->ssl_san);
    #endif
    FREE_SDS(config->acl);
    FREE_SDS(config->scriptacl);
    #ifdef ENABLE_LUA
        FREE_SDS(config->lualibs);
    #endif
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
    config->workdir = sdsnew(VARLIB_PATH);
    config->cachedir = sdsnew(VARCACHE_PATH);
    config->log_to_syslog = CFG_LOG_TO_SYSLOG;
    //not configureable
    config->startup_time = time(NULL);
    config->first_startup = false;
    config->bootstrap = false;
    //set all other sds strings to NULL
    config->http_host = NULL;
    config->http_port = NULL;
    #ifdef ENABLE_SSL
        config->ssl_port = NULL;
        config->ssl_san = NULL;
        config->ssl_cert = NULL;
        config->ssl_key = NULL;
    #endif
    config->acl = NULL;
    config->scriptacl = NULL;
    #ifdef ENABLE_LUA
        config->lualibs = NULL;
    #endif
    config->pin_hash = NULL;
    config->covercache_keep_days = CFG_COVERCACHE_KEEP_DAYS;
}

/**
 * Sets the default values for config struct
 * This function is used after reading command line arguments and
 * reads the environment variables
 * @param config pointer to config struct
 */
void mympd_config_defaults(struct t_config *config) {
    if (config->first_startup == true) {
        MYMPD_LOG_INFO("Reading environment variables");
    }
    //configureable with environment variables at first startup
    config->http_host = mympd_getenv_string("MYMPD_HTTP_HOST", CFG_MYMPD_HTTP_HOST, vcb_isname, config->first_startup);
    config->http_port = mympd_getenv_string("MYMPD_HTTP_PORT", CFG_MYMPD_HTTP_PORT, vcb_isdigit, config->first_startup);
    #ifdef ENABLE_SSL
        config->ssl = mympd_getenv_bool("MYMPD_SSL", CFG_MYMPD_SSL, config->first_startup);
        config->ssl_port = mympd_getenv_string("MYMPD_SSL_PORT", CFG_MYMPD_SSL_PORT, vcb_isdigit, config->first_startup);
        config->ssl_san = mympd_getenv_string("MYMPD_SSL_SAN", CFG_MYMPD_SSL_SAN, vcb_isname, config->first_startup);
        config->custom_cert = mympd_getenv_bool("MYMPD_CUSTOM_CERT", CFG_MYMPD_CUSTOM_CERT, config->first_startup);
        sds default_cert = sdscatfmt(sdsempty(), "%S/ssl/server.pem", config->workdir);
        sds default_key = sdscatfmt(sdsempty(), "%S/ssl/server.key", config->workdir);
        if (config->custom_cert == true) {
            config->ssl_cert = mympd_getenv_string("MYMPD_SSL_CERT", default_cert, vcb_isfilepath, config->first_startup);
            config->ssl_key = mympd_getenv_string("MYMPD_SSL_KEY", default_key, vcb_isfilepath, config->first_startup);
            FREE_SDS(default_cert);
            FREE_SDS(default_key);
        }
        else {
            config->ssl_cert = default_cert;
            config->ssl_key = default_key;
        }
    #endif
    config->acl = mympd_getenv_string("MYMPD_ACL", CFG_MYMPD_ACL, vcb_isname, config->first_startup);
    config->scriptacl = mympd_getenv_string("MYMPD_SCRIPTACL", CFG_MYMPD_SCRIPTACL, vcb_isname, config->first_startup);
    #ifdef ENABLE_LUA
        config->lualibs = mympd_getenv_string("MYMPD_LUALIBS", CFG_MYMPD_LUALIBS, vcb_isalnum, config->first_startup);
    #endif
    config->loglevel = CFG_MYMPD_LOGLEVEL;
    config->pin_hash = sdsnew(CFG_MYMPD_PIN_HASH);
    config->covercache_keep_days = mympd_getenv_int("MYMPD_COVERCACHE_KEEP_DAYS", CFG_COVERCACHE_KEEP_DAYS, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, config->first_startup);
}

/**
 * Reads or writes the config from the /var/lib/mympd/config directory
 * @param config pointer to config struct
 */
bool mympd_read_config(struct t_config *config) {
    config->http_host = state_file_rw_string_sds(config->workdir, "config", "http_host", config->http_host, vcb_isname, false);
    config->http_port = state_file_rw_string_sds(config->workdir, "config", "http_port", config->http_port, vcb_isdigit, false);

    long http_port = (long)strtoimax(config->http_port, NULL, 10);
    if (http_port <= 0 || http_port > MPD_PORT_MAX) {
        MYMPD_LOG_WARN("Invalid http port, using default %s", CFG_MYMPD_HTTP_PORT);
        config->http_port = sds_replace(config->http_port, CFG_MYMPD_HTTP_PORT);
    }
    #ifdef ENABLE_SSL
        config->ssl = state_file_rw_bool(config->workdir, "config", "ssl", config->ssl, false);
        config->ssl_port = state_file_rw_string_sds(config->workdir, "config", "ssl_port", config->ssl_port, vcb_isdigit, false);
        config->ssl_san = state_file_rw_string_sds(config->workdir, "config", "ssl_san", config->ssl_san, vcb_isname, false);
        config->custom_cert = state_file_rw_bool(config->workdir, "config", "custom_cert", config->custom_cert, false);
        if (config->custom_cert == true) {
            config->ssl_cert = state_file_rw_string_sds(config->workdir, "config", "ssl_cert", config->ssl_cert, vcb_isname, false);
            config->ssl_key = state_file_rw_string_sds(config->workdir, "config", "ssl_key", config->ssl_key, vcb_isname, false);
        }
        config->pin_hash = state_file_rw_string_sds(config->workdir, "config", "pin_hash", config->pin_hash, vcb_isname, false);

        long ssl_port = (long)strtoimax(config->ssl_port, NULL, 10);
        if (ssl_port <= 0 || ssl_port > MPD_PORT_MAX) {
            MYMPD_LOG_WARN("Invalid ssl port, using default %s", CFG_MYMPD_SSL_PORT);
            config->ssl_port = sds_replace(config->ssl_port, CFG_MYMPD_SSL_PORT);
        }
    #else
        MYMPD_LOG_NOTICE("OpenSSL is disabled, ignoring ssl and pin settings");
    #endif
    config->acl = state_file_rw_string_sds(config->workdir, "config", "acl", config->acl, vcb_isname, false);
    config->scriptacl = state_file_rw_string_sds(config->workdir, "config", "scriptacl", config->scriptacl, vcb_isname, false);
    #ifdef ENABLE_LUA
        config->lualibs = state_file_rw_string_sds(config->workdir, "config", "lualibs", config->lualibs, vcb_isname, false);
    #endif
    config->covercache_keep_days = state_file_rw_int(config->workdir, "config", "covercache_keep_days", config->covercache_keep_days, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, false);
    config->loglevel = state_file_rw_int(config->workdir, "config", "loglevel", config->loglevel, LOGLEVEL_MIN, LOGLEVEL_MAX, false);
    //overwrite configured loglevel
    config->loglevel = mympd_getenv_int("MYMPD_LOGLEVEL", config->loglevel, LOGLEVEL_MIN, LOGLEVEL_MAX, true);
    return true;
}

//private functions

/**
 * Reads environment variables on first startup
 * @param env_var variable name to read
 * @param first_startup true for first startup else false
 * @return environment value or NULL
 */
static const char *mympd_getenv(const char *env_var, bool first_startup) {
    const char *env_value = getenv_check(env_var, 100);
    if (first_startup == true) {
        return env_value;
    }
    if (env_value != NULL) {
        MYMPD_LOG_INFO("Ignoring environment variable \"%s\" with value \"%s\"", env_var, env_value);
    }
    return NULL;
}

/**
 * Gets a environment variable as sds string
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param vcb validation callback
 * @param first_startup true for first startup else false
 * @return environment variable as sds string
 */
static sds mympd_getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    if (env_value == NULL) {
        return sdsnew(default_value);
    }
    sds value = sdsnew(env_value);
    if (vcb == NULL) {
        return value;
    }
    bool rc = vcb(value);
    if (rc == true) {
        return value;
    }
    return sdsnew(default_value);
}

/**
 * Gets a environment variable as int
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param min mimum value (including)
 * @param max maximum value (including)
 * @param first_startup true for first startup else false
 * @return environment variable as integer
 */
static int mympd_getenv_int(const char *env_var, int default_value, int min, int max, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    if (env_value == NULL) {
        return default_value;
    }
    int value = (int)strtoimax(env_value, NULL, 10);
    if (value >= min && value <= max) {
        return value;
    }
    MYMPD_LOG_WARN("Invalid value for \"%s\" using default", env_var);
    return default_value;
}

#ifdef ENABLE_SSL
/**
 * Gets a environment variable as bool
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param vcb validation callback
 * @param first_startup true for first startup else false
 * @return environment variable as bool
 */
static bool mympd_getenv_bool(const char *env_var, bool default_value, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    return env_value != NULL ? strcmp(env_value, "true") == 0 ? true : false
                             : default_value;
}
#endif
