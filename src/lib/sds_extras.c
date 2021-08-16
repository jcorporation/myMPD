/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "sds_extras.h"

#include <ctype.h>
#include <string.h>

#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')

static int json_get_utf8_char_len(unsigned char ch) {
    if ((ch & 0x80) == 0) {
        return 1;
    }

    switch (ch & 0xf0) {
        case 0xf0:
            return 4;
        case 0xe0:
            return 3;
        default:
            return 2;
    }
}

sds sdscatjson(sds s, const char *p, size_t len) {
    const char *hex_digits = "0123456789abcdef";
    s = sdscatlen(s, "\"", 1);
    while (len--) {
        switch(*p) {
        case '\\':
        case '"':
            s = sdscatprintf(s, "\\%c", *p);
            break;
        case '\n': s = sdscatlen(s, "\\n", 2);     break;
        case '\r': s = sdscatlen(s, "\\r", 2);     break;
        case '\t': s = sdscatlen(s, "\\t", 2);     break;
        case '\b': s = sdscatlen(s, "\\b", 2);     break;
        case '\f': s = sdscatlen(s, "\\f", 2);     break;
        // Escape < to prevent script execution
        case '<' : s = sdscatlen(s, "\\u003C", 6); break;
        //ignore vertical tabulator and alert
        case '\v': 
        case '\a': 
            break;
        default:
            if (isprint(*p) || json_get_utf8_char_len(*p) != 1) {
                s = sdscatprintf(s, "%c", *p);
            }
            else {
                s = sdscatprintf(s, "\\u00%s%s", &hex_digits[(*p >> 4) % 0xf], &hex_digits[*p % 0xf]);
            }
            break;
        }
        p++;
    }
    return sdscatlen(s, "\"", 1);
}

sds sdscatjsonchar(sds s, const char p) {
    const char *hex_digits = "0123456789abcdef";
    switch(p) {
        case '\\':
        case '"':
            s = sdscatprintf(s, "\\%c", p);
            break;
        case '\n': s = sdscatlen(s, "\\n", 2);     break;
        case '\r': s = sdscatlen(s, "\\r", 2);     break;
        case '\t': s = sdscatlen(s, "\\t", 2);     break;
        case '\b': s = sdscatlen(s, "\\b", 2);     break;
        case '\f': s = sdscatlen(s, "\\f", 2);     break;
        // Escape < to prevent script execution
        case '<' : s = sdscatlen(s, "\\u003C", 6); break;
        //ignore vertical tabulator and alert
        case '\v': 
        case '\a':
            //this escapes are not accepted in the unescape function
            break;
        default:
            if (isprint(p)) {
                s = sdscatprintf(s, "%c", p);
            }
            else {
                s = sdscatprintf(s, "\\u00%s%s", &hex_digits[(p >> 4) % 0xf], &hex_digits[p % 0xf]);
            } 
            break;
    }
    return s;
}

static unsigned char hexdec(const char *s) {
    int a = tolower(*(const unsigned char *) s);
    int b = tolower(*(const unsigned char *) (s + 1));
    return (HEXTOI(a) << 4) | HEXTOI(b);
}

bool sds_json_unescape(const char *src, int slen, sds *dst) {
    char *send = (char *) src + slen;
    char *p;
    const char *esc1 = "\"\\/bfnrt";
    const char *esc2 = "\"\\/\b\f\n\r\t";

    while (src < send) {
        if (*src == '\\') {
            if (++src >= send) {
                return false;
            }
            if (*src == 'u') {
                if (send - src < 5) {
                    return false;
                }
                //\u.... escape. Process simple one-byte chars
                if (src[1] == '0' && src[2] == '0') {
                    /* This is \u00xx character from the ASCII range */
                    *dst = sdscatprintf(*dst, "%c", hexdec(src + 3));
                    src += 4;
                }
                else {
                    //Complex \uXXXX escapes
                    //TODO: use utf8decode
                    return false;
                }
            }
            else if ((p = (char *) strchr(esc1, *src)) != NULL) {
                *dst = sdscatprintf(*dst, "%c", esc2[p - esc1]);
            }
            else {
                //other escapes are not accepted
                return false;
            }
        }
        else {
            *dst = sdscatprintf(*dst, "%c", *src);
        }
        src++;
    }

    return true;
}

sds sdsurldecode(sds s, const char *p, size_t len, int is_form_url_encoded) {
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
                s = sdscatprintf(s, "%c", (char) ((HEXTOI(a) << 4) | HEXTOI(b)));
                i += 2;
                p += 2;
            } 
            else {
                s = sdscrop(s);
                return s;
            }
            break;
        case '+':
            if (is_form_url_encoded == 1) {
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

sds sdsreplacelen(sds s, const char *value, size_t len) {
    s = sdscrop(s);
    if (value != NULL) {
        s = sdscatlen(s, value, len);
    }
    return s;
}

sds sdsreplace(sds s, const char *value) {
    return sdsreplacelen(s, value, strlen(value));
}

sds sdscrop(sds s) {
    if (s == NULL) {
        return sdsempty();
    }
    sdsclear(s);
    return s;
}
