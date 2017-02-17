/**
 * /src/flashdb/spinor/n25q256.c
 *
 * History:
 * 2013/10/17 - [Johnson Diao] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
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
 *
 */

#include <bldfunc.h>
#include <ambhw/spinor.h>
#include <flash/spinor/n25q256.h>

extern void delay(u32 len);

static u8 n25q256_spi_mode=0; // 0-normal 1-daul 2-quad

int n25q256_write_en_normal(void)
{
	u8 status=0;
	int i=0;
	for (i=0;(!(status & N25Q256_WRITE_ENABLE_LATCH_BIT));
		i++){
		spi_nor_sendcmd(N25Q256_WRITEEN,0,0,0);
		spi_nor_readregister((u32)1,N25Q256_READ_STATUS_REG,&status);
		delay(1000);
	}
	return 0;
}

int n25q256_write_en_quad(void)
{
	u8 status=0;
	int i=0;
	for (i=0;(!(status & N25Q256_WRITE_ENABLE_LATCH_BIT));
		i++){
		spi_nor_sendcmd_quad(N25Q256_WRITEEN,0,0,0);
		spi_nor_readregister_quad((u32)1,N25Q256_READ_STATUS_REG,&status);
		delay(1000);
	}

	return 0;
}

int n25q256_write_en(void)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_write_en_quad();
	}else{
		rval=n25q256_write_en_normal();
	}
	return rval;
}

int n25q256_wait_done_quad(void)
{
	u8 status=0;
	int i;

	for (i=0;(status != 0x80); i++){
		spi_nor_readregister_quad((u32)0x1,N25Q256_READ_FLAG_STATUS_REG,&status);
		delay(1000);
	}
	return status;
}

int n25q256_wait_done_normal(void)
{
	u8 status=0;
	int i;

	for (i=0;(status != 0x80); i++){
		spi_nor_readregister((u32)0x1,N25Q256_READ_FLAG_STATUS_REG,&status);
		delay(1000);
	}
	return status;
}

int n25q256_wait_done(void)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_wait_done_quad();
	}else{
		rval=n25q256_wait_done_normal();
	}
	return rval;
}

int n25q256_4byte_addr_check_quad(u32 addr)
{
	u8 fourbyte_now=0;
	u8 fourbyte_dst=0;
	u32 i=0;
	spi_nor_readregister_quad(1,N25Q256_READ_FLAG_STATUS_REG,&fourbyte_now);
	fourbyte_now &= 0x01;
	if(addr & 0xff000000){
		fourbyte_dst = 1;
	}else{
		fourbyte_dst = 0;
	}
	if(fourbyte_dst == fourbyte_now){
		goto exit;
	}
	if(fourbyte_dst){
		for(i=0;i<5;i++){
			n25q256_write_en_quad();
			spi_nor_sendcmd_quad(N25Q256_ENTER_4_BYTE_ADDR_MODE,0,0,0);
			delay(100);
			spi_nor_readregister_quad(1,N25Q256_READ_FLAG_STATUS_REG,&fourbyte_now);
			fourbyte_now &= 0x01;
			if(fourbyte_now == fourbyte_dst){
				goto exit;
			}
		}
	}else{
		for(i=0;i<5;i++){
			n25q256_write_en_quad();
			spi_nor_sendcmd_quad(N25Q256_EXIT_4_BYTE_ADDR_MODE,0,0,0);
			delay(100);
			spi_nor_readregister_quad(1,N25Q256_READ_FLAG_STATUS_REG,&fourbyte_now);
			fourbyte_now &= 0x01;
			if(fourbyte_now == fourbyte_dst){
				goto exit;
			}
		}
	}
	exit:
	return ((fourbyte_now)?4:3);
}

int n25q256_4byte_addr_check_normal(u32 addr)
{
	u8 fourbyte_now=0;
	u8 fourbyte_dst=0;
	u32 i=0;
	spi_nor_readregister(1,N25Q256_READ_FLAG_STATUS_REG,&fourbyte_now);
	fourbyte_now &= 0x01;
	if(addr & 0xff000000){
		fourbyte_dst = 1;
	}else{
		fourbyte_dst = 0;
	}
	if(fourbyte_dst == fourbyte_now){
		goto exit;
	}
	if(fourbyte_dst){
		for(i=0;i<5;i++){
			n25q256_write_en();
			spi_nor_sendcmd(N25Q256_ENTER_4_BYTE_ADDR_MODE,0,0,0);
			delay(100);
			spi_nor_readregister(1,N25Q256_READ_FLAG_STATUS_REG,&fourbyte_now);
			fourbyte_now &= 0x01;
			if(fourbyte_now == fourbyte_dst){
				goto exit;
			}
		}
	}else{
		for(i=0;i<5;i++){
			n25q256_write_en();
			spi_nor_sendcmd(N25Q256_EXIT_4_BYTE_ADDR_MODE,0,0,0);
			delay(100);
			spi_nor_readregister(1,N25Q256_READ_FLAG_STATUS_REG,&fourbyte_now);
			fourbyte_now &= 0x01;
			if(fourbyte_now == fourbyte_dst){
				goto exit;
			}
		}
	}
	exit:
	return ((fourbyte_now)?4:3);
}

int n25q256_4byte_addr_check(u32 addr)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_4byte_addr_check_quad(addr);
	}else{
		rval=n25q256_4byte_addr_check_normal(addr);
	}
	return rval;
}

int n25q256_quad_mode(void)
{
	u8 temp;
	spi_nor_readregister(1,N25Q256_READ_ENH_VOL_CONF_REG,&temp);
	n25q256_write_en();
	spi_nor_sendcmd(N25Q256_WRITE_ENH_VOL_CONF_REG,0,0x5f,1);
	spi_nor_readregister_quad(1,N25Q256_READ_ENH_VOL_CONF_REG,&temp);
	n25q256_spi_mode = 2;
	return 0;
}

int n25q256_erase_sector_quad( u32 sector)
{
	u8 status;
	u32 addr,addr_length;
	addr = sector * N25Q256_BYTES_PER_SUBSECTOR;
	addr_length = n25q256_4byte_addr_check_quad(addr);
	n25q256_write_en_quad();
	spi_nor_sendcmd_quad(N25Q256_SUB_ERASE,0,addr,addr_length);
	status = n25q256_wait_done_quad();
	if((status & N25Q256_ERASE_ERR_BIT) != 0){
		putstr("error in erase \r\n");
		return -1;
	}
	return 0;
}

int n25q256_erase_sector_normal( u32 sector)
{
	u8 status;
	u32 addr,addr_length;
	addr = sector * N25Q256_BYTES_PER_SUBSECTOR;
	addr_length = n25q256_4byte_addr_check(addr);
	n25q256_write_en();
	spi_nor_sendcmd(N25Q256_SUB_ERASE,0,addr,addr_length);
	status = n25q256_wait_done();
	if((status & N25Q256_ERASE_ERR_BIT) != 0){
		putstr("error in erase \r\n");
		return -1;
	}
	return 0;
}

int n25q256_erase_sector( u32 sector)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_erase_sector_quad(sector);
	}else{
		rval=n25q256_erase_sector_normal(sector);
	}
	return rval;
}

int n25q256_write_sector_normal( u32 sector,u8 *src)
{
	u8 status;
	u32 addr,addr_length;
	u32 page_offset=0;

	addr = sector * N25Q256_BYTES_PER_SUBSECTOR;
	for(page_offset=0;page_offset<N25Q256_PAGES_PER_SUBSECTOR;page_offset++){
		addr_length = n25q256_4byte_addr_check(addr);
		n25q256_write_en();
		spi_nor_progdata(N25Q256_PAGE_PROG,1,0,addr,addr_length,src+page_offset*N25Q256_PAGE_SIZE,N25Q256_PAGE_SIZE);
		status = n25q256_wait_done();
		if((status & N25Q256_PROG_ERR_BIT) != 0){
			putstr("write sector return -1");
			return -1;
		}
		addr += N25Q256_PAGE_SIZE;
	}
	return 0;
}

int n25q256_write_sector_quad( u32 sector,u8 *src)
{
	u8 status;
	u32 addr,addr_length;
	u32 page_offset=0;

	addr = sector * N25Q256_BYTES_PER_SUBSECTOR;
	for(page_offset=0;page_offset<N25Q256_PAGES_PER_SUBSECTOR;page_offset++){
		addr_length = n25q256_4byte_addr_check_quad(addr);
		n25q256_write_en_quad();
		spi_nor_progdata_quad(N25Q256_QUAD_IN_FAST_PROG,1,0,addr,addr_length,src+page_offset*N25Q256_PAGE_SIZE,N25Q256_PAGE_SIZE);
		status = n25q256_wait_done_quad();
		if((status & N25Q256_PROG_ERR_BIT) != 0){
			putstr("write sector return -1");
			return -1;
		}
		addr += N25Q256_PAGE_SIZE;
	}
	return 0;
}

int n25q256_write_sector( u32 sector,u8 *src)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_write_sector_quad(sector,src);
	}else{
		rval=n25q256_write_sector_normal(sector,src);
	}
	return rval;
}

int n25q256_write_bst_normal(u32 sector,u8 *src)
{
	u8 status;
	u32 addr=0,addr_length;
	u32 page_offset=0;
	u32 header[37];

	header[0] = (0x01 << 30)| // command length
		(0x03 << 27)| // address length
		(0x08 << 22)| // dummy length
		(0x020 << 16)| // clock divider
		(0x800); //bst fixed to 2k
	header[1] = (0 << 31)| // command dtr mode
		(0 << 30)| // addr dtr mode
		(0 << 29)| // dummy dtr mode
		(0 << 28)| // data dtr mode
		(0 << 14)| // number of lanes in command
		(0 << 12)| // number of lanes in addr
		(0x2 << 10)| // number of lanes in data
		(0 << 9)| // tx,rx separate lanes
		(0 << 1)| // write enable
		(1);// read enable
	header[2] = (1 << 31)| // flow control
		(0x3 << 28)| // hold out pin selection
		(0 << 26)| // hold bit switch timing
		(0xfe << 18)| // chip select, need config by macro
		(0); // rx sampling timing adjustment
	header[3] = 0x6b; // command
	// set the bst at page 1
	header[4] = 0x00000000;// addr high
	header[5] = 0x00000100;//addr low
		// header[32] used as length of sectors, added by cddiao
	header[32] = 0x00001000; // the length of sector in n25q256 is 4096 bytes
		header[33] = readl(PLL_CORE_CTRL_REG);
		header[34] = readl(PLL_CORE_FRAC_REG);
	header[35] = readl(CLK_REF_SSI3_REG);
	header[36] = readl(CG_SSI3_REG);

	n25q256_erase_sector(0);
	n25q256_write_en();
	spi_nor_progdata(N25Q256_PAGE_PROG,1,0,0,3,(u8*)header,148);
	n25q256_wait_done();

	addr = N25Q256_PAGE_SIZE;
	for(page_offset=0;page_offset<N25Q256_PAGES_PER_SUBSECTOR-1;page_offset++){
		addr_length = n25q256_4byte_addr_check(addr);
		n25q256_write_en();
		spi_nor_progdata(N25Q256_PAGE_PROG,1,0,addr,addr_length,src+page_offset*N25Q256_PAGE_SIZE,N25Q256_PAGE_SIZE);
		status = n25q256_wait_done();
		if((status & N25Q256_PROG_ERR_BIT) != 0){
			putstr("write sector return -1");
			return -1;
		}
		addr += N25Q256_PAGE_SIZE;
	}
	return 0;
}

int n25q256_write_bst_quad(u32 sector,u8 *src)
{
	u8 status;
	u32 addr=0,addr_length;
	u32 page_offset=0;
	u32 header[37];

	header[0] = (0x01 << 30)| // command length
		(0x03 << 27)| // address length
		(0x08 << 22)| // dummy length
		(0x020 << 16)| // clock divider
		(0x800); //bst fixed to 2k
	header[1] = (0 << 31)| // command dtr mode
		(0 << 30)| // addr dtr mode
		(0 << 29)| // dummy dtr mode
		(0 << 28)| // data dtr mode
		(0 << 14)| // number of lanes in command
		(0 << 12)| // number of lanes in addr
		(0x2 << 10)| // number of lanes in data
		(0 << 9)| // tx,rx separate lanes
		(0 << 1)| // write enable
		(1);// read enable
	header[2] = (1 << 31)| // flow control
		(0x3 << 28)| // hold out pin selection
		(0 << 26)| // hold bit switch timing
		(0xfe << 18)| // chip select, need config by macro
		(0); // rx sampling timing adjustment
	header[3] = 0x6b; // command
	// set the bst at page 1
	header[4] = 0x00000000;// addr high
	header[5] = 0x00000100;//addr low
	// header[32] used as length of sectors, added by cddiao
	header[32] = 0x00001000; // the length of sector in n25q256 is 4096 bytes
	header[33] = readl(PLL_CORE_CTRL_REG);
	header[34] = readl(PLL_CORE_FRAC_REG);
	header[35] = readl(CLK_REF_SSI3_REG);
	header[36] = readl(CG_SSI3_REG);

	n25q256_erase_sector_quad(0);
	n25q256_write_en_quad();
	spi_nor_progdata_quad(N25Q256_QUAD_IN_FAST_PROG,1,0,0,3,(u8*)header,148);
	n25q256_wait_done_quad();

	addr = N25Q256_PAGE_SIZE;
	for(page_offset=0;page_offset<N25Q256_PAGES_PER_SUBSECTOR-1;page_offset++){
		addr_length = n25q256_4byte_addr_check_quad(addr);
		n25q256_write_en_quad();
		spi_nor_progdata_quad(N25Q256_QUAD_IN_FAST_PROG,1,0,addr,addr_length,src+page_offset*N25Q256_PAGE_SIZE,N25Q256_PAGE_SIZE);
		status = n25q256_wait_done_quad();
		if((status & N25Q256_PROG_ERR_BIT) != 0){
			putstr("write sector return -1");
			return -1;
		}
		addr += N25Q256_PAGE_SIZE;
	}
	return 0;
}

int n25q256_write_bst(u32 sector,u8 *src)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_write_bst_quad(sector,src);
	}else{
		rval=n25q256_write_bst_normal(sector,src);
	}
	return rval;
}

int n25q256_read_normal( u32 addr,u32 data_len,const u8 *dst)
{
	u32 addr_length;
	addr_length=n25q256_4byte_addr_check(addr);
	spi_nor_readdata(N25Q256_READ,0,addr,addr_length,data_len,dst);
	return 0;
}

int n25q256_read_quad( u32 addr,u32 data_len,const u8 *dst)
{
	u32 addr_length;
	addr_length=n25q256_4byte_addr_check_quad(addr);
	spi_nor_readdata_quad(N25Q256_QUAD_OUT_FAST_READ,10,addr,addr_length,data_len,dst);
	return 0;
}

int n25q256_read( u32 addr,u32 data_len,const u8 *dst)
{
	int rval=0;
	if (n25q256_spi_mode == 2){
		rval=n25q256_read_quad(addr,data_len,dst);
	}else{
		rval=n25q256_read_normal(addr,data_len,dst);
	}
	return rval;
}

int n25q256_init(void)
{
	spi_nor_sendcmd(N25Q256_RESET_ENABLE,0,0,0);
	spi_nor_sendcmd(N25Q256_RESET_MEMORY,0,0,0);
	//n25q256_quad_mode();
	return 0;
}
