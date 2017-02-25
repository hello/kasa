/*******************************************************************************
 * security_lib_wrapper.h
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

#ifndef __SECURITY_LIB_WRAPPER_H__
#define __SECURITY_LIB_WRAPPER_H__

enum {
    SU_LIB_OPENSSL = 0x00,
};

enum {
    SU_LIB_LOG_TO_INVALID = 0x00,
    SU_LIB_LOG_TO_STDOUT = 0x01,
};

enum {
    SU_ECODE_OK = 0x00,
    SU_ECODE_INVALID_ARGUMENT = 0x01,
    SU_ECODE_INVALID_INPUT = 0x02,
    SU_ECODE_INVALID_OUTPUT = 0x03,
    SU_ECODE_INVALID_KEY = 0x04,
    SU_ECODE_SIGNATURE_VERIFY_FAIL = 0x05,

    SU_ECODE_ERROR_FROM_LIB = 0x10,
    SU_ECODE_ERROR_NO_MEMORY = 0x11,
};

enum {
    KEY_FORMAT_PEM = 0x0,
};

enum {
    SHA_TYPE_SHA1 = 0x1,
    SHA_TYPE_SHA256 = 0x2,
    SHA_TYPE_SHA384 = 0x3,
    SHA_TYPE_SHA512 = 0x4,
};

enum {
    CYPHER_TYPE_AES128 = 0x1,
    CYPHER_TYPE_AES256 = 0x2,
    CYPHER_TYPE_AES384 = 0x3,
    CYPHER_TYPE_AES512 = 0x4,
    CYPHER_TYPE_RC4 = 0x5,
    CYPHER_TYPE_CHACHA20 = 0x6,
};

enum {
    BLOCK_CYPHER_MODE_NONE = 0x0, //for stream cypher
    BLOCK_CYPHER_MODE_ECB = 0x1,
    BLOCK_CYPHER_MODE_CBC = 0x2,
    BLOCK_CYPHER_MODE_CTR = 0x3,
    BLOCK_CYPHER_MODE_CFB = 0x4,
    BLOCK_CYPHER_MODE_OFB = 0x5,
};

#ifdef DCONFIG_COMPILE_NOT_FINISHED_MODULE
int initialize_su_library(int library_type, int log_to);
int deinitialize_su_library();

#define DMAX_SU_URL_LENGTH 256
#define DMAX_SU_CERTIFICATE_FILE_LENGTH 256

#define DMAX_SU_ARGUMENT_LENGTH 128

enum {
    SU_CLIENT_CONNECTION_STATE_INIT = 0x00,
    SU_CLIENT_CONNECTION_STATE_CONNECTED = 0x01,
    SU_CLIENT_CONNECTION_STATE_CLOSED = 0x02,

    SU_CLIENT_CONNECTION_STATE_AUTHENTICATE_ERROR = 0x05,
    SU_CLIENT_CONNECTION_STATE_NETWORK_ERROR = 0x06,
};

enum {
    SU_SERVER_CONNECTION_STATE_INIT = 0x00,
    SU_SERVER_CONNECTION_STATE_CONNECTED = 0x01,
    SU_SERVER_CONNECTION_STATE_CLOSED = 0x02,

    SU_SERVER_CONNECTION_STATE_AUTHENTICATE_ERROR = 0x05,
    SU_SERVER_CONNECTION_STATE_NETWORK_ERROR = 0x06,
};

typedef int TSUSocketHandler;

typedef void (*TSUServerAcceptCallBack)(void *owner, TSUSocketHandler socket);

typedef struct {
    char server_url[DMAX_SU_URL_LENGTH];
    char client_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];
    char server_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];
    char ca_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];

    unsigned short server_port;
    unsigned char reserved0;
    unsigned char reserved1;

    unsigned char connection_state;
    unsigned char b_have_client_sertificate;
    unsigned char b_have_server_sertificate;
    unsigned char b_have_ca_sertificate;

    void *p_client_secure_connection;
} s_client_secure_connection;

typedef struct {
    char server_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];

    unsigned short server_port;
    unsigned char server_state;
    unsigned char reserved0;

    unsigned int connected_client_number;

    void *accept_callback_context;
    TSUServerAcceptCallBack accept_callback;

    void *p_secure_server;
} s_secure_server;

typedef struct {
    char client_address[DMAX_SU_URL_LENGTH];

    char client_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];
    char server_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];
    char ca_certificate[DMAX_SU_CERTIFICATE_FILE_LENGTH];

    unsigned short server_port;
    unsigned short client_port;

    unsigned char connection_state;
    unsigned char b_have_client_sertificate;
    unsigned char b_have_server_sertificate;
    unsigned char b_have_ca_sertificate;

    void *p_server_secure_connection;

    s_secure_server *p_server;
} s_server_secure_connection;

s_secure_server *su_server_create();
void su_server_destroy(s_secure_server *server);
int su_server_set_accept_callback(s_secure_server *server, void *accept_callback_context, TSUServerAcceptCallBack accept_callback);
int su_server_set_certificate(s_secure_server *server, char *certificate, char *key);
int su_server_start(s_secure_server *server, unsigned short port);
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

#define DMAX_SYMMETRIC_KEY_LENGTH_BYTES 64
#define DMAX_PASSPHRASE_LENGTH 32

int generate_rsa_key(char *output_file, int bits, int pubexp, int key_format);
int get_public_rsa_key(char *output_file, char *input_file);

int signature_file(char *file, char *digital_signature, char *keyfile, int sha_type);
int verify_signature(char *file, char *digital_signature, char *keyfile, int sha_type);

typedef struct {
    unsigned char cypher_type;
    unsigned char cypher_mode;
    unsigned char b_key_iv_set;
    unsigned char state; // 0:idle, 1:enc, 2:dec

    unsigned short key_length_in_bytes;
    unsigned short block_length_in_bytes;

    unsigned int block_length_mask;

    unsigned char passphrase[DMAX_PASSPHRASE_LENGTH];
    unsigned char salt[DMAX_PASSPHRASE_LENGTH];

    unsigned char key[DMAX_SYMMETRIC_KEY_LENGTH_BYTES];
    unsigned char iv[DMAX_SYMMETRIC_KEY_LENGTH_BYTES];

    void *p_cypher_private_context;
} s_symmetric_cypher;

s_symmetric_cypher *create_symmetric_cypher(unsigned char cypher_type, unsigned char cypher_mode);
void destroy_symmetric_cypher(s_symmetric_cypher *cypher);

int set_symmetric_passphrase_salt(s_symmetric_cypher *cypher, char *passphrase, char *salt, unsigned char gen_method);
int set_symmetric_key_iv(s_symmetric_cypher *cypher, char *key, char *iv);

int begin_symmetric_cryption(s_symmetric_cypher *cypher, int enc);
int symmetric_cryption(s_symmetric_cypher *cypher, unsigned char *p_buf, int buf_size, unsigned char *p_outbuf, int enc);
int end_symmetric_cryption(s_symmetric_cypher *cypher);




#endif


