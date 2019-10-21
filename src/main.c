/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

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
#include <mpd/client.h>
#include <assert.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "config_defs.h"
#include "config.h"
#include "api.h"
#include "global.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"
#include "web_server.h"
#include "mympd_api.h"
#include "cert.h"
#include "handle_options.h"
#include "plugins.h"

static void mympd_signal_handler(int sig_num) {
    signal(sig_num, mympd_signal_handler);  // Reinstantiate signal handler
    s_signal_received = sig_num;
    //Wakeup mympd_api_loop
    pthread_cond_signal(&mympd_api_queue->wakeup);
    LOG_INFO("Signal %d received, exiting", sig_num);
}

static bool do_chown(const char *file_path, const char *user_name) {
    struct passwd *pwd = getpwnam(user_name);
    if (pwd == NULL) {
        LOG_ERROR("Can't get passwd entry for user %s", user_name);
        return false;
    }
  
    int rc = chown(file_path, pwd->pw_uid, pwd->pw_gid); /* Flawfinder: ignore */
    //Originaly owned by root
    if (rc == -1) {
        LOG_ERROR("Can't chown %s to %s", file_path, user_name);
        return false;
    }
    return true;
}

static bool do_chroot(struct t_config *config) {
    if (chroot(config->varlibdir) == 0) { /* Flawfinder: ignore */
        if (chdir("/") != 0) {
            return false;
        }
        //reset environment
        clearenv();
        char env_pwd[] = "PWD=/";
        putenv(env_pwd);
        //set mympd config
        config->varlibdir = sdscrop(config->varlibdir);
        if (config->syscmds == true) {
            LOG_INFO("Disabling syscmds");
            config->syscmds = false;
        }
        if (config->plugins_coverextract == true) {
            LOG_INFO("Disabling plugin coverextract");
            config->plugins_coverextract = false;
        }
        return true;
    }
    return false;
}

static bool chown_certs(t_config *config) {
    sds filename = sdscatfmt(sdsempty(), "%s/ssl/ca.pem", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    filename = sdscrop(filename);
    filename = sdscatfmt(filename, "%s/ssl/ca.key", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    filename = sdscrop(filename);
    filename = sdscatfmt(filename, "%s/ssl/server.pem", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    filename = sdscrop(filename);
    filename = sdscatfmt(filename, "%s/ssl/server.key", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    sdsfree(filename);
    return true;
}

static bool drop_privileges(t_config *config, uid_t startup_uid) {
    if (startup_uid == 0) {
        if (sdslen(config->user) > 0) {
            LOG_INFO("Droping privileges to %s", config->user);
            struct passwd *pw;
            if ((pw = getpwnam(config->user)) == NULL) {
                LOG_ERROR("getpwnam() failed, unknown user");
                return false;
            }
            else if (setgroups(0, NULL) != 0) { 
                LOG_ERROR("setgroups() failed");
                return false;
            }
            else if (setgid(pw->pw_gid) != 0) {
                LOG_ERROR("setgid() failed");
                return false;
            }
            if (config->chroot == true) {
                LOG_INFO("Chroot to %s", config->varlibdir);
                if (do_chroot(config) == false) {
                    LOG_ERROR("Chroot to %s failed", config->varlibdir);
                    return false;
                }
            }
            if (setuid(pw->pw_uid) != 0) {
                LOG_ERROR("setuid() failed");
                return false;
            }
        }
        
    }
    //check if not root
    if (getuid() == 0) {
        LOG_ERROR("myMPD should not be run with root privileges");
        return false;
    }
    return true;
}

static bool check_ssl_certs(t_config *config, uid_t startup_uid) {
    if (config->ssl == true && config->custom_cert == false) {
        sds testdirname = sdscatfmt(sdsempty(), "%s/ssl", config->varlibdir);
        int testdir_rc = testdir("SSL certificates", testdirname, true);
        if (testdir_rc < 2) {
            //chown to mympd user if root
            if (startup_uid == 0) {
                if (do_chown(testdirname, config->user) == false) {
                    sdsfree(testdirname);
                    return false;
                }
            }
            //directory created, create certificates
            if (!create_certificates(testdirname, config->ssl_san)) {
                //error creating certificates
                LOG_ERROR("Certificate creation failed");
                sdsfree(testdirname);
                return false;
            }
            sdsfree(testdirname);
            //chown to mympd user if root
            if (startup_uid == 0) {
                if (chown_certs(config)== false) {
                    return false;
                }
            }
        }
        else {
            return false;
        }
    }
    return true;
}

static bool check_dirs(t_config *config) {
    int testdir_rc;
    #ifdef DEBUG
    //release uses empty document root and delivers embedded files
    testdir_rc = testdir("Document root", DOC_ROOT, false);
    if (testdir_rc > 1) {
        return false;
    }
    #endif

    sds testdirname = sdscatfmt(sdsempty(), "%s/smartpls", config->varlibdir);
    testdir_rc = testdir("Smartpls dir", testdirname, true);
    if (testdir_rc == 1) {
        //directory created, create default smart playlists
        smartpls_default(config);
    }
    else if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }

    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/state", config->varlibdir);
    testdir_rc = testdir("State dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    
    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/pics", config->varlibdir);
    testdir_rc = testdir("Pics dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    
    //create empty document_root
    testdirname = sdscrop(testdirname);
    testdirname = sdscatfmt(testdirname, "%s/empty", config->varlibdir);
    testdir_rc = testdir("Empty dir", testdirname, true);
    if (testdir_rc > 1) {
        sdsfree(testdirname);
        return false;
    }
    
    if (config->plugins_coverextract == true) {
        testdirname = sdscrop(testdirname);
        testdirname = sdscatfmt(testdirname, "%s/covercache", config->varlibdir);
        testdir_rc = testdir("Covercache dir", testdirname, true);
        if (testdir_rc > 1) {
            sdsfree(testdirname);
            return false;
        }
    }
    sdsfree(testdirname);
    return true;
}

int main(int argc, char **argv) {
    s_signal_received = 0;
    bool init_webserver = false;
    bool init_thread_webserver = false;
    bool init_thread_mpdclient = false;
    bool init_thread_mympdapi = false;
    int rc = EXIT_FAILURE;
    #ifdef DEBUG
    set_loglevel(4);
    #else
    set_loglevel(2);
    #endif

    if (chdir("/") != 0) {
        goto end;
    }
    //only user and group have rw access
    umask(0007); /* Flawfinder: ignore */

    //get startup uid
    uid_t startup_uid = getuid();
    
    mpd_client_queue = tiny_queue_create();
    mympd_api_queue = tiny_queue_create();
    web_server_queue = tiny_queue_create();

    t_mg_user_data *mg_user_data = (t_mg_user_data *)malloc(sizeof(t_mg_user_data));
    assert(mg_user_data);

    srand((unsigned int)time(NULL)); /* Flawfinder: ignore */
    
    //mympd config defaults
    t_config *config = (t_config *)malloc(sizeof(t_config));
    assert(config);
    mympd_config_defaults(config);

    //get configuration file
    sds configfile = sdscatfmt(sdsempty(), "%s/mympd.conf", ETC_PATH);
    
    sds option = sdsempty();
    
    if (argc >= 2) {
        if (strncmp(argv[1], "/", 1) == 0) {
            configfile = sdsreplace(configfile, argv[1]);
            if (argc == 3) {
                option = sdsreplace(option, argv[2]);
            }
        }
        else {
            option = sdsreplace(option, argv[1]);
        }
    }

    LOG_INFO("Starting myMPD %s", MYMPD_VERSION);
    LOG_INFO("Libmpdclient %i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
    LOG_INFO("Mongoose %s", MG_VERSION);
    
    if (mympd_read_config(config, configfile) == false) {
        goto cleanup;
    }

    //set loglevel
    #ifdef DEBUG
    set_loglevel(4);
    #else
    set_loglevel(config->loglevel);
    #endif

    //check varlibdir
    int testdir_rc = testdir("Localstate dir", config->varlibdir, true);
    if (testdir_rc < 2) {
        //directory exists or was created, set user and group if I am root
        if (startup_uid == 0) {
            if (do_chown(config->varlibdir, config->user) == false) {
                goto cleanup;
            }
        }
    }
    else {
        goto cleanup;
    }
    
    LOG_DEBUG("myMPD started with option: %s", option);
    //handle commandline options and exit
    if (sdslen(option) > 0) {
        if (handle_option(config, argv[0], option) == false) {
            rc = EXIT_FAILURE;
        }
        else {
            rc = EXIT_SUCCESS;
        }
        goto cleanup;
    }
    
    //init plugins
    if (init_plugins(config) == false) {
        goto cleanup;
    }

    //set signal handler
    signal(SIGTERM, mympd_signal_handler);
    signal(SIGINT, mympd_signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //check for ssl certificates
    if (check_ssl_certs(config, startup_uid) == false) {
        goto cleanup;
    }

    //init webserver    
    struct mg_mgr mgr;
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

    //check for needed directories
    if (check_dirs(config) == false) {
        goto cleanup;
    }

    //Create working threads
    pthread_t mpd_client_thread, web_server_thread, mympd_api_thread;
    //mympd api
    LOG_INFO("Starting mympd api thread");
    if (pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config) == 0) {
        pthread_setname_np(mympd_api_thread, "mympd_mympdapi");
        init_thread_mympdapi = true;
    }
    else {
        LOG_ERROR("Can't create mympd_mympdapi thread");
        s_signal_received = SIGTERM;
    }
    //mpd connection
    LOG_INFO("Starting mpd client thread");
    if (pthread_create(&mpd_client_thread, NULL, mpd_client_loop, config) == 0) {
        pthread_setname_np(mpd_client_thread, "mympd_mpdclient");
        init_thread_mpdclient = true;
    }
    else {
        LOG_ERROR("Can't create mympd_client thread");
        s_signal_received = SIGTERM;
    }

    //webserver
    LOG_INFO("Starting webserver thread");
    if (pthread_create(&web_server_thread, NULL, web_server_loop, &mgr) == 0) {
        pthread_setname_np(web_server_thread, "mympd_webserver");
        init_thread_webserver = true;
    }
    else {
        LOG_ERROR("Can't create mympd_webserver thread");
        s_signal_received = SIGTERM;
    }

    //Outsourced all work to separate threads, do nothing...
    rc = EXIT_SUCCESS;

    //Try to cleanup all
    cleanup:
    if (init_thread_mpdclient == true) {
        pthread_join(mpd_client_thread, NULL);
        LOG_INFO("Stopping mpd client thread");
    }
    if (init_thread_webserver == true) {
        pthread_join(web_server_thread, NULL);
        LOG_INFO("Stopping web server thread");
    }
    if (init_thread_mympdapi == true) {
        pthread_join(mympd_api_thread, NULL);
        LOG_INFO("Stopping mympd api thread");
    }
    if (init_webserver == true) {
        web_server_free(&mgr);
    }
    tiny_queue_free(web_server_queue);
    tiny_queue_free(mpd_client_queue);
    tiny_queue_free(mympd_api_queue);
    close_plugins(config);
    mympd_free_config(config);
    sdsfree(configfile);
    sdsfree(option);
    sdsfree(mg_user_data->music_directory);
    sdsfree(mg_user_data->pics_directory);
    sdsfree(mg_user_data->rewrite_patterns);
    FREE_PTR(mg_user_data);
    if (rc == EXIT_SUCCESS) {
        printf("Exiting gracefully, thank you for using myMPD\n");
    }
    end:
    return rc;
}
