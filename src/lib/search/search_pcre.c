/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Search implementation
 */

#include "compile_time.h"
#include "src/lib/search/search_pcre.h"

#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/utf8_wrapper.h"

#include <string.h>

/**
 * Compiles a string to regex code
 * @param regex_str regex string
 * @return regex code
 */
pcre2_code *mympd_search_pcre_compile(sds regex_str) {
    MYMPD_LOG_DEBUG(NULL, "Compiling regex: \"%s\"", regex_str);
    char *regex_utf8 = utf8_wrap_casefold(regex_str, sdslen(regex_str));
    PCRE2_SIZE erroroffset;
    int rc;
    pcre2_code *re_compiled = pcre2_compile(
        (PCRE2_SPTR)regex_utf8, /* the pattern */
        PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
        0,                     /* default options */
        &rc,		           /* for error number */
        &erroroffset,          /* for error offset */
        NULL                   /* use default compile context */
    );
    if (re_compiled == NULL){
        //Compilation failed
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(rc, buffer, sizeof(buffer));
        MYMPD_LOG_ERROR(NULL, "PCRE2 compilation failed at offset %lu: \"%s\"", (unsigned long)erroroffset, buffer);
    }
    FREE_PTR(regex_utf8);
    return re_compiled;
}

/**
 * Matches the regex against a string
 * @param re_compiled the compiled regex from compile_regex
 * @param value string to match against
 * @return true if regex matches else false
 */
bool mympd_search_pcre_match(pcre2_code *re_compiled, const char *value) {
    if (re_compiled == NULL) {
        return false;
    }
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re_compiled, NULL);
    int rc = pcre2_match(
        re_compiled,          /* the compiled pattern */
        (PCRE2_SPTR)value,    /* the subject string */
        strlen(value),        /* the length of the subject */
        0,                    /* start at offset 0 in the subject */
        0,                    /* default options */
        match_data,           /* block for storing the result */
        NULL                  /* use default match context */
    );
    pcre2_match_data_free(match_data);
    if (rc >= 0) {
        return true;
    }
    //Matching failed: handle error cases
    switch(rc) {
        case PCRE2_ERROR_NOMATCH: 
            break;
        default: {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(rc, buffer, sizeof(buffer));
            MYMPD_LOG_ERROR(NULL, "PCRE2 matching error %d: \"%s\"", rc, buffer);
        }
    }
    return false;
}
