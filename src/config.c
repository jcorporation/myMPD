/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "list.h"
#include "config_defs.h"
#include "utility.h"
#include "log.h"
#include "config.h"
#include "../dist/src/inih/ini.h"

//private functions
static int mympd_inihandler(void *user, const char *section, const char *name, const char* value);
static void mympd_parse_env(struct t_config *config, const char *envvar);
static void mympd_get_env(struct t_config *config);

static int mympd_inihandler(void *user, const char *section, const char *name, const char* value) {
    t_config* p_config = (t_config*)user;
    char *crap = NULL;

    #define MATCH(s, n) strcasecmp(section, s) == 0 && strcasecmp(name, n) == 0

    if (MATCH("mpd", "host")) {
        p_config->mpd_host = sdsreplace(p_config->mpd_host, value);
    }
    else if (MATCH("mpd", "port")) {
        p_config->mpd_port = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mpd", "pass")) {
        p_config->mpd_pass = sdsreplace(p_config->mpd_pass, value);
    }
    else if (MATCH("mpd", "musicdirectory")) {
        p_config->music_directory = sdsreplace(p_config->music_directory, value);
    }
    else if (MATCH("mpd", "regex")) {
        p_config->regex = strtobool(value);
    }
    else if (MATCH("webserver", "webport")) {
        p_config->webport = sdsreplace(p_config->webport, value);
    }
#ifdef ENABLE_SSL
    else if (MATCH("webserver", "ssl")) {
        p_config->ssl = strtobool(value);
    }
    else if (MATCH("webserver", "sslport")) {
        p_config->ssl_port = sdsreplace(p_config->ssl_port, value);
    }
    else if (MATCH("webserver", "sslcert")) {
        if (strcmp(p_config->ssl_cert, value) != 0) {
            p_config->custom_cert = true;
        }
        p_config->ssl_cert = sdsreplace(p_config->ssl_cert, value);
    }
    else if (MATCH("webserver", "sslkey")) {
        if (strcmp(p_config->ssl_key, value) != 0) {
            p_config->custom_cert = true;
        }
        p_config->ssl_key = sdsreplace(p_config->ssl_key, value);
    }
    else if (MATCH("webserver", "sslsan")) {
        p_config->ssl_san = sdsreplace(p_config->ssl_san, value);
    }
#endif
    else if (MATCH("webserver", "publishlibrary")) {
        p_config->publish_library = strtobool(value);
    }
    else if (MATCH("mympd", "user")) {
        p_config->user = sdsreplace(p_config->user, value);
    }
    else if (MATCH("mympd", "chroot")) {
        p_config->chroot = strtobool(value);
    }
    else if (MATCH("mympd", "varlibdir")) {
        p_config->varlibdir = sdsreplace(p_config->varlibdir, value);
    }
    else if (MATCH("mympd", "stickers")) {
        p_config->stickers = strtobool(value);
    }
    else if (MATCH("mympd", "smartpls")) {
        p_config->smartpls =  strtobool(value);
    }
    else if (MATCH("mympd", "mixramp")) {
        p_config->mixramp = strtobool(value);
    }
    else if (MATCH("mympd", "taglist")) {
        p_config->taglist = sdsreplace(p_config->taglist, value);
    }
    else if (MATCH("mympd", "searchtaglist")) {
        p_config->searchtaglist = sdsreplace(p_config->searchtaglist, value);
    }
    else if (MATCH("mympd", "browsetaglist")) {
        p_config->browsetaglist = sdsreplace(p_config->browsetaglist, value);
    }
    else if (MATCH("mympd", "pagination")) {
        p_config->max_elements_per_page = strtoimax(value, &crap, 10);
        if (p_config->max_elements_per_page > 1000) {
            LOG_WARN("Setting max_elements_per_page to maximal value 1000");
            p_config->max_elements_per_page = 1000;
        }
    }
    else if (MATCH("mympd", "volumestep")) {
        p_config->volume_step = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "covercachekeepdays")) {
        p_config->covercache_keep_days = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "covercache")) {
        p_config->covercache = strtobool(value);
    }
    else if (MATCH("mympd", "syscmds")) {
        p_config->syscmds = strtobool(value);
    }
    else if (MATCH("mympd", "lastplayedcount")) {
        p_config->last_played_count = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "loglevel")) {
        p_config->loglevel = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "love")) {
        p_config->love = strtobool(value);
    }
    else if (MATCH("mympd", "lovechannel")) {
        p_config->love_channel = sdsreplace(p_config->love_channel, value);
    }
    else if (MATCH("mympd", "lovemessage")) {
        p_config->love_message = sdsreplace(p_config->love_message, value);
    }
    else if (MATCH("mympd", "notificationweb")) {
        p_config->notification_web = strtobool(value);
    }
    else if (MATCH("mympd", "notificationpage")) {
        p_config->notification_page = strtobool(value);
    }
    else if (MATCH("mympd", "autoplay")) {
        p_config->auto_play = strtobool(value);
    }
    else if (MATCH("mympd", "jukeboxmode")) {
        p_config->jukebox_mode = strtoimax(value, &crap, 10);
        if (p_config->jukebox_mode < 0 || p_config->jukebox_mode > 2) {
            LOG_WARN("Invalid jukeboxmode %d", p_config->jukebox_mode);
            p_config->jukebox_mode = JUKEBOX_OFF;
        }
    }
    else if (MATCH("mympd", "jukeboxplaylist")) {
        p_config->jukebox_playlist = sdsreplace(p_config->jukebox_playlist, value);
    }
    else if (MATCH("mympd", "jukeboxqueuelength")) {
        p_config->jukebox_queue_length = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "colsqueuecurrent")) {
        p_config->cols_queue_current = sdsreplace(p_config->cols_queue_current, value);
    }
    else if (MATCH("mympd", "colssearch")) {
        p_config->cols_search = sdsreplace(p_config->cols_search, value);
    }
    else if (MATCH("mympd", "colsbrowsedatabase")) {
        p_config->cols_browse_database = sdsreplace(p_config->cols_browse_database, value);
    }
    else if (MATCH("mympd", "colsbrowseplaylistsdetail")) {
        p_config->cols_browse_playlists_detail = sdsreplace(p_config->cols_browse_playlists_detail, value);
    }
    else if (MATCH("mympd", "colsbrowsefilesystem")) {
        p_config->cols_browse_filesystem = sdsreplace(p_config->cols_browse_filesystem, value);
    }
    else if (MATCH("mympd", "colsplayback")) {
        p_config->cols_playback = sdsreplace(p_config->cols_playback, value);
    }
    else if (MATCH("mympd", "colsqueuelastplayed")) {
        p_config->cols_queue_last_played = sdsreplace(p_config->cols_queue_last_played, value);
    }
    else if (MATCH("mympd", "localplayer")) {
        p_config->localplayer = strtobool(value);
    }
    else if (MATCH("mympd", "localplayerautoplay")) {
        p_config->localplayer_autoplay = strtobool(value);
    }
    else if (MATCH("mympd", "streamport")) {
        p_config->stream_port = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "streamurl")) {
        p_config->stream_url = sdsreplace(p_config->stream_url, value);
    }
    else if (MATCH("mympd", "readonly")) {
        p_config->readonly = strtobool(value);
    }
    else if (MATCH("mympd", "bookmarks")) {
        p_config->bookmarks = strtobool(value);
    }
    else if (MATCH("theme", "theme")) {
        p_config->theme = sdsreplace(p_config->theme, value);
    }
    else if (MATCH("theme", "bgcover")) {
        p_config->bg_cover = strtobool(value);
    }
    else if (MATCH("theme", "bgcolor")) {
        p_config->bg_color = sdsreplace(p_config->bg_color, value);
    }
    else if (MATCH("theme", "bgcssfilter")) {
        p_config->bg_css_filter = sdsreplace(p_config->bg_css_filter, value);
    }
    else if (MATCH("theme", "coverimage")) {
        p_config->coverimage = strtobool(value);
    }
    else if (MATCH("theme", "coverimagename")) {
        p_config->coverimage_name = sdsreplace(p_config->coverimage_name, value);
    }
    else if (MATCH("theme", "coverimagesize")) {
        p_config->coverimage_size = strtoimax(value, &crap, 10);
    }
    else if (MATCH("theme", "covergridsize")) {
        p_config->covergrid_size = strtoimax(value, &crap, 10);
    }
    else if (MATCH("theme", "locale")) {
        p_config->locale = sdsreplace(p_config->locale, value);
    }
    else if (MATCH("theme", "custom_placeholder_images")) {
        p_config->custom_placeholder_images = strtobool(value);
    }
    else if (strcasecmp(section, "syscmds") == 0) {
        LOG_DEBUG("Adding syscmd %s: %s", name, value);
        list_push(&p_config->syscmd_list, name, 0, value);
    }
    else {
        LOG_WARN("Unkown config option: %s - %s", section, name);
        return 0;  
    }
    return 1;
}

static void mympd_parse_env(struct t_config *config, const char *envvar) {
    char *name = NULL;
    char *section = NULL;
    const char *value = getenv(envvar); /* Flawfinder: ignore */
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
        "MPD_REGEX", "WEBSERVER_WEBPORT", "WEBSERVER_PUBLISHLIBRARY",
      #ifdef ENABLE_SSL
        "WEBSERVER_SSL", "WEBSERVER_SSLPORT", "WEBSERVER_SSLCERT", "WEBSERVER_SSLKEY",
        "WEBSERVER_SSLSAN", 
      #endif
        "MYMPD_LOGLEVEL", "MYMPD_USER", "MYMPD_VARLIBDIR", "MYMPD_MIXRAMP", "MYMPD_STICKERS", "MYMPD_TAGLIST", 
        "MYMPD_SEARCHTAGLIST", "MYMPD_BROWSETAGLIST", "MYMPD_SMARTPLS", "MYMPD_SYSCMDS", 
        "MYMPD_PAGINATION", "MYMPD_LASTPLAYEDCOUNT", "MYMPD_LOVE", "MYMPD_LOVECHANNEL", "MYMPD_LOVEMESSAGE",
        "MYMPD_NOTIFICATIONWEB", "MYMPD_CHROOT", "MYMPD_READONLY",
        "MYMPD_NOTIFICATIONPAGE", "MYMPD_AUTOPLAY", "MYMPD_JUKEBOXMODE", "MYMPD_BOOKMARKS",
        "MYMPD_JUKEBOXPLAYLIST", "MYMPD_JUKEBOXQUEUELENGTH", "MYMPD_COLSQUEUECURRENT",
        "MYMPD_COLSSEARCH", "MYMPD_COLSBROWSEDATABASE", "MYMPD_COLSBROWSEPLAYLISTDETAIL",
        "MYMPD_COLSBROWSEFILESYSTEM", "MYMPD_COLSPLAYBACK", "MYMPD_COLSQUEUELASTPLAYED",
        "MYMPD_LOCALPLAYER", "MYMPD_LOCALPLAYERAUTOPLAY", "MYMPD_STREAMPORT",
        "MYMPD_STREAMURL", "MYMPD_VOLUMESTEP", "MYMPD_COVERCACHEKEEPDAYS", "MYMPD_COVERCACHE",
        "MYMPD_COVERCACHEAVOID", "THEME_THEME", "THEME_CUSTOMPLACEHOLDERIMAGES",
        "THEME_BGCOVER", "THEME_BGCOLOR", "THEME_BGCSSFILTER", "THEME_COVERGRIDSIZE",
        "THEME_COVERIMAGE", "THEME_COVERIMAGENAME", "THEME_COVERIMAGESIZE",
        "THEME_LOCALE", 0};
    const char** ptr = env_vars;
    while (*ptr != 0) {
        mympd_parse_env(config, *ptr);
        ++ptr;
    }
}

//global functions
void mympd_free_config(t_config *config) {
    sdsfree(config->mpd_host);
    sdsfree(config->mpd_pass);
    sdsfree(config->webport);
#ifdef ENABLE_SSL
    sdsfree(config->ssl_port);
    sdsfree(config->ssl_cert);
    sdsfree(config->ssl_key);
    sdsfree(config->ssl_san);
#endif
    sdsfree(config->user);
    sdsfree(config->taglist);
    sdsfree(config->searchtaglist);
    sdsfree(config->browsetaglist);
    sdsfree(config->varlibdir);
    sdsfree(config->love_channel);
    sdsfree(config->love_message);
    sdsfree(config->music_directory);
    sdsfree(config->jukebox_playlist);
    sdsfree(config->cols_queue_current);
    sdsfree(config->cols_queue_last_played);
    sdsfree(config->cols_search);
    sdsfree(config->cols_browse_database);
    sdsfree(config->cols_browse_playlists_detail);
    sdsfree(config->cols_browse_filesystem);
    sdsfree(config->cols_playback);
    sdsfree(config->stream_url);
    sdsfree(config->bg_color);
    sdsfree(config->bg_css_filter);
    sdsfree(config->coverimage_name);
    sdsfree(config->locale);
    sdsfree(config->theme);
    list_free(&config->syscmd_list);
    FREE_PTR(config);
}

void mympd_config_defaults(t_config *config) {
    config->mpd_host = sdsnew("127.0.0.1");
    config->mpd_port = 6600;
    config->mpd_pass = sdsempty();
    config->webport = sdsnew("80");
#ifdef ENABLE_SSL
    config->ssl = true;
    config->ssl_port = sdsnew("443");
    config->ssl_cert = sdsnew(VARLIB_PATH"/ssl/server.pem");
    config->ssl_key = sdsnew(VARLIB_PATH"/ssl/server.key");
    config->ssl_san = sdsempty();
    config->custom_cert = false;
#endif
    config->user = sdsnew("mympd");
    config->chroot = false;
    config->varlibdir = sdsnew(VARLIB_PATH);
    config->stickers = true;
    config->mixramp = false;
    config->taglist = sdsnew("Artist,Album,AlbumArtist,Title,Track,Genre,Date,Composer,Performer");
    config->searchtaglist = sdsnew("Artist,Album,AlbumArtist,Title,Genre,Composer,Performer");
    config->browsetaglist = sdsnew("Artist,Album,AlbumArtist,Genre,Composer,Performer");
    config->smartpls = true;
    config->max_elements_per_page = 100;
    config->last_played_count = 20;
    config->syscmds = false;
    config->loglevel = 2;
    config->love = false;
    config->love_channel = sdsempty();
    config->love_message = sdsnew("love");
    config->music_directory = sdsnew("auto");
    config->notification_web = false;
    config->notification_page = true;
    config->auto_play = false;
    config->jukebox_mode = JUKEBOX_OFF;
    config->jukebox_playlist = sdsnew("Database");
    config->jukebox_queue_length = 1;
    config->cols_queue_current = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_queue_last_played = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]");
    config->cols_search = sdsnew("[\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_browse_database = sdsnew("[\"Track\",\"Title\",\"Duration\"]");
    config->cols_browse_playlists_detail = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_browse_filesystem = sdsnew("[\"Type\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    config->cols_playback = sdsnew("[\"Artist\",\"Album\"]");
    config->localplayer = false;
    config->localplayer_autoplay = false;
    config->stream_port = 8000;
    config->stream_url = sdsempty();
    config->bg_cover = false;
    config->bg_color = sdsnew("#888");
    config->bg_css_filter = sdsnew("blur(5px)");
    config->coverimage = true;
    config->coverimage_name = sdsnew("folder, cover");
    config->coverimage_size = 250;
    config->covergrid_size = 200;
    config->locale = sdsnew("default");
    config->startup_time = time(NULL);
    config->readonly = false;
    config->bookmarks = true;
    config->volume_step = 5;
    config->publish_library = true;
    config->covercache_keep_days = 7;
    config->covercache = true;
    config->theme = sdsnew("theme-default");
    config->custom_placeholder_images = false;
    config->regex = true;
    list_init(&config->syscmd_list);
}

bool mympd_read_config(t_config *config, sds configfile) {
    LOG_INFO("Parsing config file: %s", configfile);
    if (ini_parse(configfile, mympd_inihandler, config) < 0) {
        LOG_WARN("Can't parse config file %s, using defaults", configfile);
    }
    //read environment - overwrites config file definitions
    mympd_get_env(config);

    //set correct path to certificate/key, if varlibdir is non default and cert paths are default
    #ifdef ENABLE_SSL
    if (strcmp(config->varlibdir, VARLIB_PATH) != 0 && config->custom_cert == false) {
        config->ssl_cert = sdscrop(config->ssl_cert);
        config->ssl_cert = sdscatfmt(config->ssl_cert, "%s/ssl/server.pem", config->varlibdir);
        config->ssl_key = sdscrop(config->ssl_key);
        config->ssl_key = sdscatfmt(config->ssl_key, "%s/ssl/server.key", config->varlibdir);
    }
    #endif
    if (config->readonly == true) {
        mympd_set_readonly(config);
    }

    return true;
}

void mympd_set_readonly(t_config *config) {
    LOG_INFO("Entering readonly mode");
    config->readonly = true;
    if (config->bookmarks == true) {
        LOG_INFO("Disabling bookmarks");
        config->bookmarks = false;
    }
    if (config->smartpls == true) {
        LOG_INFO("Disabling smart playlists");
        config->smartpls = false;
    }
    if (config->covercache == true) {
        LOG_INFO("Disabling covercache");
        config->covercache = false;
    }
}
