/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../dist/sds/sds.h"
#include "../../src/lib/m3u.h"

void write_m3u_file(void) {
    FILE *fp = fopen("/tmp/test.m3u", "w");
    const char *m3u = "#EXTM3U\n"
        "#EXTINF:-1,Yumi Co. Radio\n"
        "#EXTGENRE:Future Funk, City Pop, Anime Groove, Vaporwave, Nu Disco, Electronic\n"
        "#PLAYLIST:Yumi Co. Radio\n"
        "#EXTIMG:http___yumicoradio_net_8000_stream.webp\n"
        "#HOMEPAGE:http://yumicoradio.net\n"
        "#COUNTRY:France\n"
        "#LANGUAGE:English\n"
        "#DESCRIPTION:24/7 webradio that plays Future Funk, City Pop, Anime Groove, Nu Disco, Electronica, a little bit of Vaporwave and some of the sub-genres derived.\n"
        "#CODEC:MP3\n"
        "#BITRATE:256\n"
        "http://yumicoradio.net:8000/stream\n";
    fputs(m3u, fp);
    fclose(fp);
}

void rm_m3u_file(void) {
    unlink("/tmp/test.m3u");
}

UTEST(m3u, test_m3u_get_field) {
    write_m3u_file();
    sds s = sdsempty();
    s = m3u_get_field(s, "#EXTIMG", "/tmp/test.m3u");
    ASSERT_STREQ("http___yumicoradio_net_8000_stream.webp", s);
    rm_m3u_file();
    sdsfree(s);
}

UTEST(m3u, test_m3u_to_json) {
    write_m3u_file();
    sds s = sdsempty();
    sds m3ufields = sdsempty();
    s = m3u_to_json(s, "/tmp/test.m3u", &m3ufields);
    const char *e = "-1,yumi co. radiofuture funk, city pop, anime groove, vaporwave, nu disco, electronicyumi co. radiohttp___yumicoradio_net_8000_stream.webphttp://yumicoradio.netfranceenglish24/7 webradio that plays future funk, city pop, anime groove, nu disco, electronica, a little bit of vaporwave and some of the sub-genres derived.mp3256";
    ASSERT_STREQ(e, m3ufields);
    rm_m3u_file();
    sdsfree(s);
    sdsfree(m3ufields);
}
