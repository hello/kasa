#ifndef __EASY_SETUP_H__
#define __EASY_SETUP_H__

#include "common.h"

/*
 * Set decryption key, if you have set it in sender side
 * parameter
 *   key: null-terminated key string
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_set_decrypt_key(char* key);

/*
 * Start easy setup
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_start();

/*
 * Stop easy setup
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_stop();

/*
 * Query easy setup result. This function blocks for some time (see EASY_SETUP_QUERY_TIMEOUT)
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_query();

int easy_setup_ioctl(int cmd, int set, void* param, int size);

/*
 * Get ssid. Call this after easy_setup_query() succeeds.
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_get_ssid(char buff[], int buff_len);

/*
 * Get psk. Call this after easy_setup_query() succeeds.
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_get_psk(char buff[], int buff_len);

/*
 * Get sender ip. Call this after easy_setup_query() succeeds.
 * returns
 *   0: succ, -1: fail
 */
int easy_setup_get_sender_ip(char buff[], int buff_len);

/*
 * Get security of configured SSID
 * returns
 *   WLAN_SCURITY_NONE: NONE
 *   WLAN_SCURITY_WEP: WEP
 *   WLAN_SCURITY_WPA: WPA
 *
 */
int easy_setup_get_security(void);

#endif /* __EASY_SETUP_H__ */
