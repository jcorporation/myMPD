/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"

#include "src/lib/cert.h"
#include "src/lib/config.h"
#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/handle_options.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/signal.h"
#include "src/lib/thread.h"
#include "src/mympd_api/mympd_api.h"
#include "src/scripts/scripts.h"
#include "src/webserver/mg_user_data.h"
#include "src/webserver/webserver.h"

#ifdef MYMPD_ENABLE_LUA
    #include <lua.h>
#endif

#ifdef MYMPD_ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef MYMPD_ENABLE_FLAC
    #include <FLAC/export.h>
#endif

#include <openssl/opensslv.h>
#include <pthread.h>

// Sanitizers

#ifdef MYMPD_ENABLE_ASAN
const char *__asan_default_options(void) {
    return "abort_on_error=1:fast_unwind_on_malloc=0:detect_stack_use_after_return=1";
}
#endif

#ifdef MYMPD_ENABLE_TSAN
const char *__asan_default_options(void) {
    return "abort_on_error=1";
}
#endif

#ifdef MYMPD_ENABLE_UBSAN
const char *__asan_default_options(void) {
    return "abort_on_error=1:print_stacktrace=1";
}
#endif

/**
 * Struct holding directory entry and description
 */
struct t_subdirs_entry {
    const char *dirname;     //!< directory name
    const char *description; //!< description of the directory
};

/**
 * Subdirs in the working directory to check
 */
static const struct t_subdirs_entry workdir_subdirs[] = {
    {DIR_WORK_CONFIG,           "Working dir"},
    {DIR_WORK_EMPTY,            "Empty dir"},
    {DIR_WORK_PICS,             "Pics dir"},
    {DIR_WORK_PICS_BACKGROUNDS, "Backgrounds dir"},
    {DIR_WORK_PICS_THUMBS,      "Thumbnails dir"},
    #ifdef MYMPD_ENABLE_LUA
    {DIR_WORK_SCRIPTS,          "Scripts dir"},
    #endif
    {DIR_WORK_SMARTPLS,         "Smartpls dir"},
    {DIR_WORK_STATE,            "State dir"},
    {DIR_WORK_STATE_DEFAULT,    "Default partition dir"},
    {DIR_WORK_TAGS,             "Tags cache dir"},
    {NULL, NULL}
};

/**
 * Subdirs in the cache directory to check
 */
static const struct t_subdirs_entry cachedir_subdirs[] = {
    {DIR_CACHE_COVER,         "Cover cache dir"},
    {DIR_CACHE_HTTP,        "HTTP cache dir"},
    {DIR_CACHE_LYRICS,        "Lyrics cache dir"},
    {DIR_CACHE_MISC,          "Misc cache dir"},
    {DIR_CACHE_THUMBS, "Thumbs cache dir"},
    {NULL, NULL}
};

/**
 * Small helper function to concatenate the dirname and call testdir
 * @param parent parent directory
 * @param subdir directory to check
 * @param description descriptive name for logging
 * @return enum testdir_status
 */
static bool check_dir(const char *parent, const char *subdir, const char *description) {
    sds testdirname = sdscatfmt(sdsempty(), "%s/%s", parent, subdir);
    int rc = testdir(description, testdirname, true, false);
    FREE_SDS(testdirname);
    return rc;
}

/**
 * Creates all the subdirs in the working and cache directories
 * @param config pointer to config struct
 * @return true on success, else error
 */
static bool check_dirs(struct t_config *config) {
    //create the cache directory
    int testdir_rc = testdir("Cache dir", config->cachedir, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        return false;
    }

    //create the working directory
    testdir_rc = testdir("Work dir", config->workdir, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        //workdir is not accessible
        return false;
    }

    #ifndef MYMPD_EMBEDDED_ASSETS
        //release uses empty document root and delivers embedded files
        testdir_rc = testdir("Document root", MYMPD_DOC_ROOT, false, false);
        if (testdir_rc != DIR_EXISTS) {
            return false;
        }
    #endif

    const struct t_subdirs_entry *p = NULL;
    //workdir
    for (p = workdir_subdirs; p->dirname != NULL; p++) {
        testdir_rc = check_dir(config->workdir, p->dirname, p->description);
        if (testdir_rc == DIR_CREATE_FAILED) {
            return false;
        }
    }
    //cachedir
    for (p = cachedir_subdirs; p->dirname != NULL; p++) {
        testdir_rc = check_dir(config->cachedir, p->dirname, p->description);
        if (testdir_rc == DIR_CREATE_FAILED) {
            return false;
        }
    }
    return true;
}

/**
 * Creates SSL certificates if enabled and required
 * @param config pointer to config struct
 * @return true on success, else false
 */
static bool create_certificates(struct t_config *config) {
    if (config->ssl == true &&
        config->custom_cert == false &&
        certificates_check(config->workdir, config->ssl_san) == false)
    {
        return false;
    }
    return true;
}

/**
 * The main function of myMPD. It handles startup, starts the threads
 * and cleans up on exit. It stays in foreground.
 * @param argc number of command line arguments
 * @param argv char array of the command line arguments
 * @return 0 on success
 */
int main(int argc, char **argv) {
    // Set initial states
    thread_logname = sdsnew("mympd");
    log_init();

    mympd_worker_threads = 0;
    #ifdef MYMPD_ENABLE_LUA
        script_worker_threads = 0;
    #endif
    s_signal_received = 0;
    struct t_config *config = NULL;
    struct t_mg_user_data *mg_user_data = NULL;
    struct mg_mgr *mgr = NULL;
    int rc = EXIT_FAILURE;
    pthread_t webserver_thread = 0;
    pthread_t mympd_api_thread = 0;
    #ifdef MYMPD_ENABLE_LUA
        pthread_t script_thread = 0;
    #endif
    int thread_rc = 0;

    // Only owner should have rw access
    umask(0077);

    // Message queues
    mympd_api_queue = mympd_queue_create("mympd_api_queue", QUEUE_TYPE_REQUEST, true);
    webserver_queue = mympd_queue_create("webserver_queue", QUEUE_TYPE_RESPONSE, false);
    #ifdef MYMPD_ENABLE_LUA
        script_queue = mympd_queue_create("script_queue", QUEUE_TYPE_REQUEST, false);
        script_worker_queue = mympd_queue_create("script_worker_queue", QUEUE_TYPE_RESPONSE, false);
    #endif

    // myMPD configuration defaults
    config = malloc_assert(sizeof(struct t_config));
    mympd_config_defaults_initial(config);

    // Parse command line options
    enum handle_options_rc options_rc = handle_options(config, argc, argv);
    switch(options_rc) {
        case OPTIONS_RC_INVALID:
            //invalid option or error
            loglevel = LOG_ERR;
            goto cleanup;
        case OPTIONS_RC_EXIT:
            //valid option and exit
            loglevel = LOG_ERR;
            rc = EXIT_SUCCESS;
            goto cleanup;
        case OPTIONS_RC_OK:
            //continue
            break;
    }

    if (config->log_to_syslog == true &&
        log_type != LOG_TO_SYSTEMD)
    {
        openlog("mympd", LOG_CONS, LOG_DAEMON);
        log_type = LOG_TO_SYSLOG;
    }

    // Check initial directories
    if (check_dirs(config) == false) {
        goto cleanup;
    }

    // Go into workdir
    errno = 0;
    if (chdir(config->workdir) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can not change directory to \"%s\"", config->workdir);
        MYMPD_LOG_ERRNO(NULL, errno);
        goto cleanup;
    }

    // Bootstrap option: Write config files and exit
    if (config->bootstrap == true) {
        if (mympd_config_rm(config) == true &&
            mympd_config_read(config) == true &&
            create_certificates(config) == true)
        {
            MYMPD_LOG_NOTICE(NULL, "Created myMPD config\n");
            rc = EXIT_SUCCESS;
        }
        goto cleanup;
    }

    mympd_config_read(config);

    // Set loglevel
    #ifndef MYMPD_DEBUG
        set_loglevel(config->loglevel);
    #endif

    #ifdef MYMPD_ENABLE_IPV6
        if (sdslen(config->acl) > 0 &&
            config->scripts_external == true)
        {
            MYMPD_LOG_WARN(NULL, "No acl support for IPv6");
        }
    #endif

    // Startup notifications
    MYMPD_LOG_NOTICE(NULL, "Starting myMPD %s", MYMPD_VERSION);
    #ifdef MYMPD_DEBUG
        MYMPD_LOG_WARN(NULL, "Debug build is running");
    #endif
    #ifdef MYMPD_ENABLE_ASAN
        MYMPD_LOG_NOTICE(NULL, "Running with address sanitizer");
    #endif
    #ifdef MYMPD_ENABLE_UBSAN
        MYMPD_LOG_NOTICE(NULL, "Running with undefined behavior sanitizer");
    #endif
    #ifdef MYMPD_ENABLE_TSAN
        MYMPD_LOG_NOTICE(NULL, "Running with thread sanitizer");
    #endif
    MYMPD_LOG_INFO(NULL, "Libmympdclient %i.%i.%i based on libmpdclient %i.%i.%i",
            LIBMYMPDCLIENT_MAJOR_VERSION, LIBMYMPDCLIENT_MINOR_VERSION, LIBMYMPDCLIENT_PATCH_VERSION,
            LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
    MYMPD_LOG_INFO(NULL, "Mongoose %s", MG_VERSION);
    MYMPD_LOG_INFO(NULL, "%s", OPENSSL_VERSION_TEXT);
    #ifdef MYMPD_ENABLE_LUA
        MYMPD_LOG_INFO(NULL, "%s", LUA_RELEASE);
    #endif
    #ifdef MYMPD_ENABLE_LIBID3TAG
        MYMPD_LOG_INFO(NULL, "Libid3tag %s", ID3_VERSION);
    #endif
    #ifdef MYMPD_ENABLE_FLAC
        MYMPD_LOG_INFO(NULL, "FLAC %d.%d.%d", FLAC_API_VERSION_CURRENT, FLAC_API_VERSION_REVISION, FLAC_API_VERSION_AGE);
    #endif
    #ifdef MYMPD_ENABLE_EXPERIMENTAL
        MYMPD_LOG_INFO(NULL, "Experimental features are enabled");
    #endif

    // Set signal handler
    if (set_signal_handler(SIGTERM) == false ||
        set_signal_handler(SIGINT) == false ||
        set_signal_handler(SIGHUP) == false)
    {
        MYMPD_LOG_EMERG(NULL, "Could not set signal handler for SIGTERM, SIGINT and SIGUP");
        goto cleanup;
    }

    // Set output buffers
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0 ||
        setvbuf(stderr, NULL, _IOLBF, 0) != 0)
    {
        MYMPD_LOG_EMERG(NULL, "Could not set stdout and stderr buffer");
        goto cleanup;
    }

    // Init webserver
    mgr = malloc_assert(sizeof(struct mg_mgr));
    mg_user_data = malloc_assert(sizeof(struct t_mg_user_data));
    if (mympd_read_ca_certificates(config) == false ||
        create_certificates(config) == false ||
        webserver_init(mgr, config, mg_user_data) == false)
    {
        goto cleanup;
    }

    // Create working threads
    // mympd api
    MYMPD_LOG_NOTICE(NULL, "Starting mympd api thread");
    if ((thread_rc = pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config)) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can't create mympd api thread");
        MYMPD_LOG_ERRNO(NULL, thread_rc);
        mympd_api_thread = 0;
        s_signal_received = SIGTERM;
    }

    #ifdef MYMPD_ENABLE_LUA
        // scripts
        MYMPD_LOG_NOTICE(NULL, "Starting script thread");
        if ((thread_rc = pthread_create(&script_thread, NULL, scripts_loop, config)) != 0) {
            MYMPD_LOG_ERROR(NULL, "Can't create script thread");
            MYMPD_LOG_ERRNO(NULL, thread_rc);
            webserver_thread = 0;
            s_signal_received = SIGTERM;
        }
    #endif

    // webserver
    MYMPD_LOG_NOTICE(NULL, "Starting webserver thread");
    if ((thread_rc = pthread_create(&webserver_thread, NULL, webserver_loop, mgr)) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can't create webserver thread");
        MYMPD_LOG_ERRNO(NULL, thread_rc);
        webserver_thread = 0;
        s_signal_received = SIGTERM;
    }

    // Outsourced all work to separate threads, do nothing...
    MYMPD_LOG_NOTICE(NULL, "myMPD is ready");
    rc = EXIT_SUCCESS;

    // Try to cleanup all
    cleanup:

    // Wait for threads
    if (webserver_thread > (pthread_t)0) {
        if ((thread_rc = pthread_join(webserver_thread, NULL)) != 0) {
            MYMPD_LOG_ERROR(NULL, "Error stopping webserver thread");
            MYMPD_LOG_ERRNO(NULL, thread_rc);
        }
        else {
            MYMPD_LOG_NOTICE(NULL, "Finished web server thread");
        }
    }
    if (mympd_api_thread > (pthread_t)0) {
        if ((thread_rc = pthread_join(mympd_api_thread, NULL)) != 0) {
            MYMPD_LOG_ERROR(NULL, "Error stopping mympd api thread");
            MYMPD_LOG_ERRNO(NULL, thread_rc);
        }
        else {
            MYMPD_LOG_NOTICE(NULL, "Finished mympd api thread");
        }
    }
    #ifdef MYMPD_ENABLE_LUA
        if (script_thread > (pthread_t)0) {
            if ((thread_rc = pthread_join(script_thread, NULL)) != 0) {
                MYMPD_LOG_ERROR(NULL, "Error stopping script thread");
                MYMPD_LOG_ERRNO(NULL, thread_rc);
            }
            else {
                MYMPD_LOG_NOTICE(NULL, "Finished script thread");
            }
        }
    #endif

    // Free queues
    mympd_queue_free(webserver_queue);
    mympd_queue_free(mympd_api_queue);
    #ifdef MYMPD_ENABLE_LUA
        mympd_queue_free(script_queue);
        mympd_queue_free(script_worker_queue);
    #endif

    // Free config
    mympd_config_free(config);

    if (mgr != NULL) {
        webserver_free(mgr);
    }
    if (mg_user_data != NULL) {
        mg_user_data_free(mg_user_data);
    }
    if (options_rc == OPTIONS_RC_OK) {
        if (rc == EXIT_SUCCESS) {
            MYMPD_LOG_NOTICE(NULL, "Exiting gracefully, thank you for using myMPD");
        }
        else {
            MYMPD_LOG_NOTICE(NULL, "Exiting erroneous, thank you for using myMPD");
        }
    }

    FREE_SDS(thread_logname);
    return rc;
}
