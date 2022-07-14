/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/api.h"

UTEST(api, test_get_cmd_id) {
    enum mympd_cmd_ids cmd_id = get_cmd_id("MYMPD_API_COLS_SAVE");
    const bool rc = cmd_id == MYMPD_API_COLS_SAVE ? true : false;
    ASSERT_TRUE(rc);
}

UTEST(api, test_get_cmd_id_method_name) {
    const char *name = get_cmd_id_method_name(MYMPD_API_COLS_SAVE);
    ASSERT_STREQ(name, "MYMPD_API_COLS_SAVE");
}

UTEST(api, test_is_protected_api_method) {
    bool rc = is_protected_api_method(MYMPD_API_COLS_SAVE);
    ASSERT_FALSE(rc);

    rc = is_protected_api_method(MYMPD_API_SETTINGS_SET);
    ASSERT_TRUE(rc);
}

UTEST(api, test_is_public_api_method) {
    bool rc = is_public_api_method(MYMPD_API_SETTINGS_SET);
    ASSERT_TRUE(rc);

    rc = is_public_api_method(INTERNAL_API_STATE_SAVE);
    ASSERT_FALSE(rc);
}

UTEST(api, test_is_mympd_only_api_method) {
    bool rc = is_mympd_only_api_method(MYMPD_API_CONNECTION_SAVE);
    ASSERT_TRUE(rc);

    rc = is_mympd_only_api_method(MYMPD_API_SETTINGS_SET);
    ASSERT_FALSE(rc);
}

UTEST(api, test_request_result) {
    struct t_work_request *request = create_request(1, 1, MYMPD_API_SETTINGS_SET, "test");
    bool rc = request == NULL ? false : true;
    ASSERT_TRUE(rc);

    struct t_work_response *response = create_response(request);
    rc = response == NULL ? false : true;
    ASSERT_TRUE(rc);
    ASSERT_EQ(request->cmd_id, response->cmd_id);
    ASSERT_STREQ(response->method, "MYMPD_API_SETTINGS_SET");

    free_request(request);
    free_response(response);
}
