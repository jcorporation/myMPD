/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>

#include "../dist/src/sds/sds.h"

#include "mympd_config_defs.h"

#ifdef ENABLE_SSL
    #include <openssl/evp.h>
#endif

#include "state_files.h"
#include "mympd_pin.h"

void mympd_set_pin(sds workdir) {
    struct termios old, new;
    if (tcgetattr(fileno(stdin), &old) != 0) {
        return;
    }
    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0) {
        return;
    }
        
    printf("Enter pin: ");
    char c;
    sds pin = sdsempty();
    while ((c = getc(stdin)) != '\n') {
        pin = sdscatprintf(pin, "%c", c);
    }
    sds hex_hash;
    if (sdslen(pin) == 0) {
        hex_hash = sdsempty();
    }
    else {
        hex_hash = hash_pin(pin);
    }
    bool rc = state_file_write(workdir, "config", "pin_hash", hex_hash);
    sdsfree(hex_hash);
    tcsetattr(fileno(stdin), TCSAFLUSH, &old);
    printf("\n");
    if (rc == true) {
        if (sdslen(pin) > 0) {
            printf("Pin is now set, restart myMPD to apply.\n");
        }
        else {
            printf("Pin is now cleared, restart myMPD to apply.\n");
        }
    }
    sdsfree(pin);
}

sds hash_pin(const char *pin) {
    sds hex_hash = sdsempty();
#ifdef ENABLE_SSL
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == NULL) {
        return hex_hash;
    }
    if (EVP_DigestInit_ex(context, EVP_sha256(), NULL) == 0) {
        EVP_MD_CTX_free(context);
        return hex_hash;
    }
    if (EVP_DigestUpdate(context, pin, strlen(pin)) == 0) {
        EVP_MD_CTX_free(context);
        return hex_hash;
    }
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if(EVP_DigestFinal_ex(context, hash, &hash_len) == 0) {
        EVP_MD_CTX_free(context);
        return hex_hash;
    }
    
    for (unsigned int i = 0; i < hash_len; i++) {
        hex_hash = sdscatprintf(hex_hash, "%02x", hash[i]);
    }
    EVP_MD_CTX_free(context);
#else
    (void) pin;
#endif
    return hex_hash;
}
