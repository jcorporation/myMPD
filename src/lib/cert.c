/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "cert.h"

#include "list.h"
#include "log.h"
#include "sds_extras.h"
#include "utility.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

//private definitions

static sds get_san(sds buffer);
static int generate_set_random_serial(X509 *crt);
static X509_REQ *generate_request(EVP_PKEY *pkey);
static void add_extension(X509V3_CTX *ctx, X509 *cert, int nid, const char *value);
static X509 *sign_certificate_request(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *req, sds san);
static EVP_PKEY *generate_keypair(int rsa_key_bits);
static X509 *generate_selfsigned_cert(EVP_PKEY *pkey);
static bool write_to_disk(sds key_file, EVP_PKEY *pkey, sds cert_file, X509 *cert);
static bool load_certificate(sds key_file, EVP_PKEY **key, sds cert_file, X509 **cert);
static bool create_ca_certificate(sds cakey_file, EVP_PKEY **ca_key, sds cacert_file, X509 **ca_cert);
static bool create_server_certificate(sds serverkey_file, EVP_PKEY **server_key,
                                      sds servercert_file, X509 **server_cert,
                                      sds custom_san, EVP_PKEY **ca_key, X509 **ca_cert);
static int check_expiration(X509 *cert, sds cert_file, int min_days, int max_days);
static bool certificates_cleanup(sds dir, const char *name);
//public functions

bool certificates_check(sds workdir, sds ssl_san) {
    sds testdirname = sdscatfmt(sdsempty(), "%s/ssl", workdir);
    int testdir_rc = testdir("SSL cert dir", testdirname, true);
    if (testdir_rc == DIR_EXISTS || testdir_rc == DIR_CREATED) {
        if (certificates_create(testdirname, ssl_san) == false) {
            //error creating certificates
            MYMPD_LOG_ERROR("Certificate creation failed");
            FREE_SDS(testdirname);
            return false;
        }
        FREE_SDS(testdirname);
    }
    else {
        FREE_SDS(testdirname);
        return false;
    }
    return true;
}

bool certificates_create(sds dir, sds custom_san) {
    bool rc_ca = false;
    bool rc_cert = false;

    //read ca certificate / private key or create it
    sds cacert_file = sdscatfmt(sdsempty(), "%s/ca.pem", dir);
    sds cakey_file = sdscatfmt(sdsempty(), "%s/ca.key", dir);
    EVP_PKEY *ca_key = NULL;
    X509 *ca_cert = NULL;

    if (load_certificate(cakey_file, &ca_key, cacert_file, &ca_cert) == false) {
        rc_ca = create_ca_certificate(cakey_file, &ca_key, cacert_file, &ca_cert);
    }
    else {
        MYMPD_LOG_NOTICE("CA certificate and private key found");
        int rc_expires = check_expiration(ca_cert, cacert_file, CA_LIFETIME_MIN, CA_LIFETIME);
        if (rc_expires == 0) {
            rc_ca = true;
        }
        else {
            EVP_PKEY_free(ca_key);
            X509_free(ca_cert);
            ca_key = NULL;
            ca_cert = NULL;
            rc_ca = certificates_cleanup(dir, "ca");
            if (rc_ca == true) {
                rc_ca = certificates_cleanup(dir, "server");
                if (rc_ca == true) {
                    rc_ca = create_ca_certificate(cakey_file, &ca_key, cacert_file, &ca_cert);
                }
            }
        }
    }

    //read server certificate / privat key or create it
    sds servercert_file = sdscatfmt(sdsempty(), "%s/server.pem", dir);
    sds serverkey_file = sdscatfmt(sdsempty(), "%s/server.key", dir);
    EVP_PKEY *server_key = NULL;
    X509 *server_cert = NULL;

    if (load_certificate(serverkey_file, &server_key, servercert_file, &server_cert) == false) {
        rc_cert = create_server_certificate(serverkey_file, &server_key, servercert_file, &server_cert,
            custom_san, &ca_key, &ca_cert);
    }
    else {
        MYMPD_LOG_NOTICE("Server certificate and private key found");
        int rc_expires = check_expiration(server_cert, servercert_file, CERT_LIFETIME_MIN, CERT_LIFETIME);
        if (rc_expires == 0) {
            rc_cert = true;
        }
        else {
            EVP_PKEY_free(server_key);
            X509_free(server_cert);
            server_key = NULL;
            server_cert = NULL;
            rc_cert = certificates_cleanup(dir, "server");
            if (rc_cert == true) {
                rc_cert = create_server_certificate(serverkey_file, &server_key, servercert_file, &server_cert,
                    custom_san, &ca_key, &ca_cert);
            }
        }
    }

    FREE_SDS(cacert_file);
    FREE_SDS(cakey_file);
    FREE_SDS(servercert_file);
    FREE_SDS(serverkey_file);
    EVP_PKEY_free(ca_key);
    X509_free(ca_cert);
    EVP_PKEY_free(server_key);
    X509_free(server_cert);
    if (rc_ca == false || rc_cert == false) {
        return false;
    }
    return true;
}

//private functions

static bool certificates_cleanup(sds dir, const char *name) {
    sds cert_file = sdscatfmt(sdsempty(), "%s/%s.pem", dir, name);
    errno = 0;
    if (unlink(cert_file) != 0) {
        MYMPD_LOG_ERROR("Error removing file \"%s\"", cert_file);
        MYMPD_LOG_ERRNO(errno);
    }
    FREE_SDS(cert_file);
    sds key_file = sdscatfmt(sdsempty(), "%s/%s.key", dir, name);
    errno = 0;
    if (unlink(key_file) != 0) {
        MYMPD_LOG_ERROR("Error removing file \"%s\"", key_file);
        MYMPD_LOG_ERRNO(errno);
    }
    FREE_SDS(key_file);

    return true;
}

static int check_expiration(X509 *cert, sds cert_file, int min_days, int max_days) {
    ASN1_TIME *not_after = X509_get_notAfter(cert);
    int pday = 0;
    int psec = 0;
    int rc = ASN1_TIME_diff(&pday, &psec, NULL, not_after);
    if (rc == 1) {
        MYMPD_LOG_DEBUG("Certificate %s expires in %d days", cert_file, pday);
        if (pday > max_days || pday < min_days) {
            MYMPD_LOG_WARN("Certificate %s must be renewed, expires in %d days", cert_file, pday);
            return 1;
        }
    }
    else {
        MYMPD_LOG_ERROR("Can not parse date from certificate file: %s", cert_file);
        return -1;
    }
    return 0;
}

static bool create_ca_certificate(sds cakey_file, EVP_PKEY **ca_key, sds cacert_file, X509 **ca_cert) {
    MYMPD_LOG_NOTICE("Creating self signed ca certificate");
    *ca_key = generate_keypair(CA_KEY_LENGTH);
    if (*ca_key == NULL) {
        return false;
    }

    *ca_cert = generate_selfsigned_cert(*ca_key);
    if (*ca_cert == NULL) {
        return false;
    }
    bool rc_ca = write_to_disk(cakey_file, *ca_key, cacert_file, *ca_cert);
    return rc_ca;
}

static bool create_server_certificate(sds serverkey_file, EVP_PKEY **server_key,
                                      sds servercert_file, X509 **server_cert,
                                      sds custom_san, EVP_PKEY **ca_key, X509 **ca_cert)
{
    MYMPD_LOG_NOTICE("Creating server certificate");
    *server_key = generate_keypair(CERT_KEY_LENGTH);
    if (*server_key == NULL) {
        return false;
    }
    X509_REQ *server_req = generate_request(*server_key);
    if (server_req == NULL) {
        return false;
    }
    //get subject alternative names
    sds san = sdsempty();
    san = get_san(san);
    if (sdslen(custom_san) > 0) {
        san = sdscatfmt(san, ", %s", custom_san);
    }
    MYMPD_LOG_NOTICE("Set server certificate san to: %s", san);
    *server_cert = sign_certificate_request(*ca_key, *ca_cert, server_req, san);
    X509_REQ_free(server_req);
    if (*server_cert == NULL) {
        FREE_SDS(san);
        return false;
    }
    FREE_SDS(san);
    bool rc_cert = write_to_disk(serverkey_file, *server_key, servercert_file, *server_cert);
    return rc_cert;
}

//loads the certificate and the key
static bool load_certificate(sds key_file, EVP_PKEY **key, sds cert_file, X509 **cert) {
	BIO *bio = NULL;
	*cert = NULL;
	*key = NULL;

	/* Load private key. */
	bio = BIO_new(BIO_s_file());
	if (!BIO_read_filename(bio, key_file)) {
	    BIO_free_all(bio);
	    return false;
	}
	*key = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
	BIO_free_all(bio);
	if (*key == NULL) {
	    return false;
	}

	/* Load certificate. */
	bio = BIO_new(BIO_s_file());
	if (!BIO_read_filename(bio, cert_file)) {
	    BIO_free_all(bio);
	    EVP_PKEY_free(*key);
	    return false;
	}
	*cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free_all(bio);
	if (!*cert) {
	    EVP_PKEY_free(*key);
	    return false;
	}

	return true;
}

/*Gets local hostname and ip for subject alternative names */
static sds get_san(sds buffer) {
    sds key = NULL;
    struct t_list san;
    list_init(&san);
    list_push(&san, "DNS:localhost", 0, NULL, NULL);

    //Retrieve short hostname
    char hostbuffer[256]; /* Flawfinder: ignore */
    int hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname == 0) {
        key = sdscatfmt(sdsempty(), "DNS:%s", hostbuffer);
        list_push(&san, key, 0, NULL, NULL);
        //Retrieve fqdn
        struct addrinfo hints = {0};
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_CANONNAME;
        struct addrinfo *res;
        if (getaddrinfo(hostbuffer, 0, &hints, &res) == 0) {
            // The hostname was successfully resolved.
            if (strcmp(hostbuffer, res->ai_canonname) != 0) {
                sdsclear(key);
                key = sdscatfmt(sdsempty(), "DNS:%s", res->ai_canonname);
                list_push(&san, key, 0, NULL, NULL);
            }
            freeaddrinfo(res);
        }
    }
    //retrieve interface ip addresses
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    char host[NI_MAXHOST];

    errno = 0;
    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) {
                continue;
            }
            int family = ifa->ifa_addr->sa_family;
            if (family == AF_INET ||
                family == AF_INET6)
            {
                int s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                          sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);

                if (s != 0) {
                    MYMPD_LOG_ERROR("getnameinfo() failed: %s\n", gai_strerror(s));
                    continue;
                }
                sdsclear(key);
                key = sdscatfmt(sdsempty(), "IP:%s", host);
                list_push(&san, key, 0, NULL, NULL);
            }
        }
        freeifaddrs(ifaddr);
    }
    else {
        MYMPD_LOG_ERROR("Can not get list of inteface ip addresses");
        MYMPD_LOG_ERRNO(errno);
    }
    //create san string
    struct t_list_node *current = san.head;
    int i = 0;
    while (current->next != NULL) {
        if (i++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscatsds(buffer, current->key);
        current = current->next;
    }
    list_clear(&san);
    FREE_SDS(key);
    return buffer;
}

/* Generates a 20 byte random serial number and sets in certificate. */
static int generate_set_random_serial(X509 *crt) {
	unsigned char serial_bytes[20]; /* Flawfinder: ignore */
	if (RAND_bytes(serial_bytes, sizeof(serial_bytes)) != 1) {
	    return 0;
    }
	serial_bytes[0] &= 0x7f; /* Ensure positive serial! */
	BIGNUM *bn = BN_new();
	BN_bin2bn(serial_bytes, sizeof(serial_bytes), bn);
	ASN1_INTEGER *serial = ASN1_INTEGER_new();
	BN_to_ASN1_INTEGER(bn, serial);

	X509_set_serialNumber(crt, serial); // Set serial.

	ASN1_INTEGER_free(serial);
	BN_free(bn);
	return 1;
}

static X509_REQ *generate_request(EVP_PKEY *pkey) {
    X509_REQ *req = X509_REQ_new();
    if (!req) {
        MYMPD_LOG_ERROR("Unable to create X509_REQ structure");
        return NULL;
    }
    X509_REQ_set_pubkey(req, pkey);

    /* Set the DN */
    time_t now = time(NULL);
    sds cn = sdscatfmt(sdsempty(), "myMPD Server Certificate %U", (unsigned long long)now);

    X509_NAME *name = X509_REQ_get_subject_name(req);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)"DE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)"myMPD", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cn, -1, -1, 0);

    FREE_SDS(cn);

    if (!X509_REQ_sign(req, pkey, EVP_sha256())) {
        MYMPD_LOG_ERROR("Error signing request");
        X509_REQ_free(req);
        return NULL;
    }
    return req;
}

static void add_extension(X509V3_CTX *ctx, X509 *cert, int nid, const char *value) {
    X509_EXTENSION *ex = X509V3_EXT_conf_nid(NULL, ctx, nid, value);
    X509_add_ext(cert, ex, -1);
    X509_EXTENSION_free(ex);
}

static X509 *sign_certificate_request(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *req, sds san) {
    X509 *cert = X509_new();
    if (!cert) {
        MYMPD_LOG_ERROR("Unable to create X509 structure");
        return NULL;
    }

    /* Set version to X509v3 */
    X509_set_version(cert, 2);

    /* Set the serial number. */
    generate_set_random_serial(cert);

    /* Set issuer to CA's subject. */
    X509_set_issuer_name(cert, X509_get_subject_name(ca_cert));

    /* This certificate is valid from now until one year from now. */
    int lifetime = CERT_LIFETIME * 24 * 60 * 60;
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), lifetime);

    /* Get the request's subject and just use it (we don't bother checking it since we generated
     * it ourself). Also take the request's public key. */
    X509_set_subject_name(cert, X509_REQ_get_subject_name(req));
    EVP_PKEY *req_pubkey = X509_REQ_get_pubkey(req);
    X509_set_pubkey(cert, req_pubkey);
    EVP_PKEY_free(req_pubkey);

    /* Set extensions. */
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, ca_cert, cert, NULL, NULL, 0);
    add_extension(&ctx, cert, NID_basic_constraints, "CA:false");
    add_extension(&ctx, cert, NID_key_usage, "Digital Signature, Key Encipherment, Data Encipherment");
    add_extension(&ctx, cert, NID_ext_key_usage, "TLS Web Server Authentication");
    add_extension(&ctx, cert, NID_subject_alt_name, san);

    /* Now perform the actual signing with the CA. */
    if (X509_sign(cert, ca_key, EVP_sha256()) == 0) {
        X509_free(cert);
        return NULL;
    }

    return cert;
}

static EVP_PKEY *generate_keypair(int rsa_key_bits) {
    EVP_PKEY_CTX *ctx;
    EVP_PKEY *pkey = NULL;

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) {
        return NULL;
    }
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, rsa_key_bits) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }
    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

/* Generates a self-signed x509 certificate. */
static X509 *generate_selfsigned_cert(EVP_PKEY *pkey) {
    /* Allocate memory for the X509 structure. */
    X509 *cert = X509_new();
    if (!cert) {
        MYMPD_LOG_ERROR("Unable to create X509 structure");
        return NULL;
    }

    /* Set version to X509v3 */
    X509_set_version(cert, 2);

    /* Set the serial number. */
    generate_set_random_serial(cert);

    /* This certificate is valid from now until ten years from now. */
    int lifetime = CA_LIFETIME * 24 * 60 * 60;
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), lifetime);

    /* Set the public key for our certificate. */
    X509_set_pubkey(cert, pkey);

    /* We want to copy the subject name to the issuer name. */
    X509_NAME *name = X509_get_subject_name(cert);

    /* Set the DN */
    time_t now = time(NULL);
    sds cn = sdscatfmt(sdsempty(), "myMPD CA %U", (unsigned long long)now);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)"DE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)"myMPD", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)cn, -1, -1, 0);
    FREE_SDS(cn);

    /* Now set the issuer name. */
    X509_set_issuer_name(cert, name);

    /* Set ca extension. */
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
    add_extension(&ctx, cert, NID_basic_constraints, "critical, CA:true");
    add_extension(&ctx, cert, NID_key_usage, "critical, Certificate Sign, CRL Sign");

    /* Self sign the certificate with our key. */
    if (!X509_sign(cert, pkey, EVP_sha256())) {
        MYMPD_LOG_ERROR("Error signing certificate");
        X509_free(cert);
        return NULL;
    }

    return cert;
}

static bool write_to_disk(sds key_file, EVP_PKEY *pkey, sds cert_file, X509 *cert) {
    /* Write the key to disk. */
    sds key_file_tmp = sdscatfmt(sdsempty(), "%s.XXXXXX", key_file);
    errno = 0;
    int fd = mkstemp(key_file_tmp);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", key_file_tmp);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(key_file_tmp);
        return false;
    }
    FILE *key_file_fp = fdopen(fd, "w");
    bool rc = PEM_write_PrivateKey(key_file_fp, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(key_file_fp);
    if (!rc) {
        MYMPD_LOG_ERROR("Unable to write private key to disk");
        FREE_SDS(key_file_tmp);
        return false;
    }
    errno = 0;
    if (rename(key_file_tmp, key_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from %s to %s failed", key_file_tmp, key_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(key_file_tmp);
        return false;
    }
    FREE_SDS(key_file_tmp);

    /* Write the certificate to disk. */
    sds cert_file_tmp = sdscatfmt(sdsempty(), "%s.XXXXXX", cert_file);
    errno = 0;
    if ((fd = mkstemp(cert_file_tmp)) < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", cert_file_tmp);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(cert_file_tmp);
        return false;
    }
    FILE *cert_file_fp = fdopen(fd, "w");
    rc = PEM_write_X509(cert_file_fp, cert);
    fclose(cert_file_fp);
    if (!rc) {
        MYMPD_LOG_ERROR("Unable to write certificate to disk");
        FREE_SDS(cert_file_tmp);
        return false;
    }
    errno = 0;
    if (rename(cert_file_tmp, cert_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from %s to %s failed", cert_file_tmp, cert_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(cert_file_tmp);
        return false;
    }
    FREE_SDS(cert_file_tmp);

    return true;
}
