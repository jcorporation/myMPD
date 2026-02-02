/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief JSON functions for sds strings
 */

#include "compile_time.h"
#include "src/lib/sds/sds_json.h"

#include "dist/sds/sds.h"
#include "src/lib/sds/sds_extras.h"

#include <string.h>

/**
 * JSON escapes special chars
 * @param c char to escape
 * @return escaped char
 */
static const char *escape_char(char c) {
    switch(c) {
        case '\\': return "\\\\";
        case '"':  return "\\\"";
        case '\b': return "\\b";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
    }
    return NULL;
}

/**
 * Append to the sds string "s" a json escaped string
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 * @param s sds string
 * @param p string to append json escaped
 * @param len length of the string to append
 * @return modified sds string
 */
sds sds_catjson_plain(sds s, const char *p, size_t len) {
    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least stringlength + 10 chars. */
    s = sdsMakeRoomFor(s, len + 10);
    size_t i = sdslen(s);
    while (len--) {
        switch(*p) {
            case '\\':
            case '"':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t': {
                if (sdsavail(s) == 0) {
                    s = sdsMakeRoomFor(s, 2);
                }
                const char *escape = escape_char(*p);
                s[i++] = escape[0];
                s[i++] = escape[1];
                sdsinclen(s, 2);
                break;
            }
            //ignore vertical tabulator and alert
            case '\v':
            case '\a':
                //this escapes are not accepted in the unescape function
                break;
            default:
                if (sdsavail(s) == 0) {
                    s = sdsMakeRoomFor(s, 1);
                }
                s[i++] = *p;
                sdsinclen(s, 1);
                break;
        }
        p++;
    }
    // Add null-term
    s[i] = '\0';
    return s;
}

/**
 * Append to the sds string "s" a quoted json escaped string
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 * @param s sds string
 * @param p string to append json escaped
 * @param len length of the string to append
 * @return modified sds string
 */
sds sds_catjson(sds s, const char *p, size_t len) {
    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least stringlength + 10 chars. */
    s = sdsMakeRoomFor(s, len + 10);
    s = sdscatlen(s, "\"", 1);
    s = sds_catjson_plain(s, p, len);
    return sdscatlen(s, "\"", 1);
}

/**
 * Append to the sds string "s" an json escaped character
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 * @param s sds string
 * @param c char to escape and append
 * @return modified sds string
 */
sds sds_catjsonchar(sds s, const char c) {
    switch(c) {
        case '\\':
        case '"':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
            s = sdscatlen(s, escape_char(c), 2);
            break;
        //ignore vertical tabulator and alert
        case '\v':
        case '\a':
            //this escapes are not accepted in the unescape function
            break;
        default:
            s = sds_catchar(s, c);
            break;
    }
    return s;
}

/**
 * Json unescapes "src" and appends the result to the sds string "dst"
 * "dst" must be a pointer to a allocated sds string.
 * @param src string to unescape
 * @param slen string length to unescape
 * @param dst pointer to sds string to append the unescaped string
 * @return true on success, false on error
 */
bool sds_json_unescape(const char *src, size_t slen, sds *dst) {
    char *send = (char *) src + slen;
    char *p;
    const char *esc1 = "\"\\/bfnrt";
    const char *esc2 = "\"\\/\b\f\n\r\t";

    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least src length chars. */
    *dst = sdsMakeRoomFor(*dst, slen);

    while (src < send) {
        if (*src == '\\') {
            if (++src >= send) {
                //escape char should not be the last
                return false;
            }
            if (*src == 'u') {
                //skip unicode escapes
                if (send - src < 5) {
                    return false;
                }
                src += 4;
            }
            else if ((p = strchr(esc1, *src)) != NULL) {
                //print unescaped value
                *dst = sds_catchar(*dst, esc2[p - esc1]);
            }
            else {
                //other escapes are not accepted
                return false;
            }
        }
        else {
            *dst = sds_catchar(*dst, *src);
        }
        src++;
    }

    return true;
}
