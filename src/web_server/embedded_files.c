/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_EMBEDDED_FILES_C
#define MYMPD_WEB_SERVER_EMBEDDED_FILES_C

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "compile_time.h"
#include "../../dist/incbin/incbin.h"

//compressed assets
INCBIN(sw_js, "htdocs/sw.js.gz");
INCBIN(mympd_webmanifest, "htdocs/mympd.webmanifest.gz");
INCBIN(index_html, "htdocs/index.html.gz");
INCBIN(coverimage_notavailable_svg, "htdocs/assets/coverimage-notavailable.svg.gz");
INCBIN(coverimage_stream_svg, "htdocs/assets/coverimage-stream.svg.gz");
INCBIN(coverimage_loading_svg, "htdocs/assets/coverimage-loading.svg.gz");
INCBIN(coverimage_booklet_svg, "htdocs/assets/coverimage-booklet.svg.gz");
INCBIN(coverimage_mympd_svg, "htdocs/assets/coverimage-mympd.svg.gz");
INCBIN(mympd_background_dark_svg, "htdocs/assets/mympd-background-dark.svg.gz");
INCBIN(mympd_background_light_svg, "htdocs/assets/mympd-background-light.svg.gz");
INCBIN(combined_css, "htdocs/css/combined.css.gz");
INCBIN(combined_js, "htdocs/js/combined.js.gz");
INCBIN(MaterialIcons_Regular_woff2, "htdocs/assets/MaterialIcons-Regular.woff2.gz");
INCBIN(ligatures_json, "htdocs/assets/ligatures.json.gz");
//uncompressed assets
INCBIN(appicon_192_png, "../htdocs/assets/appicon-192.png");
INCBIN(appicon_512_png, "../htdocs/assets/appicon-512.png");

#endif
