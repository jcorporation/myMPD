/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "validate.h"

#include "../../dist/utf8/utf8.h"
#include "log.h"
#include "sds_extras.h"

#include <ctype.h>
#include <limits.h>
#include <mpd/client.h>
#include <string.h>

//private

static const char *invalid_json_chars = "\a\b\f\v";
static const char *invalid_name_chars = "\a\b\f\n\r\t\v";
static const char *invalid_filename_chars = "\a\b\f\n\r\t\v/\\";
static const char *invalid_filepath_chars = "\a\b\f\n\r\t\v";

static const char *mympd_cols[]={"Pos", "Duration", "Type", "Priority", "LastPlayed", "Filename", "Filetype", "AudioFormat", "LastModified",
    "Lyrics", "stickerPlayCount", "stickerSkipCount", "stickerLastPlayed", "stickerLastSkipped", "stickerLike", 0};

static bool _check_for_invalid_chars(sds data, const char *invalid_chars) {
    size_t len = sdslen(data);
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\0' ||
            strchr(invalid_chars, data[i]) != NULL)
        {
            return false;
        }
        if (i + 1 < len && data[i] == '\\' && data[i + 1] == 'u') {
            MYMPD_LOG_ERROR("Unicode escapes are not supported");
            return false;
        }
    }
    return true;
}

static bool _validate_json(sds data, char start, char end) {
    size_t len = sdslen(data);
    //check if it is valid utf8
    if (utf8valid(data) != 0) {
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
        if (isalnum(data[i]) == 0 &&
            data[i] != '_' &&
            data[i] != '-')
        {
            MYMPD_LOG_WARN("Found none alphanumeric character in string");
            return false;
        }
    }
    return true;
}

bool vcb_isdigit(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isdigit(data[i]) == 0) {
            MYMPD_LOG_WARN("Found none numeric character in string");
            return false;
        }
    }
    return true;
}

bool vcb_isprint(sds data) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (isprint(data[i]) == 0) {
            MYMPD_LOG_WARN("Found none printable character in string");
            return false;
        }
    }
    return true;
}

bool vcb_ishexcolor(sds data) {
    if (data[0] != '#') {
        return false;
    }
    for (size_t i = 1; i < sdslen(data); i++) {
        if (isxdigit(data[i]) == 0) {
            MYMPD_LOG_WARN("Found none hex character in string");
            return false;
        }
    }
    return true;
}

bool vcb_isname(sds data) {
    bool rc = _check_for_invalid_chars(data, invalid_name_chars);
    if (rc == false) {
        MYMPD_LOG_WARN("Found illegal name character");
    }
    return rc;
}

bool vcb_istext(sds data) {
    bool rc = _check_for_invalid_chars(data, invalid_json_chars);
    if (rc == false) {
        MYMPD_LOG_WARN("Found illegal text character");
    }
    return rc;
}

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

bool vcb_isfilename_silent(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    return _check_for_invalid_chars(data, invalid_filename_chars);
}

bool vcb_isfilename(sds data) {
    bool rc = vcb_isfilename_silent(data);
    if (rc == false) {
        MYMPD_LOG_WARN("Found illegal filename character");
    }
    return rc;
}

bool vcb_isfilepath(sds data) {
    if (sdslen(data) == 0) {
        return false;
    }
    if (strstr(data, "://") != NULL) {
        MYMPD_LOG_WARN("Illegal file path, found URI notation");
        return false;
    }
    if (strncmp(data, "../", 3) == 0 ||
        strncmp(data, "//", 2) == 0 ||
        strstr(data, "/../") != NULL ||
        strstr(data, "/./") != NULL)
    {
        //prevent dir traversal
        MYMPD_LOG_WARN("Found dir traversal in path \"%s\"", data);
        return false;
    }
    bool rc = _check_for_invalid_chars(data, invalid_filepath_chars);
    if (rc == false) {
        MYMPD_LOG_WARN("Found illegal character in file path");
    }
    return rc;
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
    int tokens_count = 0;
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

bool vcb_ismpdtag_or_any(sds data) {
    if (strcmp(data, "any") == 0) {
        return true;
    }
    return vcb_ismpdtag(data);
}

bool vcb_ismpdsort(sds data) {
    enum mpd_tag_type tag = mpd_tag_name_iparse(data);
    if (tag == MPD_TAG_UNKNOWN &&
        strcmp(data, "filename") != 0 &&
        strcmp(data, "shuffle") != 0 &&
        strcmp(data, "LastModified") != 0 &&
        strcmp(data, "Date") != 0)
    {
        MYMPD_LOG_WARN("Unknown tag \"%s\"", data);
        return false;
    }
    return true;
}
