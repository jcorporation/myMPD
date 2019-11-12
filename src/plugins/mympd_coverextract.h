/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_COVEREXTRACT_H
#define MYMPD_COVEREXTRACT_H
#define MYMPD_COVEREXTRACT_VERSION "0.2.0"
    #ifdef __cplusplus
    extern "C" {
    #endif

    extern bool coverextract(const char *media_file_ptr, const char *cache_dir_ptr, char *image_filename, const int image_filename_len, char *image_mime_type, const int image_mime_type_len, const bool extract);

    #ifdef __cplusplus
    }
    #endif
#endif
