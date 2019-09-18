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
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
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
        p_config->mpd_host = sdscat(sdsempty(), value);
    }
    else if (MATCH("mpd", "port")) {
        p_config->mpd_port = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mpd", "pass")) {
        p_config->mpd_pass = sdscat(sdsempty(), value);
    }
    else if (MATCH("mpd", "musicdirectory")) {
        p_config->music_directory = sdscat(sdsempty(), value);
    }
    else if (MATCH("webserver", "webport")) {
        p_config->webport = sdscat(sdsempty(), value);
    }
    else if (MATCH("webserver", "ssl")) {
        p_config->ssl = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("webserver", "sslport")) {
        p_config->ssl_port = sdscat(sdsempty(), value);
    }
    else if (MATCH("webserver", "sslcert")) {
        if (strcmp(p_config->ssl_cert, value) != 0) {
            p_config->custom_cert = true;
        }
        p_config->ssl_cert = sdscat(sdsempty(), value);
    }
    else if (MATCH("webserver", "sslkey")) {
        if (strcmp(p_config->ssl_key, value) != 0) {
            p_config->custom_cert = true;
        }
        p_config->ssl_key = sdscat(sdsempty(), value);
    }
    else if (MATCH("webserver", "sslsan")) {
        p_config->ssl_san = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "user")) {
        p_config->user = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "chroot")) {
        p_config->chroot = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "varlibdir")) {
        p_config->varlibdir = sdscat(sdsempty(), value);
        p_config->varlibdir_len = sdslen(p_config->varlibdir);
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
        p_config->taglist = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "searchtaglist")) {
        p_config->searchtaglist = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "browsetaglist")) {
        p_config->browsetaglist = sdscat(sdsempty(), value);
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
        p_config->love_channel = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "lovemessage")) {
        p_config->love_message = sdscat(sdsempty(), value);
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
        p_config->jukebox_playlist = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "jukeboxqueuelength")) {
        p_config->jukebox_queue_length = strtoimax(value, &crap, 10);
    }
    else if (MATCH("mympd", "colsqueuecurrent")) {
        p_config->cols_queue_current = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "colssearch")) {
        p_config->cols_search = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "colsbrowsedatabase")) {
        p_config->cols_browse_database = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "colsbrowseplaylistsdetail")) {
        p_config->cols_browse_playlists_detail = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "colsbrowsefilesystem")) {
        p_config->cols_browse_filesystem = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "colsplayback")) {
        p_config->cols_playback = sdscat(sdsempty(), value);
    }
    else if (MATCH("mympd", "colsqueuelastplayed")) {
        p_config->cols_queue_last_played = sdscat(sdsempty(), value);
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
        p_config->stream_url = sdscat(sdsempty(), value);
    }
    else if (MATCH("theme", "bgcover")) {
        p_config->bg_cover = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("theme", "bgcolor")) {
        p_config->bg_color = sdscat(sdsempty(), value);
    }
    else if (MATCH("theme", "bgcssfilter")) {
        FREE_PTR(p_config->bg_css_filter);
        p_config->bg_css_filter = sdscat(sdsempty(), value);
    }
    else if (MATCH("theme", "coverimage")) {
        p_config->coverimage = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("theme", "coverimagename")) {
        p_config->coverimage_name = sdscat(sdsempty(), value);
    }
    else if (MATCH("theme", "coverimagesize")) {
        p_config->coverimage_size = strtoimax(value, &crap, 10);
    }
    else if (MATCH("theme", "locale")) {
        p_config->locale = sdscat(sdsempty(), value);
    }
    else if (strcasecmp(section, "syscmds") == 0) {
        sds syscmd_name = sdsnew(name);
        sds syscmd_cmd = sdsnew(value);
        list_push(&p_config->syscmd_list, syscmd_name, 0, (void *)syscmd_cmd);
        sds_free(syscmd_name);
        sds_free(syscmd_cmd);
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
        "PLUGINS_COVEREXTRACT", "MYMPD_NOTIFICATIONWEB", "MYMPD_CHROOT",
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

//global functions
void mympd_free_config(t_config *config) {
    sds_free(config->mpd_host);
    sds_free(config->mpd_pass);
    sds_free(config->webport);
    sds_free(config->ssl_port);
    sds_free(config->ssl_cert);
    sds_free(config->ssl_key);
    sds_free(config->ssl_san);
    sds_free(config->user);
    sds_free(config->taglist);
    sds_free(config->searchtaglist);
    sds_free(config->browsetaglist);
    sds_free(config->varlibdir);
    sds_free(config->love_channel);
    sds_free(config->love_message);
    sds_free(config->music_directory);
    sds_free(config->jukebox_playlist);
    sds_free(config->cols_queue_current);
    sds_free(config->cols_queue_last_played);
    sds_free(config->cols_search);
    sds_free(config->cols_browse_database);
    sds_free(config->cols_browse_playlists_detail);
    sds_free(config->cols_browse_filesystem);
    sds_free(config->cols_playback);
    sds_free(config->stream_url);
    sds_free(config->bg_color);
    sds_free(config->bg_css_filter);
    sds_free(config->coverimage_name);
    sds_free(config->locale);
    list_free(&config->syscmd_list);
    FREE_PTR(config);
}

void mympd_config_defaults(t_config *config) {
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
    config->chroot = false;
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
}

bool mympd_read_config(t_config *config, sds configfile) {
    if (access(configfile, F_OK ) != -1) {
        LOG_INFO("Parsing config file: %s", configfile);
        if (ini_parse(configfile, mympd_inihandler, config) < 0) {
            LOG_ERROR("Can't load config file %s", configfile);
            return false;
        }
    }
    else {
        LOG_WARN("Config file %s not found, using defaults", configfile);
    }
    //read environment - overwrites config file definitions
    mympd_get_env(config);

    //set correct path to certificate/key, if varlibdir is non default and cert paths are default
    if (strcmp(config->varlibdir, VARLIB_PATH) != 0 && config->custom_cert == false) {
        config->ssl_cert = sdscatprintf(sdsempty(), "%s/ssl/server.pem", config->varlibdir);
        config->ssl_key = sdscatprintf(sdsempty(), "%s/ssl/server.key", config->varlibdir);
    }

    return true;
}
