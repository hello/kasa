/**
 * bld/usb/descriptor.h
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


#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#ifndef UCHAR
#define UCHAR u8
#endif

#ifndef USHORT
#define USHORT u16
#endif

#ifndef UINT
#define UINT u32
#endif

#define ALIGN4		__attribute__ ((aligned(4))) /* GNU_C compiler */
#define ALIGN8		__attribute__ ((aligned(8))) /* GNU_C compiler */
#define ALIGN16		__attribute__ ((aligned(16))) /* GNU_C compiler */

//++++++++++++++ from depdesc.h ++++++++++++++++++++
/*--------------------------------------
    String Descriptor Settings
--------------------------------------*/
/*---- number of String Descriptors ----*/
#define USB_STR_NUM_STRING          4

/*--------------------------------------
    Endpoint Descriptor Settings
--------------------------------------*/
/*---- bEndpointAddress ----*/
#define USB_EP_EP1_ADDRESS          (USB_EP_OUT_ADDRESS | 0x01)	
#define USB_EP_EP2_ADDRESS          (USB_EP_IN_ADDRESS  | 0x01)	
#define USB_EP_EP3_ADDRESS          (USB_EP_OUT_ADDRESS | 0x02)	
#define USB_EP_EP4_ADDRESS          (USB_EP_IN_ADDRESS  | 0x02)	

/*---- wMaxPacketSize ----*/
#define USB_EP_MAX_PACKET_SIZE_512  0x0200
#define USB_EP_MAX_PACKET_SIZE_64   0x40
#define USB_EP_MAX_PACKET_SIZE_32   0x20
#define USB_EP_MAX_PACKET_SIZE_16   0x10
#define USB_EP_MAX_PACKET_SIZE_08   0x08

/*---- bInterval ----*/
#define USB_EP_CONTROL_INTERVAL     0x00
#define USB_EP_BULK_INTERVAL        0x00
#define USB_EP_INTERRUPT_INTERVAL   0x10
#define USB_EP_ISO_INTERVAL         0x00

#define USB_EP_EP1_INTERVAL         USB_EP_BULK_INTERVAL
#define USB_EP_EP2_INTERVAL         USB_EP_BULK_INTERVAL
#define USB_EP_EP3_INTERVAL         USB_EP_INTERRUPT_INTERVAL
#define USB_EP_EP4_INTERVAL         USB_EP_INTERRUPT_INTERVAL

/*--------------------------------------
    Interface Descriptor Settings
--------------------------------------*/
/*---- bInterfaceNumber ----*/
#define USB_IF_IF0_NUMBER           0

/*---- bAlternateSettings ----*/
#define USB_IF_ALT0                 0

/*---- bNumEndpoints ----*/
#define USB_IF_CFG_IF0_NUMBER_EP    2	// cytsai: modify

/*---- bInterfaceClass ----*/
#define USB_IF0_CLASS_BLD        0xff
#define USB_IF0_CLASS_MSC        0x08

/*---- bInterfaceSubClass ----*/
#define USB_IF0_SUBCLASS_BLD     0xff
#define USB_IF0_SUBCLASS_MSC     0x06

/*---- bInterfaceProtocol ----*/
#define USB_IF0_PROTOCOL_BLD     0
#define USB_IF0_PROTOCOL_MSC     0x50

/*---- iInterface ----*/
#define USB_IF_IDX                  0

/*--------------------------------------
    Configuration Descriptor Settings
--------------------------------------*/
#define USB_CFG_NUMBER_OF_IF        1    /* number of interfaces          */
#define USB_CFG_VALUE               1    /* configuration value           */
#define USB_CFG_IDX                 0    /* configuration string id       */
#define USB_CFG_CFG_ATTRIBUES       0xc0 /* characteristics               */
#define USB_CFG_MAX_POWER           100  /* maximum power in 2mA          */
#define USB_CFG_TOTAL_LENGTH        USB_CFG_LENGTH +                         \
                                    (USB_IF_LENGTH * USB_CFG_NUMBER_OF_IF) + \
                                    (USB_EP_LENGTH * USB_IF_CFG_IF0_NUMBER_EP)


/*--------------------------------------
    device descriptor
--------------------------------------*/
#define USB_DEV_USB_SPECIFICATION   0x0200  /* USB specification      */
#define USB_DEV_RELEASE_NUMBER      0x0000  /* device release number  */
#define USB_DEV_CLASS               0x00    /* class code             */
#define USB_DEV_SUBCLASS            0x00    /* sub-class code         */
#define USB_DEV_MSC_CLASS           0x00    /* class code             */
#define USB_DEV_MSC_SUBCLASS        0x00    /* sub-class code         */
#define USB_DEV_PROTOCOL            0x00    /* protocol code                    */
#define USB_DEV_MAX_PACKET_SIZE0    USB_EP0_MAX_PACKET_SIZE
                                            /* max packet size for endpoint0*/
#define USB_DEV_VENDOR_ID           0x4255  /* vendor id             */
#define USB_DEV_PRODUCT_ID_PUD_BLD  0x0001  /* product id            */
#define USB_DEV_PRODUCT_ID_PUD_MSC  0x1000  /* product id            */
#define USB_DEV_RELASE_NUMBER       0x00    /* device release number */
#define USB_DEV_MANUFACTURER_IDX    0x01    /* manifacturer string id */
#define USB_DEV_PRODUCT_IDX         0x02    /* product string id      */
#define USB_DEV_SERIAL_NUMBER_IDX   0x03    /* serial number string id */
#define USB_DEV_NUM_CONFIG          1       /* number of possible configure */


/*====================================================
    desciptor relation definition
====================================================*/
/*---- number of configration ----*/
#define USB_NUM_CONFIG                  USB_DEV_NUM_CONFIG

/*---- number of interface ----*/
#define USB_NUM_INTERFACE               USB_CFG_NUMBER_OF_IF

/*---- number of string descriptor ----*/
#define USB_NUM_STRING_DESC             USB_STR_NUM_STRING

/*---- configuration descriptor total size ----*/
#define USB_CONFIG_DESC_TOTAL_SIZE      USB_CFG_TOTAL_LENGTH

/*---- default configuration number ----*/
#define USB_DEFAULT_CONFIG              1

//----		 end		--------------------

/*--------------------------------------
    USB Request Type
--------------------------------------*/
#define USB_DEV_REQ_TYPE_STANDARD       0x00    /* standard request         */
#define USB_DEV_REQ_TYPE_CLASS          0x01    /* class specific request   */
#define USB_DEV_REQ_TYPE_VENDER         0x02    /* vendor specific request  */
#define USB_DEV_REQ_TYPE_RESERVE        0x03    /* reserved                 */
#define USB_DEV_REQ_TYPE_TYPE           0x60
#define USB_DEV_REQ_DIRECTION           0x80
#define USB_DEV_REQ_TYPE_UNSUPPORTED    0xff    /* unsupported              */

/*--------------------------------------
    USB Standard Device Request
--------------------------------------*/
#define USB_GET_STATUS                  0   /* GetStatus request            */
#define USB_CLEAR_FEATURE               1   /* ClearFeature request         */
#define USB_SET_FEATURE                 3   /* SetFeature request           */
#define USB_SET_ADDRESS                 5   /* SetAddress request           */
#define USB_GET_DESCRIPTOR              6   /* GetDescriptor request        */
#define USB_SET_DESCRIPTOR              7   /* SetDescriptor request        */
#define USB_GET_CONFIGURATION           8   /* GetConfiguratoin request     */
#define USB_SET_CONFIGURATION           9   /* SetConfiguratoin request     */
#define USB_GET_INTERFACE              10   /* GetInterface request         */
#define USB_SET_INTERFACE              11   /* SetInterface request         */
#define USB_SYNCH_FRAME                12   /* SynchFrame request           */

/*--------------------------------------
    device request
--------------------------------------*/
#define USB_DEVICE_REQUEST_SIZE         8       /* device request size      */
#define USB_DEVICE_TO_HOST              0x80    /* device to host transfer  */
#define USB_HOST_TO_DEVICE              0x00    /* host to device transfer  */
#define USB_DEVICE                      0x00    /* request to device        */
#define USB_INTERFACE                   0x01    /* request to interface     */
#define USB_ENDPOINT                    0x02    /* request to endpoint      */
#define USB_CLASS                       0x20    /* class request            */
#define USB_VENDOR                      0x40    /* vendor request           */

/*--------------------------------------
    descriptor size
--------------------------------------*/
#define USB_DEV_LENGTH              0x12    /* device descriptor size       */
#define USB_CFG_LENGTH              0x09    /* config descriptor size       */
#define USB_IF_LENGTH               0x09    /* interface descriptor size    */
#define USB_EP_LENGTH               0x07    /* endpoint descriptor size     */
#define USB_DEV_QUALIFIER_LENGTH    0x0a
/*--------------------------------------
    endpoint address
--------------------------------------*/
#define USB_EP_IN_ADDRESS           0x80    /* IN endpoint address          */
#define USB_EP_OUT_ADDRESS          0x00    /* OUT endpoint address         */

/*--------------------------------------
    endpoint attribute
--------------------------------------*/
#define USB_EP_ATR_CONTROL          0x00    /* transfer mode : control      */
#define USB_EP_ATR_ISO              0x01    /* transfer mode : isochronous  */
#define USB_EP_ATR_BULK             0x02    /* transfer mode : bulk         */
#define USB_EP_ATR_INTERRUPT        0x03    /* transfer mode : interrupt    */

/*--------------------------------------
    device request
--------------------------------------*/

typedef struct {
    UCHAR       bmRequestType;              /* request type                 */
    UCHAR       bRequest;                   /* request                      */
    USHORT      wValue;                     /* value of bRequest defined    */
    USHORT      wIndex;                     /* value of bRequest defined    */
    USHORT      wLength;                    /* data length of data stage    */
} __attribute((packed)) USB_DEVICE_REQUEST;

/*--------------------------------------
    USB Feature Selector
--------------------------------------*/
#define USB_DEVICE_REMOTE_WAKEUP        1   /* remote wake up               */
#define USB_ENDPOINT_STALL              0   /* endpoint stall               */
#define USB_ENDPOINT_HALT               USB_ENDPOINT_STALL
/*--------------------------------------
    USB descriptor type
--------------------------------------*/
#define USB_DEVICE_DESCRIPTOR           1   /* device descriptor            */
#define USB_CONFIGURATION_DESCRIPTOR    2   /* configuraton descriptor      */
#define USB_STRING_DESCRIPTOR           3   /* string descriptor            */
#define USB_INTERFACE_DESCRIPTOR        4   /* interface descriptor         */
#define USB_ENDPOINT_DESCRIPTOR         5   /* endpoint descriptor          */
#define USB_DEVICE_QUALIFIER            6
#define USB_OTHER_SPEED_CONFIGURATION   7

/*--------------------------------------
    config descriptor definitions
--------------------------------------*/
#define USB_DEVDESC_ATB_BUS_POWER       0x80    /* bus power                */
#define USB_DEVDESC_ATB_SELF_POWER      0x40    /* self power               */
#define USB_DEVDESC_ATB_RMT_WAKEUP      0x20    /* remote wake up           */

/*--------------------------------------
    Device Descriptor
--------------------------------------*/
typedef  struct Device_Descriptor {
    UCHAR       bLength;            /* size of Device Descriptor            */
    UCHAR       bDescriptorType;    /* Device Dscriptor type                */
    USHORT      bcdUSB;             /* number of USB specifications         */
    UCHAR       bDeviceClass;       /* class code                           */
    UCHAR       bDeviceSubClass;    /* sub class code                       */
    UCHAR       bDeviceProtocol;    /* protocol code                        */
    UCHAR       bMaxPacketSize0;    /* max packt size of endpoint0          */
    USHORT      idVendor;           /* Vendor id                            */
    USHORT      idProduct;          /* Protocol id                          */
    USHORT      bcdDevice;          /* Device nmber                         */
    UCHAR       iManufacturer;      /* index of string Desc(maker)          */
    UCHAR       iProduct;           /* index of string Desc(products)       */
    UCHAR       iSerialNumber;      /* index of string Desc(serial number)  */
    UCHAR       bNumConfigurations; /* number for configration              */
} __attribute((packed)) USB_DEVICE_DESC;

/*--------------------------------------
    Device Qualifier
--------------------------------------*/
typedef  struct Device_Qualifier {
    UCHAR       bLength;            /* size of Device Descriptor            */
    UCHAR       bDescriptorType;    /* Device Dscriptor type                */
    USHORT      bcdUSB;             /* number of USB specifications         */
    UCHAR       bDeviceClass;       /* class code                           */
    UCHAR       bDeviceSubClass;    /* sub class code                       */
    UCHAR       bDeviceProtocol;    /* protocol code                        */
    UCHAR       bMaxPacketSize0;    /* max packt size of endpoint0          */
    UCHAR       bNumConfigurations; /* number for configration              */
    UCHAR       bReserved;
} __attribute((packed)) USB_DEVICE_QUALIFIER_DESC;

/*--------------------------------------
    Configuration Descriptor
--------------------------------------*/
typedef  struct Configuration_Descriptor {
    UCHAR       bLength;                /* size of Configuration Descriptor */
    UCHAR       bDescriptorType;        /* Configuration Descriptor type    */
    USHORT      wTotalLength;           /* all length of data               */
    UCHAR       bNumInterfaces;         /* number of interface              */
    UCHAR       bConfigurationValue;    /* value of argument                */
    UCHAR       iConfiguration;         /* index of string Descriptor       */
    UCHAR       bmAttributes;           /* characteristic of composition    */
    UCHAR       MaxPower;               /* max power consumption            */
} __attribute((packed)) USB_CONFIG_DESC;

/*--------------------------------------
    Other Speed Configuration
--------------------------------------*/
typedef  struct Other_Speed_Configuration {
    UCHAR       bLength;                /* size of Configuration Descriptor */
    UCHAR       bDescriptorType;        /* Configuration Descriptor type    */
    USHORT      wTotalLength;           /* all length of data               */
    UCHAR       bNumInterfaces;         /* number of interface              */
    UCHAR       bConfigurationValue;    /* value of argument                */
    UCHAR       iConfiguration;         /* index of string Descriptor       */
    UCHAR       bmAttributes;           /* characteristic of composition    */
    UCHAR       bMaxPower;              /* max power consumption            */
} __attribute((packed)) USB_OTHER_SPEED_CONFIG_DESC;

/*--------------------------------------
    Interface Descriptor
--------------------------------------*/
typedef  struct Interface_Descriptor {
    UCHAR       bLength;                /* size of Interface Descriptor     */
    UCHAR       bDescriptorType;        /* Interface Descriptor type        */
    UCHAR       bInterfaceNumber;       /* Interface number                 */
    UCHAR       bAlternateSetting;      /* value                            */
    UCHAR       bNumEndpoints;          /* endpoint number                  */
    UCHAR       bInterfaceClass;        /* class code                       */
    UCHAR       bInterfaceSubClass;     /* sub class code                   */
    UCHAR       bInterfaceProtocol;     /* protocol code                    */
    UCHAR       iInterface;             /* index of string Descriptor       */
} __attribute((packed)) USB_INTERFACE_DESC;

/*--------------------------------------
    Endpoint Descriptor
--------------------------------------*/
typedef  struct Endpoint_Descriptor {
    UCHAR       bLength;                /* size of Endpoint Descriptor      */
    UCHAR       bDescriptorType;        /* Endpoint Descriptor type         */
    UCHAR       bEndpointAddress;       /* Endpoint address                 */
    UCHAR       bmAttributes;           /* Endpoint attribute               */
    USHORT      wMaxPacketSize;         /* max packet size                  */
    UCHAR       bInterval;              /* interval                         */
} __attribute((packed)) USB_ENDPOINT_DESC;

/*--------------------------------------
    String Descriptor
--------------------------------------*/
typedef  struct String_Descriptor {
    UCHAR       bLength;                /* size of String Descriptor        */
    UCHAR       bDescriptorType;        /* String Descriptor type           */
    UCHAR       bString[254];           /* UNICODE stirng                   */
} __attribute((packed)) USB_STRING_DESC;

#endif /* __USB_H__ */
