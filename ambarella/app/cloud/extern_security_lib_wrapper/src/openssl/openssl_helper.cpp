/*******************************************************************************
 * openssl_helper.cpp
 *
 * History:
 *  2015/05/15 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/engine.h>

#include "security_lib_wrapper.h"

#include "openssl_helper.h"

#ifdef DCONFIG_COMPILE_NOT_FINISHED_MODULE

int args_verify(s_secure_server_openssl *thiz, X509_VERIFY_PARAM **pm)
{
    ASN1_OBJECT *otmp = NULL;
    unsigned long flags = 0;
    int purpose = 0;
    int depth = -1;
    time_t at_time = 0;
    char *hostname = NULL;
    char *email = NULL;
    char *ipasc = NULL;

    if (thiz->b_set_policy) {
        otmp = OBJ_txt2obj(thiz->policy, 0);
        if (!otmp) {
            DOPENSSL_LOG_ERROR("invalid policy %s\n", thiz->policy);
            return (-1);
        }
    }

    if (thiz->b_set_purpose) {
        X509_PURPOSE *xptmp;
        int i = X509_PURPOSE_get_by_sname(thiz->purpose);
        if (i < 0) {
            DOPENSSL_LOG_ERROR("unrecognized purpose\n");
            return (-2);
        }
        xptmp = X509_PURPOSE_get0(i);
        purpose = X509_PURPOSE_get_id(xptmp);
    }

    if (thiz->verify_depth) {
        depth = thiz->verify_depth;
    }

    if (thiz->b_set_attime) {
        at_time = thiz->attime;
    }

    if (thiz->b_set_verify_hostname) {
        hostname = thiz->verify_hostname;
    }

    if (thiz->b_set_verify_email) {
        email = thiz->verify_email;
    }

    if (thiz->b_set_verify_ip) {
        ipasc = thiz->verify_ip;
    }

    if (thiz->b_ignore_critical) {
        flags |= X509_V_FLAG_IGNORE_CRITICAL;
    }

    if (thiz->b_issuer_checks) {
        flags |= X509_V_FLAG_CB_ISSUER_CHECK;
    }

    if (thiz->b_crl_check) {
        flags |= X509_V_FLAG_CRL_CHECK;
    }

    if (thiz->b_crl_check_all) {
        flags |= X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL;
    }

    if (thiz->b_policy_check) {
        flags |= X509_V_FLAG_POLICY_CHECK;
    }

    if (thiz->b_explicit_policy) {
        flags |= X509_V_FLAG_EXPLICIT_POLICY;
    }

    if (thiz->b_inhibit_any) {
        flags |= X509_V_FLAG_INHIBIT_ANY;
    }

    if (thiz->b_inhibit_map) {
        flags |= X509_V_FLAG_INHIBIT_MAP;
    }

    if (thiz->b_x509_strict) {
        flags |= X509_V_FLAG_X509_STRICT;
    }

    if (thiz->b_extended_crl) {
        flags |= X509_V_FLAG_EXTENDED_CRL_SUPPORT;
    }

    if (thiz->b_use_deltas) {
        flags |= X509_V_FLAG_USE_DELTAS;
    }

    if (thiz->b_policy_notify) {
        flags |= X509_V_FLAG_NOTIFY_POLICY;
    }

    if (thiz->b_check_ss_sig) {
        flags |= X509_V_FLAG_CHECK_SS_SIGNATURE;
    }

    if (thiz->b_trusted_first) {
        flags |= X509_V_FLAG_TRUSTED_FIRST;
    }

    if (thiz->b_suiteB_128_only) {
        flags |= X509_V_FLAG_SUITEB_128_LOS_ONLY;
    }

    if (thiz->b_suiteB_128) {
        flags |= X509_V_FLAG_SUITEB_128_LOS;
    }

    if (thiz->b_suiteB_192) {
        flags |= X509_V_FLAG_SUITEB_192_LOS;
    }

    if (thiz->b_partial_chain) {
        flags |= X509_V_FLAG_PARTIAL_CHAIN;
    }

    if (otmp) {
        X509_VERIFY_PARAM_add0_policy(*pm, otmp);
    }

    if (flags) {
        X509_VERIFY_PARAM_set_flags(*pm, flags);
    }

    if (purpose) {
        X509_VERIFY_PARAM_set_purpose(*pm, purpose);
    }

    if (depth >= 0) {
        X509_VERIFY_PARAM_set_depth(*pm, depth);
    }

    if (at_time) {
        X509_VERIFY_PARAM_set_time(*pm, at_time);
    }

    if (hostname && !X509_VERIFY_PARAM_set1_host(*pm, hostname, 0)) {
        DOPENSSL_LOG_ERROR("X509_VERIFY_PARAM_set1_host fail\n");
        return (-3);
    }

    if (email && !X509_VERIFY_PARAM_set1_email(*pm, email, 0)) {
        DOPENSSL_LOG_ERROR("X509_VERIFY_PARAM_set1_email fail\n");
        return (-4);
    }

    if (ipasc && !X509_VERIFY_PARAM_set1_ip_asc(*pm, ipasc)) {
        DOPENSSL_LOG_ERROR("X509_VERIFY_PARAM_set1_ip_asc fail\n");
        return (-4);
    }

    return 0;
}

int add_excert(s_secure_server_openssl *thiz, s_ssl_excert_st *p_new_exc)
{
    if (!thiz->exc) {
        if (!ssl_excert_prepend(&thiz->exc)) {
            DOPENSSL_LOG_ERROR("Error initialising xcert\n");
            return (-1);
        }
    }

    if (p_new_exc->certfile) {
        thiz->exc->certfile = p_new_exc->certfile;
        if (!ssl_excert_prepend(&exc)) {
            DOPENSSL_LOG_ERROR("Error adding xcert\n");
            return (-2);
        }
    }

    if (thiz->exc->keyfile) {
        thiz->exc->keyfile = p_new_exc->keyfile;
    }

    if (thiz->exc->chainfile) {
        thiz->exc->chainfile = p_new_exc->chainfile;
    }

    thiz->exc->build_chain = p_new_exc->build_chain;
    thiz->exc->certform = p_new_exc->certform;
    thiz->exc->keyform = p_new_exc->keyform;

    return 0;
}


int load_cert_crl_http(const char *url, BIO *err,
                       X509 **pcert, X509_CRL **pcrl)
{
    char *host = NULL, *port = NULL, *path = NULL;
    BIO *bio = NULL;
    OCSP_REQ_CTX *rctx = NULL;
    int use_ssl, rv = 0;
    if (!OCSP_parse_url(url, &host, &port, &path, &use_ssl))
    { goto err; }
    if (use_ssl) {
        if (err)
        { BIO_puts(err, "https not supported\n"); }
        goto err;
    }
    bio = BIO_new_connect(host);
    if (!bio || !BIO_set_conn_port(bio, port))
    { goto err; }
    rctx = OCSP_REQ_CTX_new(bio, 1024);
    if (!rctx)
    { goto err; }
    if (!OCSP_REQ_CTX_http(rctx, "GET", path))
    { goto err; }
    if (!OCSP_REQ_CTX_add1_header(rctx, "Host", host))
    { goto err; }
    if (pcert) {
        do {
            rv = X509_http_nbio(rctx, pcert);
        } while (rv == -1);
    } else {
        do {
            rv = X509_CRL_http_nbio(rctx, pcrl);
        } while (rv == -1);
    }

err:
    if (host)
    { OPENSSL_free(host); }
    if (path)
    { OPENSSL_free(path); }
    if (port)
    { OPENSSL_free(port); }
    if (bio)
    { BIO_free_all(bio); }
    if (rctx)
    { OCSP_REQ_CTX_free(rctx); }
    if (rv != 1) {
        if (bio && err)
            BIO_printf(bio_err, "Error loading %s from %s\n",
                       pcert ? "certificate" : "CRL", url);
        ERR_print_errors(bio_err);
    }
    return rv;
}

X509 *load_cert(BIO *err, const char *file, int format,
                const char *pass, ENGINE *e, const char *cert_descrip)
{
    X509 *x = NULL;
    BIO *cert;

    if (format == FORMAT_HTTP) {
        load_cert_crl_http(file, err, &x, NULL);
        return x;
    }

    if ((cert = BIO_new(BIO_s_file())) == NULL) {
        ERR_print_errors(err);
        goto end;
    }

    if (file == NULL) {
#ifdef _IONBF
# ifndef OPENSSL_NO_SETVBUF_IONBF
        setvbuf(stdin, NULL, _IONBF, 0);
# endif                         /* ndef OPENSSL_NO_SETVBUF_IONBF */
#endif
        BIO_set_fp(cert, stdin, BIO_NOCLOSE);
    } else {
        if (BIO_read_filename(cert, file) <= 0) {
            BIO_printf(err, "Error opening %s %s\n", cert_descrip, file);
            ERR_print_errors(err);
            goto end;
        }
    }

    if (format == FORMAT_ASN1)
    { x = d2i_X509_bio(cert, NULL); }
    else if (format == FORMAT_NETSCAPE) {
        NETSCAPE_X509 *nx;
        nx = ASN1_item_d2i_bio(ASN1_ITEM_rptr(NETSCAPE_X509), cert, NULL);
        if (nx == NULL)
        { goto end; }

        if ((strncmp(NETSCAPE_CERT_HDR, (char *)nx->header->data,
                     nx->header->length) != 0)) {
            NETSCAPE_X509_free(nx);
            BIO_printf(err, "Error reading header on certificate\n");
            goto end;
        }
        x = nx->cert;
        nx->cert = NULL;
        NETSCAPE_X509_free(nx);
    } else if (format == FORMAT_PEM)
        x = PEM_read_bio_X509_AUX(cert, NULL,
                                  (pem_password_cb *)password_callback, NULL);
    else if (format == FORMAT_PKCS12) {
        if (!load_pkcs12(err, cert, cert_descrip, NULL, NULL, NULL, &x, NULL))
        { goto end; }
    } else {
        BIO_printf(err, "bad input format specified for %s\n", cert_descrip);
        goto end;
    }
end:
    if (x == NULL) {
        BIO_printf(err, "unable to load certificate\n");
        ERR_print_errors(err);
    }
    if (cert != NULL)
    { BIO_free(cert); }
    return (x);
}

X509_CRL *load_crl(const char *infile, int format)
{
    X509_CRL *x = NULL;
    BIO *in = NULL;

    if (format == FORMAT_HTTP) {
        load_cert_crl_http(infile, bio_err, NULL, &x);
        return x;
    }

    in = BIO_new(BIO_s_file());
    if (in == NULL) {
        ERR_print_errors(bio_err);
        goto end;
    }

    if (infile == NULL)
    { BIO_set_fp(in, stdin, BIO_NOCLOSE); }
    else {
        if (BIO_read_filename(in, infile) <= 0) {
            perror(infile);
            goto end;
        }
    }
    if (format == FORMAT_ASN1)
    { x = d2i_X509_CRL_bio(in, NULL); }
    else if (format == FORMAT_PEM)
    { x = PEM_read_bio_X509_CRL(in, NULL, NULL, NULL); }
    else {
        BIO_printf(bio_err, "bad input format specified for input crl\n");
        goto end;
    }
    if (x == NULL) {
        BIO_printf(bio_err, "unable to load CRL\n");
        ERR_print_errors(bio_err);
        goto end;
    }

end:
    BIO_free(in);
    return (x);
}

EVP_PKEY *load_key(BIO *err, const char *file, int format, int maybe_stdin,
                   const char *pass, ENGINE *e, const char *key_descrip)
{
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;
    PW_CB_DATA cb_data;

    cb_data.password = pass;
    cb_data.prompt_info = file;

    if (file == NULL && (!maybe_stdin || format == FORMAT_ENGINE)) {
        BIO_printf(err, "no keyfile specified\n");
        goto end;
    }
#ifndef OPENSSL_NO_ENGINE
    if (format == FORMAT_ENGINE) {
        if (!e)
        { BIO_printf(err, "no engine specified\n"); }
        else {
            pkey = ENGINE_load_private_key(e, file, ui_method, &cb_data);
            if (!pkey) {
                BIO_printf(err, "cannot load %s from engine\n", key_descrip);
                ERR_print_errors(err);
            }
        }
        goto end;
    }
#endif
    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        ERR_print_errors(err);
        goto end;
    }
    if (file == NULL && maybe_stdin) {
#ifdef _IONBF
# ifndef OPENSSL_NO_SETVBUF_IONBF
        setvbuf(stdin, NULL, _IONBF, 0);
# endif                         /* ndef OPENSSL_NO_SETVBUF_IONBF */
#endif
        BIO_set_fp(key, stdin, BIO_NOCLOSE);
    } else if (BIO_read_filename(key, file) <= 0) {
        BIO_printf(err, "Error opening %s %s\n", key_descrip, file);
        ERR_print_errors(err);
        goto end;
    }
    if (format == FORMAT_ASN1) {
        pkey = d2i_PrivateKey_bio(key, NULL);
    } else if (format == FORMAT_PEM) {
        pkey = PEM_read_bio_PrivateKey(key, NULL,
                                       (pem_password_cb *)password_callback,
                                       &cb_data);
    }
#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
    else if (format == FORMAT_NETSCAPE || format == FORMAT_IISSGC)
    { pkey = load_netscape_key(err, key, file, key_descrip, format); }
#endif
    else if (format == FORMAT_PKCS12) {
        if (!load_pkcs12(err, key, key_descrip,
                         (pem_password_cb *)password_callback, &cb_data,
                         &pkey, NULL, NULL))
        { goto end; }
    }
#if !defined(OPENSSL_NO_RSA) && !defined(OPENSSL_NO_DSA) && !defined (OPENSSL_NO_RC4)
    else if (format == FORMAT_MSBLOB)
    { pkey = b2i_PrivateKey_bio(key); }
    else if (format == FORMAT_PVK)
        pkey = b2i_PVK_bio(key, (pem_password_cb *)password_callback,
                           &cb_data);
#endif
    else {
        BIO_printf(err, "bad input format specified for key file\n");
        goto end;
    }
end:
    if (key != NULL)
    { BIO_free(key); }
    if (pkey == NULL) {
        BIO_printf(err, "unable to load %s\n", key_descrip);
        ERR_print_errors(err);
    }
    return (pkey);
}

EVP_PKEY *load_pubkey(BIO *err, const char *file, int format, int maybe_stdin,
                      const char *pass, ENGINE *e, const char *key_descrip)
{
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;
    PW_CB_DATA cb_data;

    cb_data.password = pass;
    cb_data.prompt_info = file;

    if (file == NULL && (!maybe_stdin || format == FORMAT_ENGINE)) {
        BIO_printf(err, "no keyfile specified\n");
        goto end;
    }
#ifndef OPENSSL_NO_ENGINE
    if (format == FORMAT_ENGINE) {
        if (!e)
        { BIO_printf(bio_err, "no engine specified\n"); }
        else
        { pkey = ENGINE_load_public_key(e, file, ui_method, &cb_data); }
        goto end;
    }
#endif
    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        ERR_print_errors(err);
        goto end;
    }
    if (file == NULL && maybe_stdin) {
#ifdef _IONBF
# ifndef OPENSSL_NO_SETVBUF_IONBF
        setvbuf(stdin, NULL, _IONBF, 0);
# endif                         /* ndef OPENSSL_NO_SETVBUF_IONBF */
#endif
        BIO_set_fp(key, stdin, BIO_NOCLOSE);
    } else if (BIO_read_filename(key, file) <= 0) {
        BIO_printf(err, "Error opening %s %s\n", key_descrip, file);
        ERR_print_errors(err);
        goto end;
    }
    if (format == FORMAT_ASN1) {
        pkey = d2i_PUBKEY_bio(key, NULL);
    }
#ifndef OPENSSL_NO_RSA
    else if (format == FORMAT_ASN1RSA) {
        RSA *rsa;
        rsa = d2i_RSAPublicKey_bio(key, NULL);
        if (rsa) {
            pkey = EVP_PKEY_new();
            if (pkey)
            { EVP_PKEY_set1_RSA(pkey, rsa); }
            RSA_free(rsa);
        } else
        { pkey = NULL; }
    } else if (format == FORMAT_PEMRSA) {
        RSA *rsa;
        rsa = PEM_read_bio_RSAPublicKey(key, NULL,
                                        (pem_password_cb *)password_callback,
                                        &cb_data);
        if (rsa) {
            pkey = EVP_PKEY_new();
            if (pkey)
            { EVP_PKEY_set1_RSA(pkey, rsa); }
            RSA_free(rsa);
        } else
        { pkey = NULL; }
    }
#endif
    else if (format == FORMAT_PEM) {
        pkey = PEM_read_bio_PUBKEY(key, NULL,
                                   (pem_password_cb *)password_callback,
                                   &cb_data);
    }
#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
    else if (format == FORMAT_NETSCAPE || format == FORMAT_IISSGC)
    { pkey = load_netscape_key(err, key, file, key_descrip, format); }
#endif
#if !defined(OPENSSL_NO_RSA) && !defined(OPENSSL_NO_DSA)
    else if (format == FORMAT_MSBLOB)
    { pkey = b2i_PublicKey_bio(key); }
#endif
    else {
        BIO_printf(err, "bad input format specified for key file\n");
        goto end;
    }
end:
    if (key != NULL)
    { BIO_free(key); }
    if (pkey == NULL)
    { BIO_printf(err, "unable to load %s\n", key_descrip); }
    return (pkey);
}

int load_excert(SSL_EXCERT **pexc, BIO *err)
{
    SSL_EXCERT *exc = *pexc;
    if (!exc)
    { return 1; }
    /* If nothing in list, free and set to NULL */
    if (!exc->certfile && !exc->next) {
        ssl_excert_free(exc);
        *pexc = NULL;
        return 1;
    }
    for (; exc; exc = exc->next) {
        if (!exc->certfile) {
            BIO_printf(err, "Missing filename\n");
            return 0;
        }
        exc->cert = load_cert(err, exc->certfile, exc->certform,
                              NULL, NULL, "Server Certificate");
        if (!exc->cert)
        { return 0; }
        if (exc->keyfile) {
            exc->key = load_key(err, exc->keyfile, exc->keyform,
                                0, NULL, NULL, "Server Key");
        } else {
            exc->key = load_key(err, exc->certfile, exc->certform,
                                0, NULL, NULL, "Server Key");
        }
        if (!exc->key)
        { return 0; }
        if (exc->chainfile) {
            exc->chain = load_certs(err,
                                    exc->chainfile, FORMAT_PEM,
                                    NULL, NULL, "Server Chain");
            if (!exc->chain)
            { return 0; }
        }
    }
    return 1;
}

unsigned char *next_protos_parse(unsigned short *outlen, const char *in)
{
    size_t len;
    unsigned char *out;
    size_t i, start = 0;

    len = strlen(in);
    if (len >= 65535)
    { return NULL; }

    out = OPENSSL_malloc(strlen(in) + 1);
    if (!out)
    { return NULL; }

    for (i = 0; i <= len; ++i) {
        if (i == len || in[i] == ',') {
            if (i - start > 255) {
                OPENSSL_free(out);
                return NULL;
            }
            out[start] = i - start;
            start = i + 1;
        } else
        { out[i + 1] = in[i]; }
    }

    *outlen = len + 1;
    return out;
}

static int seeded = 0;
static int egdsocket = 0;

int app_RAND_load_file(const char *file, BIO *bio_e, int dont_warn)
{
    int consider_randfile = (file == NULL);
    char buffer[200];

#ifdef OPENSSL_SYS_WINDOWS
    BIO_printf(bio_e, "Loading 'screen' into random state -");
    BIO_flush(bio_e);
    RAND_screen();
    BIO_printf(bio_e, " done\n");
#endif

    if (file == NULL)
    { file = RAND_file_name(buffer, sizeof buffer); }
    else if (RAND_egd(file) > 0) {
        /*
         * we try if the given filename is an EGD socket. if it is, we don't
         * write anything back to the file.
         */
        egdsocket = 1;
        return 1;
    }
    if (file == NULL || !RAND_load_file(file, -1)) {
        if (RAND_status() == 0) {
            if (!dont_warn) {
                BIO_printf(bio_e, "unable to load 'random state'\n");
                BIO_printf(bio_e,
                           "This means that the random number generator has not been seeded\n");
                BIO_printf(bio_e, "with much random data.\n");
                if (consider_randfile) {
                    /* explanation does not apply when a
                                              * file is explicitly named */
                    BIO_printf(bio_e,
                               "Consider setting the RANDFILE environment variable to point at a file that\n");
                    BIO_printf(bio_e,
                               "'random' data can be kept in (the file will be overwritten).\n");
                }
            }
            return 0;
        }
    }
    seeded = 1;
    return 1;
}

long app_RAND_load_files(char *name)
{
    char *p, *n;
    int last;
    long tot = 0;
    int egd;

    for (;;) {
        last = 0;
        for (p = name; ((*p != '\0') && (*p != LIST_SEPARATOR_CHAR)); p++) ;
        if (*p == '\0')
        { last = 1; }
        *p = '\0';
        n = name;
        name = p + 1;
        if (*n == '\0')
        { break; }

        egd = RAND_egd(n);
        if (egd > 0)
        { tot += egd; }
        else
        { tot += RAND_load_file(n, -1); }
        if (last)
        { break; }
    }
    if (tot > 512)
    { app_RAND_allow_write_file(); }
    return (tot);
}

#define MAX_SESSION_ID_ATTEMPTS 10
static int generate_session_id(const SSL *ssl, unsigned char *id, unsigned int *id_len)
{
    unsigned int count = 0;
    do {
        RAND_pseudo_bytes(id, *id_len);
        /*
         * Prefix the session_id with the required prefix. NB: If our prefix
         * is too long, clip it - but there will be worse effects anyway, eg.
         * the server could only possibly create 1 session ID (ie. the
         * prefix!) so all future session negotiations will fail due to
         * conflicts.
         */
        memcpy(id, session_id_prefix,
               (strlen(session_id_prefix) < *id_len) ?
               strlen(session_id_prefix) : *id_len);
    } while (SSL_has_matching_session_id(ssl, id, *id_len) &&
             (++count < MAX_SESSION_ID_ATTEMPTS));
    if (count >= MAX_SESSION_ID_ATTEMPTS)
    { return 0; }
    return 1;
}

void MS_CALLBACK apps_ssl_info_callback(const SSL *s, int where, int ret)
{
    const char *str;
    int w;

    w = where & ~SSL_ST_MASK;

    if (w & SSL_ST_CONNECT)
    { str = "SSL_connect"; }
    else if (w & SSL_ST_ACCEPT)
    { str = "SSL_accept"; }
    else
    { str = "undefined"; }

    if (where & SSL_CB_LOOP) {
        BIO_printf(bio_err, "%s:%s\n", str, SSL_state_string_long(s));
    } else if (where & SSL_CB_ALERT) {
        str = (where & SSL_CB_READ) ? "read" : "write";
        BIO_printf(bio_err, "SSL3 alert %s:%s:%s\n",
                   str,
                   SSL_alert_type_string_long(ret),
                   SSL_alert_desc_string_long(ret));
    } else if (where & SSL_CB_EXIT) {
        if (ret == 0)
            BIO_printf(bio_err, "%s:failed in %s\n",
                       str, SSL_state_string_long(s));
        else if (ret < 0) {
            BIO_printf(bio_err, "%s:error in %s\n",
                       str, SSL_state_string_long(s));
        }
    }
}

typedef struct simple_ssl_session_st {
    unsigned char *id;
    unsigned int idlen;
    unsigned char *der;
    int derlen;
    struct simple_ssl_session_st *next;
} simple_ssl_session;

static simple_ssl_session *first = NULL;

static int add_session(SSL *ssl, SSL_SESSION *session)
{
    simple_ssl_session *sess;
    unsigned char *p;

    sess = OPENSSL_malloc(sizeof(simple_ssl_session));
    if (!sess) {
        BIO_printf(bio_err, "Out of memory adding session to external cache\n");
        return 0;
    }

    SSL_SESSION_get_id(session, &sess->idlen);
    sess->derlen = i2d_SSL_SESSION(session, NULL);

    sess->id = BUF_memdup(SSL_SESSION_get_id(session, NULL), sess->idlen);

    sess->der = OPENSSL_malloc(sess->derlen);
    if (!sess->id || !sess->der) {
        BIO_printf(bio_err, "Out of memory adding session to external cache\n");

        if (sess->id)
        { OPENSSL_free(sess->id); }
        if (sess->der)
        { OPENSSL_free(sess->der); }
        OPENSSL_free(sess);
        return 0;
    }
    p = sess->der;
    i2d_SSL_SESSION(session, &p);

    sess->next = first;
    first = sess;
    BIO_printf(bio_err, "New session added to external cache\n");
    return 0;
}

static SSL_SESSION *get_session(SSL *ssl, unsigned char *id, int idlen,
                                int *do_copy)
{
    simple_ssl_session *sess;
    *do_copy = 0;
    for (sess = first; sess; sess = sess->next) {
        if (idlen == (int)sess->idlen && !memcmp(sess->id, id, idlen)) {
            const unsigned char *p = sess->der;
            BIO_printf(bio_err, "Lookup session: cache hit\n");
            return d2i_SSL_SESSION(NULL, &p, sess->derlen);
        }
    }
    BIO_printf(bio_err, "Lookup session: cache miss\n");
    return NULL;
}

static void del_session(SSL_CTX *sctx, SSL_SESSION *session)
{
    simple_ssl_session *sess, *prev = NULL;
    const unsigned char *id;
    unsigned int idlen;
    id = SSL_SESSION_get_id(session, &idlen);
    for (sess = first; sess; sess = sess->next) {
        if (idlen == sess->idlen && !memcmp(sess->id, id, idlen)) {
            if (prev)
            { prev->next = sess->next; }
            else
            { first = sess->next; }
            OPENSSL_free(sess->id);
            OPENSSL_free(sess->der);
            OPENSSL_free(sess);
            return;
        }
        prev = sess;
    }
}

static void init_session_cache_ctx(SSL_CTX *sctx)
{
    SSL_CTX_set_session_cache_mode(sctx,
                                   SSL_SESS_CACHE_NO_INTERNAL |
                                   SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_new_cb(sctx, add_session);
    SSL_CTX_sess_set_get_cb(sctx, get_session);
    SSL_CTX_sess_set_remove_cb(sctx, del_session);
}

static void free_sessions(void)
{
    simple_ssl_session *sess, *tsess;
    for (sess = first; sess;) {
        OPENSSL_free(sess->id);
        OPENSSL_free(sess->der);
        tsess = sess;
        sess = sess->next;
        OPENSSL_free(tsess);
    }
    first = NULL;
}

int ssl_ctx_add_crls(SSL_CTX *ctx, STACK_OF(X509_CRL) *crls, int crl_download)
{
    X509_STORE *st;
    st = SSL_CTX_get_cert_store(ctx);
    add_crls_store(st, crls);
    if (crl_download)
    { store_setup_crl_download(st); }
    return 1;
}

int args_ssl_call(SSL_CTX *ctx, BIO *err, SSL_CONF_CTX *cctx,
                  STACK_OF(OPENSSL_STRING) *str, int no_ecdhe, int no_jpake)
{
    int i;
    SSL_CONF_CTX_set_ssl_ctx(cctx, ctx);
    for (i = 0; i < sk_OPENSSL_STRING_num(str); i += 2) {
        const char *param = sk_OPENSSL_STRING_value(str, i);
        const char *value = sk_OPENSSL_STRING_value(str, i + 1);
        /*
         * If no_ecdhe or named curve already specified don't need a default.
         */
        if (!no_ecdhe && !strcmp(param, "-named_curve"))
        { no_ecdhe = 1; }
#ifndef OPENSSL_NO_JPAKE
        if (!no_jpake && !strcmp(param, "-cipher")) {
            BIO_puts(err, "JPAKE sets cipher to PSK\n");
            return 0;
        }
#endif
        if (SSL_CONF_cmd(cctx, param, value) <= 0) {
            BIO_printf(err, "Error with command: \"%s %s\"\n",
                       param, value ? value : "");
            ERR_print_errors(err);
            return 0;
        }
    }
    /*
     * This is a special case to keep existing s_server functionality: if we
     * don't have any curve specified *and* we haven't disabled ECDHE then
     * use P-256.
     */
    if (!no_ecdhe) {
        if (SSL_CONF_cmd(cctx, "-named_curve", "P-256") <= 0) {
            BIO_puts(err, "Error setting EC curve\n");
            ERR_print_errors(err);
            return 0;
        }
    }
#ifndef OPENSSL_NO_JPAKE
    if (!no_jpake) {
        if (SSL_CONF_cmd(cctx, "-cipher", "PSK") <= 0) {
            BIO_puts(err, "Error setting cipher to PSK\n");
            ERR_print_errors(err);
            return 0;
        }
    }
#endif
    if (!SSL_CONF_CTX_finish(cctx)) {
        BIO_puts(err, "Error finishing context\n");
        ERR_print_errors(err);
        return 0;
    }
    return 1;
}

static int add_crls_store(X509_STORE *st, STACK_OF(X509_CRL) *crls)
{
    X509_CRL *crl;
    int i;
    for (i = 0; i < sk_X509_CRL_num(crls); i++) {
        crl = sk_X509_CRL_value(crls, i);
        X509_STORE_add_crl(st, crl);
    }
    return 1;
}

int ssl_load_stores(SSL_CTX *ctx,
                    const char *vfyCApath, const char *vfyCAfile,
                    const char *chCApath, const char *chCAfile,
                    STACK_OF(X509_CRL) *crls, int crl_download)
{
    X509_STORE *vfy = NULL, *ch = NULL;
    int rv = 0;
    if (vfyCApath || vfyCAfile) {
        vfy = X509_STORE_new();
        if (!X509_STORE_load_locations(vfy, vfyCAfile, vfyCApath))
        { goto err; }
        add_crls_store(vfy, crls);
        SSL_CTX_set1_verify_cert_store(ctx, vfy);
        if (crl_download)
        { store_setup_crl_download(vfy); }
    }
    if (chCApath || chCAfile) {
        ch = X509_STORE_new();
        if (!X509_STORE_load_locations(ch, chCAfile, chCApath))
        { goto err; }
        SSL_CTX_set1_chain_cert_store(ctx, ch);
    }
    rv = 1;
err:
    if (vfy)
    { X509_STORE_free(vfy); }
    if (ch)
    { X509_STORE_free(ch); }
    return rv;
}

static DH *load_dh_param(const char *dhfile)
{
    DH *ret = NULL;
    BIO *bio;

    if ((bio = BIO_new_file(dhfile, "r")) == NULL)
    { goto err; }
    ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
err:
    if (bio != NULL)
    { BIO_free(bio); }
    return (ret);
}

static unsigned char dh512_p[] = {
    0xDA, 0x58, 0x3C, 0x16, 0xD9, 0x85, 0x22, 0x89, 0xD0, 0xE4, 0xAF, 0x75,
    0x6F, 0x4C, 0xCA, 0x92, 0xDD, 0x4B, 0xE5, 0x33, 0xB8, 0x04, 0xFB, 0x0F,
    0xED, 0x94, 0xEF, 0x9C, 0x8A, 0x44, 0x03, 0xED, 0x57, 0x46, 0x50, 0xD3,
    0x69, 0x99, 0xDB, 0x29, 0xD7, 0x76, 0x27, 0x6B, 0xA2, 0xD3, 0xD4, 0x12,
    0xE2, 0x18, 0xF4, 0xDD, 0x1E, 0x08, 0x4C, 0xF6, 0xD8, 0x00, 0x3E, 0x7C,
    0x47, 0x74, 0xE8, 0x33,
};

static unsigned char dh512_g[] = {
    0x02,
};

static DH *get_dh512(void)
{
    DH *dh = NULL;

    if ((dh = DH_new()) == NULL)
    { return (NULL); }
    dh->p = BN_bin2bn(dh512_p, sizeof(dh512_p), NULL);
    dh->g = BN_bin2bn(dh512_g, sizeof(dh512_g), NULL);
    if ((dh->p == NULL) || (dh->g == NULL))
    { return (NULL); }
    return (dh);
}

int set_cert_stuff(SSL_CTX *ctx, char *cert_file, char *key_file)
{
    if (cert_file != NULL) {
        /*-
        SSL *ssl;
        X509 *x509;
        */

        if (SSL_CTX_use_certificate_file(ctx, cert_file,
                                         SSL_FILETYPE_PEM) <= 0) {
            BIO_printf(bio_err, "unable to get certificate from '%s'\n",
                       cert_file);
            ERR_print_errors(bio_err);
            return (0);
        }
        if (key_file == NULL)
        { key_file = cert_file; }
        if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
            BIO_printf(bio_err, "unable to get private key from '%s'\n",
                       key_file);
            ERR_print_errors(bio_err);
            return (0);
        }

        /*-
        In theory this is no longer needed
        ssl=SSL_new(ctx);
        x509=SSL_get_certificate(ssl);

        if (x509 != NULL) {
                EVP_PKEY *pktmp;
                pktmp = X509_get_pubkey(x509);
                EVP_PKEY_copy_parameters(pktmp,
                                        SSL_get_privatekey(ssl));
                EVP_PKEY_free(pktmp);
        }
        SSL_free(ssl);
        */

        /*
         * If we are using DSA, we can copy the parameters from the private
         * key
         */

        /*
         * Now we know that a key and cert have been set against the SSL
         * context
         */
        if (!SSL_CTX_check_private_key(ctx)) {
            BIO_printf(bio_err,
                       "Private key does not match the certificate public key\n");
            return (0);
        }
    }
    return (1);
}

int set_cert_key_stuff(SSL_CTX *ctx, X509 *cert, EVP_PKEY *key,
                       STACK_OF(X509) *chain, int build_chain)
{
    int chflags = chain ? SSL_BUILD_CHAIN_FLAG_CHECK : 0;
    if (cert == NULL)
    { return 1; }
    if (SSL_CTX_use_certificate(ctx, cert) <= 0) {
        BIO_printf(bio_err, "error setting certificate\n");
        ERR_print_errors(bio_err);
        return 0;
    }

    if (SSL_CTX_use_PrivateKey(ctx, key) <= 0) {
        BIO_printf(bio_err, "error setting private key\n");
        ERR_print_errors(bio_err);
        return 0;
    }

    /*
     * Now we know that a key and cert have been set against the SSL context
     */
    if (!SSL_CTX_check_private_key(ctx)) {
        BIO_printf(bio_err,
                   "Private key does not match the certificate public key\n");
        return 0;
    }
    if (chain && !SSL_CTX_set1_chain(ctx, chain)) {
        BIO_printf(bio_err, "error setting certificate chain\n");
        ERR_print_errors(bio_err);
        return 0;
    }
    if (build_chain && !SSL_CTX_build_cert_chain(ctx, chflags)) {
        BIO_printf(bio_err, "error building certificate chain\n");
        ERR_print_errors(bio_err);
        return 0;
    }
    return 1;
}

static unsigned int psk_server_cb(SSL *ssl, const char *identity,
                                  unsigned char *psk,
                                  unsigned int max_psk_len)
{
    unsigned int psk_len = 0;
    int ret;
    BIGNUM *bn = NULL;

    if (s_debug)
    { BIO_printf(bio_s_out, "psk_server_cb\n"); }
    if (!identity) {
        BIO_printf(bio_err, "Error: client did not send PSK identity\n");
        goto out_err;
    }
    if (s_debug)
        BIO_printf(bio_s_out, "identity_len=%d identity=%s\n",
                   (int)strlen(identity), identity);

    /* here we could lookup the given identity e.g. from a database */
    if (strcmp(identity, psk_identity) != 0) {
        BIO_printf(bio_s_out, "PSK error: client identity not found"
                   " (got '%s' expected '%s')\n", identity, psk_identity);
        goto out_err;
    }
    if (s_debug)
    { BIO_printf(bio_s_out, "PSK client identity found\n"); }

    /* convert the PSK key to binary */
    ret = BN_hex2bn(&bn, psk_key);
    if (!ret) {
        BIO_printf(bio_err, "Could not convert PSK key '%s' to BIGNUM\n",
                   psk_key);
        if (bn)
        { BN_free(bn); }
        return 0;
    }
    if (BN_num_bytes(bn) > (int)max_psk_len) {
        BIO_printf(bio_err,
                   "psk buffer of callback is too small (%d) for key (%d)\n",
                   max_psk_len, BN_num_bytes(bn));
        BN_free(bn);
        return 0;
    }

    ret = BN_bn2bin(bn, psk);
    BN_free(bn);

    if (ret < 0)
    { goto out_err; }
    psk_len = (unsigned int)ret;

    if (s_debug)
    { BIO_printf(bio_s_out, "fetched PSK len=%d\n", psk_len); }
    return psk_len;
out_err:
    if (s_debug)
    { BIO_printf(bio_err, "Error in PSK server callback\n"); }
    return 0;
}

int MS_CALLBACK verify_callback(int ok, X509_STORE_CTX *ctx)
{
    X509 *err_cert;
    int err, depth;

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    if (!verify_quiet || !ok) {
        BIO_printf(bio_err, "depth=%d ", depth);
        if (err_cert) {
            X509_NAME_print_ex(bio_err,
                               X509_get_subject_name(err_cert),
                               0, XN_FLAG_ONELINE);
            BIO_puts(bio_err, "\n");
        } else
        { BIO_puts(bio_err, "<no cert>\n"); }
    }
    if (!ok) {
        BIO_printf(bio_err, "verify error:num=%d:%s\n", err,
                   X509_verify_cert_error_string(err));
        if (verify_depth >= depth) {
            if (!verify_return_error)
            { ok = 1; }
            verify_error = X509_V_OK;
        } else {
            ok = 0;
            verify_error = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        }
    }
    switch (err) {
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            BIO_puts(bio_err, "issuer= ");
            X509_NAME_print_ex(bio_err, X509_get_issuer_name(err_cert),
                               0, XN_FLAG_ONELINE);
            BIO_puts(bio_err, "\n");
            break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
            BIO_printf(bio_err, "notBefore=");
            ASN1_TIME_print(bio_err, X509_get_notBefore(err_cert));
            BIO_printf(bio_err, "\n");
            break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
            BIO_printf(bio_err, "notAfter=");
            ASN1_TIME_print(bio_err, X509_get_notAfter(err_cert));
            BIO_printf(bio_err, "\n");
            break;
        case X509_V_ERR_NO_EXPLICIT_POLICY:
            if (!verify_quiet)
            { policies_print(bio_err, ctx); }
            break;
    }
    if (err == X509_V_OK && ok == 2 && !verify_quiet)
    { policies_print(bio_err, ctx); }
    if (ok && !verify_quiet)
    { BIO_printf(bio_err, "verify return:%d\n", ok); }
    return (ok);
}

int MS_CALLBACK generate_cookie_callback(SSL *ssl, unsigned char *cookie,
        unsigned int *cookie_len)
{
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
    unsigned int length, resultlength;
    union {
        struct sockaddr sa;
        struct sockaddr_in s4;
#if OPENSSL_USE_IPV6
        struct sockaddr_in6 s6;
#endif
    } peer;

    /* Initialize a random secret */
    if (!cookie_initialized) {
        if (!RAND_bytes(cookie_secret, COOKIE_SECRET_LENGTH)) {
            BIO_printf(bio_err, "error setting random cookie secret\n");
            return 0;
        }
        cookie_initialized = 1;
    }

    /* Read peer information */
    (void)BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    /* Create buffer with peer's address and port */
    length = 0;
    switch (peer.sa.sa_family) {
        case AF_INET:
            length += sizeof(struct in_addr);
            length += sizeof(peer.s4.sin_port);
            break;
#if OPENSSL_USE_IPV6
        case AF_INET6:
            length += sizeof(struct in6_addr);
            length += sizeof(peer.s6.sin6_port);
            break;
#endif
        default:
            OPENSSL_assert(0);
            break;
    }
    buffer = OPENSSL_malloc(length);

    if (buffer == NULL) {
        BIO_printf(bio_err, "out of memory\n");
        return 0;
    }

    switch (peer.sa.sa_family) {
        case AF_INET:
            memcpy(buffer, &peer.s4.sin_port, sizeof(peer.s4.sin_port));
            memcpy(buffer + sizeof(peer.s4.sin_port),
                   &peer.s4.sin_addr, sizeof(struct in_addr));
            break;
#if OPENSSL_USE_IPV6
        case AF_INET6:
            memcpy(buffer, &peer.s6.sin6_port, sizeof(peer.s6.sin6_port));
            memcpy(buffer + sizeof(peer.s6.sin6_port),
                   &peer.s6.sin6_addr, sizeof(struct in6_addr));
            break;
#endif
        default:
            OPENSSL_assert(0);
            break;
    }

    /* Calculate HMAC of buffer using the secret */
    HMAC(EVP_sha1(), cookie_secret, COOKIE_SECRET_LENGTH,
         buffer, length, result, &resultlength);
    OPENSSL_free(buffer);

    memcpy(cookie, result, resultlength);
    *cookie_len = resultlength;

    return 1;
}

int MS_CALLBACK verify_cookie_callback(SSL *ssl, unsigned char *cookie,
                                       unsigned int cookie_len)
{
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
    unsigned int length, resultlength;
    union {
        struct sockaddr sa;
        struct sockaddr_in s4;
#if OPENSSL_USE_IPV6
        struct sockaddr_in6 s6;
#endif
    } peer;

    /* If secret isn't initialized yet, the cookie can't be valid */
    if (!cookie_initialized)
    { return 0; }

    /* Read peer information */
    (void)BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    /* Create buffer with peer's address and port */
    length = 0;
    switch (peer.sa.sa_family) {
        case AF_INET:
            length += sizeof(struct in_addr);
            length += sizeof(peer.s4.sin_port);
            break;
#if OPENSSL_USE_IPV6
        case AF_INET6:
            length += sizeof(struct in6_addr);
            length += sizeof(peer.s6.sin6_port);
            break;
#endif
        default:
            OPENSSL_assert(0);
            break;
    }
    buffer = OPENSSL_malloc(length);

    if (buffer == NULL) {
        BIO_printf(bio_err, "out of memory\n");
        return 0;
    }

    switch (peer.sa.sa_family) {
        case AF_INET:
            memcpy(buffer, &peer.s4.sin_port, sizeof(peer.s4.sin_port));
            memcpy(buffer + sizeof(peer.s4.sin_port),
                   &peer.s4.sin_addr, sizeof(struct in_addr));
            break;
#if OPENSSL_USE_IPV6
        case AF_INET6:
            memcpy(buffer, &peer.s6.sin6_port, sizeof(peer.s6.sin6_port));
            memcpy(buffer + sizeof(peer.s6.sin6_port),
                   &peer.s6.sin6_addr, sizeof(struct in6_addr));
            break;
#endif
        default:
            OPENSSL_assert(0);
            break;
    }

    /* Calculate HMAC of buffer using the secret */
    HMAC(EVP_sha1(), cookie_secret, COOKIE_SECRET_LENGTH,
         buffer, length, result, &resultlength);
    OPENSSL_free(buffer);

    if (cookie_len == resultlength
            && memcmp(result, cookie, resultlength) == 0)
    { return 1; }

    return 0;
}

static int MS_CALLBACK ssl_servername_cb(SSL *s, int *ad, void *arg)
{
    tlsextctx *p = (tlsextctx *) arg;
    const char *servername = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);
    if (servername && p->biodebug)
        BIO_printf(p->biodebug, "Hostname in TLS extension: \"%s\"\n",
                   servername);

    if (!p->servername)
    { return SSL_TLSEXT_ERR_NOACK; }

    if (servername) {
        if (strcasecmp(servername, p->servername))
        { return p->extension_error; }
        if (ctx2) {
            BIO_printf(p->biodebug, "Switching server context.\n");
            SSL_set_SSL_CTX(s, ctx2);
        }
    }
    return SSL_TLSEXT_ERR_OK;
}

static int MS_CALLBACK ssl_srp_server_param_cb(SSL *s, int *ad, void *arg)
{
    srpsrvparm *p = (srpsrvparm *) arg;
    if (p->login == NULL && p->user == NULL) {
        p->login = SSL_get_srp_username(s);
        BIO_printf(bio_err, "SRP username = \"%s\"\n", p->login);
        return (-1);
    }

    if (p->user == NULL) {
        BIO_printf(bio_err, "User %s doesn't exist\n", p->login);
        return SSL3_AL_FATAL;
    }
    if (SSL_set_srp_server_param
            (s, p->user->N, p->user->g, p->user->s, p->user->v,
             p->user->info) < 0) {
        *ad = SSL_AD_INTERNAL_ERROR;
        return SSL3_AL_FATAL;
    }
    BIO_printf(bio_err,
               "SRP parameters set: username = \"%s\" info=\"%s\" \n",
               p->login, p->user->info);
    /* need to check whether there are memory leaks */
    p->user = NULL;
    p->login = NULL;
    return SSL_ERROR_NONE;
}

#endif

