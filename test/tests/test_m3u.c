/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/m3u.h"
#include "src/mympd_api/webradios.h"

#include <sys/stat.h>

static bool webradio_save(void) {
    sds name = sdsnew("Yumi Co. Radio");
    sds uri = sdsnew("http://yumicoradio.net:8000/stream");
    sds uri_old = sdsnew("");
    sds genre = sdsnew("Future Funk, City Pop, Anime Groove, Vaporwave, Nu Disco, Electronic");
    sds picture = sdsnew("http___yumicoradio_net_8000_stream.webp");
    sds homepage = sdsnew("http://yumicoradio.net");
    sds country = sdsnew("France");
    sds state = sdsnew("Paris");
    sds language = sdsnew("English");
    sds codec = sdsnew("MP3");
    sds description = sdsnew("24/7 webradio that plays Future Funk, City Pop, Anime Groove, Nu Disco, Electronica, a little bit of Vaporwave and some of the sub-genres derived.");
    int bitrate = 256;
    bool rc = mympd_api_webradio_save(workdir, name, uri, uri_old, genre, picture, homepage, country, language, codec, bitrate, description, state);
    sdsfree(name);
    sdsfree(uri);
    sdsfree(uri_old);
    sdsfree(genre);
    sdsfree(picture);
    sdsfree(homepage);
    sdsfree(country);
    sdsfree(language);
    sdsfree(codec);
    sdsfree(description);
    sdsfree(state);
    return rc;
}

UTEST(m3u, test_m3u_webradio_save) {
    init_testenv();

    bool rc = webradio_save();
    ASSERT_TRUE(rc);

    clean_testenv();
}

UTEST(m3u, test_m3u_get_field) {
    init_testenv();
    webradio_save();

    sds s = sdsempty();
    s = m3u_get_field(s, "#EXTIMG", "/tmp/mympd-test/webradios/http___yumicoradio_net_8000_stream.m3u");
    ASSERT_STREQ("http___yumicoradio_net_8000_stream.webp", s);
    sdsfree(s);

    clean_testenv();
}

UTEST(m3u, test_m3u_to_json) {
    init_testenv();
    webradio_save();

    sds s = sdsempty();
    sds m3ufields = sdsempty();
    s = m3u_to_json(s, "/tmp/mympd-test/webradios/http___yumicoradio_net_8000_stream.m3u", &m3ufields);
    const char *e = "-1,yumi co. radiofuture funk, city pop, anime groove, vaporwave, nu disco, electronicyumi co. radiohttp___yumicoradio_net_8000_stream.webphttp://yumicoradio.netfranceparisenglish24/7 webradio that plays future funk, city pop, anime groove, nu disco, electronica, a little bit of vaporwave and some of the sub-genres derived.mp3256";
    ASSERT_STREQ(e, m3ufields);
    sdsfree(s);
    sdsfree(m3ufields);

    clean_testenv();
}

UTEST(m3u, test_get_webradio_from_uri) {
    init_testenv();
    webradio_save();

    sds m3u = get_webradio_from_uri(workdir, "http://yumicoradio.net:8000/stream");
    ASSERT_GT(sdslen(m3u), (size_t)0);
    sdsfree(m3u);

    clean_testenv();
}

UTEST(m3u, test_mympd_api_webradio_list) {
    init_testenv();
    webradio_save();

    sds searchstr = sdsempty();
    sds buffer = mympd_api_webradio_list(workdir, sdsempty(), 0, searchstr, 0, 10);
    struct t_jsonrpc_parse_error parse_error;
    jsonrpc_parse_error_init(&parse_error);
    int result;
    bool rc = json_get_int_max(buffer, "$.result.totalEntities", &result, &parse_error);
    ASSERT_TRUE(rc);
    ASSERT_EQ(result, 1);
    jsonrpc_parse_error_clear(&parse_error);
    sdsfree(searchstr);
    sdsfree(buffer);

    clean_testenv();
}

UTEST(m3u, test_mympd_api_webradio_delete) {
    init_testenv();
    webradio_save();

    struct t_list filenames;
    list_init(&filenames);
    list_push(&filenames, "http___yumicoradio_net_8000_stream.m3u", 0, NULL, NULL);
    bool rc = mympd_api_webradio_delete(workdir, &filenames);
    list_clear(&filenames);
    ASSERT_TRUE(rc);

    clean_testenv();
}
