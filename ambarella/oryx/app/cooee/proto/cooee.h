#ifndef __COOEE_H__
#define __COOEE_H__

#include <easy_setup.h>

#define COOEE_KEY_STRING_LEN (16)
#define COOEE_NONCE_PAD_LEN (13)

typedef struct {
    uint8 key_bytes[COOEE_KEY_STRING_LEN];  /* key string for decoding */
    uint8 random_bytes[COOEE_NONCE_PAD_LEN]; /* random bytes */
    uint8 key_bytes_qqcon[COOEE_KEY_STRING_LEN];  /* key string for decoding for qqcon */
    uint8 random_bytes_qqcon[COOEE_NONCE_PAD_LEN]; /* random bytes for qqcon */
} cooee_param_t;

typedef struct {
    easy_setup_result_t es_result;
    ip_address_t   host_ip_address;      /* setup client's ip address */
    uint16         host_port;            /* setup client's port */
} cooee_result_t;

void cooee_get_param(void* p);
void cooee_set_result(const void* p);

int cooee_set_key(const char* key);
int cooee_set_key_qqcon(const char* key);
int cooee_get_sender_ip(char buff[], int buff_len);
int cooee_get_sender_port(uint16* port);

#endif /* __COOEE_H__ */
