/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

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
    ASSERT_EQ(1, l.list.length);
    ASSERT_EQ(USER_TIMER_ID_START, l.last_id);

    mympd_api_timer_add(&l, 10, 0, timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
    mympd_api_timer_add(&l, 10, 0, timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
    ASSERT_EQ(3, l.list.length);
    
    rc = mympd_api_timer_replace(&l, 10, 0, timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
    ASSERT_TRUE(rc);
    ASSERT_EQ(3, l.list.length);
    
    mympd_api_timer_remove(&l, TIMER_ID_CACHES_CREATE);
    ASSERT_EQ(2, l.list.length);

    mympd_api_timer_timerlist_clear(&l);
}

UTEST(timer, test_timer_parse_definition) {
    struct t_timer_list l;
    mympd_api_timer_timerlist_init(&l);
    sds e = sdsempty();
    sds s1 = sdsnew("{\"params\":{\"partition\":\"default\",\"timerid\":103,\"name\":\"example timer1\",\"interval\":86400,\"enabled\":true,\"startHour\":7,\"startMinute\":0,\"action\":\"player\",\"subaction\":\"startplay\",\"playlist\":\"test\",\"volume\":50,\"preset\":\"\",\"weekdays\":[false,false,false,false,false,true,true],\"arguments\": {\"arg1\":\"value1\"}}}");
    struct t_jsonrpc_parse_error parse_error;
    jsonrpc_parse_error_init(&parse_error);
    struct t_timer_definition *def1 =  mympd_api_timer_parse(s1, MPD_PARTITION_DEFAULT, &parse_error);
    ASSERT_TRUE(parse_error.message == NULL);
    bool rc = mympd_api_timer_add(&l, 10, 0, timer_handler_select, 103, def1);
    ASSERT_TRUE(rc);
    struct t_timer_node *timer_node = (struct t_timer_node *)l.list.head->user_data;
    ASSERT_STREQ("example timer1", timer_node->definition->name);

    sds s2 = sdsnew("{\"params\":{\"partition\":\"default\",\"timerid\":103,\"name\":\"example timer2\",\"interval\":86400,\"enabled\":true,\"startHour\":7,\"startMinute\":0,\"action\":\"player\",\"subaction\":\"startplay\",\"playlist\":\"\",\"volume\":50,\"preset\":\"test-preset\",\"weekdays\":[false,false,false,false,false,true,true],\"arguments\": {\"arg1\":\"value1\"}}}");
    struct t_timer_definition *def2 = mympd_api_timer_parse(s2, MPD_PARTITION_DEFAULT, &parse_error);
    ASSERT_TRUE(parse_error.message == NULL);
    rc = mympd_api_timer_replace(&l, 10, 0, timer_handler_select, 103, def2);
    ASSERT_TRUE(rc);
    timer_node = (struct t_timer_node *)l.list.head->user_data;
    ASSERT_STREQ("example timer2", timer_node->definition->name);

    ASSERT_TRUE(timer_node->definition->enabled);
    rc = mympd_api_timer_toggle(&l, 103, &e);
    ASSERT_TRUE(rc);
    ASSERT_STREQ("", e);
    ASSERT_FALSE(timer_node->definition->enabled);

    sdsfree(e);
    sdsfree(s1);
    sdsfree(s2);
    mympd_api_timer_timerlist_clear(&l);
    jsonrpc_parse_error_clear(&parse_error);
}

UTEST(timer, test_timer_write_read) {
    init_testenv();

    struct t_timer_list l;
    mympd_api_timer_timerlist_init(&l);
    sds s1 = sdsnew("{\"params\":{\"partition\":\"default\",\"timerid\":103,\"name\":\"example timer1\",\"interval\":86400,\"enabled\":true,\"startHour\":7,\"startMinute\":0,\"action\":\"player\",\"subaction\":\"startplay\",\"playlist\":\"\",\"volume\":50,\"preset\":\"test-preset\",\"weekdays\":[false,false,false,false,false,true,true],\"arguments\": {\"arg1\":\"value1\"}}}");
    struct t_jsonrpc_parse_error parse_error;
    jsonrpc_parse_error_init(&parse_error);
    struct t_timer_definition *def1 =  mympd_api_timer_parse(s1, MPD_PARTITION_DEFAULT, &parse_error);
    ASSERT_TRUE(parse_error.message == NULL);
    bool rc = mympd_api_timer_add(&l, 10, 0, timer_handler_select, 103, def1);
    ASSERT_TRUE(rc);

    rc = mympd_api_timer_file_save(&l, workdir);
    ASSERT_TRUE(rc);
    mympd_api_timer_timerlist_clear(&l);

    rc = mympd_api_timer_file_read(&l, workdir);
    ASSERT_EQ(1, l.list.length);
    ASSERT_TRUE(rc);
    struct t_timer_node *timer_node = (struct t_timer_node *)l.list.head->user_data;
    ASSERT_STREQ("example timer1", timer_node->definition->name);

    mympd_api_timer_timerlist_clear(&l);
    sdsfree(s1);

    clean_testenv();
}
