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

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"
#include "web_server.h"
#include "mympd_api.h"
#include "../dist/src/inih/ini.h"
#include "cert.h"

static void mympd_signal_handler(int sig_num) {
    signal(sig_num, mympd_signal_handler);  // Reinstantiate signal handler
    s_signal_received = sig_num;
    //Wakeup mympd_api_loop
    pthread_cond_signal(&mympd_api_queue->wakeup);
    LOG_INFO("Signal %d received, exiting", sig_num);
}

static int mympd_inihandler(void *user, const char *section, const char *name, const char* value) {
    t_config* p_config = (t_config*)user;
    char *crap = NULL;

    #define MATCH(s, n) strcasecmp(section, s) == 0 && strcasecmp(name, n) == 0

    if (MATCH("mpd", "host")) {
        FREE_PTR(p_config->mpd_host);
        p_config->mpd_host = strdup(value);
    }
    else if (MATCH("mpd", "port")) {
        p_config->mpd_port = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mpd", "pass")) {
        FREE_PTR(p_config->mpd_pass);
        p_config->mpd_pass = strdup(value);
    }
    else if (MATCH("mpd", "musicdirectory")) {
        FREE_PTR(p_config->music_directory);
        p_config->music_directory = strdup(value);
    }
    else if (MATCH("webserver", "webport")) {
        FREE_PTR(p_config->webport);
        p_config->webport = strdup(value);
    }
    else if (MATCH("webserver", "ssl")) {
        p_config->ssl = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("webserver", "sslport")) {
        FREE_PTR(p_config->ssl_port);
        p_config->ssl_port = strdup(value);
    }
    else if (MATCH("webserver", "sslcert")) {
        if (strcmp(p_config->ssl_cert, value) != 0) {
            p_config->custom_cert = true;
        }
        FREE_PTR(p_config->ssl_cert);
        p_config->ssl_cert = strdup(value);
    }
    else if (MATCH("webserver", "sslkey")) {
        if (strcmp(p_config->ssl_key, value) != 0) {
            p_config->custom_cert = true;
        }
        FREE_PTR(p_config->ssl_key);
        p_config->ssl_key = strdup(value);
    }
    else if (MATCH("webserver", "sslsan")) {
        FREE_PTR(p_config->ssl_san);
        p_config->ssl_san = strdup(value);
    }
    else if (MATCH("mympd", "user")) {
        FREE_PTR(p_config->user);
        p_config->user = strdup(value);
    }
    else if (MATCH("mympd", "varlibdir")) {
        FREE_PTR(p_config->varlibdir);
        p_config->varlibdir = strdup(value);
        p_config->varlibdir_len = strlen(p_config->varlibdir);
    }
    else if (MATCH("mympd", "stickers")) {
        p_config->stickers = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "smartpls")) {
        p_config->smartpls =  strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "mixramp")) {
        p_config->mixramp = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "taglist")) {
        FREE_PTR(p_config->taglist);
        p_config->taglist = strdup(value);
    }
    else if (MATCH("mympd", "searchtaglist")) {
        FREE_PTR(p_config->searchtaglist);
        p_config->searchtaglist = strdup(value);
    }
    else if (MATCH("mympd", "browsetaglist")) {
        FREE_PTR(p_config->browsetaglist);
        p_config->browsetaglist = strdup(value);
    }
    else if (MATCH("mympd", "pagination")) {
        p_config->max_elements_per_page = strtoimax(value, &crap, 10);
        if (p_config->max_elements_per_page > MAX_ELEMENTS_PER_PAGE) {
            LOG_WARN("Setting max_elements_per_page to maximal value %d", MAX_ELEMENTS_PER_PAGE);
            p_config->max_elements_per_page = MAX_ELEMENTS_PER_PAGE;
        }
    }
    else if (MATCH("mympd", "syscmds")) {
        p_config->syscmds = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "lastplayedcount")) {
        p_config->last_played_count = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "loglevel")) {
        p_config->loglevel = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "love")) {
        p_config->love = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "lovechannel")) {
        FREE_PTR(p_config->love_channel);
        p_config->love_channel = strdup(value);
    }
    else if (MATCH("mympd", "lovemessage")) {
        FREE_PTR(p_config->love_message);
        p_config->love_message = strdup(value);
    }
    else if (MATCH("plugins", "coverextract")) {
        p_config->plugins_coverextract = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "notificationweb")) {
        p_config->notification_web = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "notificationpage")) {
        p_config->notification_page = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "autoplay")) {
        p_config->auto_play = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "jukeboxmode")) {
        p_config->jukebox_mode = strtoimax(value, &crap, 10);
        if (p_config->jukebox_mode < 0 || p_config->jukebox_mode > 2) {
            LOG_WARN("Invalid jukeboxmode %d", p_config->jukebox_mode);
            p_config->jukebox_mode = JUKEBOX_OFF;
        }
    }
    else if (MATCH("mympd", "jukeboxplaylist")) {
        FREE_PTR(p_config->jukebox_playlist);
        p_config->jukebox_playlist = strdup(value);
    }
    else if (MATCH("mympd", "jukeboxqueuelength")) {
        p_config->jukebox_queue_length = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "colsqueuecurrent")) {
        FREE_PTR(p_config->cols_queue_current);
        p_config->cols_queue_current = strdup(value);
    }
    else if (MATCH("mympd", "colssearch")) {
        FREE_PTR(p_config->cols_search);
        p_config->cols_search = strdup(value);
    }
    else if (MATCH("mympd", "colsbrowsedatabase")) {
        FREE_PTR(p_config->cols_browse_database);
        p_config->cols_browse_database = strdup(value);
    }
    else if (MATCH("mympd", "colsbrowseplaylistsdetail")) {
        FREE_PTR(p_config->cols_browse_playlists_detail);
        p_config->cols_browse_playlists_detail = strdup(value);
    }
    else if (MATCH("mympd", "colsbrowsefilesystem")) {
        FREE_PTR(p_config->cols_browse_filesystem);
        p_config->cols_browse_filesystem = strdup(value);
    }
    else if (MATCH("mympd", "colsplayback")) {
        FREE_PTR(p_config->cols_playback);
        p_config->cols_playback = strdup(value);
    }
    else if (MATCH("mympd", "colsqueuelastplayed")) {
        FREE_PTR(p_config->cols_queue_last_played);
        p_config->cols_queue_last_played = strdup(value);
    }
    else if (MATCH("mympd", "localplayer")) {
        p_config->localplayer = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "localplayerautoplay")) {
        p_config->localplayer_autoplay = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "streamport")) {
        p_config->stream_port = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "streamurl")) {
        FREE_PTR(p_config->stream_url);
        p_config->stream_url = strdup(value);
    }
    else if (MATCH("theme", "bgcover")) {
        p_config->bg_cover = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("theme", "bgcolor")) {
        FREE_PTR(p_config->bg_color);
        p_config->bg_color = strdup(value);
    }
    else if (MATCH("theme", "bgcssfilter")) {
        FREE_PTR(p_config->bg_css_filter);
        p_config->bg_css_filter = strdup(value);
    }
    else if (MATCH("theme", "coverimage")) {
        p_config->coverimage = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("theme", "coverimagename")) {
        FREE_PTR(p_config->coverimage_name);
        p_config->coverimage_name = strdup(value);
    }
    else if (MATCH("theme", "coverimagesize")) {
        p_config->coverimage_size = strtoimax(value, &crap, 10);
    }
    else if (MATCH("theme", "locale")) {
        FREE_PTR(p_config->locale);
        p_config->locale = strdup(value);
    }
    else if (strcasecmp(section, "syscmds") == 0) {
        list_push(&p_config->syscmd_list, name, 0, (void *)strdup(value));
    }
    else {
        LOG_WARN("Unkown config option: %s - %s", section, name);
        return 0;  
    }
    return 1;
}

static void mympd_free_config(t_config *config) {
    FREE_PTR(config->mpd_host);
    FREE_PTR(config->mpd_pass);
    FREE_PTR(config->webport);
    FREE_PTR(config->ssl_port);
    FREE_PTR(config->ssl_cert);
    FREE_PTR(config->ssl_key);
    FREE_PTR(config->ssl_san);
    FREE_PTR(config->user);
    FREE_PTR(config->taglist);
    FREE_PTR(config->searchtaglist);
    FREE_PTR(config->browsetaglist);
    FREE_PTR(config->varlibdir);
    FREE_PTR(config->love_channel);
    FREE_PTR(config->love_message);
    FREE_PTR(config->music_directory);
    FREE_PTR(config->jukebox_playlist);
    FREE_PTR(config->cols_queue_current);
    FREE_PTR(config->cols_queue_last_played);
    FREE_PTR(config->cols_search);
    FREE_PTR(config->cols_browse_database);
    FREE_PTR(config->cols_browse_playlists_detail);
    FREE_PTR(config->cols_browse_filesystem);
    FREE_PTR(config->cols_playback);
    FREE_PTR(config->stream_url);
    FREE_PTR(config->bg_color);
    FREE_PTR(config->bg_css_filter);
    FREE_PTR(config->coverimage_name);
    FREE_PTR(config->locale);
    list_free(&config->syscmd_list);
    FREE_PTR(config);
}

static void mympd_parse_env(struct t_config *config, const char *envvar) {
    char *name = NULL;
    char *section = NULL;
    const char *value = getenv(envvar);
    if (value != NULL) {
        char *var = strdup(envvar);
        section = strtok_r(var, "_", &name);
        if (section != NULL && name != NULL) {
            LOG_DEBUG("Using environment variable %s_%s: %s", section, name, value);
            mympd_inihandler(config, section, name, value);
        }
        value = NULL;
        FREE_PTR(var);
    }
}

static void mympd_get_env(struct t_config *config) {
    const char *env_vars[]={"MPD_HOST", "MPD_PORT", "MPD_PASS", "MPD_MUSICDIRECTORY",
        "WEBSERVER_WEBPORT", "WEBSERVER_SSL", "WEBSERVER_SSLPORT", "WEBSERVER_SSLCERT", "WEBSERVER_SSLKEY",
        "WEBSERVER_SSLSAN",
        "MYMPD_LOGLEVEL", "MYMPD_USER", "MYMPD_VARLIBDIR", "MYMPD_MIXRAMP", "MYMPD_STICKERS", "MYMPD_TAGLIST", 
        "MYMPD_SEARCHTAGLIST", "MYMPD_BROWSETAGLIST", "MYMPD_SMARTPLS", "MYMPD_SYSCMDS", 
        "MYMPD_PAGINATION", "MYMPD_LASTPLAYEDCOUNT", "MYMPD_LOVE", "MYMPD_LOVECHANNEL", "MYMPD_LOVEMESSAGE",
        "PLUGINS_COVEREXTRACT", "MYMPD_NOTIFICATIONWEB",
        "MYMPD_NOTIFICATIONPAGE", "MYMPD_AUTOPLAY", "MYMPD_JUKEBOXMODE",
        "MYMPD_JUKEBOXPLAYLIST", "MYMPD_JUKEBOXQUEUELENGTH", "MYMPD_COLSQUEUECURRENT",
        "MYMPD_COLSSEARCH", "MYMPD_COLSBROWSEDATABASE", "MYMPD_COLSBROWSEPLAYLISTDETAIL",
        "MYMPD_COLSBROWSEFILESYSTEM", "MYMPD_COLSPLAYBACK", "MYMPD_COLSQUEUELASTPLAYED",
        "MYMPD_LOCALPLAYER", "MYMPD_LOCALPLAYERAUTOPLAY", "MYMPD_STREAMPORT",
        "MYMPD_STREAMURL", "THEME_BGCOVER", "THEME_BGCOLOR", "THEME_BGCSSFILTER",
        "THEME_COVERIMAGE", "THEME_COVERIMAGENAME", "THEME_COVERIMAGESIZE",
        "THEME_LOCALE",0};
    const char** ptr = env_vars;
    while (*ptr != 0) {
        mympd_parse_env(config, *ptr);
        ++ptr;
    }
}

static bool init_plugins(struct t_config *config) {
    char *error = NULL;
    handle_plugins_coverextract = NULL;
    if (config->plugins_coverextract == true) {
        size_t coverextractplugin_len = strlen(PLUGIN_PATH) + 26;
        char coverextractplugin[coverextractplugin_len];
        snprintf(coverextractplugin, coverextractplugin_len, "%s/libmympd_coverextract.so", PLUGIN_PATH);
        LOG_INFO("Loading plugin %s", coverextractplugin);
        handle_plugins_coverextract = dlopen(coverextractplugin, RTLD_LAZY);
        if (!handle_plugins_coverextract) {
            LOG_ERROR("Can't load plugin %s: %s", coverextractplugin, dlerror());
            return false;
        }
        *(void **) (&plugin_coverextract) = dlsym(handle_plugins_coverextract, "coverextract");
        if ((error = dlerror()) != NULL)  {
            LOG_ERROR("Can't load plugin %s: %s", coverextractplugin, error);
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

static bool smartpls_init(t_config *config, const char *name, const char *value) {
    size_t cfg_file_len = config->varlibdir_len + strlen(name) + 11;
    char cfg_file[cfg_file_len];
    size_t tmp_file_len = config->varlibdir_len + strlen(name) + 18;
    char tmp_file[tmp_file_len];
    int fd;
    
    if (!validate_string(name)) {
        return false;
    }
    snprintf(cfg_file, cfg_file_len, "%s/smartpls/%s", config->varlibdir, name);
    snprintf(tmp_file, tmp_file_len, "%s/smartpls/%s.XXXXXX", config->varlibdir, name);
        
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        return false;
    }
    return true;
}

static void smartpls_default(t_config *config) {
    smartpls_init(config, "myMPDsmart-bestRated", 
        "{\"type\": \"sticker\", \"sticker\": \"like\", \"maxentries\": 200}\n");
    smartpls_init(config, "myMPDsmart-mostPlayed", 
        "{\"type\": \"sticker\", \"sticker\": \"playCount\", \"maxentries\": 200}\n");
    smartpls_init(config, "myMPDsmart-newestSongs", 
        "{\"type\": \"newest\", \"timerange\": 604800}\n");
}

static void clear_covercache(t_config *config) {
    size_t covercache_len = config->varlibdir_len + 12;
    char covercache[covercache_len];
    snprintf(covercache, covercache_len, "%s/covercache", config->varlibdir);
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir != NULL) {
        struct dirent *next_file;
        while ( (next_file = readdir(covercache_dir)) != NULL ) {
            if (strncmp(next_file->d_name, ".", 1) != 0) {
                size_t filepath_len = strlen(covercache) + strlen(next_file->d_name) + 2;
                char filepath[filepath_len];
                snprintf(filepath, filepath_len, "%s/%s", covercache, next_file->d_name);
                if (unlink(filepath) != 0) {
                    printf("Error deleting %s\n", filepath);
                }
            }
        }
        closedir(covercache_dir);
    }
    else {
        printf("Error opening directory %s\n", covercache);
    }
}

static void handle_option(t_config *config, char *cmd, char *option) {
    #define MATCH_OPTION(o) strcasecmp(option, o) == 0

    if (MATCH_OPTION("certs_remove")) {
        char testdirname[400];
        snprintf(testdirname, 400, "%s/ssl", config->varlibdir);
        cleanup_certificates(testdirname);
    }
    else if (MATCH_OPTION("certs_create")) {
        char testdirname[400];
        snprintf(testdirname, 400, "%s/ssl", config->varlibdir);
        int testdir_rc = testdir("SSL certificates", testdirname, true);
        if (testdir_rc == 1) {
            create_certificates(testdirname, config->ssl_san);
        }
        else if (testdir_rc == 0) {
            LOG_INFO("Remove certificates with certs_remove before creating new ones");
        }
    }
    else if (MATCH_OPTION("reset_state")) {
        mympd_api_settings_delete(config);
    }
    else if (MATCH_OPTION("reset_smartpls")) {
        smartpls_default(config);
    }
    else if (MATCH_OPTION("reset_lastplayed")) {
        char filename[400];
        snprintf(filename, 400, "%s/state/last_played", config->varlibdir);
        unlink(filename);
    }
    else if (MATCH_OPTION("clear_covercache")) {
        clear_covercache(config);    
    }
    else if (MATCH_OPTION("version")) {
        printf("myMPD %s\n"
               "Copyright (C) 2018-2019 Juergen Mang <mail@jcgames.de>\n"
               "https://github.com/jcorporation/myMPD\n"
               "Libmpdclient %i.%i.%i\n",
               MYMPD_VERSION,
               LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION
        );
    }
    else {
        printf("myMPD %s\n"
               "Copyright (C) 2018-2019 Juergen Mang <mail@jcgames.de>\n"
               "https://github.com/jcorporation/myMPD\n\n"
               "Usage: %s [/etc/mympd.conf] <command>\n"
               "Commands (you should stop mympd before):\n"
               "  certs_create:     create ssl certificates\n"
               "  certs_remove:     remove ssl certificates\n"
               "  reset_state:      delete all myMPD settings\n"
               "  reset_smartpls:   create default smart playlists\n"
               "  reset_lastplayed: truncates last played list\n"
               "  clear_covercache: empties the covercache directory\n",
               MYMPD_VERSION,
               cmd
        );
    }
}

int main(int argc, char **argv) {
    s_signal_received = 0;
    char testdirname[400];
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
    umask(0077);
    
    mpd_client_queue = tiny_queue_create();
    mympd_api_queue = tiny_queue_create();
    web_server_queue = tiny_queue_create();

    t_mg_user_data *mg_user_data = (t_mg_user_data *)malloc(sizeof(t_mg_user_data));
    assert(mg_user_data);

    srand((unsigned int)time(NULL));
    
    //mympd config defaults
    t_config *config = (t_config *)malloc(sizeof(t_config));
    assert(config);
    config->mpd_host = strdup("127.0.0.1");
    config->mpd_port = 6600;
    config->mpd_pass = strdup("");
    config->webport = strdup("80");
    config->ssl = true;
    config->ssl_port = strdup("443");
    config->ssl_cert = strdup(VARLIB_PATH"/ssl/server.pem");
    config->ssl_key = strdup(VARLIB_PATH"/ssl/server.key");
    config->ssl_san = strdup("");
    config->custom_cert = false;
    config->user = strdup("mympd");
    config->varlibdir = strdup(VARLIB_PATH);
    config->stickers = true;
    config->mixramp = false;
    config->taglist = strdup("Artist,Album,AlbumArtist,Title,Track,Genre,Date,Composer,Performer");
    config->searchtaglist = strdup("Artist,Album,AlbumArtist,Title,Genre,Composer,Performer");
    config->browsetaglist = strdup("Artist,Album,AlbumArtist,Genre,Composer,Performer");
    config->smartpls = true;
    config->max_elements_per_page = 100;
    config->last_played_count = 20;
    config->syscmds = false;
    config->loglevel = 2;
    config->love = false;
    config->love_channel = strdup("");
    config->love_message = strdup("love");
    config->plugins_coverextract = false;
    config->music_directory = strdup("auto");
    config->notification_web = false;
    config->notification_page = true;
    config->auto_play = false;
    config->jukebox_mode = JUKEBOX_OFF;
    config->jukebox_playlist = strdup("Database");
    config->jukebox_queue_length = 1;
    config->cols_queue_current = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_queue_last_played = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]");
    config->cols_search = strdup("[\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_browse_database = strdup("[\"Track\",\"Title\",\"Duration\"]");
    config->cols_browse_playlists_detail = strdup("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_browse_filesystem = strdup("[\"Type\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_playback = strdup("[\"Artist\",\"Album\"]");
    config->localplayer = false;
    config->localplayer_autoplay = false;
    config->stream_port = 8000;
    config->stream_url = strdup("");
    config->bg_cover = false;
    config->bg_color = strdup("#888");
    config->bg_css_filter = strdup("blur(5px)");
    config->coverimage = true;
    config->coverimage_name = strdup("folder.jpg");
    config->coverimage_size = 250;
    config->locale = strdup("default");
    config->varlibdir_len = strlen(config->varlibdir);    
    list_init(&config->syscmd_list);

    //get configuration file
    size_t configfile_len = strlen(ETC_PATH) + 12;
    char *configfile = malloc(configfile_len);
    snprintf(configfile, configfile_len, "%s/mympd.conf", ETC_PATH);
    
    char *option = NULL;
    
    if (argc >= 2) {
        if (strncmp(argv[1], "/", 1) == 0) {
            FREE_PTR(configfile);
            configfile = strdup(argv[1]);
            if (argc == 3) {
                option = strdup(argv[2]);
            }
        }
        else {
            option = strdup(argv[1]);
        }
    }

    if (option == NULL) {    
        LOG_INFO("Starting myMPD %s", MYMPD_VERSION);
        LOG_INFO("Libmpdclient %i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
        LOG_INFO("Mongoose %s", MG_VERSION);
    }
    
    if (access(configfile, F_OK ) != -1) {
        if (option == NULL) {
            LOG_INFO("Parsing config file: %s", configfile);
        }
        if (ini_parse(configfile, mympd_inihandler, config) < 0) {
            LOG_ERROR("Can't load config file %s", configfile);
            goto cleanup;
        }
    }
    else {
        if (option == NULL) {
            LOG_WARN("Config file %s not found, using defaults", configfile);
        }
    }

    //read environment - overwrites config file definitions
    mympd_get_env(config);

    //set correct path to certificate/key, if varlibdir is non default and cert paths are default
    if (strcmp(config->varlibdir, VARLIB_PATH) != 0 && config->custom_cert == false) {
        FREE_PTR(config->ssl_cert);
        size_t ssl_cert_len = config->varlibdir_len + 16;
        config->ssl_cert = malloc(ssl_cert_len);
        assert(config->ssl_cert);
        snprintf(config->ssl_cert, ssl_cert_len, "%s/ssl/server.pem", config->varlibdir);
        
        FREE_PTR(config->ssl_key);
        size_t ssl_key_len = config->varlibdir_len + 16;
        config->ssl_key = malloc(ssl_key_len);
        assert(config->ssl_key);
        snprintf(config->ssl_key, ssl_key_len, "%s/ssl/server.key", config->varlibdir);
    }
    
    //set loglevel
    set_loglevel(config);
    
    //check varlibdir
    testdir_rc = testdir("State directory", config->varlibdir, true);
    if (testdir_rc == 1 || testdir_rc == 0) {
        //directory exists or was created, set user and group 
        if (do_chown(config->varlibdir, config->user) != true) {
            goto cleanup;
        }
    }
    else if (testdir_rc > 1) {
        goto cleanup;
    }
    
    //handle commandline options and exit
    if (option != NULL) {
        handle_option(config, argv[0], option);
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
        snprintf(testdirname, 400, "%s/ssl", config->varlibdir);
        testdir_rc = testdir("SSL certificates", testdirname, true);
        if (testdir_rc == 1) {
            //directory created, create certificates
            if (!create_certificates(testdirname, config->ssl_san)) {
                //error creating certificates, remove directory
                LOG_ERROR("Certificate creation failed, cleanup directory %s", testdirname);
                cleanup_certificates(testdirname);
                //disable ssl
                config->ssl = false;
            }
        }
        else if (testdir_rc > 1) {
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
    if (getuid() == 0) {
        if (config->user != NULL || strlen(config->user) != 0) {
            LOG_INFO("Droping privileges to %s", config->user);
            struct passwd *pw;
            if ((pw = getpwnam(config->user)) == NULL) {
                LOG_ERROR("getpwnam() failed, unknown user");
                goto cleanup;
            } else if (setgroups(0, NULL) != 0) { 
                LOG_ERROR("setgroups() failed");
                goto cleanup;
            } else if (setgid(pw->pw_gid) != 0) {
                LOG_ERROR("setgid() failed");
                goto cleanup;
            } else if (setuid(pw->pw_uid) != 0) {
                LOG_ERROR("setuid() failed");
                goto cleanup;
            }
        }
    }
    
    if (getuid() == 0) {
        LOG_ERROR("myMPD should not be run with root privileges");
        goto cleanup;
    }

    //check for needed directories
    testdir_rc = testdir("Document root", DOC_ROOT, false);
    if (testdir_rc > 1) {
        goto cleanup;
    }

    snprintf(testdirname, 400, "%s/smartpls", config->varlibdir);
    testdir_rc = testdir("Smartpls dir", testdirname, true);
    if (testdir_rc == 1) {
        //directory created, create default smart playlists
        smartpls_default(config);
    }
    else if (testdir_rc > 1) {
        goto cleanup;
    }

    snprintf(testdirname, 400, "%s/state", config->varlibdir);
    testdir_rc = testdir("State dir", testdirname, true);
    if (testdir_rc > 1) {
        goto cleanup;
    }
    
    snprintf(testdirname, 400, "%s/pics", config->varlibdir);
    testdir_rc = testdir("Pics dir", testdirname, true);
    if (testdir_rc > 1) {
        goto cleanup;
    }
    
    if (config->plugins_coverextract == true) {
        snprintf(testdirname, 400, "%s/covercache", config->varlibdir);
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
    FREE_PTR(configfile);
    if (init_webserver) {
        FREE_PTR(mg_user_data->music_directory);
        FREE_PTR(mg_user_data->pics_directory);
        FREE_PTR(mg_user_data->rewrite_patterns);
    }
    FREE_PTR(mg_user_data);
    if (rc == EXIT_SUCCESS) {
        printf("Exiting gracefully, thank you for using myMPD\n");
    }
    end:
    return rc;
}
