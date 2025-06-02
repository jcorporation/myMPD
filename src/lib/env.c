/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Environment handling
 */

#include "compile_time.h"
#include "src/lib/env.h"

#include "src/lib/convert.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

/**
 * Gets an environment variable and checks its length
 * @param env_var environment variable name
 * @return environment variable value or NULL if it is not set or too long
 */
const char *getenv_check(const char *env_var) {
    const char *env_value = getenv(env_var); /* Flawfinder: ignore */
    if (env_value == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Environment variable \"%s\" not set", env_var);
        return NULL;
    }
    if (env_value[0] == '\0') {
        MYMPD_LOG_DEBUG(NULL, "Environment variable \"%s\" is empty", env_var);
        return NULL;
    }
    if (strlen(env_value) > MAX_ENV_LENGTH) {
        MYMPD_LOG_WARN(NULL, "Environment variable \"%s\" is too long", env_var);
        return NULL;
    }
    MYMPD_LOG_INFO(NULL, "Got environment variable \"%s\" with value \"%s\"", env_var, env_value);
    return env_value;
}

/**
 * Gets an environment variable as sds string
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param vcb validation callback
 * @param rc Pointer to bool to set return code
 * @return environment variable as sds string or default_value if validation fails
 */
sds getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool *rc) {
    const char *env_value = getenv_check(env_var);
    *rc = false;
    if (env_value == NULL) {
        return sdsnew(default_value);
    }
    sds value = sdsnew(env_value);
    if (vcb == NULL ||
        vcb(value) == true)
    {
        *rc = true;
        return value;
    }
    FREE_SDS(value);
    return sdsnew(default_value);
}

/**
 * Gets an environment variable as int
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param rc Pointer to bool to set return code
 * @return environment variable as integer or default_value if validation fails
 */
int getenv_int(const char *env_var, int default_value, int min, int max, bool *rc) {
    const char *env_value = getenv_check(env_var);
    *rc = false;
    if (env_value == NULL) {
        return default_value;
    }
    int value;
    enum str2int_errno crc = str2int(&value, env_value);
    if (crc != STR2INT_SUCCESS) {
        return default_value;
    }
    if (value >= min && value <= max) {
        *rc = true;
        return value;
    }
    MYMPD_LOG_WARN(NULL, "Invalid value for \"%s\" using default", env_var);
    return default_value;
}

/**
 * Gets an environment variable as int
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param rc Pointer to bool to set return code
 * @return environment variable as unsigned integer or default_value if validation fails
 */
unsigned getenv_uint(const char *env_var, unsigned default_value, unsigned min, unsigned max, bool *rc) {
    const char *env_value = getenv_check(env_var);
    *rc = false;
    if (env_value == NULL) {
        return default_value;
    }
    unsigned value;
    enum str2int_errno crc = str2uint(&value, env_value);
    if (crc != STR2INT_SUCCESS) {
        return default_value;
    }
    if (value >= min && value <= max) {
        *rc = true;
        return value;
    }
    MYMPD_LOG_WARN(NULL, "Invalid value for \"%s\" using default", env_var);
    return default_value;
}

/**
 * Gets an environment variable as bool
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param rc Pointer to bool to set return code
 * @return environment variable as bool or default_value if validation fails
 */
bool getenv_bool(const char *env_var, bool default_value, bool *rc) {
    const char *env_value = getenv_check(env_var);
    *rc = false;
    if (env_value == NULL) {
        return default_value;
    }
    if (strcmp(env_value, "true") == 0) {
        *rc = true;
        return true;
    }
    if (strcmp(env_value, "false") == 0) {
        *rc = true;
        return false;
    }
    MYMPD_LOG_WARN(NULL, "Invalid value for \"%s\" using default", env_var);
    return default_value;
}
