/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_EMBEDDED_FILES_C
#define MYMPD_WEB_SERVER_EMBEDDED_FILES_C

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "mympd_config_defs.h"
#include "../../dist/incbin/incbin.h"

//compressed assets
INCBIN(sw_js, "htdocs/sw.js" ZIPEXT);
INCBIN(mympd_webmanifest, "htdocs/mympd.webmanifest" ZIPEXT);
INCBIN(index_html, "htdocs/index.html" ZIPEXT);
INCBIN(coverimage_notavailable_svg, "htdocs/assets/coverimage-notavailable.svg" ZIPEXT);
INCBIN(coverimage_stream_svg, "htdocs/assets/coverimage-stream.svg" ZIPEXT);
INCBIN(coverimage_loading_svg, "htdocs/assets/coverimage-loading.svg" ZIPEXT);
INCBIN(coverimage_booklet_svg, "htdocs/assets/coverimage-booklet.svg" ZIPEXT);
INCBIN(coverimage_mympd_svg, "htdocs/assets/coverimage-mympd.svg" ZIPEXT);
INCBIN(mympd_background_dark_svg, "htdocs/assets/mympd-background-dark.svg" ZIPEXT);
INCBIN(mympd_background_light_svg, "htdocs/assets/mympd-background-light.svg" ZIPEXT);
INCBIN(combined_css, "htdocs/css/combined.css" ZIPEXT);
INCBIN(combined_js, "htdocs/js/combined.js" ZIPEXT);
INCBIN(MaterialIcons_Regular_woff2, "htdocs/assets/MaterialIcons-Regular.woff2" ZIPEXT);
//uncompressed assets
INCBIN(appicon_192_png, "../htdocs/assets/appicon-192.png");
INCBIN(appicon_512_png, "../htdocs/assets/appicon-512.png");

#endif
