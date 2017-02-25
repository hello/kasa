/*******************************************************************************
 * openssl_helper.h
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

#ifndef __OPENSSL_HELPER_H__
#define __OPENSSL_HELPER_H__

#ifdef DCONFIG_COMPILE_NOT_FINISHED_MODULE

typedef struct srpsrvparm_st {
    char *login;
    SRP_VBASE *vb;
    SRP_user_pwd *user;
} srpsrvparm;

typedef struct {
    X509_VERIFY_PARAM *vpm;
    s_ssl_excert_st *exc;
    SSL_CTX *ctx;
    SSL_CTX *ctx2;
    SSL_CONF_CTX *cctx;
    STACK_OF(OPENSSL_STRING) *ssl_args;

    BIO *serverinfo_in;
    const char *s_serverinfo_file;

    ENGINE *e;

    const char *psk_key;
    const char *psk_identity_hint;

    char *srpuserseed;
    char *srp_verifier_file;
    srpsrvparm srp_callback_parm;

    int s_server_verify;
    int s_server_session_id_context;

    char *CApath;
    char *CAfile;
    char *chCApath;
    char *chCAfile;
    char *vfyCApath;
    char *vfyCAfile;

    unsigned char *context;
    char *dhfile;
    int badop;
    int build_chain;
    int no_tmp_rsa;
    int no_dhe;
    int no_ecdhe;
    int socket_type;

    int no_cache;
    int ext_cache;
    int rev;
    int naccept;

    BIO *bio_s_out;
    BIO *bio_s_msg;
    int s_quiet;
    int s_debug;
    int s_msg;

    char *crl_file;
    int crl_format;
    int crl_download;
    STACK_OF(X509_CRL) *crls;

    int s_cert_format;
    int s_key_format;
    int s_dcert_format;
    int s_dkey_format;

    X509 *s_cert;
    X509 *s_dcert;
    STACK_OF(X509) *s_chain;
    STACK_OF(X509) *s_dchain;

    EVP_PKEY *s_key;
    EVP_PKEY *s_dkey;

    EVP_PKEY *s_key2;
    X509 *s_cert2;

    tlsextctx tlsextcbp;
    const char *next_proto_neg_in;
    tlsextnextprotoctx next_proto;
    const char *alpn_in;
    tlsextalpnctx alpn_ctx;
    const char *srtp_profiles;

    const char *s_cert_file;
    const char *s_key_file;
    const char *s_chain_file;
    const char *s_cert_file2;
    const char *s_key_file2;
    const char *s_dcert_file;
    const char *s_dkey_file;
    const char *s_dchain_file;

    //user password
    char password[64];
    char dpassword[64];

    //input argument
    int verify_depth;
    unsigned long attime;

    unsigned char b_state;
    unsigned char b_password;
    unsigned char b_notuse_certificate;
    unsigned char b_set_policy;

    unsigned char b_set_purpose;
    unsigned char b_set_verify_path;
    unsigned char b_set_attime;
    unsigned char b_set_verify_hostname;

    unsigned char b_set_verify_email;
    unsigned char b_set_verify_ip;
    unsigned char b_ignore_critical;
    unsigned char b_issuer_checks;

    unsigned char b_crl_check;
    unsigned char b_crl_check_all;
    unsigned char b_policy_check;
    unsigned char b_explicit_policy;

    unsigned char b_inhibit_any;
    unsigned char b_inhibit_map;
    unsigned char b_x509_strict;
    unsigned char b_extended_crl;

    unsigned char b_use_deltas;
    unsigned char b_policy_notify;
    unsigned char b_check_ss_sig;
    unsigned char b_trusted_first;

    unsigned char b_suiteB_128_only;
    unsigned char b_suiteB_128;
    unsigned char b_suiteB_192;
    unsigned char b_partial_chain;

    //policy
    char policy[DMAX_SU_ARGUMENT_LENGTH];
    //purpose
    char purpose[DMAX_SU_ARGUMENT_LENGTH];
    //verify hostname
    char verify_hostname[DMAX_SU_ARGUMENT_LENGTH];
    //verify email
    char verify_email[DMAX_SU_ARGUMENT_LENGTH];
    //verify ip
    char verify_ip[DMAX_SU_ARGUMENT_LENGTH];

} s_secure_server_openssl;

int load_excert(SSL_EXCERT **pexc, BIO *err);

#endif

#endif


