/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "sds_extras.h"

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/utf8/utf8.h"

#define HEXTOI(x) (x >= '0' && x <= '9' ? x - '0' : x - 'W')

/**
 * Splits a comma separated string and trims whitespaces from single values
 * @param s string to split
 * @param count pointer to int representing the count of values
 * @return array of sds strings
 */
sds *sds_split_comma_trim(sds s, int *count) {
    *count = 0;
    sds *values = sdssplitlen(s, (ssize_t)sdslen(s), ",", 1, count);
    for (int i = 0; i < *count; i++) {
        sdstrim(values[i], " ");
    }
    return values;
}

/**
 * Hashes a string with sha1
 * @param p string to hash
 * @return the hash as a newly allocated sds string
 */
sds sds_hash(const char *p) {
    mg_sha1_ctx ctx;
    mg_sha1_init(&ctx);
    mg_sha1_update(&ctx, (unsigned char *)p, strlen(p));
    unsigned char hash[20];
    mg_sha1_final(hash, &ctx);
    sds hex_hash = sdsempty();
    for (unsigned i = 0; i < 20; i++) {
        hex_hash = sdscatprintf(hex_hash, "%02x", hash[i]);
    }
    return hex_hash;
}

/**
 * Reads the integer from start of the string, the integer is removed from string
 * @param s sds string to modify in place
 * @return the number at the beginning of the sds string
 */
int sds_toimax(sds s) {
    sds nr = sdsempty();
    while (isdigit(s[0])) {
        nr = sds_catchar(nr, s[0]);
        sdsrange(s, 1, -1);
    }
    char *crap;
    int number = (int)strtoimax(nr, &crap, 10);
    FREE_SDS(nr);
    return number;
}

/**
 * Makes the string lower case (utf8 aware)
 * @param s sds string to modify in place
 */
void sds_utf8_tolower(sds s) {
    utf8_int32_t cp;

    void *pn = utf8codepoint(s, &cp);
    while (cp != 0) {
        const size_t size = utf8codepointsize(cp);
        const utf8_int32_t lwr_cp = utf8lwrcodepoint(cp);
        const size_t lwr_size = utf8codepointsize(lwr_cp);

        if (lwr_cp != cp && lwr_size == size) {
            utf8catcodepoint(s, lwr_cp, lwr_size);
        }

        s = pn;
        pn = utf8codepoint(s, &cp);
    }
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
    while (len--) {
        s = sds_catjsonchar(s, *p);
        p++;
    }
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
    while (len--) {
        s = sds_catjsonchar(s, *p);
        p++;
    }
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
        case '\\': s = sdscatlen(s, "\\\\", 2); break;
        case '"':  s = sdscatlen(s, "\\\"", 2); break;
        case '\b': s = sdscatlen(s, "\\b", 2); break;
        case '\f': s = sdscatlen(s, "\\f", 2); break;
        case '\n': s = sdscatlen(s, "\\n", 2); break;
        case '\r': s = sdscatlen(s, "\\r", 2); break;
        case '\t': s = sdscatlen(s, "\\t", 2); break;
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
 * Appends a char to sds string s, this is faster than using sdscatfmt
 * @param s sds string
 * @param c char to append
 * @return modified sds string
 */
sds sds_catchar(sds s, const char c) {
    // Make sure there is always space for at least 1 char.
    if (sdsavail(s) == 0) {
        s = sdsMakeRoomFor(s, 1);
    }
    size_t i = sdslen(s);
    s[i++] = c;
    sdsinclen(s, 1);
    // Add null-term
    s[i] = '\0';
    return s;
}

/**
 * Json unescapes "src" and appends the result to the sds string "dst"
 * "dst" must be a pointer to a allocated sds string.
 * @param src string to unescape
 * @param slen string length to unescape
 * @param dst pointer to sds string to append the unsecaped string
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

/**
 * Checks for url safe characters
 * @param c char to check
 * @return true if string is url safe else false
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

/**
 * Replaces a sds string with a new value,
 * allocates the string if it is NULL
 * @param s sds string to replace
 * @param p replacement string
 * @param len replacement string length
 * @return modified sds string
 */
sds sds_replacelen(sds s, const char *p, size_t len) {
    if (s != NULL) {
        sdsclear(s);
    }
    else {
        s = sdsempty();
    }
    if (p != NULL) {
        s = sdscatlen(s, p, len);
    }
    return s;
}

/**
 * Replaces a sds string with a new value,
 * allocates the string if it is NULL
 * @param s sds string to replace
 * @param p replacement string
 * @return modified sds string
 */
sds sds_replace(sds s, const char *p) {
    return sds_replacelen(s, p, strlen(p));
}

/**
 * Converts a bool value to a sds string
 * @param s string to append the value
 * @param v bool value to convert
 * @return modified sds string
 */
sds sds_catbool(sds s, bool v) {
    return v == true ? sdscatlen(s, "true", 4) : sdscatlen(s, "false", 5);
}
