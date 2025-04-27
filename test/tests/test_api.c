/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/api.h"

UTEST(api, test_get_cmd_id) {
    enum mympd_cmd_ids cmd_id = get_cmd_id("MYMPD_API_VIEW_SAVE");
    ASSERT_TRUE(cmd_id == MYMPD_API_VIEW_SAVE);
}

UTEST(api, test_get_cmd_id_invalid) {
    enum mympd_cmd_ids cmd_id = get_cmd_id("INVALID");
    ASSERT_TRUE(cmd_id == GENERAL_API_UNKNOWN);
}

UTEST(api, test_get_cmd_id_method_name) {
    const char *name = get_cmd_id_method_name(MYMPD_API_VIEW_SAVE);
    ASSERT_STREQ(name, "MYMPD_API_VIEW_SAVE");
}

UTEST(api, test_get_cmd_id_method_name_invalid) {
    const char *name = get_cmd_id_method_name(TOTAL_API_COUNT + 1);
    ASSERT_TRUE(name == NULL);
}

UTEST(api, test_get_cmd_id_method_name_invalid2) {
    const char *name = get_cmd_id_method_name(-1);
    ASSERT_TRUE(name == NULL);
}

UTEST(api, test_acl_internal) {
    bool rc = check_cmd_acl(MYMPD_API_VIEW_SAVE, API_INTERNAL);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(INTERNAL_API_ALBUMART_BY_ALBUMID, API_INTERNAL);
    ASSERT_TRUE(rc);
}

UTEST(api, test_acl_public) {
    bool rc = check_cmd_acl(GENERAL_API_NOT_READY, API_PUBLIC);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(MYMPD_API_VIEW_SAVE, API_PUBLIC);
    ASSERT_TRUE(rc);
}

UTEST(api, test_acl_protected) {
    bool rc = check_cmd_acl(MYMPD_API_VIEW_SAVE, API_PROTECTED);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(MYMPD_API_SETTINGS_SET, API_PROTECTED);
    ASSERT_TRUE(rc);
}

UTEST(api, test_acl_script) {
    bool rc = check_cmd_acl(INTERNAL_API_SCRIPT_INIT, API_SCRIPT);
    ASSERT_TRUE(rc);

    rc = check_cmd_acl(MYMPD_API_SETTINGS_SET, API_SCRIPT);
    ASSERT_FALSE(rc);
}

UTEST(api, test_acl_mpd_disconnecte) {
    bool rc = check_cmd_acl(MYMPD_API_CONNECTION_SAVE, API_MPD_DISCONNECTED);
    ASSERT_TRUE(rc);

    rc = check_cmd_acl(MYMPD_API_SETTINGS_SET, API_MPD_DISCONNECTED);
    ASSERT_FALSE(rc);
}

UTEST(api, test_acl_mympd_only) {
    bool rc = check_cmd_acl(MYMPD_API_CONNECTION_SAVE, API_MYMPD_ONLY);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(MYMPD_API_SMARTPLS_UPDATE_ALL, API_MYMPD_ONLY);
    ASSERT_TRUE(rc);
}

UTEST(api, test_acl_mympd_worker_only) {
    bool rc = check_cmd_acl(MYMPD_API_CONNECTION_SAVE, API_MYMPD_WORKER_ONLY);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(MYMPD_API_WEBRADIODB_UPDATE, API_MYMPD_WORKER_ONLY);
    ASSERT_TRUE(rc);
}

UTEST(api, test_acl_invalid) {
    bool rc = check_cmd_acl(MYMPD_API_CONNECTION_SAVE, API_INVALID);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(TOTAL_API_COUNT, API_INVALID);
    ASSERT_TRUE(rc);
}

UTEST(api, test_acl_invalid_cmd_id) {
    bool rc = check_cmd_acl(TOTAL_API_COUNT + 1, API_INVALID);
    ASSERT_FALSE(rc);

    rc = check_cmd_acl(-1, API_INVALID);
    ASSERT_FALSE(rc);
}

UTEST(api, test_request_result) {
    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, 1, 1, MYMPD_API_SETTINGS_SET, "test", MPD_PARTITION_DEFAULT);
    bool rc = request == NULL ? false : true;
    ASSERT_TRUE(rc);

    struct t_work_response *response = create_response(request);
    rc = response == NULL ? false : true;
    ASSERT_TRUE(rc);
    ASSERT_EQ(request->cmd_id, response->cmd_id);
    ASSERT_STREQ(get_cmd_id_method_name(response->cmd_id), "MYMPD_API_SETTINGS_SET");

    free_request(request);
    free_response(response);
}
