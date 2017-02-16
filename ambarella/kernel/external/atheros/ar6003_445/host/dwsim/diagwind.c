//------------------------------------------------------------------------------
// <copyright file="diagwind.c" company="Atheros">
//    Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

/*
 * This file provides Diagnostic Window access to AR6K SoC.
 * It can be built as a standalone application that accepts
 * read/write commands from stdin. Alternatively, it can be
 * linked into another application such as dwsim which uses
 * the simple "diag_" API to service Diagnostic Window accesses.
 *
 * This software relies on special support in the Host driver
 * for ioctls AR6000_XIOCTL_DIAG_READ and AR6000_XIOCTL_DIAG_WRITE.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include <a_config.h>
#include <a_osapi.h>
#include <athdefs.h>
#include <a_types.h>
#include "bmi_msg.h"

char op;
int addr;
int value;
char ifname[IFNAMSIZ];
int devsock;
struct ifreq ifr;

void
diag_init(void)
{
    unsigned char *buffer;

    memset(ifname, '\0', IFNAMSIZ);
    strcpy(ifname, "eth1");
    devsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (devsock < 0)
        err(1, "socket");
    /* Allow override of ifname here */
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    /* Verify that the Target is alive.  If not, wait for it. */
    {
        int rv;
        static int waiting_msg_printed = 0;

        buffer = (unsigned char *)malloc(4);
        ((int *)buffer)[0] = AR6000_XIOCTL_CHECK_TARGET_READY;
        ifr.ifr_data = buffer;
        while ((rv=ioctl(devsock, AR6000_IOCTL_EXTENDED, &ifr)) < 0)
        {
            if (errno == ENODEV) {
                /*
                 * Give the Target device a chance to start.
                 * Then loop back and see if it's alive.
                 */
                if (!waiting_msg_printed) {
                    printf("Waiting for AR6K target....\n");
                    waiting_msg_printed = 1;
                }
                sleep(1);
            } else {
                break; /* Some unexpected error */
            }
        }
        free(buffer);
    }
}

typedef struct {
    int xioctl;
    struct ar6000_diag_window_cmd_s diagcmd;
} cmd_t;

void
diag_read(unsigned int addr, unsigned int *value)
{
    cmd_t cmd;
    unsigned int initial_value = 0xdeadbeef;

    cmd.xioctl = AR6000_XIOCTL_DIAG_READ;
    cmd.diagcmd.addr = addr;

    for (;;) {
        cmd.diagcmd.value = initial_value;
        ifr.ifr_data = (unsigned char *)&cmd;
        if (ioctl(devsock, AR6000_IOCTL_EXTENDED, &ifr) < 0) {
            err(1, ifr.ifr_name);
        }

        if (cmd.diagcmd.value != initial_value) {
            break;
        }
        printf("DIAG_READ FAILURE....RETRY addr=0x%x\n", addr);
        fflush(stdout);
        initial_value++;
    }

    *value = cmd.diagcmd.value;
}

void
diag_write(unsigned int addr, unsigned int value)
{
    cmd_t cmd;

    cmd.xioctl = AR6000_XIOCTL_DIAG_WRITE;
    cmd.diagcmd.addr = addr;
    cmd.diagcmd.value = value;

    ifr.ifr_data = (unsigned char *)&cmd;
    if (ioctl(devsock, AR6000_IOCTL_EXTENDED, &ifr) < 0) {
        err(1, ifr.ifr_name);
    }
}

int
diagwind_main(int argc, char **argv)
{
    int rv;

    diag_init();

    for(;;) {
        rv = scanf(" %c", &op);
        if (rv == EOF) {
                break;
        }

        rv = scanf(" %x", &addr);
        if (rv == EOF) {
                break;
        }

        if (op == 'w') {
                rv = scanf(" %x", &value);
                if (rv == EOF) {
                        break;
                }
        }

        if (op == 'r') {
                diag_read(addr, &value);
                printf("Read 0x%x from 0x%x\n", value, addr);
        } else if (op == 'w') {
                printf("Write 0x%x to 0x%x\n", value, addr);
                diag_write(addr, value);
        } else {
                printf("Unknown operation: 0x%x\n", op);
        }
    }

    return 0;
}
