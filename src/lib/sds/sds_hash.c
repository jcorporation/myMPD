/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Hash functions for sds strings
 */

#include "compile_time.h"
#include "src/lib/sds/sds_hash.h"

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"

#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string.h>

/**
 * Hashes a string with md5
 * @param p string to hash
 * @return the hash as a newly allocated sds string
 */
sds sds_hash_md5(const char *p) {
    mg_md5_ctx ctx;
    mg_md5_init(&ctx);
    mg_md5_update(&ctx, (unsigned char *)p, strlen(p));
    unsigned char hash[16];
    mg_md5_final(&ctx, hash);
    sds hex_hash = sdsempty();
    for (unsigned i = 0; i < 16; i++) {
        hex_hash = sdscatprintf(hex_hash, "%02x", hash[i]);
    }
    return hex_hash;
}

/**
 * Hashes a string with sha1
 * @param p string to hash
 * @return the hash as a newly allocated sds string
 */
sds sds_hash_sha1(const char *p) {
    sds hex_hash = sdsnew(p);
    return sds_hash_sha1_sds(hex_hash);
}

/**
 * Hashes a sds string with sha1 inplace
 * @param s string to hash
 * @return pointer to s
 */
sds sds_hash_sha1_sds(sds s) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)s, sdslen(s), hash);
    sdsclear(s);
    for (unsigned i = 0; i < SHA_DIGEST_LENGTH; i++) {
        s = sdscatprintf(s, "%02x", hash[i]);
    }
    return s;
}

/**
 * Hashes a string with sha256
 * @param p string to hash
 * @return the hash as a newly allocated sds string
 */
sds sds_hash_sha256(const char *p) {
    sds hex_hash = sdsnew(p);
    return sds_hash_sha256_sds(hex_hash);
}

/**
 * Hashes a sds string with sha256 inplace
 * @param s string to hash
 * @return pointer to s
 */
sds sds_hash_sha256_sds(sds s) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)s, sdslen(s), hash);
    sdsclear(s);
    for (unsigned i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        s = sdscatprintf(s, "%02x", hash[i]);
    }
    return s;
}
