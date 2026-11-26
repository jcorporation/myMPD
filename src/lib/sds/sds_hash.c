/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Hash functions for sds strings
 */

#include "src/lib/sds/sds_hash.h"

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"

#include <openssl/sha.h>
#include <string.h>

/**
 * Converts a hash to a hexadecimal string
 * @param output pointer to the output buffer (must be at least length*2+1 bytes long)
 * @param hash pointer to the hash
 * @param length length of the hash in bytes
 */
static void hash_to_hex(char *output, const unsigned char *hash, size_t length) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < length; i++) {
        output[i * 2] = hex[hash[i] >> 4];
        output[i * 2 + 1] = hex[hash[i] & 0xf];
    }
}

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
    sds hex_hash = sdsMakeRoomFor(sdsempty(), sizeof(hash) * 2);
    hash_to_hex(hex_hash, hash, sizeof(hash));
    sdsIncrLen(hex_hash, sizeof(hash) * 2);
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
    s = sdsMakeRoomFor(s, SHA_DIGEST_LENGTH * 2);
    hash_to_hex(s, hash, SHA_DIGEST_LENGTH);
    sdsIncrLen(s, SHA_DIGEST_LENGTH * 2);
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
    s = sdsMakeRoomFor(s, SHA256_DIGEST_LENGTH * 2);
    hash_to_hex(s, hash, SHA256_DIGEST_LENGTH);
    sdsIncrLen(s, SHA256_DIGEST_LENGTH * 2);
    return s;
}
