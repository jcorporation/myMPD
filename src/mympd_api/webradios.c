/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/list.h"
#include "src/mympd_api/webradios.h"

#include "dist/utf8/utf8.h"
#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/m3u.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

/**
 * Struct to hold a webradio entry in the rax tree
 */
struct t_webradio_entry {
    sds entry;     //!< json representation of the webradio m3u
    sds filename;  //!< filename of the webradio m3u
};

/**
 * Public functions
 */

/**
 * Gets the webradio m3u as json object string.
 * This function calculates the real filename for the m3u from the uri
 * @param workdir working directory
 * @param uri webradio stream uri
 * @return new sds string with the json object string
 */
sds get_webradio_from_uri(sds workdir, const char *uri) {
    sds filename = sdsnew(uri);
    sanitize_filename(filename);
    filename = sdscatlen(filename, ".m3u", 4);
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_WEBRADIOS, filename);
    sds entry = sdsempty();
    if (testfile_read(filepath) == true) {
        entry = tojson_sds(entry, "filename", filename, true);
        entry = m3u_to_json(entry, filepath, NULL);
        FREE_SDS(filepath);
        FREE_SDS(filename);
        return entry;
    }
    FREE_SDS(filepath);
    FREE_SDS(filename);
    return entry;
}

/**
 * Prints a webradio m3u as jsonrpc response
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param filename webradio m3u filename
 * @return pointer to buffer
 */
sds mympd_api_webradio_get(sds workdir, sds buffer, long request_id, sds filename) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIO_FAVORITE_GET;
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S", workdir, DIR_WORK_WEBRADIOS, filename);
    sds entry = sdsempty();
    entry = m3u_to_json(entry, filepath, NULL);
    if (sdslen(entry) == 0) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Can not parse webradio favorite file");
    }
    else {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_sds(buffer, "filename", filename, true);
        buffer = sdscatsds(buffer, entry);
        buffer = jsonrpc_end(buffer);
    }
    FREE_SDS(entry);
    FREE_SDS(filepath);
    return buffer;
}

/**
 * Prints the webradio list as a jsonrpc response
 * @param workdir working directory 
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param searchstr string to search
 * @param offset offset for the list
 * @param limit maximum entries to print
 * @return pointer to buffer
 */
sds mympd_api_webradio_list(sds workdir, sds buffer, long request_id, sds searchstr, long offset, long limit) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_WEBRADIO_FAVORITE_GET;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds webradios_dirname = sdscatfmt(sdsempty(), "%S/%s", workdir, DIR_WORK_WEBRADIOS);
    errno = 0;
    DIR *webradios_dir = opendir(webradios_dirname);
    if (webradios_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open directory \"%s\"", webradios_dirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(webradios_dirname);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Can not open webradios directory");
        return buffer;
    }

    size_t search_len = sdslen(searchstr);
    struct dirent *next_file;
    rax *webradios = raxNew();
    sds key = sdsempty();
    long real_limit = offset + limit;
    //read dir
    sds filepath = sdsempty();
    while ((next_file = readdir(webradios_dir)) != NULL ) {
        const char *ext = get_extension_from_filename(next_file->d_name);
        if (ext == NULL ||
            strcasecmp(ext, "m3u") != 0)
        {
            continue;
        }

        sdsclear(key);
        sdsclear(filepath);
        filepath = sdscatfmt(filepath, "%S/%s", webradios_dirname, next_file->d_name);
        sds entry = m3u_to_json(sdsempty(), filepath, &key);
        if (sdslen(entry) == 0) {
            //skip on parsing error
            FREE_SDS(entry);
            continue;
        }
        if (search_len == 0 ||
            utf8casestr(key, searchstr) != NULL)
        {
            struct t_webradio_entry *webradio = malloc_assert(sizeof(struct t_webradio_entry));
            webradio->filename = sdsnew(next_file->d_name);
            webradio->entry = entry;
            key = sdscatsds(key, next_file->d_name); //append filename to keep it unique
            sds_utf8_tolower(key);
            while (raxTryInsert(webradios, (unsigned char *)key, sdslen(key), webradio, NULL) == 0) {
                //duplicate - add chars until it is uniq
                key = sdscatlen(key, ":", 1);
            }
        }
        else {
            FREE_SDS(entry);
        }
    }
    closedir(webradios_dir);
    FREE_SDS(filepath);
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
        FREE_SDS(webradio->entry);
        FREE_SDS(webradio->filename);
        FREE_PTR(webradio);
    }
    raxStop(&iter);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_llong(buffer, "totalEntities", (long long)webradios->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    raxFree(webradios);
    return buffer;
}

/**
 * Saves a webradio as m3u
 * @param workdir working directory
 * @param name webradio name
 * @param uri webradio uri
 * @param uri_old old webradio uri
 * @param genre comma separated list of genres
 * @param picture picture uri
 * @param homepage homepage
 * @param country country
 * @param language language
 * @param codec codec
 * @param bitrate bitrate
 * @param description short description
 * @return true on success, else false
 */
bool mympd_api_webradio_save(sds workdir, sds name, sds uri, sds uri_old,
        sds genre, sds picture, sds homepage, sds country, sds language, sds codec, int bitrate,
        sds description)
{
    sds filename = sdsdup(uri);
    sanitize_filename(filename);
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.m3u", workdir, DIR_WORK_WEBRADIOS, filename);

    sds content = sdscatfmt(sdsempty(), "#EXTM3U\n"
        "#EXTINF:-1,%S\n"
        "#EXTGENRE:%S\n"
        "#PLAYLIST:%S\n"
        "#EXTIMG:%S\n"
        "#HOMEPAGE:%S\n"
        "#COUNTRY:%S\n"
        "#LANGUAGE:%S\n"
        "#DESCRIPTION:%S\n"
        "#CODEC:%S\n"
        "#BITRATE:%i\n"
        "%S\n",
        name, genre, name, picture, homepage, country, language, description, codec, bitrate, uri);

    bool rc = write_data_to_file(filepath, content, sdslen(content));

    if (rc == true &&
        uri_old[0] != '\0' &&
        strcmp(uri, uri_old) != 0)
    {
        sdsclear(filename);
        filename = sdscatsds(filename, uri_old);
        sanitize_filename(filename);
        sdsclear(filepath);
        filepath = sdscatfmt(filepath, "%S/%s/%S.m3u", workdir, DIR_WORK_WEBRADIOS, filename);
        rc = rm_file(filepath);
    }
    FREE_SDS(filename);
    FREE_SDS(filepath);
    FREE_SDS(content);

    return rc;
}

/**
 * Deletes webradio m3u's
 * @param workdir working directory
 * @param filenames webradio m3u filenames to delete
 * @return true on success, else false
 */
bool mympd_api_webradio_delete(sds workdir, struct t_list *filenames) {
    sds filepath = sdsempty();
    bool rc = true;
    struct t_list_node *current = filenames->head;
    while (current != NULL) {
        filepath = sdscatfmt(sdsempty(), "%S/%s/%S", workdir, DIR_WORK_WEBRADIOS, current->key);
        rc = rm_file(filepath);
        if (rc == false) {
            break;
        }
        sdsclear(filepath);
    }
    FREE_SDS(filepath);
    return rc;
}
