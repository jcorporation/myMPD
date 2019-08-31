#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 

#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>
#include <openssl/bn.h>

#include <signal.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"

//private definitions

#define RSA_KEY_BITS 2048

static int get_san(char *buffer, size_t buffer_len);
static int generate_set_random_serial(X509 *crt);
static X509_REQ *generate_request(EVP_PKEY *pkey);
static void add_extension(X509V3_CTX *ctx, X509 *cert, int nid, const char *value);
static X509 *sign_certificate_request(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *req, const char *san);
static EVP_PKEY *generate_keypair(void);
static X509 *generate_selfsigned_cert(EVP_PKEY *pkey);
static bool write_to_disk(EVP_PKEY *pkey, X509 *cert, const char *dir, const char *name);

//public functions

bool create_certificates(char *dir) {
    LOG_INFO("Creating self signed ca certificate");
    EVP_PKEY *ca_key = generate_keypair();
    if (!ca_key) {
        return false;
    }
    
    X509 *ca_cert = generate_selfsigned_cert(ca_key);
    if (!ca_cert) {
        EVP_PKEY_free(ca_key);
        return false;
    }
    
    char san[2048];
    int san_len = get_san(san, 2048);
    if (san_len > 2048) {
        //buffer to small
        return false;
    }
    LOG_INFO("Creating server certificate with san: %s", san);
    EVP_PKEY *server_key = generate_keypair();
    if (!server_key) {
        EVP_PKEY_free(ca_key);
        X509_free(ca_cert);
        return false;
    }
    X509_REQ *server_req = generate_request(server_key);
    if (!server_req) {
        EVP_PKEY_free(ca_key);
        X509_free(ca_cert);
        EVP_PKEY_free(server_key);
        return false;
    }
    X509 *server_cert = sign_certificate_request(ca_key, ca_cert, server_req, san);
    if (!server_cert) {
        EVP_PKEY_free(ca_key);
        X509_free(ca_cert);
        EVP_PKEY_free(server_key);
        X509_REQ_free(server_req);
        return false;
    }

    bool rc_ca = write_to_disk(ca_key, ca_cert, dir, "ca");
    bool rc_cert = write_to_disk(server_key, server_cert, dir, "server");

    EVP_PKEY_free(ca_key);
    X509_free(ca_cert);
    EVP_PKEY_free(server_key);
    X509_free(server_cert);
    X509_REQ_free(server_req);
    if (!rc_ca || !rc_cert) {
        return false;
    }
    return true;
}

void cleanup_certificates(char *dir) {
    const char *files[]={"ca.key", "ca.pem", "server.key", "server.pem", 0};    
    const char** ptr = files;
    while (*ptr != 0) {
        size_t tmp_file_len = strlen(dir) + strlen(*ptr) + 2;
        char tmp_file[tmp_file_len];
        snprintf(tmp_file, tmp_file_len, "%s/%s", dir, *ptr);
        if (unlink(tmp_file) != 0) {
            LOG_ERROR("Error removing file %s", tmp_file);
        }
        ++ptr;
    }
    if (rmdir(dir) != 0) {
        LOG_ERROR("Error removing directory %s", dir);
    }
}

//private functions

/*Gets local hostname and ip for subject alternative names */
static int get_san(char *buffer, size_t buffer_len) {
    char hostbuffer[256]; 

    size_t len = snprintf(buffer, buffer_len, "DNS:localhost, IP:127.0.0.1");
  
    // To retrieve hostname 
    int hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    if (hostname == -1) {
        return len;
    }
    len += snprintf(buffer + len, buffer_len - len, ", DNS:%s", hostbuffer);

    struct addrinfo hints={0};
    hints.ai_family=AF_UNSPEC;
    hints.ai_flags=AI_CANONNAME;
    struct addrinfo* res=0;
    if (getaddrinfo(hostbuffer, 0, &hints, &res) == 0) {
        // The hostname was successfully resolved.
        if (strcmp(hostbuffer, res->ai_canonname) != 0) {
            len += snprintf(buffer + len, buffer_len - len, ", DNS:%s", res->ai_canonname);
        }
        freeaddrinfo(res);
    }
  
    // To retrieve host information 
    struct hostent *host_entry = gethostbyname(hostbuffer); 
    if (host_entry != NULL) {  
        // To convert an Internet network address into ASCII string 
        char *IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
        len += snprintf(buffer + len, buffer_len - len, ", IP:%s", IPbuffer);
    }
    
    return len;
}

/* Generates a 20 byte random serial number and sets in certificate. */
static int generate_set_random_serial(X509 *crt) {
	unsigned char serial_bytes[20];
	if (RAND_bytes(serial_bytes, sizeof(serial_bytes)) != 1) return 0;
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
        LOG_ERROR("Unable to create X509_REQ structure");
        return NULL;
    }
    X509_REQ_set_pubkey(req, pkey);

    /* Set the DN */
    X509_NAME *name = X509_REQ_get_subject_name(req);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)"DE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)"myMPD", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"myMPD Server Certificate", -1, -1, 0);
    
    if (!X509_REQ_sign(req, pkey, EVP_sha256())) {
        LOG_ERROR("Error signing request");
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

static X509 *sign_certificate_request(EVP_PKEY *ca_key, X509 *ca_cert, X509_REQ *req, const char *san) {
    X509 *cert = X509_new();
    if (!cert) {
        LOG_ERROR("Unable to create X509 structure");
        return NULL;
    }
        
    /* Set version to X509v3 */
    X509_set_version(cert, 2);

    /* Set the serial number. */
    generate_set_random_serial(cert);
    
    /* Set issuer to CA's subject. */
    X509_set_issuer_name(cert, X509_get_subject_name(ca_cert));
    
    /* This certificate is valid from now until exactly ten years from now. */
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 315360000);
    
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

static EVP_PKEY *generate_keypair(void) {
    RSA *rsa = RSA_new();
    if (!rsa) {
        LOG_ERROR("Unable to create RSA structure");
        return NULL;
    }
    
    /* Allocate memory for the EVP_PKEY structure. */
    EVP_PKEY *pkey = EVP_PKEY_new();
    if (!pkey) {
        LOG_ERROR("Unable to create EVP_PKEY structure");
        return NULL;
    }
    
    /* Generate the RSA key and assign it to pkey. */
    BIGNUM *e = BN_new();
    if (!e) {
        LOG_ERROR("Unable to create BN structure");
        EVP_PKEY_free(pkey);
        return NULL;
    }
    BN_set_word(e, 65537);
    RSA_generate_key_ex(rsa, RSA_KEY_BITS, e, NULL);
    if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
        LOG_ERROR("Unable to generate RSA key");
        BN_free(e);
        EVP_PKEY_free(pkey);
        return NULL;
    }
    
    BN_free(e);
    return pkey;
}

/* Generates a self-signed x509 certificate. */
static X509 *generate_selfsigned_cert(EVP_PKEY *pkey) {
    /* Allocate memory for the X509 structure. */
    X509 *cert = X509_new();
    if (!cert) {
        LOG_ERROR("Unable to create X509 structure");
        return NULL;
    }
    
    /* Set version to X509v3 */
    X509_set_version(cert, 2);
    
    /* Set the serial number. */
    generate_set_random_serial(cert);
    
    /* This certificate is valid from now until exactly ten years from now. */
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 315360000);
    
    /* Set the public key for our certificate. */
    X509_set_pubkey(cert, pkey);
    
    /* We want to copy the subject name to the issuer name. */
    X509_NAME *name = X509_get_subject_name(cert);
    
    /* Set the DN */
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)"DE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)"myMPD", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"myMPD CA", -1, -1, 0);
    
    /* Now set the issuer name. */
    X509_set_issuer_name(cert, name);
    
    /* Set ca extension. */
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
    X509_EXTENSION *ex = X509V3_EXT_conf_nid(NULL, &ctx, NID_basic_constraints, "CA:true");
    X509_add_ext(cert, ex, -1);
    X509_EXTENSION_free(ex);
    
    /* Self sign the certificate with our key. */
    if (!X509_sign(cert, pkey, EVP_sha256())) {
        LOG_ERROR("Error signing certificate");
        X509_free(cert);
        return NULL;
    }
    
    return cert;
}

static bool write_to_disk(EVP_PKEY *pkey, X509 *cert, const char *dir, const char *name) {
    size_t pem_file_len = strlen(dir) + strlen(name) + 6;
    char pem_file[pem_file_len];
    size_t tmp_file_len = strlen(dir) + strlen(name) + 13;
    char tmp_file[tmp_file_len];
    int fd;
    
    if (!validate_string(name)) {
        return false;
    }
    
    /* Write the key to disk. */    
    snprintf(pem_file, pem_file_len, "%s/%s.key", dir, name);
    snprintf(tmp_file, tmp_file_len, "%s/%s.key.XXXXXX", dir, name);
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *pkey_file = fdopen(fd, "w");
    bool rc = PEM_write_PrivateKey(pkey_file, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(pkey_file);
    if (!rc) {
        LOG_ERROR("Unable to write private key to disk");
        return false;
    }
    if (rename(tmp_file, pem_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pem_file);
        return false;
    }
    
    /* Write the certificate to disk. */
    snprintf(pem_file, pem_file_len, "%s/%s.pem", dir, name);
    snprintf(tmp_file, tmp_file_len, "%s/%s.pem.XXXXXX", dir, name);
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        return false;
    }
    FILE *cert_file = fdopen(fd, "w");
    rc = PEM_write_X509(cert_file, cert);
    fclose(cert_file);
    if (!rc) {
        LOG_ERROR("Unable to write certificate to disk");
        return false;
    }
    if (rename(tmp_file, pem_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, pem_file);
        return false;
    }
    
    return true;
}
