/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/validate.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "dist/utf8/utf8.h"
#include "src/lib/log.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

/**
 * Private definitions
 */

static const char *invalid_json_chars = "\a\b\f\v";
static const char *invalid_name_chars = "\a\b\f\n\r\t\v";
static const char *invalid_filename_chars = "\a\b\f\n\r\t\v/\\";
static const char *invalid_filepath_chars = "\a\b\f\n\r\t\v";

static const char *mympd_cols[]={
    // Columns for songs
    "Pos", "Duration", "Type", "Priority", "LastPlayed", "Filename", "Filetype",
    "AudioFormat", "Last-Modified", "Lyrics",
    // Columns for stickers
    "playCount", "skipCount", "lastPlayed", "lastSkipped", "like", "rating", "elapsed",
     // Columns for webradiodb
    "Country", "Description", "Genre", "Homepage", "Language", "Name", "StreamUri",
    "Codec", "Bitrate",
     // Columns for radiobrowser
    "clickcount", "country", "homepage", "language", "lastchangetime", "lastcheckok",
    "tags", "url_resolved", "votes",
    // Columns for albums
    "Discs", "SongCount",
    // End
    0};

static bool check_for_invalid_chars(sds data, const char *invalid_chars);
static bool validate_json(sds data, char start, char end);
static bool is_mympd_col(sds token);

/**
 * Public functions
 */

/**
 * Checks if string is a json object
 * @param data sds string to check
 * @return true on success else false
 */
bool validate_json_object(sds data) {
    return validate_json(data, '{', '}');
}

/**
 * Checks if string is a json array
 * @param data sds string to check
 * @return true on success else false
 */
bool validate_json_array(sds data) {
    return validate_json(data, '[', ']');
}

/**
 * Checks if string is alphanumeric, including chars "_-"
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isalnum(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isalnum(data[i]) == 0 &&
            data[i] != '_' &&
            data[i] != '-')
        {
            MYMPD_LOG_WARN(NULL, "Found none alphanumeric character in string");
            return false;
        }
    }
    return true;
}

/**
 * Checks if string is a number
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isdigit(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isdigit(data[i]) == 0) {
            MYMPD_LOG_WARN(NULL, "Found none numeric character in string");
            return false;
        }
    }
    return true;
}

/**
 * Checks if string is printable
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isprint(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isprint(data[i]) == 0) {
            MYMPD_LOG_WARN(NULL, "Found none printable character in string");
            return false;
        }
    }
    return true;
}

/**
 * Checks if string is a hexcaolor starting with #
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_ishexcolor(sds data) {
    if (data[0] != '#') {
        return false;
    }
    for (size_t i = 1; i < sdslen(data); i++) {
        if (isxdigit(data[i]) == 0) {
            MYMPD_LOG_WARN(NULL, "Found none hex character in string");
            return false;
        }
    }
    return true;
}

/**
 * Checks if string contains invalid chars
 * Invalid chars are "\a\b\f\n\r\t\v"
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isname(sds data) {
    bool rc = check_for_invalid_chars(data, invalid_name_chars);
    if (rc == false) {
        MYMPD_LOG_WARN(NULL, "Found illegal name character");
    }
    return rc;
}

/**
 * Checks if string contains invalid chars
 * Invalid chars are "\a\b\f\r\v"
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_istext(sds data) {
    bool rc = check_for_invalid_chars(data, invalid_json_chars);
    if (rc == false) {
        MYMPD_LOG_WARN(NULL, "Found illegal text character");
    }
    return rc;
}

/**
 * Checks if string is a valid uri or filepath
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isuri(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    if (strstr(data, "://") != NULL) {
        //uri notation
        return true;
    }
    return vcb_isfilepath(data);
}

/**
 * Checks if string is a valid stream uri
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isstreamuri(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    if (strstr(data, "://") != NULL) {
        //uri notation
        return true;
    }
    return false;
}

/**
 * Checks if string is a valid filename
 * Does not emit a warning
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isfilename_silent(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    return check_for_invalid_chars(data, invalid_filename_chars);
}

/**
 * Checks if string is a valid filename
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isfilename(sds data) {
    bool rc = vcb_isfilename_silent(data);
    if (rc == false) {
        MYMPD_LOG_WARN(NULL, "Found illegal filename character");
    }
    return rc;
}

/**
 * Checks if string is a valid filename with path or path only
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_isfilepath(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    if (strstr(data, "://") != NULL) {
        MYMPD_LOG_WARN(NULL, "Illegal file path, found URI notation");
        return false;
    }
    if (strncmp(data, "../", 3) == 0 ||
        strncmp(data, "//", 2) == 0 ||
        strstr(data, "/../") != NULL ||
        strstr(data, "/./") != NULL)
    {
        //prevent dir traversal
        MYMPD_LOG_WARN(NULL, "Found dir traversal in path \"%s\"", data);
        return false;
    }
    bool rc = check_for_invalid_chars(data, invalid_filepath_chars);
    if (rc == false) {
        MYMPD_LOG_WARN(NULL, "Found illegal character in file path");
    }
    return rc;
}

/**
 * Checks if string is a valid path + filename
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_ispathfilename(sds data) {
    bool rc = vcb_isfilepath(data);
    if (rc == true) {
        return data[sdslen(data) - 1] == '/'
            ? false
            : true;
    }
    return false;
}

/**
 * Checks if string is a valid column name
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_iscolumn(sds data) {
    if (mpd_tag_name_iparse(data) != MPD_TAG_UNKNOWN ||
        is_mympd_col(data) == true)
    {
        return true;
    }
    MYMPD_LOG_WARN(NULL, "Unknown column: %s", data);
    return false;
}

/**
 * Checks if string is a compare operator
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_iscompareop(sds data) {
    if (data[0] == '=' ||
        data[0] == '<' ||
        data[0] == '>')
    {
        return true;
    }
    MYMPD_LOG_WARN(NULL, "Unknown compare operator: %s", data);
    return false;
}

/**
 * Checks if string is a valid comma separated list of tags
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_istaglist(sds data) {
    int tokens_count = 0;
    sds *tokens = sdssplitlen(data, (ssize_t)sdslen(data), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            MYMPD_LOG_WARN(NULL, "Unknown tag %s", tokens[i]);
            sdsfreesplitres(tokens, tokens_count);
            return false;
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    return true;
}

/**
 * Checks if string is a valid MPD tag
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_ismpdtag(sds data) {
    enum mpd_tag_type tag = mpd_tag_name_iparse(data);
    if (tag == MPD_TAG_UNKNOWN) {
        MYMPD_LOG_WARN(NULL, "Unknown tag %s", data);
        return false;
    }
    return true;
}

/**
 * Checks if string is a valid MPD tag or special value "any"
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_ismpdtag_or_any(sds data) {
    if (strcmp(data, "any") == 0 ||
        strcmp(data, "filename") == 0)
    {
        return true;
    }
    return vcb_ismpdtag(data);
}

/**
 * Checks if string is a valid sort tag
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_ismpdsort(sds data) {
    enum mpd_tag_type tag = mpd_tag_name_iparse(data);
    if (tag == MPD_TAG_UNKNOWN &&
        strcmp(data, "filename") != 0 &&
        strcmp(data, "shuffle") != 0 &&
        strcmp(data, "Last-Modified") != 0 &&
        strcmp(data, "Date") != 0 &&
        strcmp(data, "Priority") != 0)
    {
        MYMPD_LOG_WARN(NULL, "Unknown tag \"%s\"", data);
        return false;
    }
    return true;
}

/**
 * Checks if string is a valid mpd search expression
 * @param data sds string to check
 * @return true on success else false
 */
bool vcb_issearchexpression(sds data) {
    size_t len = sdslen(data);
    if (len == 0) {
        return true;
    }
    //check if it is valid utf8
    if (utf8valid(data) != 0) {
        MYMPD_LOG_ERROR(NULL, "String is not valid utf8");
        return false;
    }
    //only some basic checks
    if (len < 2 ||
        data[0] != '(' ||
        data[len - 1] != ')')
    {
        MYMPD_LOG_ERROR(NULL, "String is not a valid search expression");
        return false;
    }
    return check_for_invalid_chars(data, invalid_name_chars);
}

/**
 * Private functions
 */

/**
 * Helper function to check for invalid chars in a string
 * @param data sds string to check
 * @param invalid_chars invalid characters
 * @return true on success else false
 */
static bool check_for_invalid_chars(sds data, const char *invalid_chars) {
    size_t len = sdslen(data);
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\0' ||
            strchr(invalid_chars, data[i]) != NULL)
        {
            return false;
        }
        if (i + 1 < len && data[i] == '\\' &&
            (data[i + 1] == 'u' || data[i + 1] == 'U' || data[i + 1] == 'x'))
        {
            MYMPD_LOG_ERROR(NULL, "Unicode and hex escapes are forbidden");
            return false;
        }
    }
    return true;
}

/**
 * Helper function that checks string for json validity
 * @param data sds string to check
 * @param start char the string must start with
 * @param end char the string must end with
 * @return true on success else false
 */
static bool validate_json(sds data, char start, char end) {
    size_t len = sdslen(data);
    //check if it is valid utf8
    if (utf8valid(data) != 0) {
        MYMPD_LOG_ERROR(NULL, "String is not valid utf8");
        return false;
    }
    //only some basic checks
    if (len < 2 ||
        data[0] != start ||
        data[len - 1] != end)
    {
        MYMPD_LOG_ERROR(NULL, "String is not valid json");
        return false;
    }
    return check_for_invalid_chars(data, invalid_json_chars);
}

/**
 * Helper function that checks if token is a valid column name
 * @param token string to check
 * @return true on success else false
 */
static bool is_mympd_col(sds token) {
    const char** ptr = mympd_cols;
    while (*ptr != 0) {
        if (strncmp(token, *ptr, sdslen(token)) == 0) {
            return true;
        }
        ++ptr;
    }
    return false;
}
