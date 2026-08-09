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

#include "src/lib/log.h"
#include "src/lib/sds/sds_file.h"

#include <openssl/x509.h>

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
