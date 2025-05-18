/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Script signature functions
 */

#include "compile_time.h"
#include "src/scripts/verify.h"

#include "src/lib/log.h"
#include "src/lib/mem.h"

#include <openssl/evp.h>
#include <openssl/pem.h>

// Private definitions
static size_t calc_b64_decode_len(sds b64input);
static int b64_decode(sds b64message, unsigned char** buffer);

// Public functions

/**
 * Verifies a script signature
 * @param script Script text to verify
 * @param signature_base64 Base64 encoded signature
 * @return true on success, else false
 */
bool script_sig_verify(sds script, sds signature_base64) {
    uint8_t dataPEM[] = SCRIPTS_SIGN_KEY_PUB;
    BIO *bio = BIO_new_mem_buf(dataPEM, -1);
    if (bio == NULL) {
        MYMPD_LOG_ERROR(NULL, "Failure creating bio");
        return false;
    }

    char *name = NULL;
    char *header = NULL;
    uint8_t *data = NULL;
    long datalen = 0;
    if (PEM_read_bio(bio, &name, &header, &data, &datalen) <= 0) {
        MYMPD_LOG_ERROR(NULL, "Failure reading bio");
        BIO_free_all(bio);
        return false;
    }

    const uint8_t *data_pkey = (const uint8_t*)data;
    EVP_PKEY *pub_key = NULL;
    if (d2i_PUBKEY(&pub_key, &data_pkey, datalen) == NULL) {
        MYMPD_LOG_ERROR(NULL, "Failure reading public key");
        BIO_free_all(bio);
        return false;
    }

    int verify_status = 0;
    EVP_MD_CTX* verify_ctx = EVP_MD_CTX_create();
    if (EVP_DigestVerifyInit(verify_ctx, NULL, EVP_sha256(), NULL, pub_key) == 1 &&
        EVP_DigestVerifyUpdate(verify_ctx, script, sdslen(script)) == 1)
    {
        unsigned char *signature;
        int signature_len = b64_decode(signature_base64, &signature);
        verify_status = EVP_DigestVerifyFinal(verify_ctx, signature, (size_t)signature_len);
        FREE_PTR(signature);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Failure verifying signature");
    }
    EVP_MD_CTX_free(verify_ctx);
    EVP_PKEY_free(pub_key);
    BIO_free_all(bio);
    if (name != NULL) {
        OPENSSL_free(name);
    }
    if (header != NULL) {
        OPENSSL_free(header);
    }
    if (data != NULL) {
        OPENSSL_free(data);
    }
    return verify_status == 1
        ? true
        : false;
}

// Private functions

/**
 * Base64 decoding
 * @param b64message Message to decode
 * @param buffer Pointer to buffer to malloc
 * @return Length of buffer
 */
static int b64_decode(sds b64message, unsigned char **buffer) {
    BIO *bio;
    BIO *b64;
  
    size_t decode_len = calc_b64_decode_len(b64message);
    *buffer = malloc_assert(decode_len + 1);
    (*buffer)[decode_len] = '\0';
  
    bio = BIO_new_mem_buf(b64message, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
  
    int length = BIO_read(bio, *buffer, (int)sdslen(b64message));
    BIO_free_all(bio);
    return length;
}

/**
 * Calculates the length for base64 decoded message
 * @param b64input Base64 decoded message
 * @return size_t Length of decoded message
 */
static size_t calc_b64_decode_len(sds b64input) {
    size_t len = sdslen(b64input);
    size_t padding = 0;
  
    if (b64input[len-1] == '=' && b64input[len-2] == '=') {
        padding = 2;
    }
    else if (b64input[len-1] == '=') {
        padding = 1;
    }
    return ((len * 3) / 4) - padding;
}
