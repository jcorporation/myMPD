/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/pin.h"

#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"

#include <openssl/evp.h>
#include <string.h>
#include <termios.h>

/**
 * Reads the pin from stdin and sets it
 * @param workdir working directory
 * @return true on success else false
 */
bool pin_set(sds workdir) {
    struct termios old;
    if (tcgetattr(fileno(stdin), &old) != 0) {
        return false;
    }
    struct termios new = old;
    new.c_lflag &= ~( 0U | ECHO );
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0) {
        return false;
    }

    int c;
    sds pin = sdsempty();
    sds pin2 = sdsempty();

    printf("Enter pin: ");
    while ((c = getc(stdin)) != '\n') {
        pin = sds_catchar(pin, (char)c);
    }

    printf("\nRe-enter pin: ");
    while ((c = getc(stdin)) != '\n') {
        pin2 = sds_catchar(pin2, (char)c);
    }
    tcsetattr(fileno(stdin), TCSAFLUSH, &old);

    if (strcmp(pin, pin2) != 0) {
        FREE_SDS(pin);
        FREE_SDS(pin2);
        printf("\nPins do not match, please try again.\n");
        return false;
    }

    if (sdslen(pin) > 0) {
        pin = sds_hash_sha256_sds(pin);
    }
    bool rc = state_file_write(workdir, "config", "pin_hash", pin);

    printf("\n");
    if (rc == true) {
        if (sdslen(pin) > 0) {
            printf("Pin is now set, restart myMPD to apply.\n");
        }
        else {
            printf("Pin is now cleared, restart myMPD to apply.\n");
        }
    }
    else {
        printf("Error setting pin.\n");
    }
    FREE_SDS(pin);
    FREE_SDS(pin2);
    return true;
}

/**
 * Validates the pin
 * @param pin pin to validate
 * @param hash hash to validate against
 * @return true on success else false
 */
bool pin_validate(const char *pin, const char *hash) {
    if (hash[0] == '\0') {
        MYMPD_LOG_ERROR(NULL, "No pin is set");
        return false;
    }
    sds test_hash = sds_hash_sha256(pin);
    bool rc = false;
    if (strcmp(test_hash, hash) == 0) {
        MYMPD_LOG_INFO(NULL, "Valid pin entered");
        rc = true;
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Invalid pin entered");
    }
    FREE_SDS(test_hash);
    return rc;
}
