//------------------------------------------------------------------------------
// <copyright file="diagwind.c" company="Atheros">
//    Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
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
