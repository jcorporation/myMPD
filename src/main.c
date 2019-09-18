/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of: ympd (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   ympd project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

static void mympd_signal_handler(int sig_num) {
    signal(sig_num, mympd_signal_handler);  // Reinstantiate signal handler
    s_signal_received = sig_num;
    //Wakeup mympd_api_loop
    pthread_cond_signal(&mympd_api_queue->wakeup);
    LOG_INFO("Signal %d received, exiting", sig_num);
}

static bool init_plugins(struct t_config *config) {
    char *error = NULL;
    handle_plugins_coverextract = NULL;
    if (config->plugins_coverextract == true) {
        sds coverextractplugin = sdscatprintf(sdsempty(), "%s/libmympd_coverextract.so", PLUGIN_PATH);
        LOG_INFO("Loading plugin %s", coverextractplugin);
        handle_plugins_coverextract = dlopen(coverextractplugin, RTLD_NOW);
        sds_free(coverextractplugin);
        if (!handle_plugins_coverextract) {
            LOG_ERROR("Can't load plugin %s: %s", coverextractplugin, dlerror());
            sds_free(coverextractplugin);
            return false;
        }
        *(void **) (&plugin_coverextract) = dlsym(handle_plugins_coverextract, "coverextract");
        if ((error = dlerror()) != NULL)  {
            LOG_ERROR("Can't load plugin %s: %s", coverextractplugin, error);
            sds_free(coverextractplugin);
            return false;
        }
    }
    return true;
}

static void close_plugins(struct t_config *config) {
    if (config->plugins_coverextract == true && handle_plugins_coverextract != NULL) {
        dlclose(handle_plugins_coverextract);
    }
}

static bool do_chown(const char *file_path, const char *user_name) {
  struct passwd *pwd = getpwnam(user_name);
  if (pwd == NULL) {
      LOG_ERROR("Can't get passwd entry for user %s", user_name);
      return false;
  }
  
  if (chown(file_path, pwd->pw_uid, pwd->pw_gid) == -1) {
      LOG_ERROR("Can't chown %s to %s", file_path, user_name);
      return false;
  }
  return true;
}

static bool do_chroot(struct t_config *config) {
    if (chroot(config->varlibdir) == 0) {
        if (chdir("/") != 0) {
            return false;
        }
        //reset environment
        clearenv();
        char env_pwd[] = "PWD=/";
        putenv(env_pwd);
        //set mympd config
        FREE_PTR(config->varlibdir);
        config->varlibdir = strdup("");
        config->varlibdir_len = 0;
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
    sds filename = sdscatprintf(sdsempty(), "%s/ssl/ca.pem", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    filename = sdscatprintf(sdsempty(), "%s/ssl/ca.key", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    filename = sdscatprintf(sdsempty(), "%s/ssl/server.pem", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    filename = sdscatprintf(sdsempty(), "%s/ssl/server.key", config->varlibdir);
    if (do_chown(filename, config->user) == false) {
        sdsfree(filename);
        return false;
    }
    sdsfree(filename);
    return true;
}

int main(int argc, char **argv) {
    s_signal_received = 0;
    sds testdirname = sdsempty();
    int testdir_rc = 0;
    bool init_webserver = false;
    bool init_thread_webserver = false;
    bool init_thread_mpdclient = false;
    bool init_thread_mympdapi = false;
    int rc = EXIT_FAILURE;
    loglevel = 2;

    if (chdir("/") != 0) {
        goto end;
    }
    //only user and group have rw access
    umask(0007);

    //get startup uid
    uid_t startup_uid = getuid();
    
    mpd_client_queue = tiny_queue_create();
    mympd_api_queue = tiny_queue_create();
    web_server_queue = tiny_queue_create();

    t_mg_user_data *mg_user_data = (t_mg_user_data *)malloc(sizeof(t_mg_user_data));
    assert(mg_user_data);

    srand((unsigned int)time(NULL));
    
    //mympd config defaults
    t_config *config = (t_config *)malloc(sizeof(t_config));
    assert(config);
    mympd_config_defaults(config);

    //get configuration file
    sds configfile = sdscatprintf(sdsempty(), "%s/mympd.conf", ETC_PATH);
    
    sds option = sdsempty();
    
    if (argc >= 2) {
        if (strncmp(argv[1], "/", 1) == 0) {
            configfile = sdscat(sdsempty(), argv[1]);
            if (argc == 3) {
                option = sdscat(sdsempty(), argv[2]);
            }
        }
        else {
            option = sdscat(sdsempty(), argv[1]);
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
    testdir_rc = testdir("Localstate dir", config->varlibdir, true);
    if (testdir_rc < 2) {
        //directory exists or was created, set user and group 
        if (do_chown(config->varlibdir, config->user) == false) {
            goto cleanup;
        }
    }
    else {
        goto cleanup;
    }
    
    //handle commandline options and exit
    if (option != NULL) {
        if (handle_option(config, argv[0], option) == false) {
            rc = EXIT_FAILURE;
        }
        else {
            rc = EXIT_SUCCESS;
        }
        free(option);
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
    if (config->ssl == true && config->custom_cert == false) {
        testdirname = sdscatprintf(sdsempty(), "%s/ssl", config->varlibdir);
        testdir_rc = testdir("SSL certificates", testdirname, true);
        if (testdir_rc < 2) {
            //chown to mympd user if root
            if (startup_uid == 0) {
                if (do_chown(testdirname, config->user) == false) {
                    goto cleanup;
                }
            }
            //directory created, create certificates
            if (!create_certificates(testdirname, config->ssl_san)) {
                //error creating certificates, remove directory
                LOG_ERROR("Certificate creation failed");
                goto cleanup;
            }
            //chown to mympd user if root
            if (startup_uid == 0) {
                if (chown_certs(config)== false) {
                    goto cleanup;
                }
            }
        }
        else {
            goto cleanup;
        }
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
    if (startup_uid == 0) {
        if (config->user != NULL || strlen(config->user) != 0) {
            LOG_INFO("Droping privileges to %s", config->user);
            struct passwd *pw;
            if ((pw = getpwnam(config->user)) == NULL) {
                LOG_ERROR("getpwnam() failed, unknown user");
                goto cleanup;
            }
            else if (setgroups(0, NULL) != 0) { 
                LOG_ERROR("setgroups() failed");
                goto cleanup;
            }
            else if (setgid(pw->pw_gid) != 0) {
                LOG_ERROR("setgid() failed");
                goto cleanup;
            }
            if (config->chroot == true) {
                LOG_INFO("Chroot to %s", config->varlibdir);
                if (do_chroot(config) == false) {
                    LOG_ERROR("Chroot to %s failed", config->varlibdir);
                    goto cleanup;
                }
            }
            if (setuid(pw->pw_uid) != 0) {
                LOG_ERROR("setuid() failed");
                goto cleanup;
            }
        }
    }
    //check if not root
    if (getuid() == 0) {
        LOG_ERROR("myMPD should not be run with root privileges");
        goto cleanup;
    }

    //check for needed directories
    #ifdef DEBUG
    //release uses empty document root and delivers embedded files
    testdir_rc = testdir("Document root", DOC_ROOT, false);
    if (testdir_rc > 1) {
        goto cleanup;
    }
    #endif

    testdirname = sdscatprintf(sdsempty(), "%s/smartpls", config->varlibdir);
    testdir_rc = testdir("Smartpls dir", testdirname, true);
    if (testdir_rc == 1) {
        //directory created, create default smart playlists
        smartpls_default(config);
    }
    else if (testdir_rc > 1) {
        goto cleanup;
    }

    testdirname = sdscatprintf(sdsempty(), "%s/state", config->varlibdir);
    testdir_rc = testdir("State dir", testdirname, true);
    if (testdir_rc > 1) {
        goto cleanup;
    }
    
    testdirname = sdscatprintf(sdsempty(), "%s/pics", config->varlibdir);
    testdir_rc = testdir("Pics dir", testdirname, true);
    if (testdir_rc > 1) {
        goto cleanup;
    }
    
    //create empty document_root
    testdirname = sdscatprintf(sdsempty(), "%s/empty", config->varlibdir);
    testdir_rc = testdir("Empty dir", testdirname, true);
    if (testdir_rc > 1) {
        goto cleanup;
    }
    
    if (config->plugins_coverextract == true) {
        testdirname = sdscatprintf(sdsempty(), "%s/covercache", config->varlibdir);
        testdir_rc = testdir("Covercache dir", testdirname, true);
        if (testdir_rc > 1) {
            goto cleanup;
        }
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
    if (init_thread_mpdclient) {
        pthread_join(mpd_client_thread, NULL);
        LOG_INFO("Stopping mpd client thread");
    }
    if (init_thread_webserver) {
        pthread_join(web_server_thread, NULL);
        LOG_INFO("Stopping web server thread");
    }
    if (init_thread_mympdapi) {
        pthread_join(mympd_api_thread, NULL);
        LOG_INFO("Stopping mympd api thread");
    }
    if (init_webserver) {
        web_server_free(&mgr);
    }
    tiny_queue_free(web_server_queue);
    tiny_queue_free(mpd_client_queue);
    tiny_queue_free(mympd_api_queue);
    close_plugins(config);
    mympd_free_config(config);
    sds_free(configfile);
    sds_free(option);
    sds_free(testdirname);
    if (init_webserver) {
        sds_free(mg_user_data->music_directory);
        sds_free(mg_user_data->pics_directory);
        sds_free(mg_user_data->rewrite_patterns);
    }
    FREE_PTR(mg_user_data);
    if (rc == EXIT_SUCCESS) {
        printf("Exiting gracefully, thank you for using myMPD\n");
    }
    end:
    return rc;
}
