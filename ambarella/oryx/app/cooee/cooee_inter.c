#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "cooee.h"

int debug_enable = 0;
char filename[256] = {0};
/*
int main(int argc, char* argv[])
{
    int ret;
    int len;

    for (;;) {
        int c = getopt(argc, argv, "dk:f:");
        if (c < 0) break;

        switch (c) {
            case 'd':
                debug_enable = 1;
                break;
            case 'k':
                easy_setup_set_decrypt_key(optarg);
                break;
            case 'f':
        strncpy(filename, optarg, sizeof(filename));
              break;
            default:
                return 0;
        }
    }
*/

int  cooee_interface(char *file)
{
    int ret;
    int len;
    if(file == NULL)
    {
        printf("####Error!######## cooee_interface parameter is NULL #################### \n");
        return -1;
    }
    strcpy(filename,file);



    /* set decrypt key if you have set it in sender */
    /*
    ret = easy_setup_set_decrypt_key("today is friday?");
    if (ret) return ret; */

    ret = easy_setup_start();
    if (ret) return ret;

    /* query for result, blocks until cooee comes or times out */
    ret = easy_setup_query();
    if (!ret) {
        char ssid[33]; /* ssid of 32-char length, plus trailing '\0' */
        ret = easy_setup_get_ssid(ssid, sizeof(ssid));
        if (!ret) {
            printf("ssid: %s\n", ssid);
        }

        char psk[65]; /* psk is 64-char length, plus trailing '\0' */
        ret = easy_setup_get_psk(psk, sizeof(psk));
        if (!ret) {
            printf("password: %s\n", psk);
        }

        char ip[16]; /* ipv4 max length */
        ret = easy_setup_get_sender_ip(ip, sizeof(ip));
        if (!ret) {
            printf("sender ip: %s\n", ip);
        }

        ret = easy_setup_get_security();
        printf("security: ");
        if (ret == WLAN_SECURITY_WPA2) printf("wpa2\n");
        if (ret == WLAN_SECURITY_WPA) printf("wpa\n");
        if (ret == WLAN_SECURITY_WEP) printf("wep\n");
        if (ret == WLAN_SECURITY_NONE) printf("none\n");

    if ((filename[0] != '\0' ) && (ssid[0] != '\0') && (psk[0] != '\0')) {
        int fd_conf = -1;
        int size = 0;
        char buf[1024];
        do {
            fd_conf = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0666);
            if (fd_conf < 0) {
                perror("open");
                break;
            }
            snprintf(buf, sizeof(buf), "ssid=%s\npassword=%s\n", ssid, psk);
            len = strlen(buf);
            size = write(fd_conf, buf, len);
            if (size != len) {
                printf("Write error\n");
            }
            close(fd_conf);
            fd_conf= -1;
            printf("Save config [%s] done\n", filename);
        } while (0);
    }
    }

    if(ret == -1)
    {
        return -1;
    }
    easy_setup_stop();

    return 0;
}

