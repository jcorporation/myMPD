/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
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
    else if (MATCH("mpd", "playlistdirectory")) {
        p_config->playlist_directory = sdsreplace(p_config->playlist_directory, value);
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
    else if (MATCH("webserver", "publish")) {
        p_config->publish = strtobool(value);
    }
    else if (MATCH("webserver", "webdav")) {
        p_config->webdav = strtobool(value);
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
    else if (MATCH("mympd", "stickercache")) {
        p_config->sticker_cache = strtobool(value);
    }
    else if (MATCH("mympd", "smartpls")) {
        p_config->smartpls =  strtobool(value);
    }
    else if (MATCH("mympd", "smartplssort")) {
        p_config->smartpls_sort =  sdsreplace(p_config->smartpls_sort, value);
    }
    else if (MATCH("mympd", "smartplsprefix")) {
        p_config->smartpls_prefix =  sdsreplace(p_config->smartpls_prefix, value);
    }
    else if (MATCH("mympd", "smartplsinterval")) {
        p_config->smartpls_interval =  strtoumax(value, &crap, 10);
    }
    else if (MATCH("mympd", "generateplstags")) {
        p_config->generate_pls_tags =  sdsreplace(p_config->generate_pls_tags, value);
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
    else if (MATCH("mympd", "timer")) {
        p_config->timer = strtobool(value);
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
    else if (MATCH("mympd", "mediasession")) {
        p_config->media_session = strtobool(value);
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
    else if (MATCH("mympd", "jukeboxlastplayed")) {
        p_config->jukebox_last_played = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "jukeboxuniquetag")) {
        p_config->jukebox_unique_tag = sdsreplace(p_config->jukebox_unique_tag, value);
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
    else if (MATCH("theme", "highlightcolor")) {
        p_config->highlight_color = sdsreplace(p_config->highlight_color, value);
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
    else if (MATCH("theme", "customplaceholderimages")) {
        p_config->custom_placeholder_images = strtobool(value);
    }
    else if (strcasecmp(section, "syscmds") == 0) {
        LOG_DEBUG("Adding syscmd %s: %s", name, value);
        list_push(&p_config->syscmd_list, name, 0, value, NULL);
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
        "MPD_PLAYLISTDIRECTORY", "MPD_REGEX", "WEBSERVER_WEBPORT", "WEBSERVER_PUBLISH",
        "WEBSERVER_WEBDAV",
      #ifdef ENABLE_SSL
        "WEBSERVER_SSL", "WEBSERVER_SSLPORT", "WEBSERVER_SSLCERT", "WEBSERVER_SSLKEY",
        "WEBSERVER_SSLSAN", 
      #endif
        "MYMPD_LOGLEVEL", "MYMPD_USER", "MYMPD_VARLIBDIR", "MYMPD_MIXRAMP", "MYMPD_STICKERS", 
        "MYMPD_STICKERCACHE", "MYMPD_TAGLIST", "MYMPD_GENERATE_PLS_TAGS",
        "MYMPD_SMARTPLSSORT", "MYMPD_SMARTPLSPREFIX", "MYMPD_SMARTPLSINTERVAL",
        "MYMPD_SEARCHTAGLIST", "MYMPD_BROWSETAGLIST", "MYMPD_SMARTPLS", "MYMPD_SYSCMDS", 
        "MYMPD_PAGINATION", "MYMPD_LASTPLAYEDCOUNT", "MYMPD_LOVE", "MYMPD_LOVECHANNEL", "MYMPD_LOVEMESSAGE",
        "MYMPD_NOTIFICATIONWEB", "MYMPD_CHROOT", "MYMPD_READONLY", "MYMPD_TIMER",
        "MYMPD_NOTIFICATIONPAGE", "MYMPD_AUTOPLAY", "MYMPD_JUKEBOXMODE", "MYMPD_BOOKMARKS",
        "MYMPD_MEDIASESSION",
        "MYMPD_JUKEBOXPLAYLIST", "MYMPD_JUKEBOXQUEUELENGTH", "MYMPD_JUKEBOXLASTPLAYED",
        "MYMPD_JUKEBOXUNIQUETAG", "MYMPD_COLSQUEUECURRENT","MYMPD_COLSSEARCH", 
        "MYMPD_COLSBROWSEDATABASE", "MYMPD_COLSBROWSEPLAYLISTDETAIL",
        "MYMPD_COLSBROWSEFILESYSTEM", "MYMPD_COLSPLAYBACK", "MYMPD_COLSQUEUELASTPLAYED",
        "MYMPD_LOCALPLAYER", "MYMPD_LOCALPLAYERAUTOPLAY", "MYMPD_STREAMPORT",
        "MYMPD_STREAMURL", "MYMPD_VOLUMESTEP", "MYMPD_COVERCACHEKEEPDAYS", "MYMPD_COVERCACHE",
        "MYMPD_COVERCACHEAVOID", "THEME_THEME", "THEME_CUSTOMPLACEHOLDERIMAGES",
        "THEME_BGCOVER", "THEME_BGCOLOR", "THEME_BGCSSFILTER", "THEME_COVERGRIDSIZE",
        "THEME_COVERIMAGE", "THEME_COVERIMAGENAME", "THEME_COVERIMAGESIZE",
        "THEME_LOCALE", "THEME_HIGHLIGHTCOLOR", 0};
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
    sdsfree(config->playlist_directory);
    sdsfree(config->jukebox_playlist);
    sdsfree(config->jukebox_unique_tag);
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
    sdsfree(config->highlight_color);
    sdsfree(config->generate_pls_tags);
    sdsfree(config->smartpls_sort);
    sdsfree(config->smartpls_prefix);
    list_free(&config->syscmd_list);
    FREE_PTR(config);
}

void mympd_config_defaults(t_config *config) {
    config->mpd_host = sdsnew("/var/run/mpd/socket");
    config->mpd_port = 6600;
    config->mpd_pass = sdsempty();
    config->music_directory = sdsnew("auto");
    config->playlist_directory = sdsnew("/var/lib/mpd/playlists");
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
    config->taglist = sdsnew("Artist, Album, AlbumArtist, Title, Track, Genre, Date");
    config->searchtaglist = sdsnew("Artist, Album, AlbumArtist, Title, Genre");
    config->browsetaglist = sdsnew("Artist, Album, AlbumArtist, Genre");
    config->smartpls = true;
    config->smartpls_sort = sdsempty();
    config->smartpls_prefix = sdsnew("myMPDsmart");
    config->smartpls_interval = 14400;
    config->generate_pls_tags = sdsnew("Genre");
    config->max_elements_per_page = 100;
    config->last_played_count = 20;
    config->syscmds = false;
    config->loglevel = 2;
    config->love = false;
    config->love_channel = sdsempty();
    config->love_message = sdsnew("love");
    config->notification_web = false;
    config->notification_page = true;
    config->media_session = true;
    config->auto_play = false;
    config->jukebox_mode = JUKEBOX_OFF;
    config->jukebox_playlist = sdsnew("Database");
    config->jukebox_queue_length = 1;
    config->jukebox_unique_tag = sdsnew("Title");
    config->jukebox_last_played = 24;
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
    config->publish = false;
    config->webdav = false;
    config->covercache_keep_days = 7;
    config->covercache = true;
    config->theme = sdsnew("theme-default");
    config->highlight_color = sdsnew("#28a745");
    config->custom_placeholder_images = false;
    config->regex = true;
    config->timer = true;
    config->sticker_cache = true;
    list_init(&config->syscmd_list);
}

bool mympd_dump_config(void) {
    t_config *p_config = (t_config *)malloc(sizeof(t_config));
    assert(p_config);
    mympd_config_defaults(p_config);

    sds tmp_file = sdscatfmt(sdsempty(), "/tmp/mympd.conf.XXXXXX");
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    
    fprintf(fp, "[mpd]\n"
        "host = %s\n"
        "port = %d\n"
        "#pass = \n"
        "musicdirectory = %s\n"
        "playlistdirectory = %s\n"
        "regex = %s\n\n",
        p_config->mpd_host,
        p_config->mpd_port,
        p_config->music_directory,
        p_config->playlist_directory,
        (p_config->regex == true ? "true" : "false")
    );
    
    fprintf(fp, "[webserver]\n"
        "webport = %s\n"
    #ifdef ENABLE_SSL
        "ssl = %s\n"
        "sslport = %s\n"
        "sslcert = %s\n"
        "sslkey = %s\n"
        "sslsan = %s\n"
    #endif
        "publish = %s\n"
        "webdav = %s\n\n",
        p_config->webport,
    #ifdef ENABLE_SSL
        (p_config->ssl == true ? "true" : "false"),
        p_config->ssl_port,
        p_config->ssl_cert,
        p_config->ssl_key,
        p_config->ssl_san,
    #endif
        (p_config->publish == true ? "true" : "false"),
        (p_config->webdav == true ? "true" : "false")
    );

    fprintf(fp, "[mympd]\n"
        "user = %s\n"
        "chroot = %s\n"
        "varlibdir = %s\n"
        "stickers = %s\n"
        "stickercache = %s\n"
        "smartpls = %s\n"
        "smartplssort = %s\n"
        "smartplsprefix = %s\n"
        "smartplsinterval = %ld\n"
        "generateplstags = %s\n"
        "mixramp = %s\n"
        "taglist = %s\n"
        "searchtaglist = %s\n"
        "browsetaglist = %s\n"
        "pagination = %d\n"
        "volumestep = %d\n"
        "covercachekeepdays = %d\n"
        "covercache = %s\n"
        "syscmds = %s\n"
        "timer = %s\n"
        "lastplayedcount = %d\n"
        "loglevel = %d\n"
        "love = %s\n"
        "lovechannel = %s\n"
        "lovemessage = %s\n"
        "notificationweb = %s\n"
        "notificationpage = %s\n"
        "mediasession = %s\n"
        "autoplay = %s\n"
        "jukeboxmode = %d\n"
        "jukeboxplaylist = %s\n"
        "jukeboxqueuelength = %d\n"
        "jukeboxlastplayed = %d\n"
        "jukeboxuniquetag = %s\n"
        "colsqueuecurrent = %s\n"
        "colsqueuelastplayed = %s\n"
        "colssearch = %s\n"
        "colsbrowsedatabase = %s\n"
        "colsbrowseplaylistsdetail = %s\n"
        "colsbrowsefilesystem = %s\n"
        "colsplayback = %s\n"
        "localplayer = %s\n"
        "localplayerautoplay = %s\n"
        "streamport = %d\n"
        "#streamuri = %s\n"
        "readonly = %s\n"
        "bookmarks = %s\n\n",
        p_config->user,
        (p_config->chroot == true ? "true" : "false"),
        p_config->varlibdir,
        (p_config->stickers == true ? "true" : "false"),
        (p_config->sticker_cache == true ? "true" : "false"),
        (p_config->smartpls == true ? "true" : "false"),
        p_config->smartpls_sort,
        p_config->smartpls_prefix,
        p_config->smartpls_interval,
        p_config->generate_pls_tags,
        (p_config->mixramp == true ? "true" : "false"),
        p_config->taglist,
        p_config->searchtaglist,
        p_config->browsetaglist,
        p_config->max_elements_per_page,
        p_config->volume_step,
        p_config->covercache_keep_days,
        (p_config->covercache == true ? "true" : "false"),
        (p_config->syscmds == true ? "true" : "false"),
        (p_config->timer == true ? "true" : "false"),
        p_config->last_played_count,
        p_config->loglevel,
        (p_config->love == true ? "true" : "false"),
        p_config->love_channel,
        p_config->love_message,
        (p_config->notification_web == true ? "true" : "false"),
        (p_config->notification_page == true ? "true" : "false"),
        (p_config->media_session == true ? "true" : "false"),
        (p_config->auto_play == true ? "true" : "false"),
        p_config->jukebox_mode,
        p_config->jukebox_playlist,
        p_config->jukebox_queue_length,
        p_config->jukebox_last_played,
        p_config->jukebox_unique_tag,
        p_config->cols_queue_current,
        p_config->cols_queue_last_played,
        p_config->cols_search,
        p_config->cols_browse_database,
        p_config->cols_browse_playlists_detail,
        p_config->cols_browse_filesystem,
        p_config->cols_playback,
        (p_config->localplayer == true ? "true" : "false"),
        (p_config->localplayer_autoplay == true ? "true" : "false"),
        p_config->stream_port,
        p_config->stream_url,
        (p_config->readonly == true ? "true" : "false"),
        (p_config->bookmarks == true ? "true" : "false")
    );

    fprintf(fp, "[theme]\n"
        "theme = %s\n"
        "bgcover = %s\n"
        "bgcolor = %s\n"
        "bgcssfilter = %s\n"
        "coverimage = %s\n"
        "coverimagename = %s\n"
        "coverimagesize = %d\n"
        "covergridsize = %d\n"
        "locale = %s\n"
        "customplaceholderimages = %s\n"
        "highlightcolor = %s\n\n",
        p_config->theme,
        (p_config->bg_cover == true ? "true" : "false"),
        p_config->bg_color,
        p_config->bg_css_filter,
        (p_config->coverimage == true ? "true" : "false"),
        p_config->coverimage_name,
        p_config->coverimage_size,
        p_config->covergrid_size,
        p_config->locale,
        (p_config->custom_placeholder_images == true ? "true" : "false"),
        p_config->highlight_color
    );

    fprintf(fp, "[syscmds]\n");

    fclose(fp);
    sds conf_file = sdscatfmt(sdsempty(), "/tmp/mympd.conf");
    int rc = rename(tmp_file, conf_file);
    if (rc == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, conf_file);
    }
    sdsfree(tmp_file);
    sdsfree(conf_file);
    mympd_free_config(p_config);
    if (rc == -1) {
        return false;
    }
    printf("Default configuration dumped to /tmp/mympd.conf");
    return true;    
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
    if (config->stickers == false && config->sticker_cache == true) {
        LOG_INFO("Stickers are disabled, disabling sticker cache");
        config->sticker_cache = false;
    }
    if (config->publish == false && config->webdav == true) {
        LOG_INFO("Publish is disabled, disabling webdav");
        config->webdav = false;
    }

    if (config->chroot == true && config->syscmds == true) {
        LOG_INFO("Chroot enabled, disabling syscmds");
        config->syscmds = false;
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
