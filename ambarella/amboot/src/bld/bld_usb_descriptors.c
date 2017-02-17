/**
 * bld/bld_usb_descriptors.c
 *
 * History:
 *    2005/09/07 - [Arthur Yang] created file
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


#ifndef _DESCRIPTOR_C_
#define _DESCRIPTOR_C_

#include "descriptor.h"

/* get lower byte with 8bit for 16bit data */
#define LB16(data)  ((u8)((data) & 0x00ff))
/* get upper byte with 8bit for 16bit data */
#define HB16(data)  ((u8)(((data) & 0xff00)>>8))
#define DEVICE_DESC	0
#define CONFIG_HS	1
#define CONFIG_FS	2
#define DEVICE_QUA	3
#define STRING_0	4
#define STRING_1	5
#define BLD_STRING2	6
#define STRING_3	7


#if !defined (BLD_MSC)

/**********************
   Device descriptor 
 **********************/


ALIGN8 static USB_DEVICE_DESC uds_device_desc_bld =
{
    USB_DEV_LENGTH,               /* descriptor size             */
    USB_DEVICE_DESCRIPTOR,        /* descriptor type             */
    USB_DEV_USB_SPECIFICATION,    /* USB specification           */
    USB_DEV_CLASS,                /* class code                  */
    USB_DEV_SUBCLASS,             /* sub class code              */
    USB_DEV_PROTOCOL,             /* protocol code               */
    USB_EP_MAX_PACKET_SIZE_64,    /* max packet size for endpoint 0       */
    USB_DEV_VENDOR_ID,            /* vendor id                            */
    USB_DEV_PRODUCT_ID_PUD_BLD,   /* product id                           */
    USB_DEV_RELASE_NUMBER,        /* device release number                */
    1,                            /* manifacturer string id               */
    2,                            /* product string id                    */
    3,                            /* serial number string id              */
    USB_DEV_NUM_CONFIG            /* number of possible configuration     */
};

/********************
   Device qualifier
 ********************/
ALIGN8 static USB_DEVICE_QUALIFIER_DESC uds_device_qualifier =	
{
    USB_DEV_QUALIFIER_LENGTH,     /* descriptor size            */
    USB_DEVICE_QUALIFIER,         /* descriptor type            */
    USB_DEV_USB_SPECIFICATION,    /* USB specification          */
    USB_DEV_CLASS,                /* class code                 */
    USB_DEV_SUBCLASS,             /* sub class code             */
    USB_DEV_PROTOCOL,             /* protocol code                        */
    USB_EP_MAX_PACKET_SIZE_64,    /* max packet size for endpoint 0       */
    USB_DEV_NUM_CONFIG,           /* number of possible configuration     */
    0
};

/***********************************
    Configuration

    Configuration descriptor
        Interface descriptor
            endpoint descriptor
            .
            .
            
        class specific descriptors
        .
        .
 ***********************************/
/* for bootloader class */
ALIGN8 static u8 uds_bld_configuration1_hs[] =	
{
    /* configuration1 */
    USB_CFG_LENGTH,                 /* descriptor size           */
    USB_CONFIGURATION_DESCRIPTOR,   /* descriptor type           */
    LB16(USB_CFG_TOTAL_LENGTH),     /* total length              */
    HB16(USB_CFG_TOTAL_LENGTH),
    USB_CFG_NUMBER_OF_IF,           /* number of interface       */
    USB_CFG_VALUE,                  /* configuration value       */
    USB_CFG_IDX,                    /* configuration string id   */
    USB_CFG_CFG_ATTRIBUES,          /* characteristics           */
    USB_CFG_MAX_POWER,              /* maximum power in 2mA      */
        /* interface0 alt0 */
        USB_IF_LENGTH,              /* descriptor size          */
        USB_INTERFACE_DESCRIPTOR,   /* descriptor type          */
        USB_IF_IF0_NUMBER,          /* interface number         */
        USB_IF_ALT0,                /* alternate setting        */
        USB_IF_CFG_IF0_NUMBER_EP,   /* number of endpoint       */
        USB_IF0_CLASS_BLD,          /* interface class          */
        USB_IF0_SUBCLASS_BLD,       /* interface sub-class      */
        USB_IF0_PROTOCOL_BLD,       /* interface protocol       */
        USB_IF_IDX,                 /* interface string id      */
            /* endpoint descriptors */
            /* 1 */
            USB_EP_LENGTH,          /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,  /* descriptor type      */
            USB_EP_EP1_ADDRESS,       /* endpoint address     */
            USB_EP_ATR_BULK,          /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_512), /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_512),
            USB_EP_EP1_INTERVAL,              /* polling interval     */
            /* 2 */
            USB_EP_LENGTH,                    /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,          /* descriptor type      */
            USB_EP_EP2_ADDRESS,               /* endpoint address     */
            USB_EP_ATR_BULK,                  /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_512), /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_512),
            USB_EP_EP2_INTERVAL,              /* polling interval     */

            /* 3 */
            USB_EP_LENGTH,                    /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,          /* descriptor type      */
            USB_EP_EP3_ADDRESS,               /* endpoint address     */
            USB_EP_ATR_INTERRUPT,             /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),  /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP3_INTERVAL,              /* polling interval     */
            /* 4 */
            USB_EP_LENGTH,                    /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,          /* descriptor type      */
            USB_EP_EP4_ADDRESS,               /* endpoint address     */
            USB_EP_ATR_INTERRUPT,             /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),  /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP4_INTERVAL               /* polling interval     */

};

ALIGN8 static u8 uds_bld_configuration1_fs[] = 	
{
    /* configuration1 */
    USB_CFG_LENGTH,                  /* descriptor size          */
    USB_CONFIGURATION_DESCRIPTOR,    /* descriptor type          */
    LB16(USB_CFG_TOTAL_LENGTH),      /* total length             */
    HB16(USB_CFG_TOTAL_LENGTH),
    USB_CFG_NUMBER_OF_IF,            /* number of interface      */
    USB_CFG_VALUE,                   /* configuration value      */
    USB_CFG_IDX,                     /* configuration string id  */
    USB_CFG_CFG_ATTRIBUES,           /* characteristics          */
    USB_CFG_MAX_POWER,               /* maximum power in 2mA     */
        /* interface0 alt0 */
        
        USB_IF_LENGTH,               /* descriptor size          */
        USB_INTERFACE_DESCRIPTOR,    /* descriptor type          */
        USB_IF_IF0_NUMBER,           /* interface number         */
        USB_IF_ALT0,                 /* alternate setting        */
        USB_IF_CFG_IF0_NUMBER_EP,    /* number of endpoint       */
        USB_IF0_CLASS_BLD,           /* interface class          */
        USB_IF0_SUBCLASS_BLD,        /* interface sub-class      */
        USB_IF0_PROTOCOL_BLD,        /* interface protocol       */
        USB_IF_IDX,                  /* interface string id      */
            /* endpoint descriptors */
            /* 1 */
            USB_EP_LENGTH,           /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR, /* descriptor type      */
            USB_EP_EP1_ADDRESS,      /* endpoint address     */
            USB_EP_ATR_BULK,         /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),  /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP1_INTERVAL,     /* polling interval     */
            /* 2 */
            USB_EP_LENGTH,           /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR, /* descriptor type      */
            USB_EP_EP2_ADDRESS,      /* endpoint address     */
            USB_EP_ATR_BULK,         /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),       /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP2_INTERVAL,     /* polling interval     */

            /* 3 */
            USB_EP_LENGTH,           /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR, /* descriptor type      */
            USB_EP_EP3_ADDRESS,      /* endpoint address     */
            USB_EP_ATR_INTERRUPT,    /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),   /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP3_INTERVAL,               /* polling interval     */
            /* 4 */
            USB_EP_LENGTH,                     /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,           /* descriptor type      */
            USB_EP_EP4_ADDRESS,                /* endpoint address     */
            USB_EP_ATR_INTERRUPT,              /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),   /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP4_INTERVAL                /* polling interval     */

};

/*********************
   String descriptors
 *********************/
ALIGN8 static u8 uds_string_desc0[] = 
{
    4,                                  /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    0x09, 0x04                          /*  Primary/Sub LANGID              */
};

ALIGN8 static u8 uds_string_desc1[] =	
{
    10,                                 /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    'A', 0x00, 'M', 0x00, 'B', 0x00,    /* "AMBA"                           */
    'A', 0x00

};

/* for bootloader Class */
ALIGN8 static u8 uds_bld_string_desc2[] =
{
    56,                                 /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    'A', 0x00, 'm', 0x00, 'b', 0x00,    /* "Ambarella USB generic class"    */
    'a', 0x00, 'r', 0x00, 'e', 0x00,
    'l', 0x00, 'l', 0x00, 'a', 0x00,
    ' ', 0x00, 'U', 0x00, 'S', 0x00,
    'B', 0x00, ' ', 0x00, 'g', 0x00,
    'e', 0x00, 'n', 0x00, 'e', 0x00,
    'r', 0x00, 'i', 0x00, 'c', 0x00,
    ' ', 0x00, 'c', 0x00, 'l', 0x00,
    'a', 0x00, 's', 0x00, 's', 0x00,

};

ALIGN8 static u8 uds_string_desc3[] = 	
{
    26,                                 /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    '1', 0x00, '2', 0x00, '3', 0x00,    /*  "123456789ABC"                  */
    '4', 0x00, '5', 0x00, '6', 0x00,
    '7', 0x00, '8', 0x00, '9', 0x00,
    'A', 0x00, 'B', 0x00, 'C', 0x00

};

/*
=============================================
          Register descriptors
=============================================
*/

/* bootloader Class Descriptor */
static u8 *uds_bld_descriptors[] = 
{
    /* device descriptor */
    (u8 *)&uds_device_desc_bld,		/* device descriptor */
    /* configuration descriptor and descriptors follow this */
    uds_bld_configuration1_hs,
    uds_bld_configuration1_fs,

    (u8 *)&uds_device_qualifier, 	/* device qualifier */

    /* string descriptor */
    uds_string_desc0,
    uds_string_desc1,
    uds_bld_string_desc2,
    uds_string_desc3,

    NULL /* 0 length descriptor (terminator) */
};

#else

	/******************************
	 Mass Storage Class Descriptor
	*******************************/

ALIGN8 static USB_DEVICE_DESC uds_device_desc_bld_msc =
{
    USB_DEV_LENGTH,               /* descriptor size             */
    USB_DEVICE_DESCRIPTOR,        /* descriptor type             */
    USB_DEV_USB_SPECIFICATION,    /* USB specification           */
    USB_DEV_CLASS,            	  /* class code                  */
    USB_DEV_SUBCLASS,         	  /* sub class code              */
    USB_DEV_PROTOCOL,             /* protocol code               */
    USB_EP_MAX_PACKET_SIZE_64,    /* max packet size for endpoint 0       */
    USB_DEV_VENDOR_ID,            /* vendor id                            */
    USB_DEV_PRODUCT_ID_PUD_MSC,   /* product id                           */
    USB_DEV_RELASE_NUMBER,        /* device release number                */
    1,                            /* manifacturer string id               */
    2,                            /* product string id                    */
    3,                            /* serial number string id              */
    USB_DEV_NUM_CONFIG            /* number of possible configuration     */
};

/********************
   Device qualifier
 ********************/
ALIGN8 static USB_DEVICE_QUALIFIER_DESC uds_msc_device_qualifier =	
{
    USB_DEV_QUALIFIER_LENGTH,     /* descriptor size            */
    USB_DEVICE_QUALIFIER,         /* descriptor type            */
    USB_DEV_USB_SPECIFICATION,    /* USB specification          */
    USB_DEV_CLASS,            	  /* class code                 */
    USB_DEV_SUBCLASS,         	  /* sub class code             */
    USB_DEV_PROTOCOL,             /* protocol code                        */
    USB_EP_MAX_PACKET_SIZE_64,    /* max packet size for endpoint 0       */
    USB_DEV_NUM_CONFIG,           /* number of possible configuration     */
    0
};

/***********************************
    Configuration

    Configuration descriptor
        Interface descriptor
            endpoint descriptor
            .
            .
            
        class specific descriptors
        .
        .
 ***********************************/
/* for bootloader class */
ALIGN8 static u8 uds_bld_msc_configuration1_hs[] =	
{
    /* configuration1 */
    USB_CFG_LENGTH,                 /* descriptor size           */
    USB_CONFIGURATION_DESCRIPTOR,   /* descriptor type           */
    LB16(USB_CFG_TOTAL_LENGTH),     /* total length              */
    HB16(USB_CFG_TOTAL_LENGTH),
    USB_CFG_NUMBER_OF_IF,           /* number of interface       */
    USB_CFG_VALUE,                  /* configuration value       */
    USB_CFG_IDX,                    /* configuration string id   */
    USB_CFG_CFG_ATTRIBUES,          /* characteristics           */
    USB_CFG_MAX_POWER,              /* maximum power in 2mA      */
        /* interface0 alt0 */
        USB_IF_LENGTH,              /* descriptor size          */
        USB_INTERFACE_DESCRIPTOR,   /* descriptor type          */
        USB_IF_IF0_NUMBER,          /* interface number         */
        USB_IF_ALT0,                /* alternate setting        */
        USB_IF_CFG_IF0_NUMBER_EP,   /* number of endpoint       */
        USB_IF0_CLASS_MSC,          /* interface class          */
        USB_IF0_SUBCLASS_MSC,       /* interface sub-class      */
        USB_IF0_PROTOCOL_MSC,       /* interface protocol       */
        USB_IF_IDX,                 /* interface string id      */
            /* endpoint descriptors */
            /* 1 */
            USB_EP_LENGTH,          /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,  /* descriptor type      */
            USB_EP_EP1_ADDRESS,       /* endpoint address     */
            USB_EP_ATR_BULK,          /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_512), /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_512),
            USB_EP_EP1_INTERVAL,              /* polling interval     */
            /* 2 */
            USB_EP_LENGTH,                    /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,          /* descriptor type      */
            USB_EP_EP2_ADDRESS,               /* endpoint address     */
            USB_EP_ATR_BULK,                  /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_512), /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_512),
            USB_EP_EP2_INTERVAL,              /* polling interval     */
#if 0	/* no interrupt pipes */
            /* 3 */
            USB_EP_LENGTH,                    /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,          /* descriptor type      */
            USB_EP_EP3_ADDRESS,               /* endpoint address     */
            USB_EP_ATR_INTERRUPT,             /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),  /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP3_INTERVAL,              /* polling interval     */
            /* 4 */
            USB_EP_LENGTH,                    /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,          /* descriptor type      */
            USB_EP_EP4_ADDRESS,               /* endpoint address     */
            USB_EP_ATR_INTERRUPT,             /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),  /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP4_INTERVAL               /* polling interval     */
#endif
};

ALIGN8 static u8 uds_bld_msc_configuration1_fs[] = 	
{
    /* configuration1 */
    USB_CFG_LENGTH,                  /* descriptor size          */
    USB_CONFIGURATION_DESCRIPTOR,    /* descriptor type          */
    LB16(USB_CFG_TOTAL_LENGTH),      /* total length             */
    HB16(USB_CFG_TOTAL_LENGTH),
    USB_CFG_NUMBER_OF_IF,            /* number of interface      */
    USB_CFG_VALUE,                   /* configuration value      */
    USB_CFG_IDX,                     /* configuration string id  */
    USB_CFG_CFG_ATTRIBUES,           /* characteristics          */
    USB_CFG_MAX_POWER,               /* maximum power in 2mA     */
        /* interface0 alt0 */
        
        USB_IF_LENGTH,               /* descriptor size          */
        USB_INTERFACE_DESCRIPTOR,    /* descriptor type          */
        USB_IF_IF0_NUMBER,           /* interface number         */
        USB_IF_ALT0,                 /* alternate setting        */
        USB_IF_CFG_IF0_NUMBER_EP,    /* number of endpoint       */
        USB_IF0_CLASS_MSC,           /* interface class          */
        USB_IF0_SUBCLASS_MSC,        /* interface sub-class      */
        USB_IF0_PROTOCOL_MSC,        /* interface protocol       */
        USB_IF_IDX,                  /* interface string id      */
            /* endpoint descriptors */
            /* 1 */
            USB_EP_LENGTH,           /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR, /* descriptor type      */
            USB_EP_EP1_ADDRESS,      /* endpoint address     */
            USB_EP_ATR_BULK,         /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),  /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP1_INTERVAL,     /* polling interval     */
            /* 2 */
            USB_EP_LENGTH,           /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR, /* descriptor type      */
            USB_EP_EP2_ADDRESS,      /* endpoint address     */
            USB_EP_ATR_BULK,         /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),       /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP2_INTERVAL,     /* polling interval     */
#if 0	/* no interrupt pipes */
            /* 3 */
            USB_EP_LENGTH,           /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR, /* descriptor type      */
            USB_EP_EP3_ADDRESS,      /* endpoint address     */
            USB_EP_ATR_INTERRUPT,    /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),   /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP3_INTERVAL,               /* polling interval     */
            /* 4 */
            USB_EP_LENGTH,                     /* descriptor size      */
            USB_ENDPOINT_DESCRIPTOR,           /* descriptor type      */
            USB_EP_EP4_ADDRESS,                /* endpoint address     */
            USB_EP_ATR_INTERRUPT,              /* character address    */
            LB16(USB_EP_MAX_PACKET_SIZE_64),   /* max packet size      */
            HB16(USB_EP_MAX_PACKET_SIZE_64),
            USB_EP_EP4_INTERVAL                /* polling interval     */
#endif
};

/*********************
   String descriptors
 *********************/
ALIGN8 static u8 uds_msc_string_desc0[] = 	 
{
    4,                                  /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    0x09, 0x04                          /*  Primary/Sub LANGID              */
};

ALIGN8 static u8 uds_msc_string_desc1[] =	
{
    10,                                 /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    'e', 0x00, 'S', 0x00, 'O', 0x00,    /* "eSOL"                           */
    'L', 0x00

};

/* for bootloader Mass Storage Class */
ALIGN8 static u8 uds_bld_msc_string_desc2[] =	
{
    50,                                 /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    'P', 0x00, 'r', 0x00, 'U', 0x00,    /* "PrUSB Mass Storage Class"      */
    'S', 0x00, 'B', 0x00, ' ', 0x00,
    'M', 0x00, 'a', 0x00, 's', 0x00,
    's', 0x00, ' ', 0x00, 'S', 0x00,
    't', 0x00, 'o', 0x00, 'r', 0x00,
    'a', 0x00, 'g', 0x00, 'e', 0x00,
    ' ', 0x00, 'C', 0x00, 'l', 0x00,
    'a', 0x00, 's', 0x00, 's', 0x00,
};

ALIGN8 static u8 uds_msc_string_desc3[] = 	
{
    26,                                 /* size of String Descriptor        */
    USB_STRING_DESCRIPTOR,              /* String Descriptor type           */
    '1', 0x00, '2', 0x00, '3', 0x00,    /*  "123456789ABC"                  */
    '4', 0x00, '5', 0x00, '6', 0x00,
    '7', 0x00, '8', 0x00, '9', 0x00,
    'A', 0x00, 'B', 0x00, 'C', 0x00

};

/* bootloader Class Descriptor */
static u8 *uds_bld_msc_descriptors[] = 
{
    /* device descriptor */
    (u8 *)&uds_device_desc_bld_msc,	/* device descriptor */
    /* configuration descriptor and descriptors follow this */
    uds_bld_msc_configuration1_hs,
    uds_bld_msc_configuration1_fs,

    (u8 *)&uds_msc_device_qualifier, 	/* device qualifier */

    /* string descriptor */
    uds_msc_string_desc0,
    uds_msc_string_desc1,
    uds_bld_msc_string_desc2,
    uds_msc_string_desc3,

    NULL /* 0 length descriptor (terminator) */
};

#endif /* BLD_MSC */


#endif /* __DESCRIPTOR_C__ */
