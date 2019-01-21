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
#include <mpd/client.h>
#include <stdbool.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"
#include "web_server.h"
#include "mympd_api.h"
#include "../dist/src/inih/ini.h"


static void signal_handler(int sig_num) {
    signal(sig_num, signal_handler);  // Reinstantiate signal handler
    s_signal_received = sig_num;
}

static int inihandler(void *user, const char *section, const char *name, const char* value) {
    t_config* p_config = (t_config*)user;
    char *crap;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("mpd", "mpdhost"))
        p_config->mpdhost = strdup(value);
    else if (MATCH("mpd", "mpdport"))
        p_config->mpdport = strtol(value, &crap, 10);
    else if (MATCH("mpd", "mpdpass"))
        p_config->mpdpass = strdup(value);
    else if (MATCH("mpd", "streamport"))
        p_config->streamport = strtol(value, &crap, 10);
    else if (MATCH("webserver", "webport"))
        p_config->webport = strdup(value);
    else if (MATCH("webserver", "ssl"))
        if (strcmp(value, "true") == 0)
            p_config->ssl = true;
        else
            p_config->ssl = false;
    else if (MATCH("webserver", "sslport"))
        p_config->sslport = strdup(value);
    else if (MATCH("webserver", "sslcert"))
        p_config->sslcert = strdup(value);
    else if (MATCH("webserver", "sslkey"))
        p_config->sslkey = strdup(value);
    else if (MATCH("mympd", "user"))
        p_config->user = strdup(value);
    else if (MATCH("mympd", "coverimage"))
        if (strcmp(value, "true") == 0)
            p_config->coverimage = true;
        else
            p_config->coverimage = false;
    else if (MATCH("mympd", "coverimagename"))
        p_config->coverimagename = strdup(value);
    else if (MATCH("mympd", "coverimagesize"))
        p_config->coverimagesize = strtol(value, &crap, 10);
    else if (MATCH("mympd", "varlibdir"))
        p_config->varlibdir = strdup(value);
    else if (MATCH("mympd", "stickers"))
        if (strcmp(value, "true") == 0)
            p_config->stickers = true;
        else
            p_config->stickers = false;
    else if (MATCH("mympd", "smartpls"))
        if (strcmp(value, "true") == 0)
            p_config->smartpls = true;
        else
            p_config->smartpls = false;
    else if (MATCH("mympd", "mixramp"))
        if (strcmp(value, "true") == 0)
            p_config->mixramp = true;
        else
            p_config->mixramp = false;
    else if (MATCH("mympd", "taglist"))
        p_config->taglist = strdup(value);
    else if (MATCH("mympd", "searchtaglist"))
        p_config->searchtaglist = strdup(value);        
    else if (MATCH("mympd", "browsetaglist"))
        p_config->browsetaglist = strdup(value);
    else if (MATCH("mympd", "max_elements_per_page")) {
        p_config->max_elements_per_page = strtol(value, &crap, 10);
        if (p_config->max_elements_per_page > MAX_ELEMENTS_PER_PAGE) {
            printf("Setting max_elements_per_page to maximal value %d", MAX_ELEMENTS_PER_PAGE);
            p_config->max_elements_per_page = MAX_ELEMENTS_PER_PAGE;
        }
    }
    else if (MATCH("mympd", "syscmds"))
        if (strcmp(value, "true") == 0)
            p_config->syscmds = true;
        else
            p_config->syscmds = false;
    else if (MATCH("mympd", "localplayer"))
        if (strcmp(value, "true") == 0)
            p_config->localplayer = true;
        else
            p_config->localplayer = false;
    else if (MATCH("mympd", "streamurl"))
        p_config->streamurl = strdup(value);
    else if (MATCH("mympd", "last_played_count"))
        p_config->last_played_count = strtol(value, &crap, 10);
    else if (MATCH("mympd", "loglevel"))
        p_config->loglevel = strtol(value, &crap, 10);
    else if (MATCH("theme", "backgroundcolor"))
        p_config->backgroundcolor = strdup(value);
    else {
        printf("Unkown config option: %s - %s\n", section, name);
        return 0;  /* unknown section/name, error */
    }

    return 1;
}

int main(int argc, char **argv) {
    s_signal_received = 0;
    char testdirname[400];
    mpd_client_queue = tiny_queue_create();
    mympd_api_queue = tiny_queue_create();
    web_server_queue = tiny_queue_create();

    srand((unsigned int)time(NULL));
    
    //mympd config defaults
    t_config config;
    config.mpdhost = "127.0.0.1";
    config.mpdport = 6600;
    config.mpdpass = NULL;
    config.webport = "80";
    config.ssl = true;
    config.sslport = "443";
    config.sslcert = "/etc/mympd/ssl/server.pem";
    config.sslkey = "/etc/mympd/ssl/server.key";
    config.user = "mympd";
    config.streamport = 8000;
    config.streamurl = "";
    config.coverimage = true;
    config.coverimagename = "folder.jpg";
    config.coverimagesize = 240;
    config.varlibdir = "/var/lib/mympd";
    config.stickers = true;
    config.mixramp = true;
    config.taglist = "Artist,Album,AlbumArtist,Title,Track,Genre,Date,Composer,Performer";
    config.searchtaglist = "Artist,Album,AlbumArtist,Title,Genre,Composer,Performer";
    config.browsetaglist = "Artist,Album,AlbumArtist,Genre,Composer,Performer";
    config.smartpls = true;
    config.max_elements_per_page = 100;
    config.last_played_count = 20;
    char *etcdir = strdup(argv[1]);
    config.etcdir = strdup(dirname(etcdir));
    free(etcdir);
    config.syscmds = false;
    config.localplayer = true;
    config.loglevel = 1;
    config.backgroundcolor = "#888";
    
    if (argc == 2) {
        printf("Starting myMPD %s\n", MYMPD_VERSION);
        printf("Libmpdclient %i.%i.%i\n", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
        printf("Parsing config file: %s\n", argv[1]);
        if (ini_parse(argv[1], inihandler, &config) < 0) {
            printf("Can't load config file \"%s\"\n", argv[1]);
            return EXIT_FAILURE;
        }
    } 
    else {
        printf("myMPD  %s\n"
            "Copyright (C) 2018-2019 Juergen Mang <mail@jcgames.de>\n"
            "https://github.com/jcorporation/myMPD\n"
            "Built " __DATE__ " "__TIME__"\n\n"
            "Usage: %s /path/to/mympd.conf\n",
            MYMPD_VERSION,
            argv[0]
        );
        return EXIT_FAILURE;    
    }
    
    #ifdef DEBUG
    printf("Debug flag enabled, setting loglevel to debug\n");
    config.loglevel = 3;
    #endif
    printf("Setting loglevel to %ld\n", config.loglevel);
    loglevel = config.loglevel;

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //init webserver
    struct mg_mgr mgr;
    if (!web_server_init(&mgr, &config)) {
        return EXIT_FAILURE;
    }

    //drop privileges
    if (config.user != NULL) {
        printf("Droping privileges to %s\n", config.user);
        struct passwd *pw;
        if ((pw = getpwnam(config.user)) == NULL) {
            printf("getpwnam() failed, unknown user\n");
            web_server_free(&mgr);
            return EXIT_FAILURE;
        } else if (setgroups(0, NULL) != 0) { 
            printf("setgroups() failed\n");
            web_server_free(&mgr);
            return EXIT_FAILURE;        
        } else if (setgid(pw->pw_gid) != 0) {
            printf("setgid() failed\n");
            web_server_free(&mgr);
            return EXIT_FAILURE;
        } else if (setuid(pw->pw_uid) != 0) {
            printf("setuid() failed\n");
            web_server_free(&mgr);
            return EXIT_FAILURE;
        }
    }
    
    if (getuid() == 0) {
        printf("myMPD should not be run with root privileges\n");
        web_server_free(&mgr);
        return EXIT_FAILURE;
    }

    //check needed directories
    if (!testdir("Document root", DOC_ROOT)) 
        return EXIT_FAILURE;

    snprintf(testdirname, 400, "%s/library", DOC_ROOT);
    if (!testdir("Link to mpd music_directory", testdirname)) {
        printf("Disabling coverimage support\n");
        config.coverimage = false;
    }

    snprintf(testdirname, 400, "%s/tmp", config.varlibdir);
    if (!testdir("Temp dir", testdirname)) 
        return EXIT_FAILURE;

    snprintf(testdirname, 400, "%s/smartpls", config.varlibdir);
    if (!testdir("Smartpls dir", testdirname)) 
        return EXIT_FAILURE;

    snprintf(testdirname, 400, "%s/state", config.varlibdir);
    if (!testdir("State dir", testdirname)) 
        return EXIT_FAILURE;

    //Create working threads
    pthread_t mpd_client_thread, web_server_thread, mympd_api_thread;
    //mpd connection
    pthread_create(&mpd_client_thread, NULL, mpd_client_loop, &config);
    pthread_setname_np(mpd_client_thread, "mympd_mpdclient");
    //webserver
    pthread_create(&web_server_thread, NULL, web_server_loop, &mgr);
    pthread_setname_np(web_server_thread, "mympd_webserver");
    //mympd api
    pthread_create(&mympd_api_thread, NULL, mympd_api_loop, &config);
    pthread_setname_np(mympd_api_thread, "mympd_mympdapi");

    //Outsourced all work to separate threads, do nothing...

    //cleanup
    pthread_join(mpd_client_thread, NULL);
    pthread_join(web_server_thread, NULL);
    tiny_queue_free(web_server_queue);
    tiny_queue_free(mpd_client_queue);
    tiny_queue_free(mympd_api_queue);
    return EXIT_SUCCESS;
}
