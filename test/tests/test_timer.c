/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "test/utility.h"

#include "dist/utest/utest.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"

#include <sys/stat.h>

UTEST(timer, test_timer_add_replace_remove) {
    struct t_timer_list l;
    mympd_api_timer_timerlist_init(&l);
    ASSERT_EQ(USER_TIMER_ID_START, l.last_id);

    bool rc = mympd_api_timer_add(&l, 10, 0, timer_handler_by_id, TIMER_ID_COVERCACHE_CROP, NULL);
    ASSERT_TRUE(rc);
    ASSERT_EQ(1, l.length);
    ASSERT_EQ(USER_TIMER_ID_START, l.last_id);

    mympd_api_timer_add(&l, 10, 0, timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
    mympd_api_timer_add(&l, 10, 0, timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
    ASSERT_EQ(3, l.length);
    
    rc = mympd_api_timer_replace(&l, 10, 0, timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
    ASSERT_TRUE(rc);
    ASSERT_EQ(3, l.length);
    
    mympd_api_timer_remove(&l, TIMER_ID_CACHES_CREATE);
    ASSERT_EQ(2, l.length);

    mympd_api_timer_timerlist_clear(&l);
}

UTEST(timer, test_timer_parse_definition) {
    struct t_timer_list l;
    mympd_api_timer_timerlist_init(&l);
    struct t_timer_definition *def1 = malloc(sizeof(struct t_timer_definition));
    ASSERT_TRUE(def1 == NULL ? false : true);
    sds e = sdsempty();
    sds s1 = sdsnew("{\"params\":{\"partition\":\"default\",\"timerid\":103,\"name\":\"example timer1\",\"interval\":86400,\"enabled\":true,\"startHour\":7,\"startMinute\":0,\"action\":\"player\",\"subaction\":\"startplay\",\"playlist\":\"Database\",\"volume\":50,\"jukeboxMode\":\"off\",\"weekdays\":[false,false,false,false,false,true,true],\"arguments\": {\"arg1\":\"value1\"}}}");
    def1 = mympd_api_timer_parse(def1, s1, MPD_PARTITION_DEFAULT, &e);
    ASSERT_STREQ("", e);
    bool rc = mympd_api_timer_add(&l, 10, 0, timer_handler_select, 103, def1);
    ASSERT_TRUE(rc);
    ASSERT_STREQ("example timer1", l.list->definition->name);

    struct t_timer_definition *def2 = malloc(sizeof(struct t_timer_definition));
    ASSERT_TRUE(def2 == NULL ? false : true);
    sds s2 = sdsnew("{\"params\":{\"partition\":\"default\",\"timerid\":103,\"name\":\"example timer2\",\"interval\":86400,\"enabled\":true,\"startHour\":7,\"startMinute\":0,\"action\":\"player\",\"subaction\":\"startplay\",\"playlist\":\"Database\",\"volume\":50,\"jukeboxMode\":\"off\",\"weekdays\":[false,false,false,false,false,true,true],\"arguments\": {\"arg1\":\"value1\"}}}");
    def2 = mympd_api_timer_parse(def2, s2, MPD_PARTITION_DEFAULT, &e);
    ASSERT_STREQ("", e);
    rc = mympd_api_timer_replace(&l, 10, 0, timer_handler_select, 103, def2);
    ASSERT_TRUE(rc);
    ASSERT_STREQ("example timer2", l.list->definition->name);

    ASSERT_TRUE(l.list->definition->enabled);
    mympd_api_timer_toggle(&l, 103);
    ASSERT_FALSE(l.list->definition->enabled);

    sdsfree(e);
    sdsfree(s1);
    sdsfree(s2);
    mympd_api_timer_timerlist_clear(&l);
}

UTEST(timer, test_timer_write_read) {
    struct t_timer_list l;
    mympd_api_timer_timerlist_init(&l);
    struct t_timer_definition *def1 = malloc(sizeof(struct t_timer_definition));
    ASSERT_TRUE(def1 == NULL ? false : true);
    sds e = sdsempty();
    sds s1 = sdsnew("{\"params\":{\"partition\":\"default\",\"timerid\":103,\"name\":\"example timer1\",\"interval\":86400,\"enabled\":true,\"startHour\":7,\"startMinute\":0,\"action\":\"player\",\"subaction\":\"startplay\",\"playlist\":\"Database\",\"volume\":50,\"jukeboxMode\":\"off\",\"weekdays\":[false,false,false,false,false,true,true],\"arguments\": {\"arg1\":\"value1\"}}}");
    def1 = mympd_api_timer_parse(def1, s1, MPD_PARTITION_DEFAULT, &e);
    ASSERT_STREQ("", e);
    bool rc = mympd_api_timer_add(&l, 10, 0, timer_handler_select, 103, def1);
    ASSERT_TRUE(rc);

    rc = mympd_api_timer_file_save(&l, workdir);
    ASSERT_TRUE(rc);
    mympd_api_timer_timerlist_clear(&l);

    rc = mympd_api_timer_file_read(&l, workdir);
    ASSERT_EQ(1, l.length);
    ASSERT_TRUE(rc);
    ASSERT_STREQ("example timer1", l.list->definition->name);

    mympd_api_timer_timerlist_clear(&l);
    unlink("/tmp/mympd-test/state/timer_list");
    sdsfree(e);
    sdsfree(s1);
}
