/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <pthread.h>
#include <dirent.h>
#include <stdbool.h>
#include <signal.h>
#include <dlfcn.h>
#include <assert.h>
#include <inttypes.h>
#include <mpd/client.h>

#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "sds_extras.h"
#include "log.h"
#include "tiny_queue.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "mympd_config.h"
#include "mympd_state.h"
#include "handle_options.h"
#include "utility.h"
#include "api.h"
#include "global.h"
#include "mpd_worker.h"
#include "web_server/web_server_utility.h"
#include "web_server.h"
#include "mpd_client/mpd_client_playlists.h"
#include "mympd_api.h"
#ifdef ENABLE_SSL
  #include "cert.h"
#endif
#include "random.h"
#include "mympd_migrate.h"

_Thread_local sds thread_logname;

static void mympd_signal_handler(int sig_num) {
    signal(sig_num, mympd_signal_handler);  // Reinstantiate signal handler
    if (sig_num == SIGTERM || sig_num == SIGINT) {
        //Set loop end condiftion for threads
        s_signal_received = sig_num;
        //Wakeup queue loops
        pthread_cond_signal(&mympd_api_queue->wakeup);
        pthread_cond_signal(&mympd_script_queue->wakeup);
        MYMPD_LOG_NOTICE("Signal %s received, exiting", (sig_num == SIGTERM ? "SIGTERM" : "SIGINT"));
    }
    else if (sig_num == SIGHUP) {
        MYMPD_LOG_NOTICE("Signal SIGHUP received, saving states");
        t_work_request *request = create_request(-1, 0, MYMPD_API_STATE_SAVE, "MYMPD_API_STATE_SAVE", "");
        request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_STATE_SAVE\",\"params\":{}}");
        tiny_queue_push(mympd_api_queue, request, 0);    
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

#ifdef ENABLE_SSL
static bool check_ssl_certs(struct t_config *config) {
    if (config->ssl == false || config->custom_cert == true) {
        return true;
    }
    sds testdirname = sdscatfmt(sdsempty(), "%s/ssl", config->workdir);
    int testdir_rc = testdir("SSL cert dir", testdirname, true);
    if (testdir_rc < 2) {
        //directory created, create certificates
        if (!create_certificates(testdirname, config->ssl_san)) {
            //error creating certificates
            MYMPD_LOG_ERROR("Certificate creation failed");
            sdsfree(testdirname);
            return false;
        }
        sdsfree(testdirname);
    }
    else {
        sdsfree(testdirname);
        return false;
    }
    return true;
}
#endif

static bool check_dirs_initial(struct t_config *config, uid_t startup_uid) {
    int testdir_rc = testdir("Workdir", config->workdir, true);
    if (testdir_rc == 1) {
        config->first_startup = true;
    }
    if (testdir_rc < 2) {
        //directory exists or was created; set user and group, if uid = 0
        if (startup_uid == 0) {
            if (do_chown(config->workdir, config->user) == false) {
                return false;
            }
        }
    }
    else {
        //workdir is not accessible
        MYMPD_LOG_ERROR("Can not access %s", config->workdir);
        return false;
    }

    //config directory
    sds testdirname = sdscatfmt(sdsempty(), "%s/config", config->workdir);
    testdir_rc = testdir("Config dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    sdsfree(testdirname);
    return true;
}

static bool check_dirs(struct t_config *config) {
    int testdir_rc;
    #ifdef DEBUG
    //release uses empty document root and delivers embedded files
    testdir_rc = testdir("Document root", DOC_ROOT, false);
    if (testdir_rc > 1) {
        return false;
    }
    #endif
    //state directory
    sds testdirname = sdscatfmt(sdsempty(), "%s/state", config->workdir);
    testdir_rc = testdir("State dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    //smart playlists
    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/smartpls", config->workdir);
    testdir_rc = testdir("Smartpls dir", testdirname, true);
    if (testdir_rc == 1) {
        //directory created, create default smart playlists
        smartpls_default(config);
    }
    else if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }

    //for images
    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/pics", config->workdir);
    testdir_rc = testdir("Pics dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    
    //create empty document_root
    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/empty", config->workdir);
    testdir_rc = testdir("Empty dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }

    //lua scripting
    #ifdef ENABLE_LUA
    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/scripts", config->workdir);
    testdir_rc = testdir("Scripts dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    #endif

    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/covercache", config->workdir);
    testdir_rc = testdir("Covercache dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    sdsfree(testdirname);
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
    
    mympd_api_queue = tiny_queue_create("mympd_api_queue");
    web_server_queue = tiny_queue_create("web_server_queue");
    mympd_script_queue = tiny_queue_create("mympd_script_queue");

    //create mg_user_data struct for web_server
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *)malloc(sizeof(struct t_mg_user_data));
    assert(mg_user_data);

    //initialize random number generator
    tinymt32_init(&tinymt, (unsigned int)time(NULL));
    
    //mympd config defaults
    struct t_config *config = (struct t_config *)malloc(sizeof(struct t_config));
    assert(config);
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

    //migrate old config
    start_migrate_conf(config->workdir);

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
        set_loglevel(LOG_DEBUG);
    #else
        set_loglevel(config->loglevel);
    #endif

    if (config->syslog == true) {
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
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //init webserver    
    struct mg_mgr mgr;
    init_mg_user_data = true;
    if (!web_server_init(&mgr, config, mg_user_data)) {
        goto cleanup;
    }
    else {
        init_webserver = true;
    }

    //drop privileges
    if (drop_privileges(config, startup_uid) == false) {
        goto cleanup;
    }

    //check for ssl certificates
    #ifdef ENABLE_SSL
    if (check_ssl_certs(config) == false) {
        goto cleanup;
    }
    #endif

    //check for needed directories
    if (check_dirs(config) == false) {
        goto cleanup;
    }

    //migrate old config
    start_migrate_workdir(config->workdir);

    //Create working threads
    pthread_t web_server_thread;
    pthread_t mympd_api_thread;
    //mympd api
    MYMPD_LOG_NOTICE("Starting mympd api thread");
    if (pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config) == 0) {
        pthread_setname_np(mympd_api_thread, "mympd_api");
        init_thread_mympdapi = true;
    }
    else {
        MYMPD_LOG_ERROR("Can't create mympd_api thread");
        s_signal_received = SIGTERM;
    }
    //webserver
    MYMPD_LOG_NOTICE("Starting webserver thread");
    if (pthread_create(&web_server_thread, NULL, web_server_loop, &mgr) == 0) {
        pthread_setname_np(web_server_thread, "mympd_webserver");
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
    
    MYMPD_LOG_DEBUG("Expiring web_server_queue: %u", tiny_queue_length(web_server_queue, 10));
    int expired = expire_result_queue(web_server_queue, 0);
    tiny_queue_free(web_server_queue);
    MYMPD_LOG_DEBUG("Expired %d entries", expired);

    MYMPD_LOG_DEBUG("Expiring mympd_api_queue: %u", tiny_queue_length(mympd_api_queue, 10));
    expired = expire_request_queue(mympd_api_queue, 0);
    tiny_queue_free(mympd_api_queue);
    MYMPD_LOG_DEBUG("Expired %d entries", expired);

    MYMPD_LOG_DEBUG("Expiring mympd_script_queue: %u", tiny_queue_length(mympd_script_queue, 10));
    expired = expire_result_queue(mympd_script_queue, 0);
    tiny_queue_free(mympd_script_queue);
    MYMPD_LOG_DEBUG("Expired %d entries", expired);

    mympd_free_config_initial(config);
    if (init_config == true) {
        mympd_free_config(config);
    }
    free(config);
    if (init_mg_user_data == true) {
        free((char *)mgr.dns4.url);
        free_mg_user_data(mg_user_data);
    }
    FREE_PTR(mg_user_data);
    if (rc == EXIT_SUCCESS) {
        printf("Exiting gracefully, thank you for using myMPD\n");
    }

    end:
    sdsfree(thread_logname);
    return rc;
}
