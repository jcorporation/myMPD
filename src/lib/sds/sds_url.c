/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief URL functions for sds strings
 */

#include "compile_time.h"
#include "src/lib/sds/sds_url.h"

#include "dist/sds/sds.h"
#include "src/lib/sds/sds_extras.h"

#include <ctype.h>

/**
 * Converts hex to integer
 */
#define HEXTOI(x) ((x) >= '0' && (x) <= '9' ? (x) - '0' : (x) - 'W')

/**
 * Checks for url safe characters
 * @param c char to check
 * @return true if char is url safe else false
 */
static bool is_url_safe(char c) {
    if (isalnum(c) ||
        c == '/' || c == '-' || c == '.' ||
        c == '_')
    {
        return true;
    }
    return false;
}

/**
 * Url encodes a string
 * @param s sds string to append the encoded string
 * @param p string to url encode
 * @param len string length to url encode
 * @return modified sds string
 */
sds sds_urlencode(sds s, const char *p, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (is_url_safe(p[i])) {
            s = sds_catchar(s, p[i]);
        }
        else {
            s = sdscatprintf(s, "%%%hhX", p[i]);
        }
    }
    return s;
}

/**
 * Url decodes a string
 * @param s sds string to append the url decoded string
 * @param p string to url decode
 * @param len string length to url decode
 * @param is_form_url_encoded true decodes + chars to spaces
 * @return modified sds string
 */
sds sds_urldecode(sds s, const char *p, size_t len, bool is_form_url_encoded) {
    size_t i;
    int a;
    int b;

    for (i = 0; i < len; i++) {
        switch(*p) {
            case '%':
                if (i < len - 2 && isxdigit(*(const unsigned char *) (p + 1)) &&
                    isxdigit(*(const unsigned char *) (p + 2)))
                {
                    a = tolower(*(const unsigned char *) (p + 1));
                    b = tolower(*(const unsigned char *) (p + 2));
                    s = sds_catchar(s, (char) ((HEXTOI(a) << 4) | HEXTOI(b)));
                    i += 2;
                    p += 2;
                }
                else {
                    //error - return blank string
                    sdsclear(s);
                    return s;
                }
                break;
            case '+':
                if (is_form_url_encoded == true) {
                    s = sdscatlen(s, " ", 1);
                    break;
                }
                //fall through
            default:
                s = sdscatlen(s, p, 1);
        }
        p++;
    }
    return s;
}
