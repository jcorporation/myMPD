/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "compile_time.h"
#include "dist/incbin/incbin.h"

//compressed assets
INCBIN(sw_js, "../htdocs/sw.js.gz");
INCBIN(mympd_webmanifest, "../htdocs/mympd.webmanifest.gz");
INCBIN(index_html, "../htdocs/index.html.gz");
INCBIN(coverimage_notavailable_svg, "../htdocs/assets/coverimage-notavailable.svg.gz");
INCBIN(coverimage_stream_svg, "../htdocs/assets/coverimage-stream.svg.gz");
INCBIN(coverimage_booklet_svg, "../htdocs/assets/coverimage-booklet.svg.gz");
INCBIN(coverimage_folder_svg, "../htdocs/assets/coverimage-folder.svg.gz");
INCBIN(coverimage_mympd_svg, "../htdocs/assets/coverimage-mympd.svg.gz");
INCBIN(coverimage_playlist_svg, "../htdocs/assets/coverimage-playlist.svg.gz");
INCBIN(coverimage_smartpls_svg, "../htdocs/assets/coverimage-smartpls.svg.gz");
INCBIN(mympd_background_dark_svg, "../htdocs/assets/mympd-background-dark.svg.gz");
INCBIN(mympd_background_light_svg, "../htdocs/assets/mympd-background-light.svg.gz");
INCBIN(combined_css, "../htdocs/css/combined.css.gz");
INCBIN(combined_js, "../htdocs/js/combined.js.gz");
INCBIN(MaterialIcons_Regular_woff2, "../htdocs/assets/MaterialIcons-Regular.woff2");
INCBIN(ligatures_json, "../htdocs/assets/ligatures.json.gz");
//translation files
#ifdef I18N_bg_BG
    INCBIN(i18n_bg_BG_json, "../htdocs/assets/i18n/bg-BG.json.gz");
#endif
#ifdef I18N_de_DE
    INCBIN(i18n_de_DE_json, "../htdocs/assets/i18n/de-DE.json.gz");
#endif
#ifdef I18N_en_US
    INCBIN(i18n_en_US_json, "../htdocs/assets/i18n/en-US.json.gz");
#endif
#ifdef I18N_es_AR
    INCBIN(i18n_es_AR_json, "../htdocs/assets/i18n/es-AR.json.gz");
#endif
#ifdef I18N_es_ES
    INCBIN(i18n_es_ES_json, "../htdocs/assets/i18n/es-ES.json.gz");
#endif
#ifdef I18N_es_VE
    INCBIN(i18n_es_VE_json, "../htdocs/assets/i18n/es-VE.json.gz");
#endif
#ifdef I18N_fi_FI
    INCBIN(i18n_fi_FI_json, "../htdocs/assets/i18n/fi-FI.json.gz");
#endif
#ifdef I18N_fr_FR
    INCBIN(i18n_fr_FR_json, "../htdocs/assets/i18n/fr-FR.json.gz");
#endif
#ifdef I18N_it_IT
    INCBIN(i18n_it_IT_json, "../htdocs/assets/i18n/it-IT.json.gz");
#endif
#ifdef I18N_ja_JP
    INCBIN(i18n_ja_JP_json, "../htdocs/assets/i18n/ja-JP.json.gz");
#endif
#ifdef I18N_ko_KR
    INCBIN(i18n_ko_KR_json, "../htdocs/assets/i18n/ko-KR.json.gz");
#endif
#ifdef I18N_nl_NL
    INCBIN(i18n_nl_NL_json, "../htdocs/assets/i18n/nl-NL.json.gz");
#endif
#ifdef I18N_pl_PL
    INCBIN(i18n_pl_PL_json, "../htdocs/assets/i18n/pl-PL.json.gz");
#endif
#ifdef I18N_ru_RU
    INCBIN(i18n_ru_RU_json, "../htdocs/assets/i18n/ru-RU.json.gz");
#endif
#ifdef I18N_zh_Hans
    INCBIN(i18n_zh_Hans_json, "../htdocs/assets/i18n/zh-Hans.json.gz");
#endif
#ifdef I18N_zh_Hant
    INCBIN(i18n_zh_Hant_json, "../htdocs/assets/i18n/zh-Hant.json.gz");
#endif
//uncompressed assets
INCBIN(appicon_192_png, "../htdocs/assets/appicon-192.png");
INCBIN(appicon_512_png, "../htdocs/assets/appicon-512.png");
