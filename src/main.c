/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
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

#include "../dist/src/inih/ini.h"
#include "global.h"
#include "mpd_client.h"
#include "web_server.h"

static void signal_handler(int sig_num) {
    signal(sig_num, signal_handler);  // Reinstantiate signal handler
    s_signal_received = sig_num;
}

static int inihandler(void* user, const char* section, const char* name, const char* value) {
    t_config* p_config = (t_config*)user;
    char *crap;

    #define MATCH(n) strcmp(name, n) == 0

    if (MATCH("mpdhost"))
        p_config->mpdhost = strdup(value);
    else if (MATCH("mpdport"))
        p_config->mpdport = strtol(value, &crap, 10);
    else if (MATCH("mpdpass"))
        p_config->mpdpass = strdup(value);
    else if (MATCH("webport"))
        p_config->webport = strdup(value);
    else if (MATCH("ssl"))
        if (strcmp(value, "true") == 0)
            p_config->ssl = true;
        else
            p_config->ssl = false;
    else if (MATCH("sslport"))
        p_config->sslport = strdup(value);
    else if (MATCH("sslcert"))
        p_config->sslcert = strdup(value);
    else if (MATCH("sslkey"))
        p_config->sslkey = strdup(value);
    else if (MATCH("user"))
        p_config->user = strdup(value);
    else if (MATCH("streamport"))
        p_config->streamport = strtol(value, &crap, 10);
    else if (MATCH("coverimage"))
        if (strcmp(value, "true") == 0)
            p_config->coverimage = true;
        else
            p_config->coverimage = false;
    else if (MATCH("coverimagename"))
        p_config->coverimagename = strdup(value);
    else if (MATCH("coverimagesize"))
        p_config->coverimagesize = strtol(value, &crap, 10);
    else if (MATCH("varlibdir"))
        p_config->varlibdir = strdup(value);
    else if (MATCH("stickers"))
        if (strcmp(value, "true") == 0)
            p_config->stickers = true;
        else
            p_config->stickers = false;
    else if (MATCH("smartpls"))
        if (strcmp(value, "true") == 0)
            p_config->smartpls = true;
        else
            p_config->smartpls = false;
    else if (MATCH("mixramp"))
        if (strcmp(value, "true") == 0)
            p_config->mixramp = true;
        else
            p_config->mixramp = false;
    else if (MATCH("taglist"))
        p_config->taglist = strdup(value);
    else if (MATCH("searchtaglist"))
        p_config->searchtaglist = strdup(value);        
    else if (MATCH("browsetaglist"))
        p_config->browsetaglist = strdup(value);
    else if (MATCH("max_elements_per_page")) {
        p_config->max_elements_per_page = strtol(value, &crap, 10);
        if (p_config->max_elements_per_page > MAX_ELEMENTS_PER_PAGE) {
            printf("Setting max_elements_per_page to maximal value %d", MAX_ELEMENTS_PER_PAGE);
            p_config->max_elements_per_page = MAX_ELEMENTS_PER_PAGE;
        }
    }
    else if (MATCH("syscmds"))
        if (strcmp(value, "true") == 0)
            p_config->syscmds = true;
        else
            p_config->syscmds = false;
    else if (MATCH("localplayer"))
        if (strcmp(value, "true") == 0)
            p_config->localplayer = true;
        else
            p_config->localplayer = false;
    else if (MATCH("streamurl"))
        p_config->streamurl = strdup(value);
    else if (MATCH("last_played_count"))
        p_config->last_played_count = strtol(value, &crap, 10);
    else if (MATCH("loglevel"))
        p_config->loglevel = strtol(value, &crap, 10);
    else {
        printf("Unkown config line: %s\n", name);
        return 0;  /* unknown section/name, error */
    }

    return 1;
}

void read_syscmds() {
    DIR *dir;
    struct dirent *ent;
    char dirname[400];
    char *cmd;
    long order;
    if (config.syscmds == true) {    
        snprintf(dirname, 400, "%s/syscmds", config.etcdir);
        LOG_INFO() printf("Reading syscmds: %s\n", dirname);
        if ((dir = opendir (dirname)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (strncmp(ent->d_name, ".", 1) == 0)
                    continue;
                order = strtol(ent->d_name, &cmd, 10);
                if (strcmp(cmd, "") != 0)
                    list_push(&syscmds, strdup(cmd), order);
            }
            closedir(dir);
        }
    }
    else
        LOG_INFO() printf("Syscmds are disabled\n");
}

void read_statefiles() {
    char *crap;
    char value[400];

    LOG_INFO() printf("Reading states\n");
    if (mpd_client_state_get("notificationWeb", value)) {
        if (strcmp(value, "true") == 0)
            mympd_state.notificationWeb = true;
        else
            mympd_state.notificationWeb = false;
    }
    else {
        mympd_state.notificationWeb = false;
        mpd_client_state_set("notificationWeb", "false");
    }

    if (mpd_client_state_get("notificationPage", value)) {
        if (strcmp(value, "true") == 0)
            mympd_state.notificationPage = true;
        else
            mympd_state.notificationPage = false;
    }
    else {
        mympd_state.notificationPage = true;
        mpd_client_state_set("notificationPage", "true");
    }
    
    if (mpd_client_state_get("jukeboxMode", value))
        mympd_state.jukeboxMode = strtol(value, &crap, 10);
    else {
        mympd_state.jukeboxMode = 0;
        mpd_client_state_set("jukeboxMode", "0");
    }

    if (mpd_client_state_get("jukeboxPlaylist", value))
        mympd_state.jukeboxPlaylist = strdup(value);
    else {
        mympd_state.jukeboxPlaylist = strdup("Database");
        mpd_client_state_set("jukeboxPlaylist", "Database");
    }

    if (mpd_client_state_get("jukeboxQueueLength", value))
        mympd_state.jukeboxQueueLength = strtol(value, &crap, 10);
    else {
        mympd_state.jukeboxQueueLength = 1;
        mpd_client_state_set("jukeboxQueueLength", "1");
    }
    
    if (mpd_client_state_get("colsQueueCurrent", value))
        mympd_state.colsQueueCurrent = strdup(value);
    else {
        mympd_state.colsQueueCurrent = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        mpd_client_state_set("colsQueueCurrent", mympd_state.colsQueueCurrent);
    }
    
    if (mpd_client_state_get("colsSearch", value))
        mympd_state.colsSearch = strdup(value);
    else {
        mympd_state.colsSearch = strdup("[\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        mpd_client_state_set("colsSearch", mympd_state.colsSearch);
    }
    
    if (mpd_client_state_get("colsBrowseDatabase", value))
        mympd_state.colsBrowseDatabase = strdup(value);
    else {
        mympd_state.colsBrowseDatabase = strdup("[\"Track\",\"Title\",\"Duration\"]");
        mpd_client_state_set("colsBrowseDatabase", mympd_state.colsBrowseDatabase);
    }
    
    if (mpd_client_state_get("colsBrowsePlaylistsDetail", value))
        mympd_state.colsBrowsePlaylistsDetail = strdup(value);
    else {
        mympd_state.colsBrowsePlaylistsDetail = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        mpd_client_state_set("colsBrowsePlaylistsDetail", mympd_state.colsBrowsePlaylistsDetail);
    }
    
    if (mpd_client_state_get("colsBrowseFilesystem", value))
        mympd_state.colsBrowseFilesystem = strdup(value);
    else {
        mympd_state.colsBrowseFilesystem = strdup("[\"Type\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
        mpd_client_state_set("colsBrowseFilesystem", mympd_state.colsBrowseFilesystem);
    }
    
    if (mpd_client_state_get("colsPlayback", value))
        mympd_state.colsPlayback = strdup(value);
    else {
        mympd_state.colsPlayback = strdup("[\"Artist\",\"Album\",\"Genre\"]");
        mpd_client_state_set("colsPlayback", mympd_state.colsPlayback);
    }

    if (mpd_client_state_get("colsQueueLastPlayed", value))
        mympd_state.colsQueueLastPlayed = strdup(value);
    else {
        mympd_state.colsQueueLastPlayed = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]");
        mpd_client_state_set("colsQueueLastPlayed", mympd_state.colsQueueLastPlayed);
    }
}

int read_last_played() {
    char cfgfile[400];
    char *line;
    char *data;
    char *crap;
    size_t n = 0;
    ssize_t read;
    long value;
    
    snprintf(cfgfile, 400, "%s/state/last_played", config.varlibdir);
    FILE *fp = fopen(cfgfile, "r");
    if (fp == NULL) {
        printf("Error opening %s\n", cfgfile);
        return 0;
    }
    while ((read = getline(&line, &n, fp)) > 0) {
        value = strtol(line, &data, 10);
        if (strlen(data) > 2)
            data = data + 2;
        strtok_r(data, "\n", &crap);
        list_push(&last_played, data, value);
    }
    fclose(fp);
    return last_played.length;;
}

bool testdir(char *name, char *dirname) {
    DIR* dir = opendir(dirname);
    if (dir) {
        closedir(dir);
        LOG_INFO() printf("%s: \"%s\"\n", name, dirname);
        return true;
    }
    else {
        printf("%s: \"%s\" don't exists\n", name, dirname);
        return false;
    }
}

void *mpd_client_thread() {
    while (s_signal_received == 0) {
        mpd_client_idle(100);
    }
    mpd_client_disconnect();
    return NULL;
}

int main(int argc, char **argv) {
    s_signal_received = 0;
    char testdirname[400];
    mpd_client_queue = tiny_queue_create();
    web_server_queue = tiny_queue_create();
    //mympd_queue = tiny_queue_create();
    
    //defaults
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
    config.etcdir = dirname(etcdir);
    config.syscmds = false;
    config.localplayer = true;
    config.loglevel = 1;
    
    mpd.timeout = 3000;
    mpd.last_update_sticker_song_id = -1;
    mpd.last_song_id = -1;
    mpd.last_last_played_id = -1;
    mpd.feat_library = false;
    
    if (argc == 2) {
        LOG_INFO() printf("Starting myMPD %s\n", MYMPD_VERSION);
        LOG_INFO() printf("Libmpdclient %i.%i.%i\n", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
        LOG_INFO() printf("Parsing config file: %s\n", argv[1]);
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

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //init webserver
    if (!web_server_init()) {
        return EXIT_FAILURE;
    }

    //drop privileges
    if (config.user != NULL) {
        LOG_INFO() printf("Droping privileges to %s\n", config.user);
        struct passwd *pw;
        if ((pw = getpwnam(config.user)) == NULL) {
            printf("getpwnam() failed, unknown user\n");
            web_server_free();
            return EXIT_FAILURE;
        } else if (setgroups(0, NULL) != 0) { 
            printf("setgroups() failed\n");
            web_server_free();
            return EXIT_FAILURE;        
        } else if (setgid(pw->pw_gid) != 0) {
            printf("setgid() failed\n");
            web_server_free();
            return EXIT_FAILURE;
        } else if (setuid(pw->pw_uid) != 0) {
            printf("setuid() failed\n");
            web_server_free();
            return EXIT_FAILURE;
        }
    }
    
    if (getuid() == 0) {
        printf("myMPD should not be run with root privileges\n");
        web_server_free();
        return EXIT_FAILURE;
    }

    //check needed directories
    if (!testdir("Document root", DOC_ROOT)) 
        return EXIT_FAILURE;

    snprintf(testdirname, 400, "%s/library", DOC_ROOT);
    if (testdir("Link to mpd music_directory", testdirname)) {
        LOG_INFO() printf("Enabling featLibrary support\n");
        mpd.feat_library = true;
    }
    else {
        LOG_INFO() printf("Disabling coverimage support\n");
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

    //read myMPD states under config.varlibdir
    read_statefiles();

    //read system command files
    list_init(&syscmds);    
    read_syscmds();
    list_sort_by_value(&syscmds, true);

    //init lists for tag handling
    list_init(&mpd_tags);
    list_init(&mympd_tags);
    
    //read last played songs history file
    list_init(&last_played);
    LOG_INFO() printf("Reading last played songs: %d\n", read_last_played());
    
    //Create working threads
    pthread_t mpd_client, web_server;
    //mpd connection
    pthread_create(&mpd_client, NULL, mpd_client_thread, NULL);
    //webserver
    pthread_create(&web_server, NULL, web_server_thread, NULL);

    //Do nothing...


    //clean up
    pthread_join(mpd_client, NULL);
    pthread_join(web_server, NULL);
    list_free(&mpd_tags);
    list_free(&mympd_tags);
    tiny_queue_free(web_server_queue);
    tiny_queue_free(mpd_client_queue);
    //tiny_queue_free(mympd_queue);
    return EXIT_SUCCESS;
}
