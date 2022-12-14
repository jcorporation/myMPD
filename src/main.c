/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/config.h"
#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/handle_options.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/random.h"
#include "src/lib/sds_extras.h"
#include "src/lib/smartpls.h"
#include "src/mympd_api/mympd_api.h"
#include "src/web_server/web_server.h"

#ifdef MYMPD_ENABLE_SSL
    #include "src/lib/cert.h"
    #include <openssl/opensslv.h>
#endif

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
#include <pthread.h>
#include <pwd.h>
#include <signal.h>

#ifdef MYMPD_ENABLE_LIBASAN
const char *__asan_default_options(void) {
    return "detect_stack_use_after_return=true";
}
#endif

//global variables
_Atomic int worker_threads;
//signal handler
sig_atomic_t s_signal_received;
//message queues
struct t_mympd_queue *web_server_queue;
struct t_mympd_queue *mympd_api_queue;
struct t_mympd_queue *mympd_script_queue;

/**
 * Signal handler that stops myMPD on SIGTERM and SIGINT and saves
 * states on SIGHUP
 * @param sig_num the signal to handle
 */
static void mympd_signal_handler(int sig_num) {
    // Reinstantiate signal handler
    if (signal(sig_num, mympd_signal_handler) == SIG_ERR) {
        MYMPD_LOG_ERROR("Could not set signal handler for %d", sig_num);
    }

    switch(sig_num) {
        case SIGTERM:
        case SIGINT: {
            //Set loop end condition for threads
            s_signal_received = sig_num;
            //Wakeup queue loops
            pthread_cond_signal(&mympd_api_queue->wakeup);
            pthread_cond_signal(&mympd_script_queue->wakeup);
            pthread_cond_signal(&web_server_queue->wakeup);
            MYMPD_LOG_NOTICE("Signal \"%s\" received, exiting", (sig_num == SIGTERM ? "SIGTERM" : "SIGINT"));
            break;
        }
        case SIGHUP: {
            MYMPD_LOG_NOTICE("Signal SIGHUP received, saving states");
            struct t_work_request *request = create_request(-1, 0, INTERNAL_API_STATE_SAVE, NULL, MPD_PARTITION_DEFAULT);
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
 * Sets the owner of a file and group to the primary group of the user
 * @param file_path file to change ownership
 * @param username new owner username
 * @return true on success else false
 */
static bool do_chown(const char *file_path, const char *username) {
    errno = 0;
    struct passwd *pwd = getpwnam(username);
    if (pwd == NULL) {
        MYMPD_LOG_ERROR("User \"%s\" does not exist", username);
        MYMPD_LOG_ERRNO(errno);
        return false;
    }

    errno = 0;
    int rc = chown(file_path, pwd->pw_uid, pwd->pw_gid); /* Flawfinder: ignore */
    if (rc == -1) {
        MYMPD_LOG_ERROR("Can't chown \"%s\" to \"%s\"", file_path, username);
        MYMPD_LOG_ERRNO(errno);
        return false;
    }
    MYMPD_LOG_INFO("Changed ownership of \"%s\" to \"%s\"", file_path, username);
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
        MYMPD_LOG_NOTICE("Droping privileges to user \"%s\"", username);
        //get user
        errno = 0;
        struct passwd *pw = getpwnam(username);
        if (pw  == NULL) {
            MYMPD_LOG_ERROR("User \"%s\" does not exist", username);
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
        //purge supplementary groups
        errno = 0;
        if (setgroups(0, NULL) == -1) {
            MYMPD_LOG_ERROR("setgroups() failed");
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
        //set new supplementary groups from target user
        errno = 0;
        if (initgroups(username, pw->pw_gid) == -1) {
            MYMPD_LOG_ERROR("initgroups() failed");
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
        //change primary group to group of target user
        errno = 0;
        if (setgid(pw->pw_gid) == -1 ) {
            MYMPD_LOG_ERROR("setgid() failed");
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
        //change user
        errno = 0;
        if (setuid(pw->pw_uid) == -1) {
            MYMPD_LOG_ERROR("setuid() failed");
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
    }
    //check if not root
    if (getuid() == 0) {
        MYMPD_LOG_ERROR("myMPD should not be run with root privileges");
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
    //check user
    bool chown_dirs = false;
    if (startup_uid == 0) {
        errno = 0;
        struct passwd *pw = getpwnam(config->user);
        if (pw == NULL) {
            MYMPD_LOG_WARN("User \"%s\" does not exist", config->user);
        }
        else {
            chown_dirs = true;
        }
    }

    //create the cache directory
    int testdir_rc = testdir("Cache dir", config->cachedir, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        return false;
    }
    //directory exists or was created; set user and group
    if (chown_dirs == true) {
        MYMPD_LOG_DEBUG("Checking ownership of \"%s\"", config->cachedir);
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
    if (chown_dirs == true) {
        MYMPD_LOG_DEBUG("Checking ownership of \"%s\"", config->workdir);
        if (do_chown(config->workdir, config->user) == false) {
            return false;
        }
    }

    //create the config directory
    sds testdirname = sdscatfmt(sdsempty(), "%S/config", config->workdir);
    testdir_rc = testdir("Config dir", testdirname, true, false);
    if (testdir_rc == DIR_CREATE_FAILED) {
        FREE_SDS(testdirname);
        return false;
    }
    if (testdir_rc == DIR_CREATED) {
        MYMPD_LOG_INFO("First startup of myMPD");
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
    {"empty",            "Empty dir"},
    {"pics",             "Pics dir"},
    {"pics/backgrounds", "Backgrounds dir"},
    {"pics/thumbs",      "Thumbnails dir"},
    #ifdef MYMPD_ENABLE_LUA
    {"scripts",          "Scripts dir"},
    #endif
    {"smartpls",         "Smartpls dir"},
    {"state",            "State dir"},
    {"state/default",    "Default partition dir"},
    {"webradios",        "Webradio dir"},
    {NULL, NULL}
};

/**
 * Subdirs in the cache directory to check
 */
static const struct t_subdirs_entry cachedir_subdirs[] = {
    {"covercache", "Covercache dir"},
    {"webradiodb", "Webradiodb cache dir"},
    {"tags", "Tags cache dir"},
    {NULL, NULL}
};

/**
 * Small helper function to concatenate the dirname and call testdir
 * @param parent parent direcotory
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
 * @return true on success else error
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
 * The main function of myMPD. It handles startup, starts the threads
 * and cleans up on exit. It stays in foreground.
 * @param argc number of command line arguments
 * @param argv char array of the command line arguments
 * @return 0 on success
 */
int main(int argc, char **argv) {
    //set logging states
    thread_logname = sdsnew("mympd");
    log_on_tty = isatty(fileno(stdout)) ? true : false;
    log_to_syslog = false;
    #ifdef MYMPD_DEBUG
        set_loglevel(LOG_DEBUG);
    #else
        set_loglevel(LOG_NOTICE);
    #endif

    //set initital states
    worker_threads = 0;
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
        MYMPD_LOG_ERROR("Can not change directory to /");
        MYMPD_LOG_ERRNO(errno);
        goto cleanup;
    }

    //only user and group have rw access
    umask(0007); /* Flawfinder: ignore */

    mympd_api_queue = mympd_queue_create("mympd_api_queue", QUEUE_TYPE_REQUEST);
    web_server_queue = mympd_queue_create("web_server_queue", QUEUE_TYPE_RESPONSE);
    mympd_script_queue = mympd_queue_create("mympd_script_queue", QUEUE_TYPE_RESPONSE);

    //initialize random number generator
    tinymt32_init(&tinymt, (uint32_t)time(NULL));

    //mympd config defaults
    config = malloc_assert(sizeof(struct t_config));
    mympd_config_defaults_initial(config);

    //command line option
    int handle_options_rc = handle_options(config, argc, argv);
    switch(handle_options_rc) {
        case OPTIONS_RC_INVALID:
            //invalid option or error
            loglevel = LOG_ERR;
            goto cleanup;
        case OPTIONS_RC_EXIT:
            //valid option and exit
            loglevel = LOG_ERR;
            rc = EXIT_SUCCESS;
            goto cleanup;
    }

    //get startup uid
    uid_t startup_uid = getuid();
    MYMPD_LOG_DEBUG("myMPD started as user id %u", startup_uid);

    //check initial directories
    if (check_dirs_initial(config, startup_uid) == false) {
        goto cleanup;
    }

    //read configuration from environment or set default values
    mympd_config_defaults(config);

    //go into workdir
    errno = 0;
    if (chdir(config->workdir) != 0) {
        MYMPD_LOG_ERROR("Can not change directory to \"%s\"", config->workdir);
        MYMPD_LOG_ERRNO(errno);
        goto cleanup;
    }

    //reads the config from /var/lib/mympd/config folder or writes defaults
    mympd_read_config(config);

    #ifdef MYMPD_ENABLE_IPV6
        if (sdslen(config->acl) > 0) {
            MYMPD_LOG_WARN("No acl support for IPv6");
        }
    #endif

    //bootstrap
    if (config->bootstrap == true) {
        printf("Created myMPD config and exit\n");
        rc = EXIT_SUCCESS;
        goto cleanup;
    }

    //set loglevel
    #ifdef MYMPD_DEBUG
        MYMPD_LOG_NOTICE("Debug build is running");
        set_loglevel(LOG_DEBUG);
    #else
        set_loglevel(config->loglevel);
    #endif

    if (config->log_to_syslog == true) {
        openlog("mympd", LOG_CONS, LOG_DAEMON);
        log_to_syslog = true;
    }

    #ifdef MYMPD_ENABLE_LIBASAN
        MYMPD_LOG_NOTICE("Running with libasan memory checker");
    #endif

    MYMPD_LOG_NOTICE("Starting myMPD %s", MYMPD_VERSION);
    MYMPD_LOG_INFO("Libmympdclient %i.%i.%i based on libmpdclient %i.%i.%i",
            LIBMYMPDCLIENT_MAJOR_VERSION, LIBMYMPDCLIENT_MINOR_VERSION, LIBMYMPDCLIENT_PATCH_VERSION,
            LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
    MYMPD_LOG_INFO("Mongoose %s", MG_VERSION);
    #ifdef MYMPD_ENABLE_LUA
        MYMPD_LOG_INFO("%s", LUA_RELEASE);
    #endif
    #ifdef MYMPD_ENABLE_LIBID3TAG
        MYMPD_LOG_INFO("Libid3tag %s", ID3_VERSION);
    #endif
    #ifdef MYMPD_ENABLE_FLAC
        MYMPD_LOG_INFO("FLAC %d.%d.%d", FLAC_API_VERSION_CURRENT, FLAC_API_VERSION_REVISION, FLAC_API_VERSION_AGE);
    #endif
    #ifdef MYMPD_ENABLE_SSL
        MYMPD_LOG_INFO("%s", OPENSSL_VERSION_TEXT);
    #endif

    //set signal handler
    if (signal(SIGTERM, mympd_signal_handler) == SIG_ERR) {
        MYMPD_LOG_EMERG("Could not set signal handler for SIGTERM");
        goto cleanup;
    }
    if (signal(SIGINT, mympd_signal_handler) == SIG_ERR) {
        MYMPD_LOG_EMERG("Could not set signal handler for SIGINT");
        goto cleanup;
    }
    if (signal(SIGHUP, mympd_signal_handler) == SIG_ERR) {
        MYMPD_LOG_EMERG("Could not set signal handler for SIGHUP");
        goto cleanup;
    }

    //set output buffers
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0) {
        MYMPD_LOG_EMERG("Could not set stdout buffer");
        goto cleanup;
    }
    if (setvbuf(stderr, NULL, _IOLBF, 0) != 0) {
        MYMPD_LOG_EMERG("Could not set stderr buffer");
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

    //check for ssl certificates
    #ifdef MYMPD_ENABLE_SSL
        if (config->ssl == true &&
            config->custom_cert == false &&
            certificates_check(config->workdir, config->ssl_san) == false)
        {
            goto cleanup;
        }
    #endif

    //check for needed directories
    if (check_dirs(config) == false) {
        goto cleanup;
    }

    //default smart playlists
    if (config->first_startup == true) {
        smartpls_default(config->workdir);
    }

    //Create working threads
    //mympd api
    MYMPD_LOG_NOTICE("Starting mympd api thread");
    if ((thread_rc = pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config)) != 0) {
        MYMPD_LOG_ERROR("Can't create mympd api thread: %s", strerror(thread_rc));
        mympd_api_thread = 0;
        s_signal_received = SIGTERM;
    }

    //webserver
    MYMPD_LOG_NOTICE("Starting webserver thread");
    if ((thread_rc = pthread_create(&web_server_thread, NULL, web_server_loop, mgr)) != 0) {
        MYMPD_LOG_ERROR("Can't create webserver thread: %s", strerror(thread_rc));
        web_server_thread = 0;
        s_signal_received = SIGTERM;
    }

    //Outsourced all work to separate threads, do nothing...
    MYMPD_LOG_NOTICE("myMPD is ready");
    rc = EXIT_SUCCESS;

    //Try to cleanup all
    cleanup:

    //wait for threads
    if (web_server_thread > (pthread_t)0) {
        if ((thread_rc = pthread_join(web_server_thread, NULL)) != 0) {
            MYMPD_LOG_ERROR("Error stopping webserver thread: %s", strerror(thread_rc));
        }
        else {
            MYMPD_LOG_NOTICE("Finished web server thread");
        }
    }
    if (mympd_api_thread > (pthread_t)0) {
        if ((thread_rc = pthread_join(mympd_api_thread, NULL)) != 0) {
            MYMPD_LOG_ERROR("Error stopping mympd api thread: %s", strerror(thread_rc));
        }
        else {
            MYMPD_LOG_NOTICE("Finished mympd api thread");
        }
    }

    //free queues
    mympd_queue_free(web_server_queue);
    mympd_queue_free(mympd_api_queue);
    mympd_queue_free(mympd_script_queue);

    //free config
    mympd_free_config(config);

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
