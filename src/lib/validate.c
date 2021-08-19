/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "validate.h"

#include "../../dist/src/sds/sds.h"

#include <ctype.h>
#include <string.h>

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

const char *invalid_name_chars = "\a\b\f\n\r\t\v\0";
const char *invalid_filename_chars = "\a\b\f\n\r\t\v\0\\/?*|<>/";
const char *invalid_filepath_chars = "\a\b\f\n\r\t\v\0\\/?*|<>";

static bool check_for_invalid_chars(sds data, const char *invalid_chars) {
    for (size_t i = 0; i < sdslen(data); i++) {
        if (strchr(invalid_chars, data[i]) != NULL) {
            return false;
        }
    }
    return true;
}

bool vcb_isname(sds data) {
    return check_for_invalid_chars(data, invalid_name_chars);
}

bool vcb_isfilename(sds data) {
    return check_for_invalid_chars(data, invalid_filename_chars);
}

bool vcb_isfilepath(sds data) {
    if (strstr(data, "../") != NULL || strstr(data, "/./") != NULL || strstr(data, "//") != NULL) {
        //prevent dir traversal
        return false;
    }
    return check_for_invalid_chars(data, invalid_filepath_chars);
}

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
