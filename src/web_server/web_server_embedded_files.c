/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define INCBIN_PREFIX 
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "../../dist/src/incbin/incbin.h"

//compressed assets
INCBIN(sw_js, "htdocs/sw.js.gz");
INCBIN(mympd_webmanifest, "htdocs/mympd.webmanifest.gz");
INCBIN(index_html, "htdocs/index.html.gz");
INCBIN(coverimage_notavailable_svg, "htdocs/assets/coverimage-notavailable.svg.gz");
INCBIN(coverimage_stream_svg, "htdocs/assets/coverimage-stream.svg.gz");
INCBIN(coverimage_loading_svg, "htdocs/assets/coverimage-loading.svg.gz");
INCBIN(coverimage_booklet_svg, "htdocs/assets/coverimage-booklet.svg.gz");
INCBIN(coverimage_mympd_svg, "htdocs/assets/coverimage-mympd.svg.gz");
INCBIN(combined_css, "htdocs/css/combined.css.gz");
INCBIN(combined_js, "htdocs/js/combined.js.gz");
//uncompressed assets
INCBIN(favicon_ico, "../htdocs/assets/favicon.ico");
INCBIN(appicon_192_png, "../htdocs/assets/appicon-192.png");
INCBIN(appicon_512_png, "../htdocs/assets/appicon-512.png");
INCBIN(MaterialIcons_Regular_woff2, "../dist/htdocs/assets/MaterialIcons-Regular.woff2");
