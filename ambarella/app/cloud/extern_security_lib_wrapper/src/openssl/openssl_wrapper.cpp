/*******************************************************************************
 * openssl_wrapper.cpp
 *
 * History:
 *  2015/04/14 - [Zhi He] create file
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

#include "openssl_simple_log.h"

#include "security_lib_wrapper.h"

#define BUFSIZE (1024 * 8)

#if BUILD_OS_LINUX
#else
#define snprintf _snprintf
#endif

static int g_init_openssl = 0;
static BIO *g_log_output = NULL;

//extern void SSL_load_error_strings(void);

int initialize_su_library(int library_type, int log_to)
{
    if (!isOpenSSLSimpleLogFileOpened()) {
        gfOpenSSOpenSimpleLogFile((char *)"su_ssl_log.txt");
    }

    DOPENSSL_LOG_NOTICE("initialize_su_library() start ...\n");

    if (SU_LIB_OPENSSL != library_type) {
        DOPENSSL_LOG_ERROR("type(%d) not openssl?\n", library_type);
        return SU_ECODE_INVALID_INPUT;
    }

    if (g_init_openssl) {
        DOPENSSL_LOG_WARN("already initialized\n");
        return SU_ECODE_OK;
    }

    if (SU_LIB_LOG_TO_STDOUT == log_to) {
        if (!g_log_output) {
            g_log_output = BIO_new_fp(stderr, BIO_NOCLOSE);
        }
    }

    CRYPTO_malloc_init();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    //ENGINE_load_builtin_engines();

    //SSL_load_error_strings();
    //OpenSSL_add_ssl_algorithms();

    g_init_openssl = 1;

    DOPENSSL_LOG_NOTICE("initialize_su_library() done\n");

    return SU_ECODE_OK;
}

void deinitialize_su_library()
{
    DOPENSSL_LOG_NOTICE("deinitialize_su_library() start ...\n");

    if (!g_init_openssl) {
        DOPENSSL_LOG_WARN("not initialized yet\n");
        return;
    }

    OBJ_cleanup();
    EVP_cleanup();
    //ENGINE_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    RAND_cleanup();
    ERR_free_strings();

    if (g_log_output) {
        BIO_free(g_log_output);
        g_log_output = NULL;
    }

    g_init_openssl = 0;

    DOPENSSL_LOG_NOTICE("deinitialize_su_library() done\n");
    gfOpenSSCloseSimpleLogFile();

}

#ifdef DCONFIG_COMPILE_NOT_FINISHED_MODULE

static const char *session_id_prefix = NULL;
static int hack = 0;

typedef struct {
    int certform;
    const char *certfile;
    int keyform;
    const char *keyfile;
    const char *chainfile;
    X509 *cert;
    EVP_PKEY *key;
    STACK_OF(X509) *chain;
    int build_chain;
    struct s_ssl_excert_st *next, *prev;
} s_ssl_excert_st;

s_secure_server *su_server_create()
{
    DOPENSSL_LOG_NOTICE("su_server_create() start ...\n");
    s_secure_server *p_server = (s_secure_server *) malloc(sizeof(s_secure_server) + sizeof(s_secure_server_openssl));
    if (!p_server) {
        DOPENSSL_LOG_ERROR("no memory, request size %ld\n", (unsigned long)(sizeof(s_secure_server) + sizeof(s_secure_server_openssl)));
        return NULL;
    }

    memset(p_server, 0x0, sizeof(s_secure_server) + sizeof(s_secure_server_openssl));

    DOPENSSL_LOG_NOTICE("su_server_create() done\n");

    return p_server;
}

void su_server_destroy(s_secure_server *server);
int su_server_set_accept_callback(s_secure_server *server, void *accept_callback_context, TSUServerAcceptCallBack accept_callback);

int su_server_add_certificate(s_secure_server *server, char *certificate, char *key, int cert_form, int key_form)
{
    s_secure_server_openssl *thiz = (s_secure_server_openssl *) server->p_secure_server;
    s_ssl_excert_st ext;

    memset(&ext, 0x0, sizeof(ext));
    ext.certfile = certificate;
    ext.keyfile = key;
    ext.certform = cert_form;
    ext.keyform = key_form;

    return add_excert(thiz, &ext);
}

int su_server_start(s_secure_server *server, unsigned short port)
{
    int ret = 0;
    s_secure_server_openssl *thiz = (s_secure_server_openssl *) server->p_secure_server;
    const SSL_METHOD *meth = SSLv23_server_method();
    char *inrand = NULL;

    DOPENSSL_LOG_NOTICE("su_server_start() start ...\n");

    //load_config
    OPENSSL_load_builtin_modules();
    //CONF_modules_load(cnf, NULL, 0);

    thiz->cctx = SSL_CONF_CTX_new();
    if (!thiz->cctx) {
        DOPENSSL_LOG_ERROR("SSL_CONF_CTX_new() fail\n");
        ret = SU_ECODE_ERROR_NO_MEMORY;
        goto tag_su_server_start_exit;
    }
    SSL_CONF_CTX_set_flags(thiz->cctx, SSL_CONF_FLAG_SERVER);
    SSL_CONF_CTX_set_flags(thiz->cctx, SSL_CONF_FLAG_CMDLINE);

    if (args_verify(thiz, &thiz->vpm)) {
        DOPENSSL_LOG_ERROR("args_verify() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    thiz->e = setup_engine(g_log_output, NULL, 1);

    if (thiz->s_key_file == NULL) {
        thiz->s_key_file = thiz->s_cert_file;
    }
    if (thiz->s_key_file2 == NULL) {
        thiz->s_key_file2 = thiz->s_cert_file2;
    }

    if (!load_excert(&thiz->exc, g_log_output)) {
        DOPENSSL_LOG_ERROR("load_excert() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (!thiz->b_notuse_certificate) {
        thiz->s_key = load_key(g_log_output, thiz->s_key_file, thiz->s_key_format, 0, thiz->password, thiz->e, "server certificate private key file");
        if (!thiz->s_key) {
            ERR_print_errors(g_log_output);
            DOPENSSL_LOG_ERROR("load_key() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }

        thiz->s_cert = load_cert(g_log_output, thiz->s_cert_file, thiz->s_cert_format, NULL, thiz->e, "server certificate file");

        if (!thiz->s_cert) {
            ERR_print_errors(g_log_output);
            DOPENSSL_LOG_ERROR("load_cert() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
        if (thiz->s_chain_file) {
            thiz->s_chain = load_certs(g_log_output, thiz->s_chain_file, FORMAT_PEM, NULL, thiz->e, "server certificate chain");
            if (!thiz->s_chain) {
                DOPENSSL_LOG_ERROR("load_certs() fail\n");
                ret = SU_ECODE_ERROR_FROM_LIB;
                goto tag_su_server_start_exit;
            }
        }
        if (thiz->tlsextcbp.servername) {
            thiz->s_key2 = load_key(g_log_output, thiz->s_key_file2, thiz->s_key_format, 0, thiz->password, thiz->e, "second server certificate private key file");
            if (!thiz->s_key2) {
                ERR_print_errors(g_log_output);
                DOPENSSL_LOG_ERROR("load_key() fail\n");
                ret = SU_ECODE_ERROR_FROM_LIB;
                goto tag_su_server_start_exit;
            }

            thiz->s_cert2 = load_cert(g_log_output, thiz->s_cert_file2, thiz->s_cert_format, NULL, thiz->e, "second server certificate file");

            if (!thiz->s_cert2) {
                ERR_print_errors(g_log_output);
                DOPENSSL_LOG_ERROR("load_certs() fail\n");
                ret = SU_ECODE_ERROR_FROM_LIB;
                goto tag_su_server_start_exit;
            }
        }
    }

    if (thiz->next_proto_neg_in) {
        unsigned short len;
        thiz->next_proto.data = next_protos_parse(&len, thiz->next_proto_neg_in);
        if (thiz->next_proto.data == NULL) {
            DOPENSSL_LOG_ERROR("next_protos_parse() fail\n");
        }
        thiz->next_proto.len = len;
    } else {
        thiz->next_proto.data = NULL;
    }

    thiz->alpn_ctx.data = NULL;
    if (thiz->alpn_in) {
        unsigned short len;
        thiz->alpn_ctx.data = next_protos_parse(&len, thiz->alpn_in);
        if (thiz->alpn_ctx.data == NULL) {
            DOPENSSL_LOG_ERROR("next_protos_parse() fail\n");
        }
        thiz->alpn_ctx.len = len;
    }

    if (thiz->crl_file) {
        X509_CRL *crl;
        crl = load_crl(thiz->crl_file, thiz->crl_format);
        if (!crl) {
            DOPENSSL_LOG_ERROR("Error loading CRL\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
        thiz->crls = sk_X509_CRL_new_null();
        if (!thiz->crls || !sk_X509_CRL_push(thiz->crls, crl)) {
            DOPENSSL_LOG_ERROR("Error adding CRL\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            X509_CRL_free(crl);
            goto tag_su_server_start_exit;
        }
    }

    if (thiz->s_dcert_file) {

        if (thiz->s_dkey_file == NULL) {
            thiz->s_dkey_file = thiz->s_dcert_file;
        }

        thiz->s_dkey = load_key(g_log_output, thiz->s_dkey_file, thiz->s_dkey_format, 0, thiz->dpassword, thiz->e, "second certificate private key file");
        if (!thiz->s_dkey) {
            DOPENSSL_LOG_ERROR("load_key() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }

        thiz->s_dcert = load_cert(g_log_output, thiz->s_dcert_file, thiz->s_dcert_format, NULL, thiz->e, "second server certificate file");

        if (!thiz->s_dcert) {
            DOPENSSL_LOG_ERROR("load_cert() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
        if (thiz->s_dchain_file) {
            thiz->s_dchain = load_certs(g_log_output, thiz->s_dchain_file, FORMAT_PEM, NULL, thiz->e, "second server certificate chain");
            if (!thiz->s_dchain) {
                DOPENSSL_LOG_ERROR("load_certs() fail\n");
                ret = SU_ECODE_ERROR_FROM_LIB;
                goto tag_su_server_start_exit;
            }
        }

    }

    if (!app_RAND_load_file(NULL, g_log_output, 1) && inrand == NULL && !RAND_status()) {
        DOPENSSL_LOG_WARN(g_log_output, "warning, not much extra random data, consider using the -rand option\n");
    }
    if (inrand != NULL) {
        DOPENSSL_LOG_NOTICE(g_log_output, "%ld semi-random bytes loaded\n", app_RAND_load_files(inrand));
    }

    if (thiz->bio_s_out == NULL) {
        if (thiz->s_quiet && !thiz->s_debug) {
            thiz->bio_s_out = BIO_new(BIO_s_null());
            if (thiz->s_msg && !thiz->bio_s_msg)
            { thiz->bio_s_msg = BIO_new_fp(stdout, BIO_NOCLOSE); }
        } else {
            if (thiz->bio_s_out == NULL)
            { thiz->bio_s_out = BIO_new_fp(stdout, BIO_NOCLOSE); }
        }
    }

    if (thiz->b_notuse_certificate) {
        thiz->s_cert_file = NULL;
        thiz->s_key_file = NULL;
        thiz->s_dcert_file = NULL;
        thiz->s_dkey_file = NULL;
        thiz->s_cert_file2 = NULL;
        thiz->s_key_file2 = NULL;
    }

    thiz->ctx = SSL_CTX_new(meth);
    if (thiz->ctx == NULL) {
        DOPENSSL_LOG_ERROR("SSL_CTX_new() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (session_id_prefix) {
        if (strlen(session_id_prefix) >= 32) {
            DOPENSSL_LOG_NOTICE("warning: id_prefix is too long, only one new session will be possible\n");
        } else if (strlen(session_id_prefix) >= 16) {
            DOPENSSL_LOG_NOTICE("warning: id_prefix is too long if you use SSLv2\n");
        }
        if (!SSL_CTX_set_generate_session_id(thiz->ctx, generate_session_id)) {
            DOPENSSL_LOG_ERROR("error setting 'id_prefix'\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
        DOPENSSL_LOG_NOTICE("id_prefix '%s' set.\n", session_id_prefix);
    }
    SSL_CTX_set_quiet_shutdown(thiz->ctx, 1);
    if (hack) {
        SSL_CTX_set_options(thiz->ctx, SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG);
    }
    if (thiz->exc) {
        ssl_ctx_set_excert(thiz->ctx, thiz->exc);
    }

    if (thiz->b_state) {
        SSL_CTX_set_info_callback(thiz->ctx, apps_ssl_info_callback);
    }
    if (thiz->no_cache) {
        SSL_CTX_set_session_cache_mode(thiz->ctx, SSL_SESS_CACHE_OFF);
    } else if (thiz->ext_cache) {
        init_session_cache_ctx(thiz->ctx);
    } else {
        SSL_CTX_sess_set_cache_size(thiz->ctx, 128);
    }

    if (thiz->srtp_profiles != NULL) {
        SSL_CTX_set_tlsext_use_srtp(thiz->ctx, thiz->srtp_profiles);
    }

    if ((!SSL_CTX_load_verify_locations(thiz->ctx, thiz->CAfile, thiz->CApath)) ||
            (!SSL_CTX_set_default_verify_paths(thiz->ctx))) {
        DOPENSSL_LOG_ERROR("X509_load_verify_locations\n");
    }
    if (thiz->vpm) {
        SSL_CTX_set1_param(thiz->ctx, thiz->vpm);
    }

    ssl_ctx_add_crls(thiz->ctx, thiz->crls, 0);

    if (!args_ssl_call(thiz->ctx, g_log_output, thiz->cctx, thiz->ssl_args, thiz->no_ecdhe, 1)) {
        DOPENSSL_LOG_ERROR("args_ssl_call fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (!ssl_load_stores(thiz->ctx, thiz->vfyCApath, thiz->vfyCAfile, thiz->chCApath, thiz->chCAfile, thiz->crls, thiz->crl_download)) {
        DOPENSSL_LOG_ERROR("Error loading store locations\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (thiz->s_cert2) {
        thiz->ctx2 = SSL_CTX_new(meth);
        if (thiz->ctx2 == NULL) {
            DOPENSSL_LOG_ERROR("SSL_CTX_new() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
    }

    if (thiz->ctx2) {
        DOPENSSL_LOG_NOTICE("Setting secondary ctx parameters\n");

        if (session_id_prefix) {
            if (strlen(session_id_prefix) >= 32) {
                DOPENSSL_LOG_NOTICE("warning: id_prefix is too long, only one new session will be possible\n");
            } else if (strlen(session_id_prefix) >= 16) {
                DOPENSSL_LOG_NOTICE("warning: id_prefix is too long if you use SSLv2\n");
            }
            if (!SSL_CTX_set_generate_session_id(thiz->ctx2, generate_session_id)) {
                DOPENSSL_LOG_ERROR("error setting 'id_prefix'\n");
                ret = SU_ECODE_ERROR_FROM_LIB;
                goto tag_su_server_start_exit;
            }
            DOPENSSL_LOG_NOTICE("id_prefix '%s' set.\n", session_id_prefix);
        }
        SSL_CTX_set_quiet_shutdown(thiz->ctx2, 1);
        if (hack)
        { SSL_CTX_set_options(thiz->ctx2, SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG); }
        if (thiz->exc) {
            ssl_ctx_set_excert(thiz->ctx2, thiz->exc);
        }

        if (thiz->b_state) {
            SSL_CTX_set_info_callback(thiz->ctx2, apps_ssl_info_callback);
        }

        if (thiz->no_cache) {
            SSL_CTX_set_session_cache_mode(thiz->ctx2, SSL_SESS_CACHE_OFF);
        } else if (thiz->ext_cache) {
            init_session_cache_ctx(thiz->ctx2);
        } else {
            SSL_CTX_sess_set_cache_size(thiz->ctx2, 128);
        }

        if ((!SSL_CTX_load_verify_locations(thiz->ctx2, thiz->CAfile, thiz->CApath)) || (!SSL_CTX_set_default_verify_paths(thiz->ctx2))) {
            DOPENSSL_LOG_ERROR("SSL_CTX_load_verify_locations() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
        if (thiz->vpm) {
            SSL_CTX_set1_param(thiz->ctx2, thiz->vpm);
        }

        ssl_ctx_add_crls(thiz->ctx2, thiz->crls, 0);

        if (!args_ssl_call(thiz->ctx2, bio_err, thiz->cctx, thiz->ssl_args, thiz->no_ecdhe, 1)) {
            DOPENSSL_LOG_ERROR("args_ssl_call() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
    }

    if (thiz->next_proto.data) {
        SSL_CTX_set_next_protos_advertised_cb(thiz->ctx, thiz->next_proto_cb, &thiz->next_proto);
    }
    if (thiz->alpn_ctx.data) {
        SSL_CTX_set_alpn_select_cb(thiz->ctx, thiz->alpn_cb, &thiz->alpn_ctx);
    }

    if (!thiz->no_dhe) {
        DH *dh = NULL;

        if (thiz->dhfile) {
            dh = load_dh_param(thiz->dhfile);
        } else if (thiz->s_cert_file) {
            dh = load_dh_param(thiz->s_cert_file);
        }

        if (dh != NULL) {
            DOPENSSL_LOG_NOTICE("Setting temp DH parameters\n");
        } else {
            DOPENSSL_LOG_NOTICE("Using default temp DH parameters\n");
            dh = get_dh512();
        }
        (void)BIO_flush(g_log_output);

        SSL_CTX_set_tmp_dh(thiz->ctx, dh);

        if (thiz->ctx2) {
            if (!thiz->dhfile) {
                DH *dh2 = load_dh_param(thiz->s_cert_file2);
                if (dh2 != NULL) {
                    DOPENSSL_LOG_NOTICE("Setting temp DH parameters\n");
                    (void)BIO_flush(g_log_output);

                    DH_free(dh);
                    dh = dh2;
                }
            }
            SSL_CTX_set_tmp_dh(thiz->ctx2, dh);
        }

        DH_free(dh);
    }

    if (!set_cert_key_stuff(thiz->ctx, thiz->s_cert, thiz->s_key, thiz->s_chain, thiz->build_chain)) {
        DOPENSSL_LOG_ERROR("set_cert_key_stuff() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (thiz->s_serverinfo_file != NULL && !SSL_CTX_use_serverinfo_file(thiz->ctx, thiz->s_serverinfo_file)) {
        DOPENSSL_LOG_ERROR("SSL_CTX_use_serverinfo_file() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (thiz->ctx2 && !set_cert_key_stuff(thiz->ctx2, thiz->s_cert2, thiz->s_key2, NULL, thiz->build_chain)) {
        DOPENSSL_LOG_ERROR("set_cert_key_stuff() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    if (thiz->s_dcert != NULL) {
        if (!set_cert_key_stuff(thiz->ctx, thiz->s_dcert, thiz->s_dkey, thiz->s_dchain, thiz->build_chain)) {
            DOPENSSL_LOG_ERROR("set_cert_key_stuff() fail\n");
            ret = SU_ECODE_ERROR_FROM_LIB;
            goto tag_su_server_start_exit;
        }
    }

    if (!thiz->no_tmp_rsa) {
        SSL_CTX_set_tmp_rsa_callback(thiz->ctx, thiz->tmp_rsa_cb);
        if (thiz->ctx2) {
            SSL_CTX_set_tmp_rsa_callback(thiz->ctx2, thiz->tmp_rsa_cb);
        }
    }

    if (thiz->psk_key != NULL) {
        if (thiz->s_debug) {
            DOPENSSL_LOG_NOTICE("PSK key given or JPAKE in use, setting server callback\n");
        }
        SSL_CTX_set_psk_server_callback(thiz->ctx, psk_server_cb);
    }

    if (!SSL_CTX_use_psk_identity_hint(thiz->ctx, thiz->psk_identity_hint)) {
        DOPENSSL_LOG_ERROR("error setting PSK identity hint to context\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto tag_su_server_start_exit;
    }

    SSL_CTX_set_verify(thiz->ctx, thiz->s_server_verify, verify_callback);
    SSL_CTX_set_session_id_context(thiz->ctx, (void *)&thiz->s_server_session_id_context,
                                   sizeof(thiz->s_server_session_id_context));

    /* Set DTLS cookie generation and verification callbacks */
    SSL_CTX_set_cookie_generate_cb(thiz->ctx, generate_cookie_callback);
    SSL_CTX_set_cookie_verify_cb(thiz->ctx, verify_cookie_callback);

    if (thiz->ctx2) {
        SSL_CTX_set_verify(thiz->ctx2, thiz->s_server_verify, verify_callback);
        SSL_CTX_set_session_id_context(thiz->ctx2,
                                       (void *)&thiz->s_server_session_id_context,
                                       sizeof(thiz->s_server_session_id_context));

        thiz->tlsextcbp.biodebug = g_log_output;
        SSL_CTX_set_tlsext_servername_callback(thiz->ctx2, ssl_servername_cb);
        SSL_CTX_set_tlsext_servername_arg(thiz->ctx2, &thiz->tlsextcbp);
        SSL_CTX_set_tlsext_servername_callback(thiz->ctx, ssl_servername_cb);
        SSL_CTX_set_tlsext_servername_arg(thiz->ctx, &thiz->tlsextcbp);
    }

    if (thiz->srp_verifier_file != NULL) {
        thiz->srp_callback_parm.vb = SRP_VBASE_new(thiz->srpuserseed);
        thiz->srp_callback_parm.user = NULL;
        thiz->srp_callback_parm.login = NULL;
        if ((ret = SRP_VBASE_init(thiz->srp_callback_parm.vb, thiz->srp_verifier_file)) != SRP_NO_ERROR) {
            DOPENSSL_LOG_ERROR("Cannot initialize SRP verifier file \"%s\":ret=%d\n", thiz->srp_verifier_file, ret);
            goto tag_su_server_start_exit;
        }
        SSL_CTX_set_verify(thiz->ctx, SSL_VERIFY_NONE, verify_callback);
        SSL_CTX_set_srp_cb_arg(thiz->ctx, &thiz->srp_callback_parm);
        SSL_CTX_set_srp_username_callback(thiz->ctx, ssl_srp_server_param_cb);
    } else if (thiz->CAfile != NULL) {
        SSL_CTX_set_client_CA_list(thiz->ctx, SSL_load_client_CA_file(thiz->CAfile));
        if (thiz->ctx2) {
            SSL_CTX_set_client_CA_list(thiz->ctx2, SSL_load_client_CA_file(thiz->CAfile));
        }
    }

    DOPENSSL_LOG_NOTICE("su_server_start() done\n");
    return SU_ECODE_OK;

tag_su_server_start_exit:

    if (thiz->ctx != NULL)
    { SSL_CTX_free(thiz->ctx); }
    if (thiz->s_cert)
    { X509_free(thiz->s_cert); }
    if (thiz->crls)
    { sk_X509_CRL_pop_free(thiz->crls, X509_CRL_free); }
    if (thiz->s_dcert)
    { X509_free(thiz->s_dcert); }
    if (thiz->s_key)
    { EVP_PKEY_free(thiz->s_key); }
    if (thiz->s_dkey)
    { EVP_PKEY_free(thiz->s_dkey); }
    if (thiz->s_chain)
    { sk_X509_pop_free(thiz->s_chain, X509_free); }
    if (thiz->s_dchain)
    { sk_X509_pop_free(thiz->s_dchain, X509_free); }
    if (thiz->pass)
    { OPENSSL_free(thiz->pass); }
    if (thiz->dpass)
    { OPENSSL_free(thiz->dpass); }
    if (thiz->vpm)
    { X509_VERIFY_PARAM_free(thiz->vpm); }
    free_sessions();
    if (thiz->tlscstatp.host)
    { OPENSSL_free(thiz->tlscstatp.host); }
    if (thiz->tlscstatp.port)
    { OPENSSL_free(thiz->tlscstatp.port); }
    if (thiz->tlscstatp.path)
    { OPENSSL_free(thiz->tlscstatp.path); }
    if (thiz->ctx2 != NULL)
    { SSL_CTX_free(thiz->ctx2); }
    if (thiz->s_cert2)
    { X509_free(thiz->s_cert2); }
    if (thiz->s_key2)
    { EVP_PKEY_free(thiz->s_key2); }
    if (thiz->serverinfo_in != NULL)
    { BIO_free(thiz->serverinfo_in); }
    if (thiz->next_proto.data)
    { OPENSSL_free(thiz->next_proto.data); }
    if (thiz->alpn_ctx.data) {
        OPENSSL_free(thiz->alpn_ctx.data);
    }
    ssl_excert_free(thiz->exc);
    if (thiz->ssl_args)
    { sk_OPENSSL_STRING_free(thiz->ssl_args); }
    if (thiz->cctx)
    { SSL_CONF_CTX_free(thiz->cctx); }
    if (g_log_output != NULL) {
        BIO_free(g_log_output);
        g_log_output = NULL;
    }
    if (thiz->bio_s_msg != NULL) {
        BIO_free(thiz->bio_s_msg);
        thiz->bio_s_msg = NULL;
    }

    return ret;
}

int su_server_stop(s_secure_server *server);

s_client_secure_connection *su_client_connection_create();
void su_client_connection_destroy(s_client_secure_connection *client_connection);
int su_client_connection_connect(s_client_secure_connection *client_connection, char *server_url, unsigned short server_port, char *client_sertificate);
int su_client_connection_write(s_client_secure_connection *client_connection, unsigned char *p_data, int data_len);
int su_client_connection_read(s_client_secure_connection *client_connection, unsigned char *p_data, int max_data_len);

s_server_secure_connection *su_server_connection_create(TSUSocketHandler socket);
void su_server_connection_destroy(s_server_secure_connection *server_connection);
int su_server_connection_write(s_server_secure_connection *server_connection, unsigned char *p_data, int data_len);
int su_server_connection_read(s_server_secure_connection *server_connection, unsigned char *p_data, int max_data_len);

#endif

int __pkey_ctrl_string_rsa_bits(EVP_PKEY_CTX *ctx, int bits)
{
    char bits_string[16] = {0};
    snprintf(bits_string, 16, "%d", bits);

    return EVP_PKEY_CTX_ctrl_str(ctx, "rsa_keygen_bits", bits_string);
}

int __pkey_ctrl_string_rsa_pubexp(EVP_PKEY_CTX *ctx, int pubexp)
{
    char bits_string[16] = {0};
    snprintf(bits_string, 16, "%d", pubexp);

    return EVP_PKEY_CTX_ctrl_str(ctx, "rsa_keygen_pubexp", bits_string);
}

EVP_PKEY_CTX *__pkey_get_ctx(const char *algname, ENGINE *e)
{
    EVP_PKEY_CTX *ctx = NULL;
    const EVP_PKEY_ASN1_METHOD *ameth;
    ENGINE *tmpeng = NULL;
    int pkey_id;

    ameth = EVP_PKEY_asn1_find_str(&tmpeng, algname, -1);

#ifndef OPENSSL_NO_ENGINE
    if (!ameth && e) {
        ameth = ENGINE_get_pkey_asn1_meth_str(e, algname, -1);
    }
#endif

    if (!ameth) {
        printf("[error]: Algorithm %s not found\n", algname);
        return NULL;
    }

    EVP_PKEY_asn1_get0_info(&pkey_id, NULL, NULL, NULL, NULL, ameth);
#ifndef OPENSSL_NO_ENGINE
    if (tmpeng) {
        ENGINE_finish(tmpeng);
    }
#endif
    ctx = EVP_PKEY_CTX_new_id(pkey_id, e);

    if (!ctx) {
        printf("[error]: EVP_PKEY_CTX_new_id fail\n");
        return NULL;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        printf("[error]: EVP_PKEY_keygen_init fail\n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

static EVP_PKEY *__rsa_load_key(const char *file)
{
    int ret = 0;
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if (file == NULL) {
        printf("[error]: no keyfile specified\n");
        return NULL;
    }

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        printf("[error]: no memory, BIO_new fail\n");
        return NULL;
    }

    ret = BIO_read_filename(key, file);
    if (ret <= 0) {
        printf("[error]: BIO_read_filename fail\n");
        return NULL;
    }

    pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, NULL);

    BIO_free(key);

    if (pkey == NULL) {
        printf("[error]: PEM_read_bio_PrivateKey fail\n");
    }

    return (pkey);
}

static EVP_PKEY *__rsa_load_pubkey(const char *file)
{
    int ret = 0;
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if (file == NULL) {
        printf("[error]: no keyfile specified\n");
        return NULL;
    }

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        printf("[error]: no memory, BIO_new fail\n");
        return NULL;
    }

    ret = BIO_read_filename(key, file);
    if (ret <= 0) {
        printf("[error]: BIO_read_filename fail\n");
        return NULL;
    }

    pkey = PEM_read_bio_PUBKEY(key, NULL, NULL, NULL);

    BIO_free(key);

    if (pkey == NULL) {
        printf("[error]: PEM_read_bio_PUBKEY fail\n");
    }

    return (pkey);
}

int __dgst_do_fp(BIO *out, unsigned char *buf, BIO *bp, int sep, int binout,
                 EVP_PKEY *key, unsigned char *sigin, int siglen,
                 const char *sig_name, const char *md_name,
                 const char *file, BIO *bmd)
{
    size_t len;
    int i;

    for (;;) {
        i = BIO_read(bp, (char *)buf, BUFSIZE);
        if (i < 0) {
            printf("[error]: Read Error in %s\n", file);
            return 1;
        }
        if (i == 0)
        { break; }
    }
    if (sigin) {
        EVP_MD_CTX *ctx;
        BIO_get_md_ctx(bp, &ctx);
        i = EVP_DigestVerifyFinal(ctx, sigin, (unsigned int)siglen);
        if (i > 0)
        { printf("Verified OK\n"); }
        else if (i == 0) {
            printf("Verification Failure\n");
            return 1;
        } else {
            printf("[error]: Error Verifying Data\n");
            return 1;
        }
        return 0;
    }
    if (key) {
        EVP_MD_CTX *ctx;
        BIO_get_md_ctx(bp, &ctx);
        len = BUFSIZE;
        if (!EVP_DigestSignFinal(ctx, buf, &len)) {
            printf("[error]: Error Signing Data\n");
            return 1;
        }
    } else {
        len = BIO_gets(bp, (char *)buf, BUFSIZE);
        if ((int)len < 0) {
            printf("[error]: BIO_gets fail\n");
            return 1;
        }
    }

    if (binout)
    { BIO_write(out, buf, len); }
    else if (sep == 2) {
        for (i = 0; i < (int)len; i++)
        { BIO_printf(out, "%02x", buf[i]); }
        BIO_printf(out, " *%s\n", file);
    } else {
        if (sig_name) {
            BIO_puts(out, sig_name);
            if (md_name)
            { BIO_printf(out, "-%s", md_name); }
            BIO_printf(out, "(%s)= ", file);
        } else if (md_name)
        { BIO_printf(out, "%s(%s)= ", md_name, file); }
        else
        { BIO_printf(out, "(%s)= ", file); }
        for (i = 0; i < (int)len; i++) {
            if (sep && (i != 0))
            { BIO_printf(out, ":"); }
            BIO_printf(out, "%02x", buf[i]);
        }
        BIO_printf(out, "\n");
    }
    return 0;
}

int generate_rsa_key(char *output_file, int bits, int pubexp, int key_format)
{
    int ret = 0;
    BIO *out = NULL;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;

    if (!output_file) {
        printf("[error]: NULL output_file\n");
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto generate_rsa_key_exit;
    }

    ctx = __pkey_get_ctx("RSA", NULL);
    if (!ctx) {
        printf("[error]: __pkey_get_ctx fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto generate_rsa_key_exit;
    }

    out = BIO_new_file(output_file, "wb");
    if (!out) {
        printf("[error]: BIO_new_file(%s) fail\n", output_file);
        ret = SU_ECODE_INVALID_OUTPUT;
        goto generate_rsa_key_exit;
    }

    ret = __pkey_ctrl_string_rsa_bits(ctx, bits);
    ret = __pkey_ctrl_string_rsa_pubexp(ctx, pubexp);

    ret = EVP_PKEY_keygen(ctx, &pkey);
    if (ret <= 0) {
        printf("[error]: EVP_PKEY_keygen(%d) fail\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto generate_rsa_key_exit;
    }

    ret = PEM_write_bio_PrivateKey(out, pkey, NULL, NULL, 0, NULL, NULL);
    if (ret <= 0) {
        printf("[error]: PEM_write_bio_PrivateKey(%d) fail\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto generate_rsa_key_exit;
    }

    ret = SU_ECODE_OK;

generate_rsa_key_exit:
    if (pkey) {
        EVP_PKEY_free(pkey);
        pkey = NULL;
    }
    if (ctx) {
        EVP_PKEY_CTX_free(ctx);
        ctx = NULL;
    }
    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    return ret;
}

int get_public_rsa_key(char *output_file, char *input_file)
{
    EVP_PKEY *pkey = NULL;
    RSA *rsa = NULL;
    BIO *out = NULL;
    int ret = 0;

    if (!output_file || !input_file) {
        printf("[error]: NULL input filename or NULL output filename\n");
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto get_public_rsa_key_exit;
    }

    pkey = __rsa_load_key(input_file);
    if (!pkey) {
        printf("[error]: __rsa_load_key(%s) fail\n", input_file);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    rsa = EVP_PKEY_get1_RSA(pkey);
    EVP_PKEY_free(pkey);
    pkey = NULL;

    out = BIO_new(BIO_s_file());
    if (!out) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    ret = BIO_write_filename(out, output_file);
    if (0 >= ret) {
        printf("[error]: BIO_write_filename fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    ret = PEM_write_bio_RSA_PUBKEY(out, rsa);
    if (0 >= ret) {
        printf("[error]: PEM_write_bio_RSA_PUBKEY fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    ret = SU_ECODE_OK;

get_public_rsa_key_exit:
    if (pkey) {
        EVP_PKEY_free(pkey);
        pkey = NULL;
    }
    if (rsa) {
        RSA_free(rsa);
        rsa = NULL;
    }
    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    return ret;
}

int signature_file(char *file, char *digital_signature, char *keyfile, int sha_type)
{
    const EVP_MD *md = NULL;
    BIO *in = NULL, *inp = NULL;
    BIO *out = NULL;
    EVP_PKEY *sigkey = NULL;
    BIO *bmd = NULL;
    EVP_MD_CTX *mctx = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    unsigned char *buf = NULL;
    int ret = 0;

    char sha_name[32] = {0};

    if (!g_init_openssl) {
        //g_init_openssl = 1;
        //OpenSSL_add_all_algorithms();
        initialize_su_library(SU_LIB_OPENSSL, SU_LIB_LOG_TO_STDOUT);
    }

    if (SHA_TYPE_SHA256 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha256");
    } else if (SHA_TYPE_SHA1 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha1");
    } else if (SHA_TYPE_SHA384 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha384");
    } else if (SHA_TYPE_SHA512 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha512");
    } else {
        printf("[error]: bad sha_type %d\n", sha_type);
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto signature_file_exit;
    }

    md = EVP_get_digestbyname(sha_name);
    if (!md) {
        printf("[error]: EVP_get_digestbyname(%s) fail\n", sha_name);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    in = BIO_new(BIO_s_file());
    if (!in) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = BIO_read_filename(in, file);
    if (0 >= ret) {
        printf("[error]: BIO_read_filename(%s) fail\n", file);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    out = BIO_new_file(digital_signature, "wb");
    if (!out) {
        printf("[error]: BIO_new_file(%s)\n", digital_signature);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    sigkey = __rsa_load_key(keyfile);
    if (!sigkey) {
        printf("[error]: __rsa_load_key(%s)\n", keyfile);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    bmd = BIO_new(BIO_f_md());
    if (!bmd) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = BIO_get_md_ctx(bmd, &mctx);
    if (0 >= ret) {
        printf("[error]: BIO_get_md_ctx() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = EVP_DigestSignInit(mctx, &pctx, md, NULL, sigkey);
    if (0 >= ret) {
        printf("[error]: EVP_DigestSignInit() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    inp = BIO_push(bmd, in);

    if ((buf = (unsigned char *)OPENSSL_malloc(BUFSIZE)) == NULL) {
        printf("[error]: no memory() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = __dgst_do_fp(out, buf, inp, 0, 1, sigkey, NULL, 0, NULL, NULL, file, bmd);
    if (ret) {
        printf("[error]: __dgst_do_fp() fail, ret %d\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = SU_ECODE_OK;

signature_file_exit:

    if (buf != NULL) {
        OPENSSL_cleanse(buf, BUFSIZE);
        OPENSSL_free(buf);
        buf = NULL;
    }

    if (in != NULL) {
        BIO_free(in);
        in = NULL;
    }

    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    if (sigkey) {
        EVP_PKEY_free(sigkey);
        sigkey = NULL;
    }

    if (bmd != NULL) {
        BIO_free(bmd);
    }

    return ret;
}

int verify_signature(char *file, char *digital_signature, char *keyfile, int sha_type)
{
    int ret = 0;
    unsigned char *sigbuf = NULL;
    int siglen = 0;
    unsigned char *buf = NULL;
    BIO *out = NULL;

    const EVP_MD *md = NULL;

    EVP_PKEY *sigkey = NULL;
    EVP_MD_CTX *mctx = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    BIO *bmd = NULL;
    BIO *in = NULL, *inp;
    BIO *sigbio = NULL;

    char sha_name[32] = {0};

    if (!g_init_openssl) {
        //g_init_openssl = 1;
        //OpenSSL_add_all_algorithms();
        initialize_su_library(SU_LIB_OPENSSL, SU_LIB_LOG_TO_STDOUT);
    }

    if (SHA_TYPE_SHA256 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha256");
    } else if (SHA_TYPE_SHA1 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha1");
    } else if (SHA_TYPE_SHA384 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha384");
    } else if (SHA_TYPE_SHA512 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha512");
    } else {
        printf("[error]: bad sha_type %d\n", sha_type);
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto verify_signature_exit;
    }

    md = EVP_get_digestbyname(sha_name);
    if (!md) {
        printf("[error]: EVP_get_digestbyname(%s) fail\n", sha_name);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    in = BIO_new(BIO_s_file());
    if (!in) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = BIO_read_filename(in, file);
    if (0 >= ret) {
        printf("[error]: BIO_read_filename(%s) fail\n", file);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    out = BIO_new_fp(stdout, BIO_NOCLOSE);

    sigkey = __rsa_load_pubkey(keyfile);
    if (!sigkey) {
        printf("[error]: __rsa_load_pubkey(%s)\n", keyfile);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    bmd = BIO_new(BIO_f_md());
    if (!bmd) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = BIO_get_md_ctx(bmd, &mctx);
    if (0 >= ret) {
        printf("[error]: BIO_get_md_ctx() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = EVP_DigestVerifyInit(mctx, &pctx, md, NULL, sigkey);
    if (0 >= ret) {
        printf("[error]: EVP_DigestVerifyInit() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    sigbio = BIO_new_file(digital_signature, "rb");
    if (!sigbio) {
        printf("[error]: no memory, BIO_new_file() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    siglen = EVP_PKEY_size(sigkey);
    sigbuf = (unsigned char *) OPENSSL_malloc(siglen);
    if (!sigbuf) {
        printf("[error]: Out of memory\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    siglen = BIO_read(sigbio, sigbuf, siglen);
    if (siglen <= 0) {
        printf("[error]: Error reading signature file %s\n", digital_signature);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    inp = BIO_push(bmd, in);

    if ((buf = (unsigned char *)OPENSSL_malloc(BUFSIZE)) == NULL) {
        printf("[error]: no memory() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = __dgst_do_fp(out, buf, inp, 0, 1, sigkey, sigbuf, siglen, NULL, NULL, file, bmd);
    if (ret) {
        printf("[error]: __dgst_do_fp() fail, ret %d\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    if (ret) {
        printf("verify fail\n");
        ret = SU_ECODE_SIGNATURE_VERIFY_FAIL;
        goto verify_signature_exit;
    }

    printf("verify OK\n");
    ret = SU_ECODE_OK;

verify_signature_exit:

    if (buf != NULL) {
        OPENSSL_cleanse(buf, BUFSIZE);
        OPENSSL_free(buf);
        buf = NULL;
    }

    if (in != NULL) {
        BIO_free(in);
        in = NULL;
    }

    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    if (sigkey) {
        EVP_PKEY_free(sigkey);
        sigkey = NULL;
    }

    if (bmd != NULL) {
        BIO_free(bmd);
    }

    if (sigbio) {
        BIO_free(sigbio);
        sigbio = NULL;
    }

    return ret;
}

s_symmetric_cypher *create_symmetric_cypher(unsigned char cypher_type, unsigned char cypher_mode)
{
    if (CYPHER_TYPE_AES128 != cypher_type) {
        printf("[error]: create_symmetric_cypher: only support aes128 now\n");
        return NULL;
    }

    if (BLOCK_CYPHER_MODE_CTR != cypher_mode) {
        printf("[error]: create_symmetric_cypher: only support CTR mode now\n");
        return NULL;
    }

    s_symmetric_cypher *p_cypher = (s_symmetric_cypher *) malloc(sizeof(s_symmetric_cypher) + sizeof(EVP_CIPHER_CTX));
    if (!p_cypher) {
        printf("[error]: create_symmetric_cypher: no memory\n");
        return NULL;
    }

    memset(p_cypher, 0x0, sizeof(s_symmetric_cypher) + sizeof(EVP_CIPHER_CTX));
    p_cypher->p_cypher_private_context = (void *)((unsigned char *)p_cypher + sizeof(s_symmetric_cypher));

    p_cypher->cypher_type = cypher_type;
    p_cypher->cypher_mode = cypher_mode;
    p_cypher->b_key_iv_set = 0;
    p_cypher->state = 0;

    p_cypher->key_length_in_bytes = 16;
    p_cypher->block_length_in_bytes = 16;
    p_cypher->block_length_mask = 0xfffffff0;

    return p_cypher;
}

void destroy_symmetric_cypher(s_symmetric_cypher *cypher)
{
    if (cypher) {
        if ((0 != cypher->state) && (cypher->p_cypher_private_context)) {
            EVP_CIPHER_CTX *p_ctx = (EVP_CIPHER_CTX *) cypher->p_cypher_private_context;
            EVP_CIPHER_CTX_cleanup(p_ctx);
            cypher->state = 0;
        }
        free(cypher);
    }
    return;
}

int set_symmetric_passphrase_salt(s_symmetric_cypher *cypher, char *passphrase, char *salt, unsigned char gen_method)
{
    if ((!cypher) || (!passphrase) || (!salt)) {
        printf("[error]: set_symmetric_passphrase_salt: NULL params\n");
        return (-1);
    }

    printf("[error]: to do\n");
    return 0;
}

int set_symmetric_key_iv(s_symmetric_cypher *cypher, char *key, char *iv)
{
    if ((!cypher) || (!key) || (!iv)) {
        printf("[error]: set_symmetric_key_iv: NULL params\n");
        return (-1);
    }

    unsigned int len = strlen(key);
    if (len > cypher->key_length_in_bytes) {
        len = cypher->key_length_in_bytes;
    }
    memcpy(cypher->key, key, len);
    cypher->key[len] = 0x0;

    len = strlen(iv);
    if (len > cypher->key_length_in_bytes) {
        len = cypher->key_length_in_bytes;
    }
    memcpy(cypher->iv, iv, len);
    cypher->iv[len] = 0x0;

    cypher->b_key_iv_set = 1;
    return 0;
}

int begin_symmetric_cryption(s_symmetric_cypher *cypher, int enc)
{
    if ((!cypher) || (!cypher->p_cypher_private_context)) {
        printf("[error]: begin_symmetric_cryption: NULL params\n");
        return (-1);
    }

    if (0 != cypher->state) {
        printf("[error]: begin_symmetric_cryption: bad state %d\n", cypher->state);
        return (-2);
    }

    if (0 == cypher->b_key_iv_set) {
        printf("[error]: begin_symmetric_cryption: key iv not set yet\n");
        return (-3);
    }

    EVP_CIPHER_CTX *p_ctx = (EVP_CIPHER_CTX *) cypher->p_cypher_private_context;
    EVP_CIPHER_CTX_init(p_ctx);

    EVP_CipherInit_ex(p_ctx, EVP_aes_128_ctr(), NULL, NULL, NULL, enc);
    EVP_CipherInit_ex(p_ctx, NULL, NULL, cypher->key, cypher->iv, enc);

    if (enc) {
        cypher->state = 1;
    } else {
        cypher->state = 2;
    }

    return 0;
}

int symmetric_cryption(s_symmetric_cypher *cypher, unsigned char *p_buf, int buf_size, unsigned char *p_outbuf, int enc)
{
    if (cypher && cypher->p_cypher_private_context && p_buf && buf_size && p_outbuf) {
        if ((enc && (1 == cypher->state)) || ((!enc) && (2 == cypher->state))) {
            EVP_CIPHER_CTX *p_ctx = (EVP_CIPHER_CTX *) cypher->p_cypher_private_context;
            buf_size &= cypher->block_length_mask;
            int out_size = buf_size;
            if (!EVP_CipherUpdate(p_ctx, p_outbuf, &out_size, p_buf, buf_size)) {
                printf("[error]: EVP_CipherUpdate\n");
                return (-1);
            }

            return out_size;
        } else {
            printf("[error]: symmetric_cryption: bad state %d, enc %d\n", cypher->state, enc);
            return (-2);
        }
    }

    printf("[error]: symmetric_cryption: bad parameters\n");
    return (-3);
}

int end_symmetric_cryption(s_symmetric_cypher *cypher)
{
    if (cypher && cypher->p_cypher_private_context && cypher->state) {
        EVP_CIPHER_CTX *p_ctx = (EVP_CIPHER_CTX *) cypher->p_cypher_private_context;
        EVP_CIPHER_CTX_cleanup(p_ctx);
        cypher->state = 0;
    } else {
        if (cypher) {
            printf("[error]: end_symmetric_cryption: bad state or parameters, cypher->p_cypher_private_context %p, cypher->state %d\n", cypher->p_cypher_private_context, cypher->state);
        } else {
            printf("[error]: end_symmetric_cryption: bad state or parameters, NULL cypher\n");
        }
        return (-1);
    }

    return 0;
}

