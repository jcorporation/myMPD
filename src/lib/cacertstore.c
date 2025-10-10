/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief CA cert store handling
 */

#include "compile_time.h"
#include "src/lib/cacertstore.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"

#include <openssl/x509.h>

/**
 * Paths to check for the ca cert store
 */
const char *check_ca_cert_paths[] = {
    "/etc/ssl/ca-bundle.pem",
    "/etc/ssl/certs/ca-certificates.crt",
    "/etc/ssl/certs/ca-bundle.crt",
    "/etc/pki/tls/certs/ca-bundle.crt",
    "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
    NULL
};

/**
 * Try to find the system ca cert store
 * @param silent Print no error messages?
 * @return const char* or NULL if not found
 */
const char *find_ca_cert_store(bool silent) {
    const char *file = X509_get_default_cert_file();
    if (testfile_read(file) == true) {
        return file;
    }
    MYMPD_LOG_DEBUG(NULL, "Default CA cert store not accessible, try out some standard locations");
    // Fallback to manual detection
    const char **p = check_ca_cert_paths;
    while (*p != NULL) {
        if (testfile_read(*p) == true) {
            return *p;
        }
        p++;
    }
    if (silent == false) {
        MYMPD_LOG_ERROR(NULL, "CA cert store not found.");
    }
    return NULL;
}
