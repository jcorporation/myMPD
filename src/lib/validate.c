/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "validate.h"

#include "../../dist/src/sds/sds.h"
#include "../../dist/src/utf8decode/utf8decode.h"
#include "log.h"

#include <ctype.h>
#include <limits.h>
#include <mpd/client.h>
#include <string.h>

//private

static const char *invalid_json_chars = "\a\b\f\v\0";
static const char *invalid_name_chars = "\a\b\f\n\r\t\v\0";
static const char *invalid_filename_chars = "\a\b\f\n\r\t\v\0\\/?*|<>/";
static const char *invalid_filepath_chars = "\a\b\f\n\r\t\v\0\\/?*|<>";

static const char *mympd_cols[]={"Pos", "Duration", "Type", "LastPlayed", "Filename", "Filetype", "Fileformat", "LastModified", 
    "Lyrics", "stickerPlayCount", "stickerSkipCount", "stickerLastPlayed", "stickerLastSkipped", "stickerLike", 0};

static bool _check_for_invalid_chars(sds data, const char *invalid_chars) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (strchr(invalid_chars, data[i]) != NULL) {
            return false;
        }
    }
    return true;
}

static bool _validate_json(sds data, char start, char end) {
    size_t len = sdslen(data);
    //check if it is valid utf8
    if (check_utf8((uint8_t *)data, len) == UTF8_REJECT) {
        MYMPD_LOG_ERROR("String is not valid utf8");
        return false;
    }
    //only some basic checks
    if (len < 2 ||
        data[0] != start ||
        data[len - 1] != end)
    {
        MYMPD_LOG_ERROR("String is not valid json");
        return false;
    }
    return _check_for_invalid_chars(data, invalid_json_chars);
}

static bool _is_mympd_col(sds token) {
    const char** ptr = mympd_cols;
    while (*ptr != 0) {
        if (strncmp(token, *ptr, sdslen(token)) == 0) {
            return true;
        }
        ++ptr;
    }
    return false;
}

//public

bool validate_json(sds data) {
    return _validate_json(data, '{', '}');
}

bool validate_json_array(sds data) {
    return _validate_json(data, '[', ']');
}

bool vcb_isalnum(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isalnum(data[i]) == 0 && data[i] != '_') {
            return false;
        }
    }
    return true;
}

bool vcb_isprint(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isprint(data[i]) == 0) {
            return false;
        }
    }
    return true;
}

bool vcb_ishexcolor(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isxdigit(data[i]) == 0 && data[i] != '#') {
            return false;
        }
    }
    return true;
}

bool vcb_isname(sds data) {
    return _check_for_invalid_chars(data, invalid_name_chars);
}

bool vcb_istext(sds data) {
    return _check_for_invalid_chars(data, invalid_json_chars);
}

bool vcb_isfilename(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    return _check_for_invalid_chars(data, invalid_filename_chars);
}

bool vcb_isfilepath(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    if (strstr(data, "../") != NULL || strstr(data, "/./") != NULL || strstr(data, "//") != NULL) {
        //prevent dir traversal
        return false;
    }
    return _check_for_invalid_chars(data, invalid_filepath_chars);
}

bool vcb_iscolumn(sds data) {
    if (mpd_tag_name_iparse(data) != MPD_TAG_UNKNOWN ||
        _is_mympd_col(data) == true)
    {
        return true;
    }
    MYMPD_LOG_WARN("Unknown column: %s", data);
    return false;
}

bool vcb_istaglist(sds data) {
    int tokens_count;
    sds *tokens = sdssplitlen(data, (ssize_t)sdslen(data), ",", 1, &tokens_count);
    for (int i = 0; i < tokens_count; i++) {
        sdstrim(tokens[i], " ");
        enum mpd_tag_type tag = mpd_tag_name_iparse(tokens[i]);
        if (tag == MPD_TAG_UNKNOWN) {
            MYMPD_LOG_WARN("Unknown tag %s", tokens[i]);
            sdsfreesplitres(tokens, tokens_count);
            return false;
        }
    }
    sdsfreesplitres(tokens, tokens_count);
    return true;
}

bool vcb_ismpdtag(sds data) {
    enum mpd_tag_type tag = mpd_tag_name_iparse(data);
    if (tag == MPD_TAG_UNKNOWN) {
        MYMPD_LOG_WARN("Unknown tag %s", data);
        return false;
    }
    return true;
}

bool vcb_ismpdsort(sds data) {
    enum mpd_tag_type tag = mpd_tag_name_iparse(data);
    if (tag == MPD_TAG_UNKNOWN &&
        strcmp(data, "filename") != 0 &&
        strcmp(data, "shuffle") != 0 &&
        strcmp(data, "Last-Modified") != 0 &&
        strcmp(data, "Date") != 0)
    {
        MYMPD_LOG_WARN("Unknown tag \"%s\"", data);
        return false;

    }
    return true;
}

//deprecated validation checks

bool validate_string(const char *data) {
    if (strchr(data, '/') != NULL || strchr(data, '\n') != NULL || strchr(data, '\r') != NULL ||
        strchr(data, '\t') != NULL ||
        strchr(data, '"') != NULL || strchr(data, '\'') != NULL || strchr(data, '\\') != NULL) {
        return false;
    }
    return true;
}

bool validate_string_not_empty(const char *data) {
    if (data == NULL) { 
        return false;
    }
    if (strlen(data) == 0) {
        return false;
    }
    return validate_string(data);
}

bool validate_string_not_dir(const char *data) {
    bool rc = validate_string_not_empty(data);
    if (rc == true) {
        if (strcmp(data, ".") == 0 || strcmp(data, "..") == 0) {
            rc = false;
        }    
    }
    return rc;
}

bool validate_uri(const char *data) {
    if (strncmp(data, "../", 3) == 0 || strstr(data, "/../") != NULL || strstr(data, "/./") != NULL) {
        return false;
    }
    return true;
}

bool validate_songuri(const char *data) {
    if (data == NULL) {
        return false;
    }
    if (strlen(data) == 0) {
        return false;
    }
    if (strcmp(data, "/") == 0) {
        return false;
    }
    if (strchr(data, '.') == NULL) {
        return false;
    }
    return true;
}

bool is_streamuri(const char *uri) {
    if (uri != NULL && strstr(uri, "://") != NULL) {
        return true;
    }
    return false;
}
