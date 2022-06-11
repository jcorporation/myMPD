/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_webradios.h"

#include "../../dist/utf8/utf8.h"
#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/m3u.h"
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
struct t_webradio_entry {
    sds entry;
    sds filename;
};

//public functions

sds get_webradio_from_uri(struct t_config *config, const char *uri) {
    sds filename = sdsnew(uri);
    sds_sanitize_filename(filename);
    filename = sdscatlen(filename, ".m3u", 4);
    sds filepath = sdscatfmt(sdsempty(), "%s/webradios/%s", config->workdir, filename);
    sds entry = sdsempty();
    if (access(filepath, F_OK) == 0) { /* Flawfinder: ignore */
        entry = tojson_char(entry, "filename", filename, true);
        entry = m3u_to_json(entry, filepath, NULL);
        sdsfree(filepath);
        sdsfree(filename);
        return entry;
    }
    sdsfree(filepath);
    sdsfree(filename);
    return entry;
}

sds mympd_api_webradio_get(struct t_config *config, sds buffer, sds method, long request_id, const char *filename) {
    sds filepath = sdscatfmt(sdsempty(), "%s/webradios/%s", config->workdir, filename);
    sds entry = sdsempty();
    entry = m3u_to_json(entry, filepath, NULL);
    if (sdslen(entry) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "database", "error", "Can not parse webradio favorite file");
    }
    else {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "filename", filename, true);
        buffer = sdscatsds(buffer, entry);
        buffer = jsonrpc_result_end(buffer);
    }
    FREE_SDS(entry);
    FREE_SDS(filepath);
    return buffer;
}

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

    size_t search_len = sdslen(searchstr);
    struct dirent *next_file;
    rax *webradios = raxNew();
    sds key = sdsempty();
    long real_limit = offset + limit;
    //read dir
    while ((next_file = readdir(webradios_dir)) != NULL ) {
        char *ext = strrchr(next_file->d_name, '.');
        if (ext == NULL ||
            strcmp(ext, ".m3u") != 0)
        {
            continue;
        }

        sdsclear(key);
        sds filename = sdscatfmt(sdsempty(), "%s/%s", webradios_dirname, next_file->d_name);
        sds entry = m3u_to_json(sdsempty(), filename, &key);
        if (sdslen(entry) == 0) {
            //skip on parsing error
            sdsfree(entry);
            sdsfree(filename);
            continue;
        }
        if (search_len == 0 ||
            utf8casestr(key, searchstr) != NULL)
        {
            struct t_webradio_entry *webradio = malloc_assert(sizeof(struct t_webradio_entry));
            webradio->filename = filename;
            webradio->entry = entry;
            key = sdscatsds(key, filename); //append filename to keep it unique
            sds_utf8_tolower(key);
            while (raxTryInsert(webradios, (unsigned char *)key, sdslen(key), webradio, NULL) == 0) {
                //duplicate - add chars until it is uniq
                key = sdscatlen(key, ":", 1);
            }
        }
        else {
            sdsfree(filename);
            sdsfree(entry);
        }
    }
    closedir(webradios_dir);
    FREE_SDS(webradios_dirname);
    FREE_SDS(key);
    //print result
    long entity_count = 0;
    long entities_returned = 0;
    raxIterator iter;
    raxStart(&iter, webradios);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        struct t_webradio_entry *webradio = (struct t_webradio_entry *)iter.data;
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_sds(buffer, "filename", webradio->filename, true);
            buffer = sdscatsds(buffer, webradio->entry);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        sdsfree(webradio->entry);
        sdsfree(webradio->filename);
        FREE_PTR(webradio);
    }
    raxStop(&iter);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_llong(buffer, "totalEntities", (long long)webradios->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    raxFree(webradios);
    return buffer;
}

bool mympd_api_webradio_save(struct t_config *config, sds name, sds uri, sds uri_old,
        sds genre, sds picture, sds homepage, sds country, sds language, sds codec, int bitrate,
        sds description)
{
    sds tmp_file = sdscatfmt(sdsempty(), "%s/webradios/%s.XXXXXX", config->workdir, name);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, OPEN_FLAGS_WRITE);
    bool rc = true;
    int printed = fprintf(fp, "#EXTM3U\n"
        "#EXTINF:-1,%s\n"
        "#EXTGENRE:%s\n"
        "#PLAYLIST:%s\n"
        "#EXTIMG:%s\n"
        "#HOMEPAGE:%s\n"
        "#COUNTRY:%s\n"
        "#LANGUAGE:%s\n"
        "#DESCRIPTION:%s\n"
        "#CODEC:%s\n"
        "#BITRATE:%d\n"
        "%s\n",
        name, genre, name, picture, homepage, country, language, description, codec, bitrate, uri);
    if (printed < 0) {
        MYMPD_LOG_ERROR("Could not write to file \"%s\"", tmp_file);
        rc = false;
    }
    if (fclose(fp) != 0) {
        MYMPD_LOG_ERROR("Could not close file \"%s\"", tmp_file);
        rc = false;
    }
    sds filename = sdsdup(uri);
    sds_sanitize_filename(filename);
    sds filepath = sdscatfmt(sdsempty(), "%s/webradios/%s.m3u", config->workdir, filename);
    errno = 0;
    if (rc == true) {
        if (rename(tmp_file, filepath) == -1) {
            MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
            MYMPD_LOG_ERRNO(errno);
            FREE_SDS(tmp_file);
            FREE_SDS(filepath);
            FREE_SDS(filename);
            return false;
        }
        if (strcmp(uri, uri_old) != 0 &&
            uri_old[0] != '\0')
        {
            //streamuri changed
            sdsclear(filename);
            filename = sdscatsds(filename, uri_old);
            sds_sanitize_filename(filename);
            sdsclear(filepath);
            filepath = sdscatfmt(filepath, "%s/webradios/%s.m3u", config->workdir, filename);
            errno = 0;
            if (unlink(filepath) == -1) {
                MYMPD_LOG_ERROR("Deleting old file \"%s\" failed", filepath);
                MYMPD_LOG_ERRNO(errno);
                rc = false;
            }
        }
    }
    else {
        //remove incomplete tmp file
        if (unlink(tmp_file) != 0) {
            MYMPD_LOG_ERROR("Could not remove incomplete tmp file \"%s\"", tmp_file);
            MYMPD_LOG_ERRNO(errno);
            rc = false;
        }
    }
    FREE_SDS(tmp_file);
    FREE_SDS(filepath);
    FREE_SDS(filename);
    return rc;
}

bool mympd_api_webradio_delete(struct t_config *config, const char *filename) {
    sds filepath = sdscatfmt(sdsempty(), "%s/webradios/%s", config->workdir, filename);
    errno = 0;
    if (unlink(filepath) == -1) {
        MYMPD_LOG_ERROR("Unlinking webradio file \"%s\" failed", filepath);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(filepath);
        return false;
    }
    FREE_SDS(filepath);
    return true;
}
