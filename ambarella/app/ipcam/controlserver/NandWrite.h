/*******************************************************************************
 * NandWrite.h
 *
 * Histroy:
 *  2011-5-9 2011 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef NANDWRITE_H
#define NANDWRITE_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mtd/mtd-user.h>

#define FLPART_MAGIC 0x8732dfe6

struct flpart {
    uint32_t crc32;    /* CRC32 checksum of image*/
    uint32_t ver_num;  /* Version number*/
    uint32_t ver_date; /* Version date*/
    uint32_t img_len;  /* Length of image in the partition*/
    uint32_t mem_addr; /* Starting address to copy to RAM*/
    uint32_t flag;     /* Special properties of this partition*/
    uint32_t magic;    /* Magic number*/
    void display()
    {
      printf ("CRC32    :0x%08x\n", crc32);
      printf ("ver_num  :0x%08x\n", ver_num);
      printf ("ver_date :0x%08x\n", ver_date);
      printf ("img_len  :0x%08x\n", img_len);
      printf ("mem_addr :0x%08x\n", mem_addr);
      printf ("flag     :0x%08x\n", flag);
      printf ("magic    :0x%08x\n", magic);
    }
}__attribute__((packed));

struct netdev {
    /*This section contains networking related settings*/
    uint8_t  mac[6];  /* Ethernet MAC */
    uint32_t ip;      /* Boot loader's LAN IP */
    uint32_t mask;    /* Boot loader's LAN mask */
    uint32_t gw;      /* Boot loader's Lan gateway */
    void display()
    {
      printf ("MAC             :%02x:%02x:%02x:%02x:%02x:%02x\n",
              mac[0],
              mac[1],
              mac[2],
              mac[3],
              mac[4],
              mac[5]);
      printf ("Lan IP          :%d.%d.%d.%d\n",
              ip&0xff,
              (ip>>8)&0xff,
              (ip>>16)&0xff,
              (ip>>24)&0xff);
      printf ("Lan Mask        :%d.%d.%d.%d\n",
              mask&0xff,
              (mask>>8)&0xff,
              (mask>>16)&0xff,
              (mask>>24)&0xff);
      printf ("Lan Gateway     :%d.%d.%d.%d\n",
              gw&0xff,
              (gw>>8)&0xff,
              (gw>>16)&0xff,
              (gw>>24)&0xff);
    }
}__attribute__((packed));

#define FLDEV_CMD_LINE_SIZE 1024
struct fldev {
    char     sn[32];                       /* Serial number */
    uint8_t  usbdl_mode;                   /* USB download mode */
    uint8_t  auto_boot;                    /* Automatic boot */
    char     cmdline[FLDEV_CMD_LINE_SIZE]; /* Boot command line options */
    uint8_t  rsv[2];
    uint32_t splash_id;
    /* This section contains networking related settings */
    netdev   eth[2];
    netdev   wifi[2];
    netdev   usb_eth[2];
    /* This section contains settings which can be updated through network */
    uint8_t  auto_dl;      /* Automatic download */
    uint32_t tftpd;        /* Boot loader's TFTP server */
    uint32_t pri_addr;     /* RTOS download address */
    char     pri_file[32]; /* RTOS file name */
    uint8_t  pri_comp;     /* RTOS compressed? */
    uint32_t rmd_addr;     /* Ramdisk download address */
    char     rmd_file[32]; /* Ramdisk file name */
    uint8_t  rmd_comp;     /* Ramdisk compressed? */
    uint32_t dsp_addr;     /* DSP download address */
    char     dsp_file[32]; /* DSP file name */
    uint8_t  dsp_comp;     /* DSP compressed? */
    uint8_t  rsv2[2];
    uint32_t magic;        /* Magic number */
    void display ()
    {
      printf ("sn              :%s\n", sn);
      printf ("magic number    :0x%08x\n", magic);
      printf ("usbdl_mode      :0x%08x\n", usbdl_mode);
      printf ("auto_boot       :0x%08x\n", auto_boot);
      printf ("cmdline         :%s\n", cmdline);
      printf ("splash_id       :0x%08x\n", splash_id);
      for (uint32_t i = 0; i < (sizeof(eth)/sizeof(netdev)); ++ i) {
        printf("Network Device eth%u's parameters\n", i);
        eth[i].display();
      }
      for (uint32_t i = 0; i < (sizeof(wifi)/sizeof(netdev)); ++ i) {
        printf("Wireless Network Device wifi%u's parameters\n", i);
        wifi[i].display();
      }
      for (uint32_t i = 0; i < (sizeof(usb_eth)/sizeof(netdev)); ++ i) {
        printf("USB Network Device usb_eth%u's parameters\n", i);
        usb_eth[i].display();
      }
      printf ("force_ptb       :0x%08x\n", rsv[0]);
      printf ("Pri Addr        :0x%08x\n", pri_addr);
      printf ("Pri File        :%s\n", pri_file);
      printf ("Tftp Server     :%d.%d.%d.%d\n",
              tftpd&0xff,
              (tftpd>>8)&0xff,
              (tftpd>>16)&0xff,
              (tftpd>>24)&0xff);
    }
}__attribute__((packed));

#define PART_MAX_WITH_RSV 32
#define PTB_SIZE          4096

#define PTB_PAD_SIZE      \
  (PTB_SIZE - PART_MAX_WITH_RSV * sizeof(struct flpart) - sizeof(struct fldev))

struct flpart_table {
    flpart  part[PART_MAX_WITH_RSV]; /*Partitions*/
    fldev   dev;                     /*Device properties*/
//    uint8_t rsv[];
    uint8_t rsv[PTB_PAD_SIZE];

    void clear () {
      memset (part, 0, sizeof(flpart) * PART_MAX_WITH_RSV);
      memset (&dev, 0, sizeof(fldev));
    }
    void display ()
    {
      const char *partName[] =
      {
       "bst", "ptb", "bld", "hal", "pba", "pri", "sec",
       "bak", "rmd", "rom", "dsp", "lnx", "swp", "add",
       "adc", "raw", "stg2", "stg", "prf", "cal","all"
      };

      for (int i = 0; i < PART_MAX_WITH_RSV; ++ i) {
        if (part[i].img_len != 0) {
          printf ("Part %d: %s\n", i, partName[i]);
          part[i].display ();
          printf ("\n");
        }
      }
      dev.display();
      printf ("\n");
    }
}__attribute__((packed));

class NandWrite
{
  public:
    NandWrite();
    virtual ~NandWrite();
    bool changeMacAddr(const char *mac);
    bool setNetbootParameters(const char *ip,
                              const char *mask,
                              const char *gw,
                              const char *tftpd,
                              const char *mac,
                              const char *file,
                              uint32_t priAddr);
    bool setNetbootParameters(const char *ip,
                              const char *mask,
                              uint32_t gw,
                              uint32_t tftpd,
                              const char *mac,
                              const char *file,
                              uint32_t priAddr);
    bool setNetbootParameters(uint32_t ip,
                              uint32_t mask,
                              uint32_t gw,
                              uint32_t tftpd,
                              const char *mac,
                              const char *file,
                              uint32_t priAddr);
  private:
    bool getPartTable ();
    bool setPartTable (flpart_table *table);
    bool stringToHwAddr (const char *str, uint8_t *mac);
    bool stringToIPAddr (const char *str, uint32_t *ip);
  private:
    flpart_table *mFlpartTable;
};

#endif /* NANDWRITE_H_ */
