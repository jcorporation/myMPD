/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/rax/rax.h"
#include "../dist/src/sds/sds.h"
#include "handle_options.h"
#include "lib/api.h"
#include "lib/log.h"
#include "lib/mem.h"
#include "lib/random.h"
#include "lib/sds_extras.h"
#include "lib/utility.h"
#include "mympd_api.h"
#include "mympd_api/mympd_api_playlists.h"
#include "mympd_config.h"
#include "web_server.h"

#ifdef ENABLE_SSL
  #include "lib/cert.h"
#endif

#include <grp.h>
#include <mpd/client.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>

_Thread_local sds thread_logname;

static void mympd_signal_handler(int sig_num) {
    signal(sig_num, mympd_signal_handler);  // Reinstantiate signal handler
    if (sig_num == SIGTERM || sig_num == SIGINT) {
        //Set loop end condition for threads
        s_signal_received = sig_num;
        //Wakeup queue loops
        pthread_cond_signal(&mympd_api_queue->wakeup);
        pthread_cond_signal(&mympd_script_queue->wakeup);
        MYMPD_LOG_NOTICE("Signal %s received, exiting", (sig_num == SIGTERM ? "SIGTERM" : "SIGINT"));
    }
    else if (sig_num == SIGHUP) {
        MYMPD_LOG_NOTICE("Signal SIGHUP received, saving states");
        struct t_work_request *request = create_request(-1, 0, INTERNAL_API_STATE_SAVE, NULL);
        request->data = sdscat(request->data, "}}");
        mympd_queue_push(mympd_api_queue, request, 0);    
    }
}

static bool do_chown(const char *file_path, const char *user_name) {
    struct passwd *pwd = getpwnam(user_name);
    if (pwd == NULL) {
        MYMPD_LOG_ERROR("Can't get passwd entry for user \"%s\"", user_name);
        return false;
    }

    errno = 0;  
    int rc = chown(file_path, pwd->pw_uid, pwd->pw_gid); /* Flawfinder: ignore */
    //Originally owned by root
    if (rc == -1) {
        MYMPD_LOG_ERROR("Can't chown \"%s\" to \"%s\"", file_path, user_name);
        MYMPD_LOG_ERRNO(errno);
        return false;
    }
    return true;
}

static bool drop_privileges(struct t_config *config, uid_t startup_uid) {
    if (startup_uid == 0 && sdslen(config->user) > 0) {
        MYMPD_LOG_NOTICE("Droping privileges to %s", config->user);
        //get user
        struct passwd *pw;
        errno = 0;
        if ((pw = getpwnam(config->user)) == NULL) {
            MYMPD_LOG_ERROR("getpwnam() failed, unknown user \"%s\"", config->user);
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
        if (initgroups(config->user, pw->pw_gid) == -1) {
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

static bool check_dirs_initial(struct t_config *config, uid_t startup_uid) {
    int testdir_rc = testdir("Workdir", config->workdir, true);
    if (testdir_rc == DIR_CREATE_FAILED) {
        //workdir is not accessible
        return false;
    }
    
    if (testdir_rc == DIR_CREATED) {
        config->first_startup = true;
        //directory exists or was created; set user and group, if uid = 0
        if (startup_uid == 0 &&
            do_chown(config->workdir, config->user) == false)
        {
            return false;
        }
    }

    //config directory
    sds testdirname = sdscatfmt(sdsempty(), "%s/config", config->workdir);
    testdir_rc = testdir("Config dir", testdirname, true);
    if (testdir_rc == DIR_CREATE_FAILED) {
        FREE_SDS(testdirname);
        return false;
    }
    FREE_SDS(testdirname);
    return true;
}

struct t_workdir_subdirs_entry {
    const char *dirname;
    const char *description;
};

const struct t_workdir_subdirs_entry workdir_subdirs[] = {
    {"state",      "State dir"},
    {"smartpls",   "Smartpls dir"},
    {"pics",       "Pics dir"},
    {"empty",      "Empty dir"},
    {"covercache", "Covercache dir"},
    #ifdef ENABLE_LUA
    {"scripts",    "Scripts dir"},
    #endif
    {NULL, NULL}
};

static bool check_dirs(struct t_config *config) {
    int testdir_rc;
    #ifdef DEBUG
        //release uses empty document root and delivers embedded files
        testdir_rc = testdir("Document root", DOC_ROOT, false);
        if (testdir_rc != DIR_EXISTS) {
            return false;
        }
    #endif
    sds testdirname = sdsempty();
    const struct t_workdir_subdirs_entry *p = NULL;
    for (p = workdir_subdirs; p->dirname != NULL; p++) {
        testdirname = sdscatfmt(testdirname, "%s/%s", config->workdir, p->dirname);
        testdir_rc = testdir(p->description, testdirname, true);
        if (testdir_rc == DIR_CREATED) {
            if (strcmp(p->dirname, "smartpls") == 0) {
                //directory created, create default smart playlists
                mympd_api_smartpls_default(config);
            }
        }
        if (testdir_rc == DIR_CREATE_FAILED) {
            FREE_SDS(testdirname);
            return false;
        }
        sdsclear(testdirname);
    }
    FREE_SDS(testdirname);
    return true;
}

int main(int argc, char **argv) {
    thread_logname = sdsnew("mympd");
    log_on_tty = isatty(fileno(stdout)) ? true : false;
    log_to_syslog = false;

    worker_threads = 0; 
    s_signal_received = 0;
    bool init_config = false;
    bool init_webserver = false;
    bool init_mg_user_data = false;
    bool init_thread_webserver = false;
    bool init_thread_mympdapi = false;
    int rc = EXIT_FAILURE;
    #ifdef DEBUG
    set_loglevel(LOG_DEBUG);
    #else
    set_loglevel(LOG_NOTICE);
    #endif

    errno = 0;
    if (chdir("/") != 0) {
        MYMPD_LOG_ERROR("Can not change directory to /");
        MYMPD_LOG_ERRNO(errno);
        goto end;
    }
    //only user and group have rw access
    umask(0007); /* Flawfinder: ignore */

    //get startup uid
    uid_t startup_uid = getuid();
    
    mympd_api_queue = mympd_queue_create("mympd_api_queue");
    web_server_queue = mympd_queue_create("web_server_queue");
    mympd_script_queue = mympd_queue_create("mympd_script_queue");

    //create mg_user_data struct for web_server
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *)malloc_assert(sizeof(struct t_mg_user_data));

    //initialize random number generator
    tinymt32_init(&tinymt, (unsigned int)time(NULL));
    
    //mympd config defaults
    struct t_config *config = (struct t_config *)malloc_assert(sizeof(struct t_config));
    mympd_config_defaults_initial(config);
   
    //command line option
    if (handle_options(config, argc, argv) == false) {
        loglevel = LOG_ERR;
        goto cleanup;
    }

    //check initial directories
    if (check_dirs_initial(config, startup_uid) == false) {
        goto cleanup;
    }

    //go into workdir
    errno = 0;
    if (chdir(config->workdir) != 0) {
        MYMPD_LOG_ERROR("Can not change directory to \"%s\"", config->workdir);
        MYMPD_LOG_ERRNO(errno);
        goto cleanup;
    }

    //read configuration
    init_config = true;
    mympd_config_defaults(config);
    mympd_read_config(config);

    //bootstrap
    if (config->bootstrap == true) {
        printf("Created myMPD config and exit\n");
        rc = EXIT_SUCCESS;
        goto cleanup;
    }

    //set loglevel
    #ifdef DEBUG
        MYMPD_LOG_NOTICE("Debug build is running");
        set_loglevel(LOG_DEBUG);
    #else
        set_loglevel(config->loglevel);
    #endif

    if (config->log_to_syslog == true) {
        openlog("mympd", LOG_CONS, LOG_DAEMON);
        log_to_syslog = true;
    }

    MYMPD_LOG_NOTICE("Starting myMPD %s", MYMPD_VERSION);
    MYMPD_LOG_NOTICE("Libmympdclient %i.%i.%i based on libmpdclient %i.%i.%i", 
            LIBMYMPDCLIENT_MAJOR_VERSION, LIBMYMPDCLIENT_MINOR_VERSION, LIBMYMPDCLIENT_PATCH_VERSION,
            LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
    MYMPD_LOG_NOTICE("Mongoose %s", MG_VERSION);

    //set signal handler
    signal(SIGTERM, mympd_signal_handler);
    signal(SIGINT, mympd_signal_handler);
    signal(SIGHUP, mympd_signal_handler);

    //set output buffers
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //init webserver    
    struct mg_mgr mgr;
    init_mg_user_data = true;
    init_webserver = web_server_init(&mgr, config, mg_user_data);
    if (init_webserver == false) {
        goto cleanup;
    }

    //drop privileges
    if (drop_privileges(config, startup_uid) == false) {
        goto cleanup;
    }

    //check for ssl certificates
    #ifdef ENABLE_SSL
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

    //Create working threads
    pthread_t web_server_thread;
    pthread_t mympd_api_thread;
    //mympd api
    MYMPD_LOG_NOTICE("Starting mympd api thread");
    if (pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config) == 0) {
        init_thread_mympdapi = true;
    }
    else {
        MYMPD_LOG_ERROR("Can't create mympd_api thread");
        s_signal_received = SIGTERM;
    }
    //webserver
    MYMPD_LOG_NOTICE("Starting webserver thread");
    if (pthread_create(&web_server_thread, NULL, web_server_loop, &mgr) == 0) {
        init_thread_webserver = true;
    }
    else {
        MYMPD_LOG_ERROR("Can't create mympd_webserver thread");
        s_signal_received = SIGTERM;
    }

    //Outsourced all work to separate threads, do nothing...
    rc = EXIT_SUCCESS;

    //Try to cleanup all
    cleanup:
    if (init_thread_webserver == true) {
        pthread_join(web_server_thread, NULL);
        MYMPD_LOG_NOTICE("Stopping web server thread");
    }
    if (init_thread_mympdapi == true) {
        pthread_join(mympd_api_thread, NULL);
        MYMPD_LOG_NOTICE("Stopping mympd api thread");
    }
    if (init_webserver == true) {
        web_server_free(&mgr);
    }
    
    int expired = expire_result_queue(web_server_queue, 0);
    mympd_queue_free(web_server_queue);
    MYMPD_LOG_DEBUG("Expired %d entries from web_server_queue", expired);

    expired = expire_request_queue(mympd_api_queue, 0);
    mympd_queue_free(mympd_api_queue);
    MYMPD_LOG_DEBUG("Expired %d entries from mympd_api_queue", expired);

    expired = expire_result_queue(mympd_script_queue, 0);
    mympd_queue_free(mympd_script_queue);
    MYMPD_LOG_DEBUG("Expired %d entries from mympd_script_queue", expired);

    mympd_free_config_initial(config);
    if (init_config == true) {
        mympd_free_config(config);
    }
    free(config);
    if (init_mg_user_data == true) {
        free((char *)mgr.dns4.url);
        mg_user_data_free(mg_user_data);
    }
    FREE_PTR(mg_user_data);
    if (rc == EXIT_SUCCESS) {
        printf("Exiting gracefully, thank you for using myMPD\n");
    }

    end:
    FREE_SDS(thread_logname);
    return rc;
}
