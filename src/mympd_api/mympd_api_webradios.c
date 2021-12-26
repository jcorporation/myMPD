/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_webradios.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private definitions
sds m3u_to_json(sds buffer, sds filename, sds *plname);

//public functions
sds mympd_api_webradio_list(struct t_config *config, sds buffer, sds method, long request_id, sds searchstr, long offset, long limit) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds webradios_dirname = sdscatfmt(sdsempty(), "%s/webradios", config->workdir);
    errno = 0;
    DIR *webradios_dir = opendir(webradios_dirname);
    if (webradios_dir == NULL) {
        MYMPD_LOG_ERROR("Can not open directory \"%s\"", webradios_dirname);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(webradios_dirname);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "database", "error", "Can not open webradios directory");
        return buffer;
    }

    sds_utf8_tolower(searchstr);
    size_t search_len = sdslen(searchstr);
    struct dirent *next_file;
    long entity_count = 0;
    long entities_returned = 0;
    sds filename = sdsempty();
    sds entry = sdsempty();
    sds plname = sdsempty();
    while ((next_file = readdir(webradios_dir)) != NULL ) {
        sds extension = sds_get_extension_from_filename(next_file->d_name);
        if (strcmp(extension, "m3u") != 0) {
            FREE_SDS(extension);
            continue;
        }
        FREE_SDS(extension);
        sdsclear(filename);
        filename = sdscatfmt(filename, "%s/%s", webradios_dirname, next_file->d_name);
        sdsclear(entry);
        sdsclear(plname);
        entry = m3u_to_json(entry, filename, &plname);
        sds_utf8_tolower(plname);
        if (search_len == 0 || strstr(plname, searchstr) != NULL) {
            if (entity_count >= offset &&
                entities_returned < limit)
            {
                if (entities_returned++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                buffer = tojson_char(buffer, "uri", next_file->d_name, true);
                buffer = sdscatsds(buffer, entry);
                buffer = sdscatlen(buffer, "}", 1);
            }
            entity_count++;
        }
    }
    closedir(webradios_dir);
    FREE_SDS(filename);
    FREE_SDS(webradios_dirname);
    FREE_SDS(entry);
    FREE_SDS(plname);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

bool mympd_api_webradio_save(struct t_config *config, sds name, sds uri, sds genre, sds picture) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s/webradios/%.XXXXXX", config->workdir, name);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, OPEN_FLAGS_WRITE);
    fprintf(fp, "#EXTM3U\n"
        "#EXTINF:-1,%s\n"
        "#EXTGENRE:%s\n"
        "#PLAYLIST:%s\n"
        "#EXTIMG:%s\n"
        "%s\n",
        name, genre, name, picture, uri);
    fclose(fp);
    sds filename = sdsdup(name);
    sds_sanitize_filename(filename);
    sds filepath = sdscatfmt(sdsempty(), "%s/webradios/%s.m3u", config->workdir, filename);
    errno = 0;
    if (rename(tmp_file, filepath) == -1) {
        MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        FREE_SDS(filepath);
        return false;
    }
    FREE_SDS(tmp_file);
    FREE_SDS(filepath);
    return true;
}

bool mympd_api_webradio_delete(struct t_config *config, const char *name) {
    sds filename = sdscatfmt(sdsempty(), "%s/webradios/%s.m3u", config->workdir, name);
    errno = 0;
    if (unlink(filename) == -1) {
        MYMPD_LOG_ERROR("Unlinking webradio file \"%s\" failed", filename);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(filename);
        return false;
    }
    FREE_SDS(filename);
    return true;
}

//private functions

sds m3u_to_json(sds buffer, sds filename, sds *plname) {
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        return buffer;
    }
    sds line = sdsempty();
    //check ext m3u header
    sds_getline(&line, fp, 1000);
    if (strcmp(line, "#EXTM3U") != 0) {
        MYMPD_LOG_WARN("Invalid ext m3u file");
        sdsfree(line);
        fclose(fp);
        return buffer;
    }
    int line_count = 0;
    while (sds_getline(&line, fp, 1000) == 0) {
        if (line[0] != '#') {
            continue;
        }
        if (line_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatlen(buffer, "\"", 1);
        int i = 1;
        while (line[i] != '\0' &&
                line[i] != ':')
        {
            buffer = sds_catjsonchar(buffer, line[i]);
            i++;
        }
        buffer = sdscatlen(buffer, "\":\"", 3);
        i++;
        while (line[i] != '\0') {
            buffer = sds_catjsonchar(buffer, line[i]);
            *plname = sdscatprintf(*plname, "%c", line[i]);
            i++;
        }
        buffer = sdscatlen(buffer, "\"", 1);
    }
    FREE_SDS(line);
    fclose(fp);
    return buffer;
}
