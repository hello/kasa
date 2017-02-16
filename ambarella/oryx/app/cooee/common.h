#ifndef __EASY_SETUP_COMMON_H__
#define __EASY_SETUP_COMMON_H__

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned int

#define int8 char
#define int16 short
#define int32 int

enum {
    WLAN_SECURITY_NONE = 0,
    WLAN_SECURITY_WEP,
    WLAN_SECURITY_WPA,
    WLAN_SECURITY_WPA2,
};

/* direct log message to your system utils, default printf */
extern int debug_enable;
#define LOGE(fmt, arg...) do {if (1 | debug_enable) printf(fmt, ##arg);} while (0);
#define LOGD(fmt, arg...) do {if (debug_enable) printf(fmt, ##arg);} while (0);

#endif /* __EASY_SETUP_COMMON_H__ */
