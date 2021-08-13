/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_migrate.h"

#include "../dist/src/frozen/frozen.h"
#include "lib/sds_extras.h"
#include "lib/log.h"
#include "lib/state_files.h"
#include "lib/utility.h"
#include "mpd_shared/mpd_shared_playlists.h"
#include "mpd_shared/mpd_shared_search.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private functions
static bool is_old_config(const char *workdir);
void parse_ini_line (const char *line, sds *key, sds *value);
static bool migrate_smartpls_files(const char *workdir);
static bool migrate_smartpls_file(const char *workdir, const char *playlist);
static void migrate_state_files(const char *workdir);
static void rename_file(const char *workdir, const char *src, const char *dst);

//public functions
void start_migrate_conf(const char *workdir) {
    if (is_old_config(workdir) == false) {
        return;
    }
    MYMPD_LOG_INFO("Detected old configuration, migrating mympd.conf");
    //find and open mympd.conf
    FILE *fp = fopen("/etc/mympd.conf", "r");
    if (fp == NULL) {
        fp = fopen("/etc/webapps/mympd/mympd.conf", "r");
    }
    if (fp == NULL) {
        fp = fopen("/etc/opt/mympd.conf", "r");
    }
    if (fp == NULL) {
        return;
    }
    //parse mympd.conf and write config files
    sds key = sdsempty();
    sds value = sdsempty();
    char *line = NULL;
    size_t n = 0;
    while (getline(&line, &n, fp) > 0) {
        sdsclear(key);
        sdsclear(value);
        parse_ini_line(line, &key, &value);
        if (strcmp(key, "httphost") == 0) { 
            state_file_write(workdir, "config", "http_host", value);
        }
        else if (strcmp(key, "httpport") == 0) { 
            state_file_write(workdir, "config", "http_port", value);
        }
        else if (strcmp(key, "ssl") == 0) { 
            state_file_write(workdir, "config", "ssl", value);
        }
        else if (strcmp(key, "sslport") == 0) { 
            state_file_write(workdir, "config", "ssl_port", value);
        }
        else if (strcmp(key, "sslcert") == 0) { 
            state_file_write(workdir, "config", "ssl_cert", value);
        }
        else if (strcmp(key, "sslkey") == 0) { 
            state_file_write(workdir, "config", "ssl_key", value);
        }
        else if (strcmp(key, "sslsan") == 0) { 
            state_file_write(workdir, "config", "ssl_san", value);
        }
        else if (strcmp(key, "scriptacl") == 0) { 
            state_file_write(workdir, "config", "scriptacl", value);
        }
        else if (strcmp(key, "lualibs") == 0) { 
            state_file_write(workdir, "config", "lualibs", value);
        }
        else if (strcmp(key, "loglevel") == 0) { 
            state_file_write(workdir, "config", "loglevel", value);
        }
        else if (strcmp(key, "customcert") == 0) { 
            state_file_write(workdir, "config", "custom_cert", value);
        }
        else if (strcmp(key, "acl") == 0) { 
            state_file_write(workdir, "config", "acl", value);
        }
    }
    fclose(fp);
    FREE_PTR(line);
    sdsfree(key);
    sdsfree(value);
}

void start_migrate_workdir(const char *workdir) {
    if (is_old_config(workdir) == false) {
        return;
    }
    MYMPD_LOG_INFO("Detected old configuration, migrating state files");
    migrate_smartpls_files(workdir);
    migrate_state_files(workdir);
}

//private functions
static bool is_old_config(const char *workdir) {
    //check for pre v8 version
    sds filename = sdscatprintf(sdsempty(), "%s/state/advanced", workdir);
    FILE *fp = fopen(filename, "r");
    sdsfree(filename);
    if (fp == NULL) {
        return false;
    }
    fclose(fp);
    return true;
}

void parse_ini_line (const char *line, sds *key, sds *value) {
    sds sds_line = sdsnew(line);
    sdstrim(sds_line, " \n\r\t");
    if (sdslen(sds_line) == 0 || sds_line[0] == '#') {
        sdsfree(sds_line);
        return;
    }
    int count;
    sds *tokens = sdssplitlen(sds_line, sdslen(sds_line), "=", 1, &count);
    if (count == 2) {
        sdstrim(tokens[0], " ");
        sdstrim(tokens[1], " \"");
        *key = sdsreplace(*key, tokens[0]);
        *value = sdsreplace(*value, tokens[1]);
    }
    sdsfreesplitres(tokens, count);
    sdsfree(sds_line);
}

static void migrate_state_files(const char *workdir) {
    //new state file names
    rename_file(workdir, "coverimage_name", "coverimage_names");
    rename_file(workdir, "advanced", "webui_settings");
    rename_file(workdir, "sylt_ext", "lyrics_sylt_ext");
    rename_file(workdir, "uslt_ext", "lyrics_uslt_ext");
    rename_file(workdir, "vorbis_sylt", "lyrics_vorbis_sylt");
    rename_file(workdir, "vorbis_uslt", "lyrics_vorbis_uslt");
    rename_file(workdir, "cols_browse_database", "cols_browse_database_detail");
    rename_file(workdir, "taglist", "tag_list");
    rename_file(workdir, "browsetaglist", "tag_list_browse");
    rename_file(workdir, "searchtaglist", "tag_list_search");

    //obsolet state files
    //most of these are saved in webui_settings now
    const char* state_files[]={"bg_color", "bg_cover", "bg_css_filter", "coverimage", "coverimage_size", "coverimage_size_small", 
        "locale", "love", "love_channel", "love_message", "notification_page", "notification_web", "home", "lyrics", "theme", "timer",
        "stickers", "bookmarks", "bookmark_list", "bg_image", "highlight_color", "media_session", 0};
    const char** ptr = state_files;
    while (*ptr != 0) {
        sds filename = sdscatfmt(sdsempty(), "%s/state/%s", workdir, *ptr);
        unlink(filename);
        sdsfree(filename);
        ++ptr;
    }
}

static void rename_file(const char *workdir, const char *src, const char *dst) {
    sds src_file = sdscatprintf(sdsempty(), "%s/state/%s", workdir, src);
    sds dst_file = sdscatprintf(sdsempty(), "%s/state/%s", workdir, dst);
    rename(src_file, dst_file);
    sdsfree(src_file);
    sdsfree(dst_file);
}

static bool migrate_smartpls_files(const char *workdir) {
    sds dirname = sdscatfmt(sdsempty(), "%s/smartpls", workdir);
    errno = 0;
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        MYMPD_LOG_ERROR("Can't open smart playlist directory \"%s\"", dirname);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(dirname);
        return false;
    }
    
    struct dirent *next_file;
    while ((next_file = readdir(dir)) != NULL) {
        if (next_file->d_type == DT_REG) {
            migrate_smartpls_file(workdir, next_file->d_name);
        }
    }
    closedir (dir);
    sdsfree(dirname);
    return true;
}

static bool migrate_smartpls_file(const char *workdir, const char *playlist) {
    char *smartpltype = NULL;
    int je;
    bool rc = true;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    
    sds filename = sdscatfmt(sdsempty(), "%s/smartpls/%s", workdir, playlist);
    char *content = json_fread(filename);
    if (content == NULL) {
        MYMPD_LOG_ERROR("Cant read smart playlist \"%s\"", playlist);
        sdsfree(filename);
        return false;
    }
    je = json_scanf(content, (int)strlen(content), "{type: %Q}", &smartpltype);
    if (je != 1) {
        MYMPD_LOG_ERROR("Cant read smart playlist type from \"%s\"", filename);
        sdsfree(filename);
        return false;
    }
    sdsfree(filename);
    if (strcmp(smartpltype, "search") == 0) {
        je = json_scanf(content, (int)strlen(content), "{expression: %Q}", &p_charbuf1);
        if (je == 1) {
            FREE_PTR(smartpltype);
            FREE_PTR(content);
            return true;
        }
        MYMPD_LOG_INFO("Converting smart playlist file \"%s\" to new format", filename);
        je = json_scanf(content, (int)strlen(content), "{tag: %Q, searchstr: %Q, sort: %Q}", &p_charbuf1, &p_charbuf2, &p_charbuf3);
        if (je == 3) {
            sds expression = sdsempty();
            if (strcmp(p_charbuf1, "expression") == 0) {
                expression = sdscat(expression, p_charbuf2);
                rc = mpd_shared_smartpls_save(workdir, "search", playlist, expression, 0, 0, p_charbuf3);
            }
            else {
                expression = sdscat(expression, "(");
                expression = escape_mpd_search_expression(expression, p_charbuf1, "==", p_charbuf2);
                expression = sdscat(expression, ")");
                rc = mpd_shared_smartpls_save(workdir, "search", playlist, expression, 0, 0, p_charbuf3);
            }
            sdsfree(expression);
            if (rc == false) {
                MYMPD_LOG_ERROR("Migration of smart playlist \"%s\" failed", playlist);
            }
            FREE_PTR(p_charbuf3);
        }
        else {
            rc = false;
        }
        FREE_PTR(p_charbuf1);
        FREE_PTR(p_charbuf2);
    }
    FREE_PTR(smartpltype);
    FREE_PTR(content);
    return rc;
}
