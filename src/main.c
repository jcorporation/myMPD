/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/cert.h"
#include "src/lib/config.h"
#include "src/lib/config_def.h"
#include "src/lib/env.h"
#include "src/lib/event.h"
#include "src/lib/filehandler.h"
#include "src/lib/handle_options.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/passwd.h"
#include "src/lib/sds_extras.h"
#include "src/mympd_api/mympd_api.h"
#include "src/web_server/web_server.h"

#ifdef MYMPD_ENABLE_LUA
    #include <lua.h>
#endif

#ifdef MYMPD_ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef MYMPD_ENABLE_FLAC
    #include <FLAC/export.h>
#endif

#include <grp.h>
#include <openssl/opensslv.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>

// sanitizers

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

//global variables
_Atomic int worker_threads;
_Atomic int http_script_threads;
//signal handler
sig_atomic_t s_signal_received;
//message queues
struct t_mympd_queue *web_server_queue;
struct t_mympd_queue *mympd_api_queue;
struct t_mympd_queue *mympd_script_thread_queue;

/**
 * Signal handler that stops myMPD on SIGTERM and SIGINT and saves
 * states on SIGHUP
 * @param sig_num the signal to handle
 */
static void mympd_signal_handler(int sig_num) {
    switch(sig_num) {
        case SIGTERM:
        case SIGINT: {
            MYMPD_LOG_NOTICE(NULL, "Signal \"%s\" received, exiting", (sig_num == SIGTERM ? "SIGTERM" : "SIGINT"));
            //Set loop end condition for threads
            s_signal_received = sig_num;
            //Wakeup queue loops
            pthread_cond_signal(&mympd_api_queue->wakeup);
            pthread_cond_signal(&mympd_script_thread_queue->wakeup);
            pthread_cond_signal(&web_server_queue->wakeup);
            event_eventfd_write(mympd_api_queue->event_fd);
            if (web_server_queue->mg_mgr != NULL) {
                mg_wakeup(web_server_queue->mg_mgr, web_server_queue->mg_conn_id, "", 0);
            }
            break;
        }
        case SIGHUP: {
            MYMPD_LOG_NOTICE(NULL, "Signal SIGHUP received, saving states");
            struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STATE_SAVE, NULL, MPD_PARTITION_DEFAULT);
            request->data = sdscatlen(request->data, "}}", 2);
            mympd_queue_push(mympd_api_queue, request, 0);
            break;
        }
        default: {
            //Other signals are not handled
        }
    }
}

/**
 * Sets the mympd_signal_handler for the given signal
 * @param sig_num signal to handle
 * @return true on success, else false
 */
static bool set_signal_handler(int sig_num) {
    struct sigaction sa;
    sa.sa_handler = mympd_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler
    if (sigaction(sig_num, &sa, NULL) == -1) {
        return false;
    }
    return true;
}

/**
 * Drops the privileges and sets the new groups.
 * Ensures that myMPD does not run as root.
 * @param username drop privileges to this username
 * @param startup_uid initial uid of myMPD process
 * @return true on success else false
 */
static bool drop_privileges(sds username, uid_t startup_uid) {
    if (startup_uid == 0 &&
        sdslen(username) > 0)
    {
        MYMPD_LOG_NOTICE(NULL, "Dropping privileges to user \"%s\"", username);
        //get passwd entry
        struct passwd pwd;
        if (get_passwd_entry(&pwd, username) == NULL) {
            MYMPD_LOG_ERROR(NULL, "User \"%s\" does not exist", username);
            return false;
        }
        errno = 0;
        if (setgroups(0, NULL) == -1 ||                 //purge supplementary groups
            initgroups(username, pwd.pw_gid) == -1 ||  //set new supplementary groups from target user
            setgid(pwd.pw_gid) == -1 ||                //change primary group to group of target user
            setuid(pwd.pw_uid) == -1)                  //change user
        {
            MYMPD_LOG_ERROR(NULL, "Dropping privileges failed");
            MYMPD_LOG_ERRNO(NULL, errno);
            return false;
        }
    }
    //check if not root
    if (getuid() == 0) {
        MYMPD_LOG_ERROR(NULL, "myMPD should not be run with root privileges");
        return false;
    }
    return true;
}

/**
 * Creates the working, cache and config directories.
 * Sets first_startup to true if the config directory is created.
 * This function is run before dropping privileges.
 * @param config pointer to config struct
 * @param startup_uid initial uid of myMPD process
 * @return true on success else false
 */
static bool check_dirs_initial(struct t_config *config, uid_t startup_uid) {
    bool chown_dirs = false;
    if (startup_uid == 0) {
        //check for user
        struct passwd pwd;
        if (get_passwd_entry(&pwd, config->user) == NULL) {
            MYMPD_LOG_ERROR(NULL, "User \"%s\" does not exist", config->user);
            return false;
        }
        chown_dirs = true;
    }

    //create the cache directory
    int testdir_rc = testdir("Cache dir", config->cachedir, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        return false;
    }
    //directory exists or was created; set user and group
    if (chown_dirs == true &&
        is_dir(config->cachedir) == true)
    {
        MYMPD_LOG_DEBUG(NULL, "Checking ownership of \"%s\"", config->cachedir);
        if (do_chown(config->cachedir, config->user) == false) {
            return false;
        }
    }

    //create the working directory
    testdir_rc = testdir("Work dir", config->workdir, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        //workdir is not accessible
        return false;
    }
    //directory exists or was created; set user and group
    if (chown_dirs == true &&
        is_dir(config->workdir) == true)
    {
        MYMPD_LOG_DEBUG(NULL, "Checking ownership of \"%s\"", config->workdir);
        if (do_chown(config->workdir, config->user) == false) {
            return false;
        }
    }

    //create the config directory
    sds testdirname = sdscatfmt(sdsempty(), "%S/%s", config->workdir, DIR_WORK_CONFIG);
    testdir_rc = testdir("Config dir", testdirname, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        FREE_SDS(testdirname);
        return false;
    }
    //directory exists or was created; set user and group
    if (chown_dirs == true) {
        MYMPD_LOG_DEBUG(NULL, "Checking ownership of \"%s\"", testdirname);
        if (do_chown(testdirname, config->user) == false) {
            return false;
        }
    }
    if (testdir_rc == DIR_CREATED) {
        MYMPD_LOG_INFO(NULL, "First startup of myMPD");
        config->first_startup = true;
    }
    FREE_SDS(testdirname);

    return true;
}

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
    {DIR_WORK_WEBRADIOS,        "Webradio dir"},
    {NULL, NULL}
};

/**
 * Subdirs in the cache directory to check
 */
static const struct t_subdirs_entry cachedir_subdirs[] = {
    {DIR_CACHE_COVER,         "Covercache dir"},
    {DIR_CACHE_WEBRADIODB,    "Webradiodb cache dir"},
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
    int testdir_rc;
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
    //set logging states
    thread_logname = sdsnew("mympd");
    log_on_tty = isatty(fileno(stdout));
    log_to_syslog = false;
    #ifdef MYMPD_DEBUG
        set_loglevel(LOG_DEBUG);
        MYMPD_LOG_NOTICE(NULL, "Debug build is running");
    #else
        set_loglevel(
            getenv_int("MYMPD_LOGLEVEL", CFG_MYMPD_LOGLEVEL, LOGLEVEL_MIN, LOGLEVEL_MAX)
        );
    #endif

    //set initial states
    worker_threads = 0;
    http_script_threads = 0;
    s_signal_received = 0;
    struct t_config *config = NULL;
    struct t_mg_user_data *mg_user_data = NULL;
    struct mg_mgr *mgr = NULL;
    int rc = EXIT_FAILURE;
    pthread_t web_server_thread = 0;
    pthread_t mympd_api_thread = 0;
    int thread_rc = 0;

    //goto root directory
    errno = 0;
    if (chdir("/") != 0) {
        MYMPD_LOG_ERROR(NULL, "Can not change directory to /");
        MYMPD_LOG_ERRNO(NULL, errno);
        goto cleanup;
    }

    //only owner should have rw access
    umask(0077);

    mympd_api_queue = mympd_queue_create("mympd_api_queue", QUEUE_TYPE_REQUEST, true);
    web_server_queue = mympd_queue_create("web_server_queue", QUEUE_TYPE_RESPONSE, false);
    mympd_script_thread_queue = mympd_queue_create("mympd_script_thread_queue", QUEUE_TYPE_RESPONSE, false);

    //mympd config defaults
    config = malloc_assert(sizeof(struct t_config));
    mympd_config_defaults_initial(config);

    //command line option
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

    //get startup uid
    uid_t startup_uid = getuid();
    MYMPD_LOG_DEBUG(NULL, "myMPD started as user id %u", startup_uid);

    //check initial directories
    if (check_dirs_initial(config, startup_uid) == false) {
        goto cleanup;
    }

    //go into workdir
    errno = 0;
    if (chdir(config->workdir) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can not change directory to \"%s\"", config->workdir);
        MYMPD_LOG_ERRNO(NULL, errno);
        goto cleanup;
    }

    //read the configuration from environment or set default values
    //environment values are only respected at first startup
    mympd_config_defaults(config);

    //bootstrap - write config files and exit
    if (config->bootstrap == true) {
        if (drop_privileges(config->user, startup_uid) == true &&
            mympd_config_rm(config) == true &&
            mympd_config_rw(config, true) == true &&
            create_certificates(config) == true)
        {
            printf("Created myMPD config\n");
            rc = EXIT_SUCCESS;
        }
        goto cleanup;
    }

    //tries to read the config from /var/lib/mympd/config folder
    //if this is not the first startup of myMPD
    //initial files are written later, after dropping privileges
    if (config->first_startup == false) {
        mympd_config_rw(config, false);
    }

    #ifdef MYMPD_ENABLE_IPV6
        if (sdslen(config->acl) > 0) {
            MYMPD_LOG_WARN(NULL, "No acl support for IPv6");
        }
    #endif

    //set loglevel
    #ifndef MYMPD_DEBUG
        set_loglevel(config->loglevel);
    #endif

    if (config->log_to_syslog == true) {
        openlog("mympd", LOG_CONS, LOG_DAEMON);
        log_to_syslog = true;
    }

    #ifdef MYMPD_ENABLE_ASAN
        MYMPD_LOG_NOTICE(NULL, "Running with address sanitizer");
    #endif
    #ifdef MYMPD_ENABLE_UBSAN
        MYMPD_LOG_NOTICE(NULL, "Running with undefined behavior sanitizer");
    #endif
    #ifdef MYMPD_ENABLE_TSAN
        MYMPD_LOG_NOTICE(NULL, "Running with thread sanitizer");
    #endif

    MYMPD_LOG_NOTICE(NULL, "Starting myMPD %s", MYMPD_VERSION);
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

    //set signal handler
    if (set_signal_handler(SIGTERM) == false ||
        set_signal_handler(SIGINT) == false ||
        set_signal_handler(SIGHUP) == false)
    {
        MYMPD_LOG_EMERG(NULL, "Could not set signal handler for SIGTERM, SIGINT and SIGUP");
        goto cleanup;
    }

    //set output buffers
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0 ||
        setvbuf(stderr, NULL, _IOLBF, 0) != 0)
    {
        MYMPD_LOG_EMERG(NULL, "Could not set stdout and stderr buffer");
        goto cleanup;
    }

    //init webserver
    mgr = malloc_assert(sizeof(struct mg_mgr));
    mg_user_data = malloc_assert(sizeof(struct t_mg_user_data));
    if (web_server_init(mgr, config, mg_user_data) == false) {
        goto cleanup;
    }

    //drop privileges
    if (drop_privileges(config->user, startup_uid) == false) {
        goto cleanup;
    }

    //saves the config to /var/lib/mympd/config folder
    //at first startup of myMPD or if version has changed
    if (config->first_startup == true ||
        mympd_version_check(config->workdir) == false)
    {
        MYMPD_LOG_INFO(NULL, "Writing configuration files");
        mympd_config_rw(config, true);
        MYMPD_LOG_INFO(NULL, "Setting myMPD version to %s", MYMPD_VERSION);
        mympd_version_set(config->workdir);
    }

    //check ssl certificates
    if (create_certificates(config) == false ||
        webserver_read_certs(mg_user_data, config) == false)
    {
        goto cleanup;
    }

    //check for required directories
    if (check_dirs(config) == false) {
        goto cleanup;
    }

    //Create working threads
    //mympd api
    MYMPD_LOG_NOTICE(NULL, "Starting mympd api thread");
    if ((thread_rc = pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config)) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can't create mympd api thread");
        MYMPD_LOG_ERRNO(NULL, thread_rc);
        mympd_api_thread = 0;
        s_signal_received = SIGTERM;
    }

    //webserver
    MYMPD_LOG_NOTICE(NULL, "Starting webserver thread");
    if ((thread_rc = pthread_create(&web_server_thread, NULL, web_server_loop, mgr)) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can't create webserver thread");
        MYMPD_LOG_ERRNO(NULL, thread_rc);
        web_server_thread = 0;
        s_signal_received = SIGTERM;
    }

    //Outsourced all work to separate threads, do nothing...
    MYMPD_LOG_NOTICE(NULL, "myMPD is ready");
    rc = EXIT_SUCCESS;

    //Try to cleanup all
    cleanup:

    //wait for threads
    if (web_server_thread > (pthread_t)0) {
        if ((thread_rc = pthread_join(web_server_thread, NULL)) != 0) {
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

    //free queues
    mympd_queue_free(web_server_queue);
    mympd_queue_free(mympd_api_queue);
    mympd_queue_free(mympd_script_thread_queue);

    //free config
    mympd_config_free(config);

    if (mgr != NULL) {
        web_server_free(mgr);
    }
    if (mg_user_data != NULL) {
        mg_user_data_free(mg_user_data);
    }
    if (rc == EXIT_SUCCESS) {
        printf("Exiting gracefully, thank you for using myMPD\n");
    }
    else {
        printf("Exiting erroneous, thank you for using myMPD\n");
    }

    FREE_SDS(thread_logname);
    return rc;
}
