#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include <cooee.h>
#include <scan.h>

/* COOEE query timeout, default 10s */
#define QUERY_TIMEOUT_MS (50*1000)
/* COOEE query interval, default 200ms */
#define QUERY_INTERVAL_MS (200)
/* wlan interface */
#define WLAN_IFACE "wlan0"

#define WLC_EASY_SETUP_STOP (0)
#define WLC_EASY_SETUP_START (1)
#define WLC_GET_EASY_SETUP (340)
#define WLC_SET_EASY_SETUP (341)

#define COOEE_KEY_STRING_LEN (16)
#define EASY_SETUP_PROTO_COOEE (0)
#define EASY_SETUP_STATE_DONE (5)

#define COOEE_IPV4    (4)
#define COOEE_IPV6    (6)

/* Linux network driver ioctl encoding */
typedef struct wl_ioctl {
    uint cmd;        /* common ioctl definition */
    void *buf;       /* pointer to user buffer */
    uint len;        /* length of user buffer */
    unsigned char set;  /* 1=set IOCTL; 0=query IOCTL */
    uint used;       /* bytes read or written (optional) */
    uint needed;     /* bytes needed (optional) */
} wl_ioctl_t;

typedef struct {
    uint8 enable;    /* START/STOP/RESTART easy setup */
    uint8 protocol;  /* set parameters to which protocol */
} wlc_easy_setup_params_t;

/**
 * Structure for parameters (protocol).
 */
typedef struct {
    wlc_easy_setup_params_t es_params;       /* common parameters */
    uint8 key_string[COOEE_KEY_STRING_LEN];  /* key string for decoding */
} cooee_params_t;

/**
 * Structure for storing a Service Set Identifier (i.e. Name of Access Point)
 */
typedef struct {
    uint8 len;     /**< SSID length */
    uint8 val[32]; /**< SSID name (AP name)  */
} cooee_ssid_t;

/**
 * Structure for storing a MAC address (Wi-Fi Media Access Control address).
 */
typedef struct {
    uint8 octet[6]; /**< Unique 6-byte MAC address */
} cooee_mac_t;

/**
 * Structure for storing a IP address.
 */
typedef struct {
    uint8 version;   /**< IPv4 or IPv6 type */

    union {
        uint32 v4;   /**< IPv4 IP address */
        uint32 v6[4];/**< IPv6 IP address */
    } ip;
} cooee_ip_address_t;

/**
 * Structure for Cooee result report to upper when Cooee finished.
 */
typedef struct {
    uint8                security_key[64];     /* target AP's psk */
    uint8                security_key_length;  /* length of psk */
    cooee_ssid_t         ap_ssid;              /* target AP's name */
    cooee_mac_t          ap_bssid;             /* target AP's mac address */
    uint8                ap_channel;           /* target AP's channel */
    cooee_ip_address_t   host_ip_address;      /* setup client's ip address */
} cooee_result_t;

/**
 * Structure for Cooee result query by upper.
 */
typedef struct
{
    uint state;             /* runtime state */
    cooee_result_t result;  /* cooee result */
} easy_setup_result_t;

static int g_ioc_fd = -1; /* socket fd */
static char g_key_string[COOEE_KEY_STRING_LEN] = "";
static easy_setup_result_t g_info = {
    .state = 0,
};

int easy_setup_set_decrypt_key(char* key) {
    if (!key) {
        LOGE("null key passed in.\n");
        return -1;
    }

    strncpy(g_key_string, key, COOEE_KEY_STRING_LEN);

    return 0;
}

int easy_setup_start() {
    struct ifreq ifr;
    wl_ioctl_t ioc;
    int ret = 0;

    /* open socket to kernel */
    if ((g_ioc_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOGE("create control socket failed: %d(%s)\n", errno, strerror(errno));
        return -1;
    }

    cooee_params_t param;
    memset(&param, 0, sizeof(param));
    param.es_params.enable = WLC_EASY_SETUP_START;
    param.es_params.protocol = EASY_SETUP_PROTO_COOEE;
    if (strlen(g_key_string)) {
        memcpy(param.key_string, g_key_string, COOEE_KEY_STRING_LEN);
    }

    void* ptr = &param;
    ioc.cmd = WLC_SET_EASY_SETUP;
    ioc.buf = ptr;
    ioc.len = sizeof(param);
    ioc.set = 1;

    strncpy(ifr.ifr_name, WLAN_IFACE, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = 0;
    ifr.ifr_data = (caddr_t) &ioc;

    if ((ret = ioctl(g_ioc_fd, SIOCDEVPRIVATE, &ifr)) < 0) {
        LOGE("easy setup start failed: %d(%s)\n",
                errno, strerror(errno));
    }

    return ret;
}

int easy_setup_stop() {
    struct ifreq ifr;
    wl_ioctl_t ioc;
    int ret = 0;

    if (g_ioc_fd < 0) {
        LOGE("easy setup query: control socket not initialized.\n");
        return -1;
    }

    cooee_params_t param;
    memset(&param, 0, sizeof(param));
    param.es_params.enable = WLC_EASY_SETUP_STOP;
    param.es_params.protocol = EASY_SETUP_PROTO_COOEE;

    void* ptr = &param;
    ioc.cmd = WLC_SET_EASY_SETUP;
    ioc.buf = ptr;
    ioc.len = sizeof(param);
    ioc.set = 1;

    strncpy(ifr.ifr_name, WLAN_IFACE, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = 0;
    ifr.ifr_data = (caddr_t) &ioc;

    if ((ret = ioctl(g_ioc_fd, SIOCDEVPRIVATE, &ifr)) < 0) {
        LOGE("easy setup stop failed: %d(%s)\n",
                errno, strerror(errno));
    }

    close(g_ioc_fd);

    return ret;
}

int easy_setup_query() {
    struct ifreq ifr;
    wl_ioctl_t ioc;
    int ret = 0;

    if (g_ioc_fd < 0) {
        LOGE("easy setup query: control socket not initialized.\n");
        return -1;
    }

    memset(&g_info, 0, sizeof(g_info));

    /* TEST CODE do it */
    ioc.cmd = WLC_GET_EASY_SETUP;
    ioc.buf = &g_info;
    ioc.len = sizeof(g_info);
    ioc.set = 0;

    strncpy(ifr.ifr_name, WLAN_IFACE, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = 0;
    ifr.ifr_data = (caddr_t) &ioc;

    uint last_state = 0;
    int tries = QUERY_TIMEOUT_MS/QUERY_INTERVAL_MS;
    while (tries--) {
        ret = ioctl(g_ioc_fd, SIOCDEVPRIVATE, &ifr);
        if (ret < 0) {
            LOGE("easy setup query failed: %d(%s)\n", errno, strerror(errno));
            return -1;
        } else {
//            int i;

            if (last_state != g_info.state) {
                LOGD("state: %d --> %d\n", last_state, g_info.state);
                last_state = g_info.state;
                if (last_state == EASY_SETUP_STATE_DONE) {
                    break;
                }
            }
        }

        usleep(QUERY_INTERVAL_MS * 1000);
    }

    if (tries <= 0) {
        LOGE("easy setup query timed out.\n");
        return -1;
    }

    return 0;
}

int easy_setup_ioctl(int cmd, int set, void* param, int size) {
    struct ifreq ifr;
    wl_ioctl_t ioc;
    int ret = 0;

    if (g_ioc_fd < 0) {
        LOGE("easy setup query: control socket not initialized.\n");
        return -1;
    }

    void* ptr = param;
    ioc.cmd = cmd;
    ioc.buf = ptr;
    ioc.len = size;
    ioc.set = set;

    strncpy(ifr.ifr_name, WLAN_IFACE, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ-1] = 0;
    ifr.ifr_data = (caddr_t) &ioc;

    if ((ret = ioctl(g_ioc_fd, SIOCDEVPRIVATE, &ifr)) < 0) {
        LOGD("easy setup ioctl failed: %d(%s)\n",
                errno, strerror(errno));
        return -1;
    }

    return 0;
}

int easy_setup_get_ssid(char buff[], int buff_len) {
    int i;
    cooee_result_t* c = &g_info.result;

    if (g_info.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    if (buff_len < c->ap_ssid.len+1) {
        LOGE("insufficient buffer provided: %d < %d\n", buff_len, c->ap_ssid.len+1);
        return -1;
    }

    for (i=0; i<c->ap_ssid.len; i++) {
        buff[i] = c->ap_ssid.val[i];
    }
    buff[i] = 0;

    return 0;
}

int easy_setup_get_psk(char buff[], int buff_len) {
    int i;
    cooee_result_t* c = &g_info.result;

    if (g_info.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    if (buff_len < c->security_key_length+1) {
        LOGE("insufficient buffer provided: %d < %d\n", buff_len, c->security_key_length+1);
        return -1;
    }

    for (i=0; i<c->security_key_length; i++) {
        buff[i] = c->security_key[i];
    }
    buff[i] = 0;

    return 0;
}

int easy_setup_get_sender_ip(char buff[], int buff_len) {
    char ip_text[16];
    cooee_result_t* c = &g_info.result;

    if (g_info.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    if (c->host_ip_address.version != COOEE_IPV4) {
        return -1;
    }

    int ip = c->host_ip_address.ip.v4;
/*    int ret = */snprintf(ip_text, sizeof(ip_text), "%d.%d.%d.%d",
            (ip>>24)&0xff,
            (ip>>16)&0xff,
            (ip>>8)&0xff,
            (ip>>0)&0xff);
    ip_text[16-1] = 0;

    if ((size_t) buff_len < strlen(ip_text)+1) {
        LOGE("insufficient buffer provided: %d < %d\n", buff_len, strlen(ip_text)+1);
        return -1;
    }

    strcpy(buff, ip_text);

    return 0;
}

int easy_setup_get_security() {
    cooee_result_t* c = &g_info.result;

    if (g_info.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }
    printf("ap_channel: %d\n", c->ap_channel);

    return scan_and_get_security(c->ap_channel, c->ap_bssid.octet);
}

