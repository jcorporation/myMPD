/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief CA cert store handling
 */

#include "compile_time.h"
#include "src/lib/config/cacertstore.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"

#include <openssl/x509.h>

/**
 * Paths to check for the ca cert store
 */
const char *check_ca_cert_paths[] = {
    "/etc/ssl/ca-bundle.pem",  // openSUSE
    "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", // Fedora
    "/etc/pki/tls/certs/ca-bundle.crt",  // Fedora
    "/etc/ssl/certs/ca-bundle.crt",
    "/etc/ssl/certs/ca-certificates.crt",  // Debian
    "/var/lib/ca-certificates/ca-bundle.pem",  // openSUSE
    NULL
};

/**
 * Try to find the system ca cert store
 * @param silent Print no error messages?
 * @return const char* or NULL if not found
 */
const char *find_ca_cert_store(bool silent) {
    const char **p = check_ca_cert_paths;
    while (*p != NULL) {
        if (testfile_read(*p) == true) {
            return *p;
        }
        p++;
    }
    MYMPD_LOG_DEBUG(NULL, "No system specific CA cert store found, using OpenSSL default");
    const char *file = X509_get_default_cert_file();
    if (testfile_read(file) == true) {
        return file;
    }
    if (silent == false) {
        MYMPD_LOG_EMERG(NULL, "CA cert store not found.");
    }
    return NULL;
}

/**
 * Reads the ca certificates
 * @param config Pointer to central config
 * @return true on success or disabled certificate checking, else false
 */
bool mympd_read_ca_certificates(struct t_config *config) {
    if (config->cert_check == false) {
        return true;
    }
    if (config->ca_cert_store == NULL ||
        sdslen(config->ca_cert_store) == 0)
    {
        MYMPD_LOG_EMERG(NULL, "System certificate store not found.");
        return false;
    }
    MYMPD_LOG_INFO(NULL, "Reading ca certificates from %s", config->ca_cert_store);
    config->ca_certs = sdsempty();
    int nread;
    config->ca_certs = sds_getfile(config->ca_certs, config->ca_cert_store, CACERT_STORE_SIZE_MAX, false, true, &nread);
    if (nread == FILE_TO_BIG) {
        MYMPD_LOG_EMERG(NULL, "System certificate store too big.");
        return false;
    }
    if (nread <= FILE_IS_EMPTY) {
        MYMPD_LOG_EMERG(NULL, "System certificate store not found or empty.");
        return false;
    }
    return true;
}
