/*******************************************************************************
 * NandWrite.cpp
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

#include "NandWrite.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NandWrite::NandWrite()
  : mFlpartTable (0)
{
}

NandWrite::~NandWrite()
{
  if (mFlpartTable) {
    free (mFlpartTable);
  }
}

bool NandWrite::changeMacAddr(const char *mac)
{
  if ((12 == strlen(mac)) || (17 == strlen(mac))) {
    uint8_t hwaddr[6] = {0};
    if (stringToHwAddr(mac, hwaddr) && getPartTable()) {
      memcpy (mFlpartTable->dev.eth[0].mac, hwaddr, 6);
#ifdef DEBUG
      mFlpartTable->display ();
#endif
      return setPartTable(mFlpartTable);
    }
  }
  return false;
}

bool NandWrite::setNetbootParameters(uint32_t ip,
                                     uint32_t mask,
                                     uint32_t gw,
                                     uint32_t tftpd,
                                     const char *mac,
                                     const char *file,
                                     uint32_t priAddr)
{
  uint8_t hwaddr[6] = {0};
  if (((12 == strlen(mac) || (17 == strlen(mac)))) &&
      stringToHwAddr(mac, hwaddr) &&
      getPartTable()) {
    memcpy(mFlpartTable->dev.eth[0].mac, hwaddr, 6);
    mFlpartTable->dev.eth[0].ip   = ip;
    mFlpartTable->dev.eth[0].mask = mask;
    mFlpartTable->dev.eth[0].gw   = gw;
    mFlpartTable->dev.tftpd    = tftpd;
    mFlpartTable->dev.pri_addr = priAddr;
    memset (mFlpartTable->dev.pri_file,
            0,
            sizeof(mFlpartTable->dev.pri_file));
    memcpy (mFlpartTable->dev.pri_file, file, strlen(file));
    /* Set Board enter AMBoot */
    mFlpartTable->dev.auto_boot = 0;
    /* Set Board to boot from network */
    mFlpartTable->dev.auto_dl = 1;
    return setPartTable(mFlpartTable);
  }
  return false;
}

bool NandWrite::setNetbootParameters(const char *ip,
                                     const char *mask,
                                     uint32_t gw,
                                     uint32_t tftpd,
                                     const char *mac,
                                     const char *file,
                                     uint32_t priAddr)
{
  if (ip    &&
      mask  &&
      file  &&
      (strlen(file) <= 32)) {
    uint32_t ipaddr  = 0;
    uint32_t netmask = 0;
    if (stringToIPAddr(ip, &ipaddr)    &&
        stringToIPAddr(mask, &netmask)) {
      return setNetbootParameters (ipaddr,
                                   netmask,
                                   gw,
                                   tftpd,
                                   mac,
                                   file,
                                   priAddr);
    }
  }
  return false;
}

bool NandWrite::setNetbootParameters(const char *ip,
                                     const char *mask,
                                     const char *gw,
                                     const char *tftpd,
                                     const char *mac,
                                     const char *file,
                                     uint32_t priAddr)
{
  if (ip    &&
      mask  &&
      gw    &&
      tftpd &&
      file  &&
      (strlen(file) <= 32)) {
    uint32_t ipaddr  = 0;
    uint32_t netmask = 0;
    uint32_t gateway = 0;
    uint32_t tftpip  = 0;
    if (stringToIPAddr(ip, &ipaddr)    &&
        stringToIPAddr(mask, &netmask) &&
        stringToIPAddr(gw, &gateway)   &&
        stringToIPAddr(tftpd, &tftpip)) {
      return setNetbootParameters (ipaddr,
                                   netmask,
                                   gateway,
                                   tftpip,
                                   mac,
                                   file,
                                   priAddr);
    }
  }
  return false;
}

bool NandWrite::getPartTable()
{
  int ptbFd = -1;
  uint32_t ptb_offset = 0;
  loff_t ptb_bad_offset = 0;
  mtd_info_t ptb_meminfo;

  memset (&ptb_meminfo, 0, sizeof(mtd_info_t));
  if (mFlpartTable) {
    delete mFlpartTable;
    mFlpartTable = 0;
  }

  if (-1 == (ptbFd = open("/dev/mtd1", O_RDONLY))) {
    return false;
  }

  if (ioctl(ptbFd, MEMGETINFO, &ptb_meminfo) != 0) {
    perror ("PTB MEMGETINFO");
    close (ptbFd);
    return false;
  }

  for (ptb_offset = 0;
       ptb_offset < ptb_meminfo.size;
       ptb_offset += ptb_meminfo.erasesize) {
    int ret = -1;
    ptb_bad_offset = ptb_offset;
    if ((ret = ioctl(ptbFd, MEMGETBADBLOCK, &ptb_bad_offset)) < 0) {
      perror ("MEMGETBADBLOCK");
      close (ptbFd);
      return false;
    }
    if (ret == 0) {
      break;
    }
#ifdef DEBUG
    fprintf (stderr, "Bad block at %x, from %x will be skipped\n",
             (int)ptb_bad_offset, ptb_offset);
#endif
  }
  if (ptb_offset >= ptb_meminfo.size) {
    fprintf (stderr, "Can't find good block in PTB.\n");
    close (ptbFd);
    return false;
  }
  mFlpartTable = (struct flpart_table *)malloc (ptb_meminfo.erasesize);
  memset (mFlpartTable, 0, ptb_meminfo.erasesize);
  if (pread(ptbFd, (void *)mFlpartTable, ptb_meminfo.erasesize, ptb_offset) !=
      ((ssize_t)ptb_meminfo.erasesize)) {
    perror ("pread PTB");
    close (ptbFd);
    return false;
  }
#ifdef DEBUG
  mFlpartTable->display ();
#endif

  close (ptbFd);

  return true;
}

bool NandWrite::setPartTable (flpart_table * table)
{
  int ptbFd = -1;
  uint32_t ptb_offset = 0;
  loff_t ptb_bad_offset = 0;
  mtd_info_t ptb_meminfo;

  if (!table) {
    return false;
  }
  memset (&ptb_meminfo, 0, sizeof (mtd_info_t));

  if (-1 == (ptbFd = open("/dev/mtd1", O_RDWR))) {
    perror ("Open PTB");
    return false;
  }
  if (0 != ioctl(ptbFd, MEMGETINFO, &ptb_meminfo)) {
    perror ("PTB MEMGETINFO");
    close (ptbFd);
    return false;
  }
  if (ptb_meminfo.erasesize < PTB_SIZE) {
    fprintf (stderr, "PTB can't fit into erasesize.\n");
    close (ptbFd);
    return false;
  }

  for (ptb_offset = 0;
       ptb_offset < ptb_meminfo.size;
       ptb_offset += ptb_meminfo.erasesize) {
    int ret = -1;
    ptb_bad_offset = ptb_offset;
    if ((ret = ioctl(ptbFd, MEMGETBADBLOCK, &ptb_bad_offset)) < 0) {
      perror ("MEMGETBADBLOCK");
      close (ptbFd);
      return false;
    }

    if (ret == 0) {
      erase_info_t erase;
      erase.start = ptb_offset;
      erase.length = ptb_meminfo.erasesize;
      if ((ret = ioctl(ptbFd, MEMERASE, &erase)) != 0) {
        perror ("PTB MEMERASE");
        continue;
      }
      break;
    }
  }

  if (ptb_offset >= ptb_meminfo.size) {
    fprintf(stderr, "Can't find good block in PTB.\n");
    close (ptbFd);
    return false;
  }

  if (table->dev.magic != FLPART_MAGIC) {
    fprintf (stderr, "Invalid dev magic: 0x%08x|0x%08x\n",
             table->dev.magic,
             FLPART_MAGIC);
    close (ptbFd);
    return false;
  }

  for (int count = 0; count < PART_MAX_WITH_RSV; ++ count) {
    if (table->part[count].img_len != 0 &&
        table->part[count].magic != FLPART_MAGIC) {
      fprintf (stderr, "Invalid partition table magic(%d): 0x%08x|0x%08x\n",
               count,
               table->part[count].magic,
               FLPART_MAGIC);
      close (ptbFd);
      return false;
    }
  }
  if (pwrite (ptbFd, (void *)table, ptb_meminfo.erasesize, ptb_offset) !=
      (ssize_t)ptb_meminfo.erasesize) {
    perror ("pwrite PTB");
    close (ptbFd);
    return false;
  }

  close (ptbFd);

  return true;
}

bool NandWrite::stringToHwAddr (const char *str, uint8_t *mac)
{
  /* This method doesn't check if the string is a valid MAC address
   * and doesn't check if length of mac is >= 6
   */
  if ((12 == strlen(str)) || (17 == strlen(str))) {
    const char * num = str;
    for (int i = 0; i < 6; ++ i) {
      int part = 0;
      uint8_t val = 0;
      while ((*num == ':') || (*num == '-')) {
        ++ num;
        if ((size_t)(num - str) >= strlen(str)) {
          return false;
        }
      }
      while (part ++ < 2) {
        uint8_t hex = 0;
        if ((*num <= '9') && (*num >= '0')) {
          hex = *num - '0';
        } else if ((((*num | 0x20) - 'a') <= 5)) {
          hex = (*num | 0x20) - 'a' + 10;
        } else {
          return false;
        }
        ++ num;
        val <<= 4;
        val |= hex;
      }
      mac[i] = val;
    }
    return true;
  }
  return false;
}

bool NandWrite::stringToIPAddr (const char *str, uint32_t *ip)
{
  /* Only IPv4 Address is supported */
  if (strlen(str) <= 15) {
    char ch;
    int octets = 0;
    unsigned char *tmp = (unsigned char *)ip;
    bool isDigital = false;
    *ip = 0x00000000;
    for (int i = 0; i < 4; ++ i) {
      isDigital = false;
      while ((ch = *str++) != '\0') {
        if (ch >= '0' && ch <= '9') {
          unsigned int num = tmp[i] * 10 + (ch - '0');
          if (num > 255) {
            return false;
          }
          tmp [i] = (unsigned char)num;
          if (false == isDigital) {
            ++ octets;
            isDigital = true;
          }
        } else if (ch == '.') {
          if (i > 2) {
            return false;
          }
          if (!isDigital) {
            return false;
          }
          break;
        } else {
          return false;
        }
      }
    }
    if (octets == 4) {
      return true;
    }
  }
  return false;
}
