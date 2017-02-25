/*
 * kernel/private/drivers/vout/it66121/it66121.c
 *
 * History:
 *	2013/01/08 - [Johnson Diao]
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ambpriv_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <plat/clk.h>
#include "vout_pri.h"
#include "it66121.h"


struct it66121_t{
	struct i2c_client *client;
	struct work_struct work;
	u64 RCLK;
	u64 VideoPixelClock;
    u8 VIC;
    u8 pixelrep;
	u8 bChangeMode;
	HDMI_Colorimetry Colorimetry;
	u8 run;
};

static unsigned char CommunBuff[128] ;
HDMI_Aspec aspec ;
static RX_CAP RxCapability ;

static int IT66121_SYNCEMBED=0;
module_param(IT66121_SYNCEMBED, int, S_IRUGO|S_IWUSR);

static u8 it66121_read_reg(struct it66121_t *it66121,u32 addr)
{
	u8 temp;
	temp = i2c_smbus_read_byte_data(it66121->client,addr);
	//DRV_PRINT("read [%x]=%x\r\n",addr,temp);
	return temp;
}

static int it66121_write_reg(struct it66121_t *it66121,u32 addr, u8 mask, u8 data)
{
	struct i2c_client *client;
	u8 temp;
	client = it66121->client;
	if (mask == 0xff){
		//DRV_PRINT("write [%x]=%x\r\n",addr,data);
		i2c_smbus_write_byte_data(client,addr,data);
	}else{
		temp = (u8)i2c_smbus_read_byte_data(client,addr);
		temp &= (~mask);
		temp |= (data & mask);
		//DRV_PRINT("write [%x]=%x\r\n",addr,temp);
		i2c_smbus_write_byte_data(client,addr,temp);
	}
	return 0;
}

static int it66121_write_reg_direct(struct it66121_t *it66121,u32 addr,  u8 data)
{
	it66121_write_reg(it66121,addr,0xff,data);
	return 0;
}


static int it66121_load_table(struct it66121_t *it66121, struct it66121_reg_table *table)
{
	int i;
	for( i = 0;;i++ ){
		if ((table[i].addr == 0) &&
			(table[i].mask == 0) &&
			(table[i].value == 0)){
			return 0;
		}
		if ((table[i].mask == 0) &&
			(table[i].value == 0)){
			mdelay(100);
			continue;
		}

		it66121_write_reg(it66121,table[i].addr,table[i].mask,table[i].value);
	}
	return 0;
}

void HDMITX_DisableVideoOutput(struct it66121_t *it66121)
{
	u8 uc = it66121_read_reg(it66121,REG_TX_SW_RST) | B_HDMITX_VID_RST ;
	it66121_write_reg_direct(it66121,REG_TX_SW_RST,uc);
	it66121_write_reg_direct(it66121,REG_TX_AFE_DRV_CTRL,B_TX_AFE_DRV_RST|B_TX_AFE_DRV_PWD);
	it66121_write_reg(it66121,0x62, 0x90, 0x00);
	it66121_write_reg(it66121,0x64, 0x89, 0x00);
	return;
}

#define Switch_HDMITX_Bank(x)   it66121_write_reg(it66121,0x0f,1, (x)&1)

void DumpHDMITXReg(struct it66121_t *it66121)
{
    int i,j ;
    u8 ucData ;

    HDMITX_DEBUG_PRINTF(("       "));
    for(j = 0 ; j < 16 ; j++){
        HDMITX_DEBUG_PRINTF((" %02X",(int)j));
        if((j == 3)||(j==7)||(j==11)){
            HDMITX_DEBUG_PRINTF(("  "));
        }
    }
    HDMITX_DEBUG_PRINTF(("\n        -----------------------------------------------------\n"));

    Switch_HDMITX_Bank(0);

    for(i = 0 ; i < 0x100 ; i+=16){
        HDMITX_DEBUG_PRINTF(("[%3X]  ",i));
        for(j = 0 ; j < 16 ; j++){
            if( (i+j)!= 0x17){
                ucData = it66121_read_reg(it66121,(u8)((i+j)&0xFF));
                HDMITX_DEBUG_PRINTF((" %02X",(int)ucData));
            }else{
                HDMITX_DEBUG_PRINTF((" %X",(int)ucData)); // for DDC FIFO
            }
            if((j == 3)||(j==7)||(j==11)){
                HDMITX_DEBUG_PRINTF((" -"));
            }
        }
        HDMITX_DEBUG_PRINTF(("\n"));
        if((i % 0x40) == 0x30){
            HDMITX_DEBUG_PRINTF(("        -----------------------------------------------------\n"));
        }
    }
    Switch_HDMITX_Bank(1);
    for(i = 0x130; i < 0x200 ; i+=16){
        HDMITX_DEBUG_PRINTF(("[%3X]  ",i));
        for(j = 0 ; j < 16 ; j++){
            ucData = it66121_read_reg(it66121,(u8)((i+j)&0xFF));
            HDMITX_DEBUG_PRINTF((" %02X",(int)ucData));
            if((j == 3)||(j==7)||(j==11)){
                HDMITX_DEBUG_PRINTF((" -"));
            }
        }
        HDMITX_DEBUG_PRINTF(("\n"));
        if((i % 0x40) == 0x20){
            HDMITX_DEBUG_PRINTF(("        -----------------------------------------------------\n"));
        }
    }
    HDMITX_DEBUG_PRINTF(("        -----------------------------------------------------\n"));
    Switch_HDMITX_Bank(0);
}

#define InitCEC() it66121_write_reg(it66121,0x0F, 0x08, 0x00)
#define DisableCEC() it66121_write_reg(it66121,0x0F, 0x08, 0x08)
u64 CalcRCLK(struct it66121_t *it66121)
{
	int i ;
	long sum, RCLKCNT  ;

	InitCEC();
	sum = 0 ;
	for( i = 0 ; i < 5 ; i++ ){
		it66121_write_reg_direct(it66121,0x09, 1);
		msleep(100);
		it66121_write_reg_direct(it66121,0x09, 0);
		RCLKCNT = it66121_read_reg(it66121,0x47);
		RCLKCNT <<= 8 ;
		RCLKCNT |= it66121_read_reg(it66121,0x46);
		RCLKCNT <<= 8 ;
		RCLKCNT |= it66121_read_reg(it66121,0x45);
		sum += RCLKCNT ;
	}
	DisableCEC();
	RCLKCNT = sum * 32 ;
	HDMITX_DEBUG_PRINTF(("RCLK = %ld,%03ld,%03ld\n",RCLKCNT/1000000,(RCLKCNT%1000000)/1000,RCLKCNT%1000));
	return RCLKCNT ;
}

static int it66121_init_fun(struct it66121_t *it66121)
{
	it66121_load_table(it66121,HDMITX_Init_Table);
	it66121_load_table(it66121,HDMITX_DefaultVideo_Table);
	it66121_load_table(it66121,HDMITX_SetHDMI_Table);
	it66121_load_table(it66121,HDMITX_DefaultAVIInfo_Table);
	it66121->RCLK = CalcRCLK(it66121);
	DumpHDMITXReg(it66121);
	return 0;
}

static void it66121_hdmitx_clearDDCFIFO(struct it66121_t *it66121)
{
	it66121_write_reg(it66121,REG_TX_DDC_MASTER_CTRL,B_TX_MASTERDDC|B_TX_MASTERHOST,B_TX_MASTERDDC|B_TX_MASTERHOST);
	it66121_write_reg(it66121,REG_TX_DDC_CMD,CMD_FIFO_CLR,CMD_FIFO_CLR);
}

static void it66121_hdmitx_abortDDC(struct it66121_t *it66121)
{
	u8 SWReset,DDCMaster ;
	u8 uc, timeout, i ;

	SWReset = it66121_read_reg(it66121,REG_TX_SW_RST);
	//CPDesire = it66121_read_reg(REG_TX_HDCP_DESIRE);
	DDCMaster = it66121_read_reg(it66121,REG_TX_DDC_MASTER_CTRL);

	//it66121_write_reg(REG_TX_HDCP_DESIRE,CPDesire&(~B_TX_CPDESIRE)); // @emily change order
	//it66121_write_reg(REG_TX_SW_RST,SWReset|B_TX_HDCP_RST_HDMITX);		 // @emily change order
	it66121_write_reg(it66121,REG_TX_DDC_MASTER_CTRL,B_TX_MASTERDDC|B_TX_MASTERHOST,B_TX_MASTERDDC|B_TX_MASTERHOST);

	for( i = 0 ; i < 2 ; i++ ){
		it66121_write_reg(it66121,REG_TX_DDC_CMD,CMD_DDC_ABORT,CMD_DDC_ABORT);

		for( timeout = 0 ; timeout < 200 ; timeout++ ){
			uc = it66121_read_reg(it66121,REG_TX_DDC_STATUS);
			if (uc&B_TX_DDC_DONE){
				break ; // success
			}
			if( uc & (B_TX_DDC_NOACK|B_TX_DDC_WAITBUS|B_TX_DDC_ARBILOSE) ){
				break ;
			}
			msleep(1); // delay 1 ms to stable.
		}
	}

}


static int it66121_check(struct it66121_t *it66121,u8 *pHPD, u8 *pHPDChange)
{
	u8 intdata1,intdata2,intdata3,sysstat;
	u8  intclr3 = 0 ;
	static u8 PrevHPD = 0;
	u8 HPD;
	sysstat = it66121_read_reg(it66121,REG_TX_SYS_STATUS);

	if((sysstat & (B_TX_HPDETECT)) == (B_TX_HPDETECT)){
		HPD = 1;
	}else{
		HPD = 0;
	}

	if(PrevHPD != HPD){
		*pHPDChange = 1;
		PrevHPD = HPD;
	}else{
		*pHPDChange = 0;
	}

	if(sysstat & B_TX_INT_ACTIVE){
		intdata1 = it66121_read_reg(it66121,REG_TX_INT_STAT1);
		if(intdata1 & B_TX_INT_AUD_OVERFLOW){
			it66121_write_reg_direct(it66121,REG_TX_SW_RST,(B_HDMITX_AUD_RST|B_TX_AREF_RST));
			it66121_write_reg_direct(it66121,REG_TX_SW_RST,~(B_HDMITX_AUD_RST|B_TX_AREF_RST));
		}
		if(intdata1 & B_TX_INT_DDCFIFO_ERR){
			it66121_hdmitx_clearDDCFIFO(it66121);
		}
		if(intdata1 & B_TX_INT_DDC_BUS_HANG){
			it66121_hdmitx_abortDDC(it66121);
		}
		if(intdata1 & (B_TX_INT_HPD_PLUG)){
			if(pHPDChange){
				*pHPDChange = 1 ;
			}
		}
		intdata2 = it66121_read_reg(it66121,REG_TX_INT_STAT2);
		intdata3= it66121_read_reg(it66121,0xEE);
		if( intdata3 ){
			it66121_write_reg_direct(it66121,0xEE,intdata3); // clear ext interrupt ;
		}
		it66121_write_reg_direct(it66121,REG_TX_INT_CLR0,0xFF);
		it66121_write_reg_direct(it66121,REG_TX_INT_CLR1,0xFF);
		intclr3 = (it66121_read_reg(it66121,REG_TX_SYS_STATUS))|B_TX_CLR_AUD_CTS | B_TX_INTACTDONE ;
		it66121_write_reg_direct(it66121,REG_TX_SYS_STATUS,intclr3); // clear interrupt.
		intclr3 &= ~(B_TX_INTACTDONE);
		it66121_write_reg_direct(it66121,REG_TX_SYS_STATUS,intclr3); // INTACTDONE reset to zero.
	}

	if(pHPDChange){
		if((*pHPDChange==1) &&(HPD==0))	{
			it66121_write_reg_direct(it66121,REG_TX_AFE_DRV_CTRL,B_TX_AFE_DRV_RST|B_TX_AFE_DRV_PWD);
		}
	}
	*pHPD = HPD	;
	return 0;
}

void setHDMITX_AVMute(struct it66121_t *it66121,u8 bEnable)
{
	Switch_HDMITX_Bank(0);
	it66121_write_reg(it66121,REG_TX_GCP,B_TX_SETAVMUTE, bEnable?B_TX_SETAVMUTE:0 );
	it66121_write_reg_direct(it66121,REG_TX_PKT_GENERAL_CTRL,B_TX_ENABLE_PKT|B_TX_REPEAT_PKT);
}


void hdmitx_SetInputMode(struct it66121_t *it66121,u8 InputColorMode,u8 bInputSignalType)
{
	u8 ucData ;

	ucData = it66121_read_reg(it66121,REG_TX_INPUT_MODE);
	ucData &= ~(M_TX_INCOLMOD|B_TX_2X656CLK|B_TX_SYNCEMB|B_TX_INDDR|B_TX_PCLKDIV2);

	switch(InputColorMode & F_MODE_CLRMOD_MASK){
	case F_MODE_YUV422:
		ucData |= B_TX_IN_YUV422 ;
		break ;
	case F_MODE_YUV444:
		ucData |= B_TX_IN_YUV444 ;
		break ;
	case F_MODE_RGB444:
	default:
		ucData |= B_TX_IN_RGB ;
		break ;
	}
	if(bInputSignalType & T_MODE_PCLKDIV2){
		ucData |= B_TX_PCLKDIV2 ;
	}
	if(bInputSignalType & T_MODE_CCIR656){
		ucData |= B_TX_2X656CLK ;
	}
	if(bInputSignalType & T_MODE_SYNCEMB){
		ucData |= B_TX_SYNCEMB ;
	}
	if(bInputSignalType & T_MODE_INDDR){
		ucData |= B_TX_INDDR ;
	}
	it66121_write_reg_direct(it66121,REG_TX_INPUT_MODE,ucData);
}

void hdmitx_SetCSCScale(struct it66121_t *it66121,u8 bInputMode,u8 bOutputMode)
{
	u8 ucData,csc = 0;
	u8 i ;
	u8 filter = 0 ; // filter is for Video CTRL DN_FREE_GO,EN_DITHER,and ENUDFILT

	// (1) YUV422 in,RGB/YUV444 output (Output is 8-bit,input is 12-bit)
	// (2) YUV444/422  in,RGB output (CSC enable,and output is not YUV422)
	// (3) RGB in,YUV444 output   (CSC enable,and output is not YUV422)
	//
	// YUV444/RGB24 <-> YUV422 need set up/down filter.
	HDMITX_DEBUG_PRINTF(("hdmitx_SetCSCScale(u8 bInputMode = %x,u8 bOutputMode = %x)\n", (int)bInputMode, (int)bOutputMode)) ;
	switch(bInputMode&F_MODE_CLRMOD_MASK){
	  case F_MODE_YUV422:
		HDMITX_DEBUG_PRINTF(("Input mode is YUV422\n"));
		switch(bOutputMode&F_MODE_CLRMOD_MASK){
		case F_MODE_YUV444:
			HDMITX_DEBUG_PRINTF(("Output mode is YUV444\n"));
			csc = B_HDMITX_CSC_BYPASS ;
			if(bInputMode & F_VIDMODE_EN_UDFILT){
				filter |= B_TX_EN_UDFILTER ;
			}
			if(bInputMode & F_VIDMODE_EN_DITHER){
				filter |= B_TX_EN_DITHER | B_TX_DNFREE_GO ;
			}
			break ;
		case F_MODE_YUV422:
			HDMITX_DEBUG_PRINTF(("Output mode is YUV422\n"));
			csc = B_HDMITX_CSC_BYPASS ;
			break ;

		case F_MODE_RGB444:
			HDMITX_DEBUG_PRINTF(("Output mode is RGB24\n"));
			csc = B_HDMITX_CSC_YUV2RGB ;
			if(bInputMode & F_VIDMODE_EN_UDFILT){
				filter |= B_TX_EN_UDFILTER ;
			}
			if(bInputMode & F_VIDMODE_EN_DITHER){
				filter |= B_TX_EN_DITHER | B_TX_DNFREE_GO ;
			}
			break ;
		}
		break ;
	}
#ifndef DISABLE_HDMITX_CSC
	// set the CSC metrix registers by colorimetry and quantization
	if(csc == B_HDMITX_CSC_RGB2YUV){
		HDMITX_DEBUG_PRINTF(("CSC = RGB2YUV %x ",csc));
		switch(bInputMode&(F_VIDMODE_ITU709|F_VIDMODE_16_235)){
		case F_VIDMODE_ITU709|F_VIDMODE_16_235:
			HDMITX_DEBUG_PRINTF(("ITU709 16-235 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_RGB2YUV_ITU709_16_235[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_RGB2YUV_ITU709_16_235[i]));}
			break ;
		case F_VIDMODE_ITU709|F_VIDMODE_0_255:
			HDMITX_DEBUG_PRINTF(("ITU709 0-255 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_RGB2YUV_ITU709_0_255[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_RGB2YUV_ITU709_0_255[i]));}
			break ;
		case F_VIDMODE_ITU601|F_VIDMODE_16_235:
			HDMITX_DEBUG_PRINTF(("ITU601 16-235 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_RGB2YUV_ITU601_16_235[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_RGB2YUV_ITU601_16_235[i]));}
			break ;
		case F_VIDMODE_ITU601|F_VIDMODE_0_255:
		default:
			HDMITX_DEBUG_PRINTF(("ITU601 0-255 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_RGB2YUV_ITU601_0_255[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_RGB2YUV_ITU601_0_255[i]));}
			break ;
		}
	}

	if (csc == B_HDMITX_CSC_YUV2RGB){
		HDMITX_DEBUG_PRINTF(("CSC = YUV2RGB %x ",csc));

		switch(bInputMode&(F_VIDMODE_ITU709|F_VIDMODE_16_235)){
		case F_VIDMODE_ITU709|F_VIDMODE_16_235:
			HDMITX_DEBUG_PRINTF(("ITU709 16-235 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_YUV2RGB_ITU709_16_235[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_YUV2RGB_ITU709_16_235[i]));}
			break ;
		case F_VIDMODE_ITU709|F_VIDMODE_0_255:
			HDMITX_DEBUG_PRINTF(("ITU709 0-255 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_YUV2RGB_ITU709_0_255[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_YUV2RGB_ITU709_0_255[i]));}
			break ;
		case F_VIDMODE_ITU601|F_VIDMODE_16_235:
			HDMITX_DEBUG_PRINTF(("ITU601 16-235 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_YUV2RGB_ITU601_16_235[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_YUV2RGB_ITU601_16_235[i]));}
			break ;
		case F_VIDMODE_ITU601|F_VIDMODE_0_255:
		default:
			HDMITX_DEBUG_PRINTF(("ITU601 0-255 "));
			for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){ it66121_write_reg_direct(it66121,REG_TX_CSC_YOFF+i,bCSCMtx_YUV2RGB_ITU601_0_255[i]) ; HDMITX_DEBUG_PRINTF(("reg%02X <- %02X\n",(int)(i+REG_TX_CSC_YOFF),(int)bCSCMtx_YUV2RGB_ITU601_0_255[i]));}
			break ;
		}
	}
#else// DISABLE_HDMITX_CSC
	csc = B_HDMITX_CSC_BYPASS ;
#endif// DISABLE_HDMITX_CSC

	if( csc == B_HDMITX_CSC_BYPASS ){
		it66121_write_reg(it66121,0xF, 0x10, 0x10);
	}else{
		it66121_write_reg(it66121,0xF, 0x10, 0x00);
	}
	ucData = it66121_read_reg(it66121,REG_TX_CSC_CTRL) & ~(M_TX_CSC_SEL|B_TX_DNFREE_GO|B_TX_EN_DITHER|B_TX_EN_UDFILTER);
	ucData |= filter|csc ;

	it66121_write_reg_direct(it66121,REG_TX_CSC_CTRL,ucData);

	// set output Up/Down Filter,Dither control

}

void hdmitx_SetupAFE(struct it66121_t *it66121,u8 level)
{

	it66121_write_reg_direct(it66121,REG_TX_AFE_DRV_CTRL,B_TX_AFE_DRV_RST);/* 0x10 */
	switch(level){
	case 2:// high
		it66121_write_reg(it66121,0x62, 0x90, 0x80);
		it66121_write_reg(it66121,0x64, 0x89, 0x80);
		it66121_write_reg(it66121,0x68, 0x10, 0x80);
		HDMITX_DEBUG_PRINTF(("hdmitx_SetupAFE()===================HIGHT\n"));
		break ;
	default:
		it66121_write_reg(it66121,0x62, 0x90, 0x10);
		it66121_write_reg(it66121,0x64, 0x89, 0x09);
		it66121_write_reg(it66121,0x68, 0x10, 0x10);
		HDMITX_DEBUG_PRINTF(("hdmitx_SetupAFE()===================LOW\n"));
		break ;
	}

	it66121_write_reg(it66121,REG_TX_SW_RST,B_TX_REF_RST_HDMITX|B_HDMITX_VID_RST,0);
	it66121_write_reg_direct(it66121,REG_TX_AFE_DRV_CTRL,0x0);
	msleep(1);
}


void hdmitx_FireAFE(struct it66121_t *it66121)
{
	Switch_HDMITX_Bank(0);
	it66121_write_reg_direct(it66121,REG_TX_AFE_DRV_CTRL,0x0);
}

u8 HDMITX_EnableVideoOutput(struct it66121_t *it66121,u8 inputColorMode,u8 outputColorMode,u8 level
								,u8 inputtype)
{

	it66121_write_reg_direct(it66121,REG_TX_SW_RST,B_HDMITX_VID_RST|B_HDMITX_AUD_RST|B_TX_AREF_RST|B_TX_HDCP_RST_HDMITX);
	Switch_HDMITX_Bank(1);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB1,0x00);
	Switch_HDMITX_Bank(0);

	setHDMITX_AVMute(it66121,1);

	hdmitx_SetInputMode(it66121,inputColorMode,inputtype);

	hdmitx_SetCSCScale(it66121,inputColorMode,outputColorMode);

	it66121_write_reg_direct(it66121,REG_TX_HDMI_MODE,B_TX_HDMI_MODE);

	hdmitx_SetupAFE(it66121,level); // pass if High Freq request
	it66121_write_reg_direct(it66121,REG_TX_SW_RST,B_HDMITX_AUD_RST|B_TX_AREF_RST|B_TX_HDCP_RST_HDMITX);

	hdmitx_FireAFE(it66121);

	return 1 ;
}

#define hdmitx_ENABLE_AVI_INFOFRM_PKT()  { it66121_write_reg_direct(it66121,REG_TX_AVI_INFOFRM_CTRL,B_TX_ENABLE_PKT|B_TX_REPEAT_PKT); }


u8 hdmitx_SetAVIInfoFrame(struct it66121_t *it66121,AVI_InfoFrame *pAVIInfoFrame)
{
	int i ;
	u8 checksum ;

	if(!pAVIInfoFrame){
		return -1 ;
	}
	Switch_HDMITX_Bank(1);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB1,pAVIInfoFrame->pktbyte.AVI_DB[0]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB2,pAVIInfoFrame->pktbyte.AVI_DB[1]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB3,pAVIInfoFrame->pktbyte.AVI_DB[2]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB4,pAVIInfoFrame->pktbyte.AVI_DB[3]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB5,pAVIInfoFrame->pktbyte.AVI_DB[4]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB6,pAVIInfoFrame->pktbyte.AVI_DB[5]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB7,pAVIInfoFrame->pktbyte.AVI_DB[6]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB8,pAVIInfoFrame->pktbyte.AVI_DB[7]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB9,pAVIInfoFrame->pktbyte.AVI_DB[8]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB10,pAVIInfoFrame->pktbyte.AVI_DB[9]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB11,pAVIInfoFrame->pktbyte.AVI_DB[10]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB12,pAVIInfoFrame->pktbyte.AVI_DB[11]);
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_DB13,pAVIInfoFrame->pktbyte.AVI_DB[12]);
	for(i = 0,checksum = 0; i < 13 ; i++){
		checksum -= pAVIInfoFrame->pktbyte.AVI_DB[i] ;
	}
	checksum -= AVI_INFOFRAME_VER+AVI_INFOFRAME_TYPE+AVI_INFOFRAME_LEN ;
	it66121_write_reg_direct(it66121,REG_TX_AVIINFO_SUM,checksum);

	Switch_HDMITX_Bank(0);
	hdmitx_ENABLE_AVI_INFOFRM_PKT();
	return 0 ;
}


u8 HDMITX_EnableAVIInfoFrame(struct it66121_t *it66121,u8 bEnable,u8 *pAVIInfoFrame)
{
	if(hdmitx_SetAVIInfoFrame(it66121,(AVI_InfoFrame *)pAVIInfoFrame) == 0){
		return 1 ;
	}
	return 0 ;
}


void ConfigAVIInfoFrame(struct it66121_t *it66121,u8 VIC, u8 pixelrep,u8 bOutputColorMode)
{
	AVI_InfoFrame *AviInfo;
	AviInfo = (AVI_InfoFrame *)CommunBuff ;

	AviInfo->pktbyte.AVI_HB[0] = AVI_INFOFRAME_TYPE|0x80 ;
	AviInfo->pktbyte.AVI_HB[1] = AVI_INFOFRAME_VER ;
	AviInfo->pktbyte.AVI_HB[2] = AVI_INFOFRAME_LEN ;

	switch(bOutputColorMode){
	case F_MODE_YUV444:
		// AviInfo->info.ColorMode = 2 ;
		AviInfo->pktbyte.AVI_DB[0] = (2<<5)|(1<<4);
		break ;
	case F_MODE_YUV422:
		// AviInfo->info.ColorMode = 1 ;
		AviInfo->pktbyte.AVI_DB[0] = (1<<5)|(1<<4);
		break ;
	case F_MODE_RGB444:
	default:
		// AviInfo->info.ColorMode = 0 ;
		AviInfo->pktbyte.AVI_DB[0] = (0<<5)|(1<<4);
		break ;
	}
	AviInfo->pktbyte.AVI_DB[1] = 8 ;
	AviInfo->pktbyte.AVI_DB[1] |= (aspec != HDMI_16x9)?(1<<4):(2<<4); // 4:3 or 16:9
	AviInfo->pktbyte.AVI_DB[1] |= (it66121->Colorimetry != HDMI_ITU709)?(1<<6):(2<<6); // 4:3 or 16:9
	AviInfo->pktbyte.AVI_DB[2] = 0 ;
	AviInfo->pktbyte.AVI_DB[3] = VIC ;
	AviInfo->pktbyte.AVI_DB[4] =  pixelrep & 3 ;
	AviInfo->pktbyte.AVI_DB[5] = 0 ;
	AviInfo->pktbyte.AVI_DB[6] = 0 ;
	AviInfo->pktbyte.AVI_DB[7] = 0 ;
	AviInfo->pktbyte.AVI_DB[8] = 0 ;
	AviInfo->pktbyte.AVI_DB[9] = 0 ;
	AviInfo->pktbyte.AVI_DB[10] = 0 ;
	AviInfo->pktbyte.AVI_DB[11] = 0 ;
	AviInfo->pktbyte.AVI_DB[12] = 0 ;

	HDMITX_EnableAVIInfoFrame(it66121,1, (unsigned char *)AviInfo);
}


#define MaxIndex (sizeof(TimingTable)/sizeof(struct CRT_TimingSetting))
u8 setHDMITX_SyncEmbeddedByVIC(struct it66121_t *it66121,u8 VIC,u8 bInputType)
{
	int i ;
	u8 fmt_index=0;

	// if Embedded Video,need to generate timing with pattern register
	Switch_HDMITX_Bank(0);

	HDMITX_DEBUG_PRINTF(("setHDMITX_SyncEmbeddedByVIC(%d,%x)\n",(int)VIC,(int)bInputType));
	if( VIC > 0 ){
		for(i=0;i< MaxIndex;i ++){
			if(TimingTable[i].fmt==VIC){
				fmt_index=i;
				HDMITX_DEBUG_PRINTF(("fmt_index=%02x)\n",(int)fmt_index));
				HDMITX_DEBUG_PRINTF(("***Fine Match Table ***\n"));
				break;
			}
		}
	}
	else{
		HDMITX_DEBUG_PRINTF(("***No Match VIC == 0 ***\n"));
		return 0 ;
	}

	if(i>=MaxIndex){
		//return 0;
		HDMITX_DEBUG_PRINTF(("***No Match VIC ***\n"));
		return 0 ;
	}
	//if( bInputSignalType & T_MODE_SYNCEMB )
	{
		int HTotal, HDES, VTotal, VDES;
		int HDEW, VDEW, HFP, HSW, VFP, VSW;
		int HRS, HRE;
		int VRS, VRE;
		int H2ndVRRise;
		int VRS2nd, VRE2nd;
		u8 Pol;

		HTotal  =TimingTable[fmt_index].HTotal;
		HDEW	=TimingTable[fmt_index].HActive;
		HFP	 =TimingTable[fmt_index].H_FBH;
		HSW	 =TimingTable[fmt_index].H_SyncW;
		HDES	=HSW+TimingTable[fmt_index].H_BBH;
		VTotal  =TimingTable[fmt_index].VTotal;
		VDEW	=TimingTable[fmt_index].VActive;
		VFP	 =TimingTable[fmt_index].V_FBH;
		VSW	 =TimingTable[fmt_index].V_SyncW;
		VDES	=VSW+TimingTable[fmt_index].V_BBH;

		Pol = (TimingTable[fmt_index].HPolarity==Hpos)?(1<<1):0 ;
		Pol |= (TimingTable[fmt_index].VPolarity==Vpos)?(1<<2):0 ;

		// SyncEmb case=====
		if( bInputType & T_MODE_CCIR656){
			HRS = HFP - 1;
		}else{
			HRS = HFP - 2;
		}
		HRE = HRS + HSW;
		H2ndVRRise = HRS+ HTotal/2;

		VRS = VFP;
		VRE = VRS + VSW;

		// VTotal>>=1;

		if(PROG == TimingTable[fmt_index].Scan){ // progressive mode
			VRS2nd = 0xFFF;
			VRE2nd = 0x3F;
		}else{ // interlaced mode
			if(39 == TimingTable[fmt_index].fmt){
				VRS2nd = VRS + VTotal - 1;
				VRE2nd = VRS2nd + VSW;
			}else{
				VRS2nd = VRS + VTotal;
				VRE2nd = VRS2nd + VSW;
			}
		}

		it66121_write_reg(it66121,0x90, 0x06, Pol);
		// write H2ndVRRise
		it66121_write_reg(it66121,0x90, 0xF0, (H2ndVRRise&0x0F)<<4);
		it66121_write_reg_direct(it66121,0x91, (H2ndVRRise&0x0FF0)>>4);
		// write HRS/HRE
		it66121_write_reg_direct(it66121,0x95, HRS&0xFF);
		it66121_write_reg_direct(it66121,0x96, HRE&0xFF);
		it66121_write_reg_direct(it66121,0x97, ((HRE&0x0F00)>>4)+((HRS&0x0F00)>>8));
		// write VRS/VRE
		it66121_write_reg_direct(it66121,0xa0, VRS&0xFF);
		it66121_write_reg_direct(it66121,0xa1, ((VRE&0x0F)<<4)+((VRS&0x0F00)>>8));
		it66121_write_reg_direct(it66121,0xa2, VRS2nd&0xFF);
		it66121_write_reg_direct(it66121,0xa6, (VRE2nd&0xF0)+((VRE&0xF0)>>4));
		it66121_write_reg_direct(it66121,0xa3, ((VRE2nd&0x0F)<<4)+((VRS2nd&0xF00)>>8));
		it66121_write_reg_direct(it66121,0xa4, H2ndVRRise&0xFF);
		it66121_write_reg_direct(it66121,0xa5, (/*EnDEOnly*/0<<5)+((TimingTable[fmt_index].Scan==INTERLACE)?(1<<4):0)+((H2ndVRRise&0xF00)>>8));
		it66121_write_reg(it66121,0xb1, 0x51, ((HRE&0x1000)>>6)+((HRS&0x1000)>>8)+((HDES&0x1000)>>12));
		it66121_write_reg(it66121,0xb2, 0x05, ((H2ndVRRise&0x1000)>>10)+((H2ndVRRise&0x1000)>>12));
	}
	return 1 ;
}


void HDMITX_SetOutput(struct it66121_t *it66121)
{
	int inputColorMode=0;
	unsigned long TMDSClock;
	u8 level=0;
	u8 inputtype=0;
	DRV_PRINT("%s\r\n","Setting HDMI out...");
	it66121->Colorimetry = HDMI_ITU601;

#if 0
	aspec = HDMI_4x3 ;
#else
	aspec = HDMI_16x9 ;
#endif

	if (IT66121_SYNCEMBED == 1){
		inputtype = T_MODE_SYNCEMB;
	}else{
		inputtype = 0;
	}

	inputColorMode = F_MODE_YUV422;

	TMDSClock = it66121->VideoPixelClock*(it66121->pixelrep+1);

	if( TMDSClock>80000000L ){
		level = 2 ;
	}else if(TMDSClock>20000000L){
		level = 1 ;
	}else{
		level = 0 ;
	}

	setHDMITX_SyncEmbeddedByVIC(it66121,it66121->VIC,inputtype);

	HDMITX_EnableVideoOutput(it66121,inputColorMode,F_MODE_YUV422,level,inputtype);

	ConfigAVIInfoFrame(it66121,it66121->VIC, it66121->pixelrep,F_MODE_YUV422);

	setHDMITX_AVMute(it66121,0);
}

void hdmitx_AbortDDC(struct it66121_t *it66121)
{
	u8 CPDesire,SWReset,DDCMaster ;
	u8 uc, timeout, i ;
	// save the SW reset,DDC master,and CP Desire setting.
	SWReset = it66121_read_reg(it66121,REG_TX_SW_RST);
	CPDesire = it66121_read_reg(it66121,REG_TX_HDCP_DESIRE);
	DDCMaster = it66121_read_reg(it66121,REG_TX_DDC_MASTER_CTRL);

	it66121_write_reg_direct(it66121,REG_TX_HDCP_DESIRE,CPDesire&(~B_TX_CPDESIRE)); // @emily change order
	it66121_write_reg_direct(it66121,REG_TX_SW_RST,SWReset|B_TX_HDCP_RST_HDMITX);		 // @emily change order
	it66121_write_reg_direct(it66121,REG_TX_DDC_MASTER_CTRL,B_TX_MASTERDDC|B_TX_MASTERHOST);

	// 2009/01/15 modified by Jau-Chih.Tseng@ite.com.tw
	// do abort DDC twice.
	for( i = 0 ; i < 2 ; i++ ){
		it66121_write_reg_direct(it66121,REG_TX_DDC_CMD,CMD_DDC_ABORT);

		for( timeout = 0 ; timeout < 200 ; timeout++ ){
			uc = it66121_read_reg(it66121,REG_TX_DDC_STATUS);
			if (uc&B_TX_DDC_DONE){
				break ; // success
			}
			if( uc & (B_TX_DDC_NOACK|B_TX_DDC_WAITBUS|B_TX_DDC_ARBILOSE) ){
				break ;
			}
			msleep(1);
		}
	}
}

void hdmitx_ClearDDCFIFO(struct it66121_t *it66121)
{
	it66121_write_reg_direct(it66121,REG_TX_DDC_MASTER_CTRL,B_TX_MASTERDDC|B_TX_MASTERHOST);
	it66121_write_reg_direct(it66121,REG_TX_DDC_CMD,CMD_FIFO_CLR);
}

u8 getHDMITX_EDIDBytes(struct it66121_t *it66121,u8 *pData,u8 bSegment,u8 offset,u8 Count)
{
	u8 RemainedCount,ReqCount ;
	u8 bCurrOffset ;
	u8 TimeOut ;
	u8 *pBuff = pData ;
	u8 ucdata ;

	if(!pData){
		return -1 ;
	}
	if(it66121_read_reg(it66121,REG_TX_INT_STAT1) & B_TX_INT_DDC_BUS_HANG){
		HDMITX_DEBUG_PRINTF(("Called hdmitx_AboutDDC()\n"));
		hdmitx_AbortDDC(it66121);
	}

	hdmitx_ClearDDCFIFO(it66121);

	RemainedCount = Count ;
	bCurrOffset = offset ;

	Switch_HDMITX_Bank(0);

	while(RemainedCount > 0){
		ReqCount = (RemainedCount > DDC_FIFO_MAXREQ)?DDC_FIFO_MAXREQ:RemainedCount ;
		HDMITX_DEBUG_PRINTF(("getHDMITX_EDIDBytes(): ReqCount = %d,bCurrOffset = %d\n",(int)ReqCount,(int)bCurrOffset));

		it66121_write_reg_direct(it66121,REG_TX_DDC_MASTER_CTRL,B_TX_MASTERDDC|B_TX_MASTERHOST);
		it66121_write_reg_direct(it66121,REG_TX_DDC_CMD,CMD_FIFO_CLR);

		for(TimeOut = 0 ; TimeOut < 200 ; TimeOut++){
			ucdata = it66121_read_reg(it66121,REG_TX_DDC_STATUS);

			if(ucdata&B_TX_DDC_DONE){
				break ;
			}
			if((ucdata & B_TX_DDC_ERROR)||(it66121_read_reg(it66121,REG_TX_INT_STAT1) & B_TX_INT_DDC_BUS_HANG)){
				HDMITX_DEBUG_PRINTF(("Called hdmitx_AboutDDC()\n"));
				hdmitx_AbortDDC(it66121);
				return -1 ;
			}
		}
		it66121_write_reg_direct(it66121,REG_TX_DDC_MASTER_CTRL,B_TX_MASTERDDC|B_TX_MASTERHOST);
		it66121_write_reg_direct(it66121,REG_TX_DDC_HEADER,DDC_EDID_ADDRESS); // for EDID ucdata get
		it66121_write_reg_direct(it66121,REG_TX_DDC_REQOFF,bCurrOffset);
		it66121_write_reg_direct(it66121,REG_TX_DDC_REQCOUNT,(u8)ReqCount);
		it66121_write_reg_direct(it66121,REG_TX_DDC_EDIDSEG,bSegment);
		it66121_write_reg_direct(it66121,REG_TX_DDC_CMD,CMD_EDID_READ);

		bCurrOffset += ReqCount ;
		RemainedCount -= ReqCount ;

		for(TimeOut = 250 ; TimeOut > 0 ; TimeOut --){
			msleep(1);
			ucdata = it66121_read_reg(it66121,REG_TX_DDC_STATUS);
			if(ucdata & B_TX_DDC_DONE){
				break ;
			}
			if(ucdata & B_TX_DDC_ERROR){
				HDMITX_DEBUG_PRINTF(("getHDMITX_EDIDBytes(): DDC_STATUS = %02X,fail.\n",(int)ucdata));
				return -1 ;
			}
		}
		if(TimeOut == 0){
			HDMITX_DEBUG_PRINTF(("getHDMITX_EDIDBytes(): DDC TimeOut. \n"));
			return -1 ;
		}
		do{
			*(pBuff++) = it66121_read_reg(it66121,REG_TX_DDC_READFIFO);
			ReqCount -- ;
		}while(ReqCount > 0);

	}
	return 0 ;
}


u8 getHDMITX_EDIDBlock(struct it66121_t *it66121,int EDIDBlockID,u8 *pEDIDData)
{
	if(!pEDIDData){
		return 0 ;
	}
	if(getHDMITX_EDIDBytes(it66121,pEDIDData,EDIDBlockID/2,(EDIDBlockID%2)*128,128) == -1){
		return 0 ;
	}
	return 1 ;
}

u8 ParseCEAEDID(u8 *pCEAEDID)
{
	u8 offset,End ;
	u8 count ;
	u8 tag ;
	int i ;

	if( pCEAEDID[0] != 0x02 || pCEAEDID[1] != 0x03 ) return 0 ; // not a CEA BLOCK.
	End = pCEAEDID[2]  ; // CEA description.

	RxCapability.VDOMode[0] = 0x00 ;
	RxCapability.VDOMode[1] = 0x00 ;
	RxCapability.VDOMode[2] = 0x00 ;
	RxCapability.VDOMode[3] = 0x00 ;
	RxCapability.VDOMode[4] = 0x00 ;
	RxCapability.VDOMode[5] = 0x00 ;
	RxCapability.VDOMode[6] = 0x00 ;
	RxCapability.VDOMode[7] = 0x00 ;
	RxCapability.PA[0] = 0x00 ;
	RxCapability.PA[1] = 0x00 ;

	RxCapability.VideoMode = pCEAEDID[3] ;

	RxCapability.NativeVDOMode = 0xff ;

	for( offset = 4 ; offset < End ; ){
		tag = pCEAEDID[offset] >> 5 ;
		count = pCEAEDID[offset] & 0x1f ;
		switch( tag ){
		case 0x01: // Audio Data Block ;
			RxCapability.AUDDesCount = count/3 ;
			HDMITX_DEBUG_PRINTF(("RxCapability.AUDDesCount = %d\n",(int)RxCapability.AUDDesCount));
			offset++ ;
			for( i = 0 ; i < RxCapability.AUDDesCount && i < MAX_AUDDES_COUNT ; i++ ){
				RxCapability.AUDDes[i].uc[0] = pCEAEDID[offset+i*3] ;
				RxCapability.AUDDes[i].uc[1] = pCEAEDID[offset+i*3+1] ;
				RxCapability.AUDDes[i].uc[2] = pCEAEDID[offset+i*3+2] ;
			}
			offset += count ;
			break ;

		case 0x02: // Video Data Block ;
			offset ++ ;
			for( i = 0,RxCapability.NativeVDOMode = 0xff ; i < count ; i++){
				u8 VIC ;
				VIC = pCEAEDID[offset+i] & (~0x80);
				// if( FindModeTableEntryByVIC(VIC) != -1 )
				if(VIC<64){
					RxCapability.VDOMode[VIC/8] |= (1<<(VIC%8));
					HDMITX_DEBUG_PRINTF(("VIC = %d, RxCapability.VDOMode[%d]=%02X\n",(int)VIC,(int)VIC/8,(int)RxCapability.VDOMode[VIC/8] ));
					if(( pCEAEDID[offset+i] & 0x80 )&&(RxCapability.NativeVDOMode==0xFF)){
						RxCapability.NativeVDOMode = VIC ;
						HDMITX_DEBUG_PRINTF(("native = %d\n",RxCapability.NativeVDOMode));
					}
				}
			}
			offset += count ;
			break ;

		case 0x03: // Vendor Specific Data Block ;
			offset ++ ;
			RxCapability.IEEEOUI = (int)pCEAEDID[offset+2] ;
			RxCapability.IEEEOUI <<= 8 ;
			RxCapability.IEEEOUI += (int)pCEAEDID[offset+1] ;
			RxCapability.IEEEOUI <<= 8 ;
			RxCapability.IEEEOUI += (int)pCEAEDID[offset] ;
			HDMITX_DEBUG_PRINTF(("IEEEOUI = %02X %02X %02X %x",(int)pCEAEDID[offset+2],(int)pCEAEDID[offset+1],(int)pCEAEDID[offset],RxCapability.IEEEOUI));
			if( RxCapability.IEEEOUI== 0x0C03){
				u8 nextoffset ;
				RxCapability.PA[0] = pCEAEDID[offset+3] ;
				RxCapability.PA[1] = pCEAEDID[offset+4] ;
				if(count>5){
					RxCapability.dc.uc = pCEAEDID[offset+5]&0x70;
				}
				if(count>6){
					RxCapability.MaxTMDSClock = pCEAEDID[offset+6];
				}
				if(count>7){
					nextoffset = 8 ;
					if(pCEAEDID[offset+7] & 0x80) { nextoffset += 2 ; }  // latency
					if(pCEAEDID[offset+7] & 0x40) { nextoffset += 2 ; }  // interlace latency
					if(pCEAEDID[offset+7] & 0x20) {
						HDMITX_DEBUG_PRINTF(("next offset = %d",(int)nextoffset));
						RxCapability.Valid3D = (pCEAEDID[offset+nextoffset] & 0x80)?1:0 ;
					}  // interlace latency
				}
			}
			offset += count ; // ignore the remaind.

			break ;

		case 0x04: // Speaker Data Block ;
			offset ++ ;
			RxCapability.SpeakerAllocBlk.uc[0] = pCEAEDID[offset] ;
			RxCapability.SpeakerAllocBlk.uc[1] = pCEAEDID[offset+1] ;
			RxCapability.SpeakerAllocBlk.uc[2] = pCEAEDID[offset+2] ;
			offset += 3 ;
			break ;
		case 0x05: // VESA Data Block ;
			offset += count+1 ;
			break ;
		case 0x07: // Extended Data Block ;
			offset += count+1 ; //ignore
			break ;
		default:
			offset += count+1 ; // ignore
		}
	}
	RxCapability.ValidCEA = 1 ;
	return 1 ;
}

u8 ParseEDID(struct it66121_t *it66121)
{
	unsigned char *EDID_Buf;
	u8 CheckSum ;
	u8 BlockCount ;
	u8 err = 0 ;
	u8 bValidCEA = 0 ;
	u8 i;

	EDID_Buf = CommunBuff;
	RxCapability.ValidCEA = 0 ;
	RxCapability.ValidHDMI = 0 ;
	RxCapability.dc.uc = 0;

	getHDMITX_EDIDBlock(it66121,0, EDID_Buf);

	for( i = 0, CheckSum = 0 ; i < 128 ; i++ ){
		CheckSum += EDID_Buf[i] ; CheckSum &= 0xFF ;
	}
	if( CheckSum != 0 ){
		DRV_PRINT("!!!!!!! checkSum != 0 !!!!!\r\n");
		return 0 ;
	}
	if( EDID_Buf[0] != 0x00 ||
		EDID_Buf[1] != 0xFF ||
		EDID_Buf[2] != 0xFF ||
		EDID_Buf[3] != 0xFF ||
		EDID_Buf[4] != 0xFF ||
		EDID_Buf[5] != 0xFF ||
		EDID_Buf[6] != 0xFF ||
		EDID_Buf[7] != 0x00){
		DRV_PRINT("!!!!!!!!!buffer is wrong !!!!!!\r\n");
		return 0 ;
	}

	BlockCount = EDID_Buf[0x7E] ;

	if( BlockCount == 0 ){
		return 1 ; // do nothing.
	}else if ( BlockCount > 4 ){
		BlockCount = 4 ;
	}
	 // read all segment for test
	for( i = 1 ; i <= BlockCount ; i++ ){
		err = getHDMITX_EDIDBlock(it66121,i, EDID_Buf);

		if( err ){
			if( !bValidCEA && EDID_Buf[0] == 0x2 && EDID_Buf[1] == 0x3 ){
				err = ParseCEAEDID(EDID_Buf);
				HDMITX_DEBUG_PRINTF(("err = %s\n",err?"SUCCESS":"FAIL"));
				if( err ){
					HDMITX_DEBUG_PRINTF(("RxCapability.IEEEOUI = %x\n",RxCapability.IEEEOUI));
					if(RxCapability.IEEEOUI==0x0c03){
						RxCapability.ValidHDMI = 1 ;
						bValidCEA = 1 ;
					}else{
						RxCapability.ValidHDMI = 0 ;
					}
				}
			}
		}
	}
	return err ;
}

#define DIFF(a,b) (((a)>(b))?((a)-(b)):((b)-(a)))
u64 CalcPCLK(struct it66121_t *it66121)
{
	u8 uc, div ;
	int i ;
	unsigned int sum , count, PCLK   ;

	Switch_HDMITX_Bank(0);
	uc = it66121_read_reg(it66121,0x5F) & 0x80 ;

	if( ! uc ){
		return 0 ;
	}

	it66121_write_reg(it66121,0xD7, 0xF0, 0x80);
	msleep(1);
	it66121_write_reg(it66121,0xD7, 0x80, 0x00);

	count = it66121_read_reg(it66121,0xD7) & 0xF ;
	count <<= 8 ;
	count |= it66121_read_reg(it66121,0xD8);

	for( div = 7 ; div > 0 ; div-- ){
	    // printf("div = %d\n",(int)div) ;
		if(count < (1<<(11-div)) ){
			break ;
		}
	}
	it66121_write_reg(it66121,0xD7, 0x70, div<<4);

    uc = it66121_read_reg(it66121,0xD7) & 0x7F ;
	for( i = 0 , sum = 0 ; i < 100 ; i ++ ){
		it66121_write_reg_direct(it66121,0xD7, uc|0x80) ;
		msleep(1);
		it66121_write_reg_direct(it66121,0xD7, uc) ;

		count = it66121_read_reg(it66121,0xD7) & 0xF ;
		count <<= 8 ;
		count |= it66121_read_reg(it66121,0xD8);
		sum += count ;
	}
	sum /= 100 ; count = sum ;

	HDMITX_DEBUG_PRINTF(("RCLK(in GetPCLK) = %lld\n",it66121->RCLK));
	HDMITX_DEBUG_PRINTF(("div = %d, count = %d\n",(int)div,(int)count) );
	HDMITX_DEBUG_PRINTF(("count = %d\n",count) );

	//PCLK = it66121->RCLK * 128 / count * 16 ;
	PCLK = it66121->RCLK *128;
	AMBCLK_DO_DIV_ROUND(PCLK,(count*16));
	PCLK *= (1<<div);

	if( it66121_read_reg(it66121,0x70) & 0x10 ){
		//PCLK /= 2 ;
		AMBCLK_DO_DIV_ROUND(PCLK,2);
	}

	HDMITX_DEBUG_PRINTF(("PCLK = %d\n",PCLK) );
	return PCLK ;
}

int hdmitx_getInputHTotal(struct it66121_t *it66121)
{
    u8 uc;
    int hTotal;
    it66121_write_reg(it66121,0x0F,1,0);
    it66121_write_reg(it66121,0xA8,8,8);

    uc = it66121_read_reg(it66121,0xB2);
    hTotal = (uc&1)?(1<<12):0;
    uc = it66121_read_reg(it66121,0x91);
    hTotal |= ((int)uc)<<4;
    uc = it66121_read_reg(it66121,0x90);
    hTotal |= (uc&0xF0) >> 4;
    it66121_write_reg(it66121,0xA8,8,0);
    return hTotal;
}

int hdmitx_getInputVTotal(struct it66121_t *it66121)
{
    int uc ;
    int vTotal ;
    it66121_write_reg(it66121,0x0F,1,0);
    it66121_write_reg(it66121,0xA8,8,8);

    uc = it66121_read_reg(it66121,0x99);
    vTotal = ((int)uc&0xF)<<8;
    uc = it66121_read_reg(it66121,0x98);
    vTotal |= uc;
    it66121_write_reg(it66121,0xA8,8,0);
    return vTotal ;
}

u8 hdmitx_isInputInterlace(struct it66121_t *it66121)
{
    u8 uc ;

    it66121_write_reg(it66121,0x0F,1,0);
    it66121_write_reg(it66121,0xA8,8,8);

    uc = it66121_read_reg(it66121,0xA5);
    it66121_write_reg(it66121,0xA8,8,0);
    return uc&(1<<4)?1:0 ;
}

int HDMITX_SearchVICIndex( u64 PCLK, int HTotal, int VTotal, u8 ScanMode )
{
    #define SEARCH_COUNT 4
    unsigned long  pclkDiff;
    int i;
    u8 hit;
    int iMax[SEARCH_COUNT]={0};
    u8 hitMax[SEARCH_COUNT]={0};
    u8 i2;

    for( i = 0 ; i < SizeofVMTable; i++ ){
        if( s_VMTable[i].VIC == 0 ) break ;
        hit=0;
        if( ScanMode == s_VMTable[i].ScanMode ){
            hit++;

            if( ScanMode == INTERLACE ){
                if( DIFF(VTotal/2, s_VMTable[i].VTotal) < 10 ){
                    hit++;
                }
            }else{
                if( DIFF(VTotal, s_VMTable[i].VTotal) < 10 ){
                    hit++;
                }
            }

            if( hit == 2 ){
                if( DIFF(HTotal, s_VMTable[i].HTotal) < 40 ){
                    hit++;

                    pclkDiff = DIFF(PCLK, s_VMTable[i].PCLK);
                   // pclkDiff = (pclkDiff * 100) / s_VMTable[i].PCLK;
                   pclkDiff *= 100;
				   AMBCLK_DO_DIV_ROUND(pclkDiff,s_VMTable[i].PCLK);

                    if( pclkDiff < 100 ){
                        hit += ( 100 - pclkDiff );
                    }
                }
            }
        }

        HDMITX_DEBUG_PRINTF(("i = %d, hit = %d\n",i,(int)hit));

        if( hit ){
            for( i2=0 ; i2<SEARCH_COUNT ; i2++ ){
                if( hitMax[i2] < hit ){
                    HDMITX_DEBUG_PRINTF(("replace iMax[%d] = %d => %d\n",(int)i2, iMax[i2], i ));
                    hitMax[i2] = hit;
                    iMax[i2]=i;
                    break;
                }
            }
        }
    }

    i=-1;
    hit=0;
    for( i2=0 ; i2<SEARCH_COUNT ; i2++ ){
        HDMITX_DEBUG_PRINTF(("[%d] i = %d, hit = %d\n",(int)i2, iMax[i2],(int)hitMax[i2]));
        if( hitMax[i2] > hit ){
            hit = hitMax[i2];
            i = iMax[i2];
        }
    }

    if( hit > 2 ){
		HDMITX_DEBUG_PRINTF(("i = %d, hit = %d\n",i,(int)hit));
		HDMITX_DEBUG_PRINTF((">> mode : %d %lld x %lld @%lld (%s)\n",
			(int)s_VMTable[i].VIC, s_VMTable[i].HActive, s_VMTable[i].VActive, s_VMTable[i].PCLK, (s_VMTable[i].ScanMode==0)?"i":"p" ));
    }else{
        i=-1;
        HDMITX_DEBUG_PRINTF(("no matched\n"));
    }

    return i;
}


void HDMITX_MonitorInputVideoChange(struct it66121_t *it66121)
{
    static u64 prevPCLK = 0 ;
    static int prevHTotal = 0 ;
    static int prevVTotal = 0 ;
    static u8 prevScanMode ;
    u64 currPCLK,temp;
    u64 diff ;
    int currHTotal, currVTotal ;
    u8 currScanMode ;
	int i ;

    currPCLK = CalcPCLK(it66121) ;
    currHTotal = hdmitx_getInputHTotal(it66121) ;
    currVTotal = hdmitx_getInputVTotal(it66121) ;
    currScanMode = hdmitx_isInputInterlace(it66121) ? INTERLACE:PROG ;
    diff = DIFF(currPCLK,prevPCLK);

	HDMITX_DEBUG_PRINTF(("HDMITX_MonitorInputVideoChange : pclk=%lld, ht=%u, vt=%u, dif=%lld\n",
		currPCLK, currHTotal, currVTotal, diff ));

    if( currHTotal == 0 || currVTotal == 0 || currPCLK == 0 ){
        it66121->bChangeMode = 0;
		return ;
    }

	temp = currPCLK;
	AMBCLK_DO_DIV_ROUND(temp,20);
    if( diff > temp){
        it66121->bChangeMode = 1 ;
    }else{
        diff = DIFF(currHTotal, prevHTotal) ;
        if( diff > 20 ){
            it66121->bChangeMode = 1 ;
        }
        diff = DIFF(currVTotal, prevVTotal) ;
        if( diff > 20 ){
            it66121->bChangeMode = 1 ;
        }
    }

    if( it66121->bChangeMode ){
		HDMITX_DEBUG_PRINTF(("PCLK = %lld -> %lld\n",prevPCLK, currPCLK));
        HDMITX_DEBUG_PRINTF(("HTotal = %d -> %d\n",prevHTotal, currHTotal));
        HDMITX_DEBUG_PRINTF(("VTotal = %d -> %d\n",prevVTotal, currVTotal));
        HDMITX_DEBUG_PRINTF(("ScanMode = %s -> %s\n",prevScanMode?"P":"I", currScanMode?"P":"I"));

		HDMITX_DEBUG_PRINTF(("PCLK = %lld,(%dx%d) %s %s\n",currPCLK, currHTotal,currVTotal, (currScanMode==INTERLACE)?"INTERLACED":"PROGRESS",it66121->bChangeMode?"CHANGE MODE":"NO CHANGE MODE"));

        setHDMITX_AVMute(it66121,1);


        i = HDMITX_SearchVICIndex( currPCLK, currHTotal, currVTotal, currScanMode );

        if( i >= 0 ){
            it66121->VIC = s_VMTable[i].VIC;
            it66121->pixelrep = s_VMTable[i].PixelRep ;
            it66121->VideoPixelClock = currPCLK ;
        }else{
            it66121->VIC = 0;
            it66121->pixelrep = 0;
            it66121->VideoPixelClock = 0 ;
        }
    }

    prevPCLK = currPCLK ;
    prevHTotal = currHTotal ;
    prevVTotal = currVTotal ;
    prevScanMode = currScanMode ;

}

static void handle_hdmi(struct work_struct *work)
{
	u8 pHPD,pHPDChange;
	static int param=0;
	struct it66121_t *it66121 = container_of(work,struct it66121_t,work);

	it66121->bChangeMode = 1;
	for(;it66121->run;){
		it66121_check(it66121,&pHPD,&pHPDChange);
		if (param != IT66121_SYNCEMBED){
			it66121->bChangeMode = 1;
			param = IT66121_SYNCEMBED;
		}
		if (pHPDChange){
			if( pHPD ){
				DRV_PRINT("HDMI connected\r\n");
				it66121_load_table(it66121,HDMITX_PwrOn_Table);
				ParseEDID(it66121);
				it66121->bChangeMode = 1;
			} else{
				// unplug mode, ...
				DRV_PRINT("HDMI unpluged\r\n");
				HDMITX_DisableVideoOutput(it66121);
				it66121_load_table(it66121,HDMITX_PwrDown_Table);
			}
		}else{
			HDMITX_MonitorInputVideoChange(it66121);
			if(pHPD && it66121->bChangeMode){
				HDMITX_SetOutput(it66121);
				it66121->bChangeMode = 0;
			}
		}
		msleep(500);
	}
	return;
}

static int it66121_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	struct it66121_t *it66121;
	u8 it66121_id[4]={0},i=0;

	it66121 = kzalloc(sizeof(struct it66121_t),GFP_KERNEL);
	if (it66121 == NULL){
		return -ENOMEM;
	}
	it66121->client = client;
	i2c_set_clientdata(client,it66121);
	DRV_PRINT("IT66121 id= ");
	for( i = 0;i<4;i++){
		it66121_id[i]=it66121_read_reg(it66121,i);
		DRV_PRINT("%x ",it66121_id[i]);
	}
	DRV_PRINT("\r\n");

	it66121_init_fun(it66121);
	INIT_WORK(&it66121->work,(void *)handle_hdmi);
	it66121->run = 1;
	schedule_work(&it66121->work);
	DRV_PRINT("IT66121 insmod success\r\n");
	return 0;
}

static int it66121_remove(struct i2c_client *client)
{
	struct it66121_t *it66121;
	it66121 = i2c_get_clientdata(client);
	it66121->run = 0;
	kfree(it66121);
	return 0;
}

static struct of_device_id it66121_of_match[] = {
	{ .compatible = "ambarella,it66121",},
	{},
};
MODULE_DEVICE_TABLE(of, it66121_of_match);


static const struct i2c_device_id it66121_idtable[] = {
	{ "it66121",0},
	{}
};
MODULE_DEVICE_TABLE(i2c,it66121_idtable);
static struct i2c_driver i2c_driver_it66121 = {
	.driver = {
		.name = "it66121",
		.owner = THIS_MODULE,
		.of_match_table = it66121_of_match,
	},
	.id_table	=	it66121_idtable,
	.probe	=	it66121_probe,
	.remove	=	it66121_remove,
};

static int __init it66121_init(void)
{
	int rval;

	rval = i2c_add_driver(&i2c_driver_it66121);
	if(rval < 0){
		DRV_PRINT("IT66121 i2c driver register failed\r\n");
		return rval;
	}
	DRV_PRINT("IT66121 inited\r\n");
	return rval;
}

static void __exit it66121_exit(void)
{
	i2c_del_driver(&i2c_driver_it66121);
	DRV_PRINT("IT66121 removed\r\n");
}

module_init(it66121_init);
module_exit(it66121_exit);

MODULE_DESCRIPTION("IT66121 digital to HDMI converter");
MODULE_AUTHOR("Johnson Diao,<cddiao@ambarella.com>");
MODULE_LICENSE("Proprietary");
