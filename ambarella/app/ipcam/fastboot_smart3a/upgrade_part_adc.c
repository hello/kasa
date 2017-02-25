/**
 * upgrade_partition.c
 *
 * History:
 *    2015/03/30 - [jbxing] created file which is based on Jian he's prototype
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 */


/*========================== Header Files ====================================*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <basetypes.h>
#include <mtd/mtd-user.h>
//#include <linux/jffs2.h>

#include "upgrade_part_adc.h"

/*===========================================================================*/
#define MAX_PAGE_SIZE    2048
#define MAX_OOB_SIZE    64

//#define FLPART_MAGIC    0x8732dfe6
//#define __ARMCC_PACK__
//#define __ATTRIB_PACK__  __attribute__ ((packed))
//#define ETH_INSTANCES        2
//#define USE_WIFI        1
//#define CMD_LINE_SIZE        512
//#define MAC_SIZE        6
//#define SN_SIZE            32

//essential
/* mtd struct for image */
typedef struct nand_update_file_header_s {
    unsigned char    magic_number[8];           /*   AMBUPGD'\0'     8 chars including '\0' */
    unsigned short    header_ver_major;
    unsigned short    header_ver_minor;
    unsigned int    header_size;               /* payload starts at header_start_address + header_size */
    unsigned int  payload_type;                /* NAND_UPGRADE_FILE_TYPE_xxx */
    unsigned char    payload_description[256];  /* payload description string, end with '\0' */
    unsigned int  payload_size;                /* payload of upgrade file, after header */
    unsigned int  payload_crc32;               /* payload crc32 checksum, crc calculation from
                                                  header_start_address  + header_size,
                                                  crc calculation size is payload_size */
}nand_update_file_header_t;
//essential
typedef struct nand_update_global_s {
    /* Buffer array used for writing data */
    unsigned char writebuf[MAX_PAGE_SIZE];
    unsigned char oobbuf[MAX_OOB_SIZE];
    unsigned char oobreadbuf[MAX_OOB_SIZE];
    /* oob layouts to pass into the kernel as default */
    struct nand_oobinfo none_oobinfo;
    struct nand_oobinfo jffs2_oobinfo;
    struct nand_oobinfo yaffs_oobinfo;
    struct nand_oobinfo autoplace_oobinfo;
} nand_update_global_t;
//#define PROGRAM "upgrade_partition"
//#define VERSION "Revision: 1.310.04 "
//#define AMB_VERSION "Ambarella: 0-20150121 "
//#define MAX_LEVEL    32
//#define NANDWRITE_OPTIONS_BASE        0
//#define NETWORK_OPTION_BASE        20



const uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



static inline uint32_t crc32(uint32_t val, const void *ss, int len)
{
    const unsigned char *s = ss;
    while (--len >= 0)
        val = crc32_tab[(val ^ *s++) & 0xff] ^ (val >> 8);
    return val;
}

static void init_nand_update_global(nand_update_global_t *pG)
{
    pG->none_oobinfo.useecc = MTD_NANDECC_OFF;
    pG->autoplace_oobinfo.useecc = MTD_NANDECC_AUTOPLACE;

    pG->jffs2_oobinfo.useecc = MTD_NANDECC_PLACE;
    pG->jffs2_oobinfo.eccbytes = 6;
    pG->jffs2_oobinfo.eccpos[0] = 0;
    pG->jffs2_oobinfo.eccpos[1] = 1;
    pG->jffs2_oobinfo.eccpos[2] = 2;
    pG->jffs2_oobinfo.eccpos[3] = 3;
    pG->jffs2_oobinfo.eccpos[4] = 6;
    pG->jffs2_oobinfo.eccpos[5] = 7;

    pG->yaffs_oobinfo.useecc = MTD_NANDECC_PLACE;
    pG->yaffs_oobinfo.eccbytes = 6;
    pG->yaffs_oobinfo.eccpos[0] = 8;
    pG->yaffs_oobinfo.eccpos[1] = 9;
    pG->yaffs_oobinfo.eccpos[2] = 10;
    pG->yaffs_oobinfo.eccpos[3] = 13;
    pG->yaffs_oobinfo.eccpos[4] = 14;
    pG->yaffs_oobinfo.eccpos[5] = 15;

    memset(pG->oobbuf, 0xff, sizeof(pG->oobbuf));
}

int upgrade_partition(int index, const char* img)
{
    char     *mtd_device ;
    int      mtdoffset = 0;
    int      quiet = 0;
    int      writeoob = 0;
    int      skip_head = 0;
    int      forcejffs2 = 0;
    int      forceyaffs = 0;
    int      pad = 0;
    nand_update_global_t *ptr_update=NULL;
    int      blockalign = 1; /*default to using 16K block size */ //essential

    int cnt, fd, ifd, pagelen, baderaseblock, blockstart = -1;
    int ret, readlen, oobinfochanged = 0;
    int image_crc = ~0U, image_length = 0, imglen = 0;
    struct nand_oobinfo old_oobinfo;
    struct mtd_info_user meminfo;
    struct mtd_oob_buf oob;
    loff_t offs;
    unsigned char readbuf[MAX_PAGE_SIZE];
    int file_offset = mtdoffset;
    int buf_num = 0;
    nand_update_file_header_t image_head;
    char device_name_convert[20]={0};

    sprintf(device_name_convert,"/dev/mtd%d",index);
    mtd_device   =device_name_convert;

    ptr_update = malloc(sizeof(nand_update_global_t));
    if (!ptr_update) {
        perror("can not malloc buffer for update global.\n");
        exit(1);
    }
    init_nand_update_global(ptr_update);

    if (pad && writeoob) {
        fprintf(stderr, "Can't pad when oob data is present.\n");
        exit(1);
    }
    //update_partition_with_img();
    // Open the device
    if ((fd = open(mtd_device, O_RDWR)) == -1) {
        perror("open flash");
        exit(1);
    }

    // Fill in MTD device capability structure
    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
        perror("MEMGETINFO");
        close(fd);
        exit(1);
    }

    // Set erasesize to specified number of blocks - to match jffs2
    // (virtual) block size
    meminfo.erasesize *= blockalign;

    // Make sure device page sizes are valid
    if (!(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
        !(meminfo.oobsize == 8 && meminfo.writesize == 256) &&
        !(meminfo.oobsize == 64 && meminfo.writesize == 2048)) {
        fprintf(stderr, "Unknown flash (not normal NAND)\n");
        close(fd);
        exit(1);
    }

    /*
    * force oob layout for jffs2 or yaffs ?
    * Legacy support
    */
    if (forcejffs2 || forceyaffs) {
        if (meminfo.oobsize == 8) {
            if (forceyaffs) {
                fprintf (stderr, "YAFSS cannot operate on 256 Byte page size");
                goto restoreoob;
            }
            // Adjust number of ecc bytes
            ptr_update->jffs2_oobinfo.eccbytes = 3;
        }
    }

    oob.length = meminfo.oobsize;
    oob.ptr = ptr_update->oobbuf;

    // Open the input file
    if ((ifd = open(img, O_RDONLY)) == -1) {
         perror("open input file");
         goto restoreoob;
    }

    // get image length
    imglen = lseek(ifd, 0, SEEK_END);
    lseek (ifd, 0, SEEK_SET);
    if(skip_head)
    {
        if(read(ifd, &image_head, sizeof(nand_update_file_header_t))
            != sizeof(nand_update_file_header_t)) {
            perror ("File I/O error on input file");
            goto closeall;
        }
        lseek(ifd, image_head.header_size, SEEK_SET);
        imglen =image_head.payload_size;
        file_offset +=image_head.header_size;
    }
    image_length = imglen;

    pagelen = meminfo.writesize + ((writeoob == 1) ? meminfo.oobsize : 0);

    // Check, if file is pagealigned
    if ((!pad) && ((imglen % pagelen) != 0)) {
        fprintf (stderr, "Input file is not page aligned\n");
        goto closeall;
    }

    // Check, if length fits into device
    if ( ((imglen / pagelen) * meminfo.writesize) > (meminfo.size - mtdoffset)) {
        fprintf (stderr, "Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
            imglen, pagelen, meminfo.writesize, meminfo.size);
        perror ("Input file does not fit into device");
        goto closeall;
    }

    //crc32_table = crc32_filltable(NULL, 0);

    // Get data from input and write to the device
    while (imglen && (mtdoffset < meminfo.size)) {
        // new eraseblock , check for bad block(s)
        // Stay in the loop to be sure if the mtdoffset changes because
        // of a bad block, that the next block that will be written to
        // is also checked. Thus avoiding errors if the block(s) after the
        // skipped block(s) is also bad (number of blocks depending on
        // the blockalign
        while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) {
            blockstart = mtdoffset & (~meminfo.erasesize + 1);
            offs = blockstart;
            baderaseblock = 0;
            if (!quiet)
                fprintf (stdout, "Writing data to block %x\n", blockstart);

            //Check all the blocks in an erase block for bad blocks
            do {
                if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
                    perror("ioctl(MEMGETBADBLOCK)");
                    goto closeall;
                }
                if (ret == 1) {
                    baderaseblock = 1;
                    if (!quiet)
                        fprintf (stderr, "Bad block at %x, %u block(s) "
                        "from %x will be skipped\n",
                        (int) offs, blockalign, blockstart);
                }

                if (baderaseblock) {
                    mtdoffset = blockstart + meminfo.erasesize;
                }
                offs +=  meminfo.erasesize / blockalign ;
            } while ( offs < blockstart + meminfo.erasesize );

        }

        readlen = meminfo.writesize;
        if (pad && (imglen < readlen)) {
            readlen = imglen;
            memset(ptr_update->writebuf + readlen, 0xff, meminfo.writesize - readlen);
        }

        // Read Page Data from input file
        if ((cnt = pread(ifd, ptr_update->writebuf, readlen,file_offset)) != readlen) {
            if (cnt == 0)// EOF
                break;
            perror ("File I/O error on input file");
            goto closeall;
        }

        image_crc = crc32(image_crc, ptr_update->writebuf, cnt);

        if (writeoob) {
            int i, start, len, filled;
            // Read OOB data from input file, exit on failure
            if ((cnt = pread(ifd, ptr_update->oobreadbuf, meminfo.oobsize,file_offset)) != meminfo.oobsize) {
                perror ("File I/O error on input file");
                goto closeall;
            }
            /*
             *  We use autoplacement and have the oobinfo with the autoplacement
             * information from the kernel available
             *
             * Modified to support out of order oobfree segments,
             * such as the layout used by diskonchip.c
             */
            if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE)) {
                for (filled = 0, i = 0; old_oobinfo.oobfree[i][1] && (i < MTD_MAX_OOBFREE_ENTRIES); i++) {
                    /* Set the reserved bytes to 0xff */
                    start = old_oobinfo.oobfree[i][0];
                    len = old_oobinfo.oobfree[i][1];
                    memcpy(ptr_update->oobbuf + start,
                        ptr_update->oobreadbuf + filled,
                        len);
                    filled += len;
                }
            } else {
                // Set at least the ecc byte positions to 0xff
                start = old_oobinfo.eccbytes;
                len = meminfo.oobsize - start;
                memcpy(ptr_update->oobbuf + start,
                    ptr_update->oobreadbuf + start, len);
            }
            // Write OOB data first, as ecc will be placed in there
            oob.start = mtdoffset;
            if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
                perror ("ioctl(MEMWRITEOOB)");
                goto closeall;
            }
            imglen -= meminfo.oobsize;
        }

        // Write out the Page data
        if (pwrite(fd, ptr_update->writebuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
            perror ("pwrite");
            goto closeall;
        }

        // read out the Page data
        if (pread(fd, readbuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
            perror ("pread");
            goto closeall;
        }

        buf_num=0;
        while ((ptr_update->writebuf[buf_num]==readbuf[buf_num]) && (buf_num < readlen)) buf_num++;

        //if (((blockstart/MAX_PAGE_SIZE/64) % 10) == 9)
        if (buf_num < readlen) {
            //printf("offs[%x ],blockstart[%x],mtdoffset[%x],writesize[0x%x], buf_num[0x%x]\n",(int)offs,blockstart,mtdoffset,meminfo.writesize, buf_num);

            // set bad blocks
            offs = (loff_t) blockstart;
            if ((ret = ioctl(fd, MEMSETBADBLOCK, &offs)) < 0) {
                perror("ioctl(MEMSETBADBLOCK)");
                goto closeall;
            }
            if ((ret == 0) && (!quiet)) {
                fprintf (stdout, "set Bad block at %x !\n",blockstart);
                file_offset = file_offset - (mtdoffset-blockstart);
                imglen = imglen+ (mtdoffset-blockstart);;
                mtdoffset = blockstart + meminfo.erasesize;
            }
        } else {
            imglen -= readlen;
            mtdoffset += meminfo.writesize;
            file_offset+= meminfo.writesize;
        }
    }

closeall: //essential
    close(ifd);

restoreoob:  //essential
    close(fd);

    if (imglen > 0) {
        perror ("Data was only partially written due to error\n");
        exit (1);
    }

    image_crc ^= ~0U;
    printf ("image_length = 0x%08x\n", image_length);
    printf ("image_crc = 0x%08x\n", image_crc);
    free(ptr_update);

    sync();
    return 0;
}
