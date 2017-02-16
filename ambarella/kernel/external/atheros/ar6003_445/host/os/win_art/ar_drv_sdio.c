/*
 *
 * Copyright (c) 2004-2009 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
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
 *
 */

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <string.h>
#include <malloc.h>
#include <initguid.h>

#include "athtypes_win.h"
#include "ntddsd_raw.h"
#include "hif.h"
#include "hif_internal.h"
#include "bmi.h"

#include "ar_drv.h"
#include "ar6xapi_win.h"
#include "htc.h"
#include "htc_internal.h"
#include "targaddrs.h"
#include "ar6k.h"
#include "export.h"
#include "utils.h"
#include "addrs.h"

#ifdef DEBUG

#define  ATH_DEBUG_DBG_LOG       ATH_DEBUG_MAKE_MODULE_MASK(0)
#define  ATH_DEBUG_WLAN_CONNECT  ATH_DEBUG_MAKE_MODULE_MASK(1)
#define  ATH_DEBUG_WLAN_SCAN     ATH_DEBUG_MAKE_MODULE_MASK(2)
#define  ATH_DEBUG_WLAN_TX       ATH_DEBUG_MAKE_MODULE_MASK(3)
#define  ATH_DEBUG_WLAN_RX       ATH_DEBUG_MAKE_MODULE_MASK(4)
#define  ATH_DEBUG_HTC_RAW       ATH_DEBUG_MAKE_MODULE_MASK(5)
#define  ATH_DEBUG_HCI_BRIDGE    ATH_DEBUG_MAKE_MODULE_MASK(6)

static ATH_DEBUG_MASK_DESCRIPTION driver_debug_desc[] = {
    { ATH_DEBUG_DBG_LOG      , "Target Debug Logs"},
    { ATH_DEBUG_WLAN_CONNECT , "WLAN connect"},
    { ATH_DEBUG_WLAN_SCAN    , "WLAN scan"},
    { ATH_DEBUG_WLAN_TX      , "WLAN Tx"},
    { ATH_DEBUG_WLAN_RX      , "WLAN Rx"},
    { ATH_DEBUG_HTC_RAW      , "HTC Raw IF tracing"},
    { ATH_DEBUG_HCI_BRIDGE   , "HCI Bridge Setup"},
    { ATH_DEBUG_HCI_RECV     , "HCI Recv tracing"},
    { ATH_DEBUG_HCI_SEND     , "HCI Send tracing"},
    { ATH_DEBUG_HCI_DUMP     , "HCI Packet dumps"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(driver,
                                 "driver",
                                 "Linux Driver Interface",
                                 ATH_DEBUG_MASK_DEFAULTS | ATH_DEBUG_WLAN_SCAN | 
                                 ATH_DEBUG_HCI_BRIDGE,
                                 ATH_DEBUG_DESCRIPTION_COUNT(driver_debug_desc),
                                 driver_debug_desc);
                                 
#endif

static const TCHAR psVID_PID_Mercury[] = "sd\\vid_0271&pid_0201";
static const TCHAR psVID_PID_Dragon[] = "sd\\vid_0271&pid_010b";
static const TCHAR psVID_PID_Venus[] = "sd\\vid_0271&pid_0300";

int bmienable = 0;
unsigned int bypasswmi = 0;
unsigned int skipflash = 0;
unsigned int enabletimerwar = 0;
unsigned int fwmode = HI_OPTION_FW_MODE_BSS_STA;
unsigned int mbox_yield_limit = 99;
int reduce_credit_dribble = 1 + HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_ONE_HALF;
#ifdef CONFIG_HOST_TCMD_SUPPORT
unsigned int testmode =0;
#endif
unsigned int firmware_bridge = 0;

#ifdef DEBUG
A_UINT32 g_dbg_flags = DBG_DEFAULTS;
unsigned int debugflags = 0;
int debugdriver = 0;
unsigned int debughtc = 0;
unsigned int debugbmi = 0;
unsigned int debughif = 0;
unsigned int txcreditsavailable[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditsconsumed[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditintrenable[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditintrenableaggregate[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int setuphci = 0;
unsigned int ar3khcibaud = 0;
unsigned int hciuartscale = 0;
unsigned int hciuartstep = 0;
int debuglevel = 0;
#endif /* DEBUG */
unsigned int setupbtdev = 1;

unsigned int resetok = 1;
unsigned int tx_attempt[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int tx_post[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int tx_complete[HTC_MAILBOX_NUM_MAX] = {0};

#ifdef BLOCK_TX_PATH_FLAG
int blocktx = 0;
#endif /* BLOCK_TX_PATH_FLAG */

#define	MAX_ENDPOINTS		4
#define MAX_RX_BUF_SIZE	(AR6000_MAX_RX_BUFFERS * AR6000_BUFFER_SIZE)

typedef struct {
    HCI_TRANSPORT_CONFIG_INFO   HCIConfig;
    A_BOOL                      HCIAttached;
    A_BOOL                      HCIStopped;
    A_UINT32                    RecvStateFlags;
    A_UINT32                    SendStateFlags;
    HCI_TRANSPORT_PACKET_TYPE   WaitBufferType;
    HTC_PACKET_QUEUE            SendQueue;         /* write queue holding HCI Command and ACL packets */
    HTC_PACKET_QUEUE            HCIACLRecvBuffers;  /* recv queue holding buffers for incomming ACL packets */
    HTC_PACKET_QUEUE            HCIEventBuffers;    /* recv queue holding buffers for incomming event packets */
    AR6K_DEVICE                 *pDev;
    A_MUTEX_T                   HCIRxLock;
    A_MUTEX_T                   HCITxLock;
    int                         CreditsMax;
    int                         CreditsConsumed;
    int                         CreditsAvailable;
    int                         CreditSize;
    int                         CreditsCurrentSeek;
    int                         SendProcessCount;
} GMBOX_PROTO_HCI_UART;

volatile  int sRxBufIndex, eRxBufIndex;
HANDLE rxEvent;
int dk_htc_start = 0;
unsigned char rxBuffer[MAX_RX_BUF_SIZE];
int  rRxBuffers[MAX_ENDPOINTS];
static HANDLE g_handle = NULL;
unsigned int irqprocmode = HIF_DEVICE_IRQ_SYNC_ONLY;
//unsigned char htc_txBuffer[MAX_RX_BUF_SIZE];
unsigned char hci_rxBuffer[MAX_RX_BUF_SIZE];

// Extern functions
A_STATUS BMIFastDownload(HIF_DEVICE *device, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);
void target_register_tbl_attach(A_UINT32 target_type);
void hif_register_tbl_attach(A_UINT32 hif_type);

/* Function declarations */
static A_STATUS ar6000_init(int devIndex);
static int ar6000_data_tx(A_PBUF_T *pbuf, int devIndex);

static void ar6000_destroy(int devIndex, unsigned int unregister);

static void ar6000_cookie_init(AR_SOFTC_T *ar);
static void ar6000_cookie_cleanup(AR_SOFTC_T *ar);
static void ar6000_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie);
static struct ar_cookie *ar6000_alloc_cookie(AR_SOFTC_T *ar);

/*
 * HTC service connection handlers
 */
static A_STATUS ar6000_avail_ev(void *context, void *hif_handle);
static A_STATUS ar6000_unavail_ev(void *context, void *hif_handle);
static void ar6000_target_failure(void *Instance, A_STATUS Status);
static void ar6000_rx(void *Context, HTC_PACKET *pPacket);
static void ar6000_rx_refill(void *Context,HTC_ENDPOINT_ID Endpoint);
static void ar6000_tx_complete_multiple(void *Context, HTC_PACKET_QUEUE *pPackets);
static HTC_SEND_FULL_ACTION ar6000_tx_queue_full(void *Context, HTC_PACKET *pPacket);
static void ar6000_alloc_netbufs(A_NETBUF_QUEUE_T *q, A_UINT16 num);
static void ar6000_deliver_frames_to_nw_stack(HTC_ENDPOINT_ID eid, void *osbuf);
static A_STATUS HCIReadMessagePending(void *pContext, A_UINT8 LookAheadBytes[], int ValidBytes);

/*
 * Driver specific functions
 */
static A_BOOL AdapterOnOff(BOOL bEnable, TCHAR * pszGuid, TCHAR *pszDescription, GUID* FileGUID, DWORD RequiredSize);
static A_BOOL DumpDeviceWithInfo(HDEVINFO Devs, PSP_DEVINFO_DATA DevInfo, LPCTSTR Info);
static A_BOOL GetDeviceHandle(GUID Guid, HANDLE *phDevice);
static A_BOOL GetDevicePath(GUID Guid, PSP_DEVICE_INTERFACE_DETAIL_DATA *ppDeviceInterfaceDetail);
static TCHAR*  GetVidPidString(A_UINT32 subSystemID);

static HANDLE GetArDeviceHandle(int devIndex);
static int GetDeviceIndex (HANDLE hDevice);

/*
 * Static variables
 */
static void *ar6000_devices[MAX_AR6000];
static struct ar_cookie s_ar_cookie_mem[MAX_COOKIE_NUM];
static arSubSystemID;

#define AR6000_GET(devIndex) ((AR_SOFTC_T *)ar6000_devices[devIndex])

/* Debug log support */

/*
 * Flag to govern whether the debug logs should be parsed in the kernel
 * or reported to the application.
 */

A_UINT32
dbglog_get_debug_hdr_ptr(AR_SOFTC_T *ar)
{
    A_UINT32 param;
    A_UINT32 address;
    A_STATUS status;

    address = TARG_VTOP(ar->arTargetType, HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_dbglog_hdr));
    if ((status = ar6000_ReadDataDiag(ar->arHifDevice, address,
                                      (A_UCHAR *)&param, 4)) != A_OK)
    {
        param = 0;
    }

    return param;
}

/*
 * The dbglog module has been initialized. Its ok to access the relevant
 * data stuctures over the diagnostic window.
 */
void
ar6000_dbglog_init_done(AR_SOFTC_T *ar)
{
    ar->dbglog_init_done = TRUE;
}

A_UINT32
dbglog_get_debug_fragment(A_INT8 *datap, A_UINT32 len, A_UINT32 limit)
{
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 numargs;
    A_UINT32 length;
    A_UINT32 fraglen;

    count = fraglen = 0;
    buffer = (A_INT32 *)datap;
    length = (limit >> 2);

    if (len <= limit) {
        fraglen = len;
    } else {
        while (count < length) {
            numargs = DBGLOG_GET_NUMARGS(buffer[count]);
            fraglen = (count << 2);
            count += numargs + 1;
        }
    }

    return fraglen;
}

void
dbglog_parse_debug_logs(A_INT8 *datap, A_UINT32 len)
{
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 timestamp;
    A_UINT32 debugid;
    A_UINT32 moduleid;
    A_UINT32 numargs;
    A_UINT32 length;

    count = 0;
    buffer = (A_INT32 *)datap;
    length = (len >> 2);
    while (count < length) {
        debugid = DBGLOG_GET_DBGID(buffer[count]);
        moduleid = DBGLOG_GET_MODULEID(buffer[count]);
        numargs = DBGLOG_GET_NUMARGS(buffer[count]);
        timestamp = DBGLOG_GET_TIMESTAMP(buffer[count]);
        switch (numargs) {
            case 0:
            AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("%d %d (%d)\n", moduleid, debugid, timestamp));
            break;

            case 1:
            AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("%d %d (%d): 0x%x\n", moduleid, debugid,
                            timestamp, buffer[count+1]));
            break;

            case 2:
            AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("%d %d (%d): 0x%x, 0x%x\n", moduleid, debugid,
                            timestamp, buffer[count+1], buffer[count+2]));
            break;

            default:
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Invalid args: %d\n", numargs));
        }
        count += numargs + 1;
    }
}

int
ar6000_dbglog_get_debug_logs(AR_SOFTC_T *ar)
{
    struct dbglog_hdr_s debug_hdr;
    struct dbglog_buf_s debug_buf;
    A_UINT32 address;
    A_UINT32 length;
    A_UINT32 dropped;
    A_UINT32 firstbuf;
    A_UINT32 debug_hdr_ptr;

    if (!ar->dbglog_init_done) return A_ERROR;


    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if (ar->dbgLogFetchInProgress) {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        return A_EBUSY;
    }

        /* block out others */
    ar->dbgLogFetchInProgress = TRUE;

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    debug_hdr_ptr = dbglog_get_debug_hdr_ptr(ar);
    printf("debug_hdr_ptr: 0x%x\n", debug_hdr_ptr);

    /* Get the contents of the ring buffer */
    if (debug_hdr_ptr) {
        address = TARG_VTOP(ar->arTargetType, debug_hdr_ptr);
        length = sizeof(struct dbglog_hdr_s);
        ar6000_ReadDataDiag(ar->arHifDevice, address,
                            (A_UCHAR *)&debug_hdr, length);
        address = TARG_VTOP(ar->arTargetType, (A_UINT32)debug_hdr.dbuf);
        firstbuf = address;
        dropped = debug_hdr.dropped;
        length = sizeof(struct dbglog_buf_s);
        ar6000_ReadDataDiag(ar->arHifDevice, address,
                            (A_UCHAR *)&debug_buf, length);

        do {
            address = TARG_VTOP(ar->arTargetType, (A_UINT32)debug_buf.buffer);
            length = debug_buf.length;
            if ((length) && (debug_buf.length <= debug_buf.bufsize)) {
                /* Rewind the index if it is about to overrun the buffer */
                if (ar->log_cnt > (DBGLOG_HOST_LOG_BUFFER_SIZE - length)) {
                    ar->log_cnt = 0;
                }
                if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address,
                                    (A_UCHAR *)&ar->log_buffer[ar->log_cnt], length))
                {
                    break;
                }
                ar6000_dbglog_event(ar, dropped, &ar->log_buffer[ar->log_cnt], length);
                ar->log_cnt += length;
            } else {
                AR_DEBUG_PRINTF(ATH_DEBUG_DBG_LOG,("Length: %d (Total size: %d)\n",
                                debug_buf.length, debug_buf.bufsize));
            }

            address = TARG_VTOP(ar->arTargetType, (A_UINT32)debug_buf.next);
            length = sizeof(struct dbglog_buf_s);
            if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address, (A_UCHAR *)&debug_buf, length))
            {
                break;
            }

        } while (address != firstbuf);
    }

    ar->dbgLogFetchInProgress = FALSE;

    return A_OK;
}

void
ar6000_dbglog_event(AR_SOFTC_T *ar, A_UINT32 dropped,
                    A_INT8 *buffer, A_UINT32 length)
{
    AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Dropped logs: 0x%x\nDebug info length: %d\n",
                    dropped, length));

    /* Interpret the debug logs */
    dbglog_parse_debug_logs(buffer, length);
}

A_UINT32 GetHIFType()
{
    switch(arSubSystemID){
        case 0x6041:
        case 0x6042:
        case 0x6043:
        case 0x6044:
        case 0x6045:
        case 0x6046:
        case 0x6047:
        case 0x6048:
        case 0x6049:
        case 0x6050:
        case 0x6051:
        case 0x6052:
        case 0x6053:
        case 0x6054:
        case 0x6099:
            return HIF_TYPE_AR6002;
        case 0x6301:
        case 0x6302:
        case 0x6303:
        case 0x6304:
        case 0x6305:
        case 0x6306:
        case 0x6307:
        case 0x6308:
        case 0x6309:
        case 0x6310:
            return HIF_TYPE_AR6003;
        default:
            return HIF_TYPE_MCKINLEY;
    }
}

A_BOOL SDIO_InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID)
{
    A_STATUS status;
    HANDLE hDevice;
    DWORD blockLength;
    DWORD clock;
    AR_SOFTC_T *ar;
    int i;
    char width = 0;
    int devIndex = 0;
    A_UINT32 param;

	//printf ("--- InitAR6000_ene - enter\n");
#ifdef DEBUG
    a_module_debug_support_init();

        /* check for debug mask overrides */   
    if (debughtc != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(htc,debughtc);    
    }    
    if (debugbmi != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(bmi,debugbmi);       
    }
    if (debughif != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(hif,debughif);       
    }
    if (debugdriver != 0) {
        ATH_DEBUG_SET_DEBUG_MASK(driver,debugdriver);       
    }
    
    A_REGISTER_MODULE_DEBUG_INFO(driver);
    
    //A_MEMZERO(&osdrvCallbacks,sizeof(osdrvCallbacks));
    //osdrvCallbacks.deviceInsertedHandler = ar6000_avail_ev;
    //osdrvCallbacks.deviceRemovedHandler = ar6000_unavail_ev;
  
    /* Set the debug flags if specified at load time */
    if(debugflags != 0)
    {
        g_dbg_flags = debugflags;
    }
#endif

    if(!GetDeviceHandle(GUID_DEVINTERFACE_SDRAW2, &hDevice))
    {
        if((!GetDeviceHandle(GUID_DEVINTERFACE_SDRAW, &hDevice)))
        {   
            printf("Failed to find handle\n");
            return FALSE;
        }
    }

    g_handle = hDevice;
    *handle = hDevice;
    printf("[yy]g_handle = 0x%x\n",g_handle);
    hif_register_tbl_attach(GetHIFType());
    addHifDevice(hDevice);

    status = HIFGetBlockLength(hDevice, &blockLength);
    
    if (status != A_OK)
    {
        printf("Failed HIFGetBlockLength\n");
        return FALSE;
    }
    else
    {
        printf("Get Block length: %d\n", blockLength);
    }

    //fei modified, to fix the issue of big data transfer failed from target to host, with Ellen card
    // ENE card do not had such issue
    if(blockLength > 512)
    {
        blockLength = 512;  
        status = HIFSetBlockLength(hDevice, blockLength);
        if (status != A_OK) {
            printf("Failed HIFSetBlockLength\n");
            return FALSE;
        } else {
            //printf("Block length: %d\n", blockLength);
        }

        status = HIFGetBlockLength(hDevice, &blockLength);
        if (status != A_OK) {
            printf("Failed HIFGetBlockLength\n");
            return FALSE;
        } else {
            printf("Set Block length: %d\n", blockLength);
        }
    }       
    //  status = HIFSetClock(hDevice,25000);  //dtan
    status = HIFSetClock(hDevice,20000);

    if (status != A_OK) {
        printf("Failed HIFSetClock\n");
        return FALSE;
    }   
        
    status = HIFGetClock(hDevice, &clock);
    if (status != A_OK) {
        printf("Failed HIFGetClock\n");
    } else {
        printf("Clock frequency: %d\n", clock);
    }

    status = HIFSetBusWidth(hDevice,1);
    if (status != A_OK) {
        printf("Failed HIFSetBusWidth\n");
        return FALSE;
    }       
    status = HIFGetBusWidth(hDevice, &width);
    if (status != A_OK) {
        printf("Failed HIFGetBusWidth\n");
        return FALSE;
    } else {
        printf("Bus Width: %d\n", width);
    }

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("ar6000_available\n"));

    for (i=0; i < MAX_AR6000; i++)
    {
        if (ar6000_devices[i] == NULL)
        {
            break;
        }
    }

    if (i == MAX_AR6000) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("ar6000_available: max devices reached\n"));
        return FALSE;
    }

    /* Save this. It gives a bit better readability especially since */
    /* we use another local "i" variable below.                      */
    devIndex = i;

    ar = A_MALLOC(sizeof(AR_SOFTC_T));
    if (ar== NULL) {
        printf("ar6000_available: Could not allocate memory\n");
        return FALSE;
    }
	//printf ("--- ar = 0x%x\n", ar);
    A_MEMZERO(ar, sizeof(AR_SOFTC_T));

    ar->arDeviceHandle  = g_handle;           
    ar->arHifDevice     = getHifDevice(g_handle);
    ar->arDeviceIndex   = devIndex;
	
    /*
     * If requested, perform some magic which requires no cooperation from
     * the Target.  It causes the Target to ignore flash and execute to the
     * OS from ROM.
     *
     * This is intended to support recovery from a corrupted flash on Targets
     * that support flash.
     */
    //if (skipflash)
    //{
    //    ar6000_reset_device_skipflash(ar->arHifDevice);
    //}

    BMIInit();
    {
        struct bmi_target_info targ_info;

        if (BMIGetTargetInfo(ar->arHifDevice, &targ_info) != A_OK)
        {
            printf("Failed BMIGetTargetInfo\n");
            A_FREE(ar);
            return FALSE;
        }

        ar->arVersion.target_ver = targ_info.target_ver;
        ar->arTargetType = targ_info.target_type;

        target_register_tbl_attach(targ_info.target_type);


            /* do any target-specific preparation that can be done through BMI */
        if (ar6000_prepare_target(ar->arHifDevice,
                                  targ_info.target_type,
                                  targ_info.target_ver) != A_OK)
        {
            A_FREE(ar);
            return FALSE;
        }

        *nTargetID = targ_info.target_ver;
        printf("Target Version = 0X%X\n", targ_info.target_ver);
        printf("Target Type = 0X%X\n", targ_info.target_type);
    }
    
    /* We only register the device in the global list if we succeed. */
    /* If the device is in the global list, it will be destroyed     */
    /* when the module is unloaded.                                  */
    ar6000_devices[devIndex] = ar;
    
    param = HTC_PROTOCOL_VERSION;
    if (BMIWriteMemory(ar->arHifDevice,
                       HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_app_host_interest),
                       (A_UCHAR *)&param,
                       4)!= A_OK)
    {
         AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for htc version failed \n"));
         return FALSE;
    }

    if (enabletimerwar) {
        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for enabletimerwar failed \n"));
            return FALSE;
        }

        param |= HI_OPTION_TIMER_WAR;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for enabletimerwar failed \n"));
            return FALSE;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Timer WAR enabled\n"));
    }

    /* set the firmware mode to STA/IBSS/AP */
    {
        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIReadMemory for setting fwmode failed \n"));
            return FALSE;
        }

        param |= (1 << HI_OPTION_NUM_DEV_SHIFT);
        param |= (fwmode << HI_OPTION_FW_MODE_SHIFT);
        if (firmware_bridge) {
            param |= (firmware_bridge << HI_OPTION_FW_BRIDGE_SHIFT);
        }

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for setting fwmode failed \n"));
            return FALSE;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("Firmware mode set\n"));
    }

    // No need to reserve RAM space for patch as AR6001 is flash based
    if (ar->arTargetType == TARGET_TYPE_AR6001) {
        param = 0;
        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_end_RAM_reserve_sz),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for hi_end_RAM_reserve_sz failed \n"));
            return FALSE;
        }
    }

    /*
     * Hardcode the address use for the extended board data
     * Ideally this should be pre-allocate by the OS at boot time
     * But since it is a new feature and board data is loaded
     * at init time, we have to workaround this from host.
     * It is difficult to patch the firmware boot code,
     * but possible in theory.
     */
    if (ar->arTargetType == TARGET_TYPE_AR6003) {
        if (ar->arVersion.target_ver == AR6003_REV2_VERSION) {
            param = AR6003_REV2_BOARD_EXT_DATA_ADDRESS;
        } else {
            param = AR6003_REV3_BOARD_EXT_DATA_ADDRESS;
        }
        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_ext_data),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for hi_board_ext_data failed \n"));
            return A_ERROR;
        }
    }
    if (A_FAILED(ar6000_set_htc_params(ar->arHifDevice,
                                       ar->arTargetType,
                                       mbox_yield_limit,
                                       0        //use default number of control buffers
                                       ))) {
        return FALSE;
    }
    
    //param = 1;
    //if (BMIWriteMemory(ar->arHifDevice,
    //                   HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_data_initialized),
    //                   (A_UCHAR *)&param,
    //                   4)!= A_OK)
    //{
    //     AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMIWriteMemory for board data initialize failed \n"));
    //     return FALSE;
    //}
    //printf("hi_board_data_initialized set\n");

    if (setupbtdev != 0) {
        if (A_FAILED(ar6000_set_hci_bridge_flags(ar->arHifDevice,
                                                 ar->arTargetType,
                                                 setupbtdev))) {
            return FALSE;                                            
        }
    }

    //printf ("--- InitAR6000_ene - exit\n");
    return TRUE;
}

A_BOOL SDIO_EndAR6000_ene(int devIndex)
{
    ar6000_destroy(devIndex, 1); 
    dk_htc_start = 0;
    return A_OK;
}

A_BOOL SDIO_ar6000_cleanup_module(void)
{
    int i = 0;

    for (i=0; i < MAX_AR6000; i++) {
        if (ar6000_devices[i] != NULL) {
            ar6000_destroy(i, 1);
        }
    }

    a_module_debug_support_cleanup();
    
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("ar6000_cleanup: success\n"));
    return TRUE;
}

static void 
ar6000_alloc_netbufs(A_NETBUF_QUEUE_T *q, A_UINT16 num)
{
    void * osbuf;

    while(num) {
        if((osbuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE))) {
            A_NETBUF_ENQUEUE(q, osbuf);
        } else {
            break;
        }
        num--;
    }

    if(num) {
        A_PRINTF("ar6000_alloc_netbufs - allocation of netbuf failed");
    }
}


/*
 * HTC Event handlers
 */

HANDLE SDIO_open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName){
    //A_UINT32        param;
    AR_SOFTC_T      *ar;
    HTC_INIT_INFO   htcInfo;
    HTC_TARGET *target;
    int i;
    
    printf("\n*** open device ***\n");

    // fix the issue of swrith card, 
    // make the rRxBuffers sync up with rx_queue
    for(i=0; i<MAX_ENDPOINTS; i++)
    {
        rRxBuffers[i] = 0;
    }

    ar = (AR_SOFTC_T *)ar6000_devices[devIndex];
	//printf ("--- devIndex = %d; ar = 0x%x; ar->arHifDevice = 0x%x\n", devIndex, ar, ar->arHifDevice);

    if(dk_htc_start)
    {
        HTCStop(ar->arHtcTarget);
        HTCDestroy(ar->arHtcTarget);
        CloseHandle(rxEvent);
        dk_htc_start = 0;
    }
    
                                     
    A_MEMZERO(&htcInfo,sizeof(htcInfo));
    htcInfo.pContext = ar;
    htcInfo.TargetFailure = ar6000_target_failure;
    
    ar->arHtcTarget = HTCCreate(ar->arHifDevice,&htcInfo);
    
    if (ar->arHtcTarget == NULL) {
        return ((HANDLE)A_ERROR);    
    }
    /*HTCCreate will call DevSetupGMbox but gmbox ADDR is null so we add ADDR and call DevSetupGMbox again*/
    //target = GET_HTC_TARGET_FROM_HANDLE(ar->arHtcTarget);
    //SetExtendedMboxWindowInfo(MANUFACTURER_ID_AR6003_BASE,
                             //&target->Device.MailBoxInfo);
    //DevSetupGMbox(&target->Device);

    A_MUTEX_INIT(&ar->arLock);

    return ar->arDeviceHandle;
}

static void ar6000_target_failure(void *Instance, A_STATUS Status)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)Instance;

    if (Status != A_OK) {
        
        printf("ar6000_target_failure: target asserted \n");
         
        /* try dumping target assertion information (if any) */
        ar6000_dump_target_assert_info(ar->arHifDevice,ar->arTargetType);

        /*
         * Fetch the logs from the target via the diagnostic
         * window.
         */
        ar6000_dbglog_get_debug_logs(ar);
    }
}

static A_STATUS
ar6000_unavail_ev(void *context, void *hif_handle)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)context;
        /* NULL out it's entry in the global list */
    ar6000_destroy(ar->arDeviceIndex, 1);  
    return A_OK;
}

/*
 * We need to differentiate between the surprise and planned removal of the
 * device because of the following consideration:
 * - In case of surprise removal, the hcd already frees up the pending
 *   for the device and hence there is no need to unregister the function
 *   driver inorder to get these requests. For planned removal, the function
 *   driver has to explictly unregister itself to have the hcd return all the
 *   pending requests before the data structures for the devices are freed up.
 *   Note that as per the current implementation, the function driver will
 *   end up releasing all the devices since there is no API to selectively
 *   release a particular device.
 * - Certain commands issued to the target can be skipped for surprise
 *   removal since they will anyway not go through.
 */
static void
ar6000_destroy(int devIndex, unsigned int unregister)
{
    AR_SOFTC_T *ar;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("+ar6000_destroy \n"));

    if((ar = ar6000_devices[devIndex]) == NULL)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s(): Failed to get device structure.\n", __FUNCTION__));
        return;
    }

    /* Stop the transmit queues */
    
    if (ar->arHtcTarget != NULL) {
        //ar6000_cleanup_hci(ar);
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,(" Shutting down HTC .... \n"));
        /* stop HTC */
        HTCStop(ar->arHtcTarget);
        /* destroy HTC */
        HTCDestroy(ar->arHtcTarget);
    }
        
    if (resetok) {
        /* try to reset the device if we can
         * The driver may have been configure NOT to reset the target during
         * a debug session */
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,(" Attempting to reset target on instance destroy.... \n"));
        if (ar->arHifDevice != NULL) {
            ar6000_reset_device(ar->arHifDevice, ar->arTargetType, TRUE, FALSE);
        }
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,(" Host does not want target reset. \n"));
    }

    if (ar->arHifDevice != NULL) {
        /*release the device so we do not get called back on remove incase we
         * we're explicity destroyed by module unload */
        HIFReleaseDevice(ar->arHifDevice);       
    }
        
       /* Done with cookies */
    ar6000_cookie_cleanup(ar);

    /* Cleanup BMI */
    //////BMIInit();

    /* Clear the tx counters */
    memset(tx_attempt, 0, sizeof(tx_attempt));
    memset(tx_post, 0, sizeof(tx_post));
    memset(tx_complete, 0, sizeof(tx_complete));


    /* Free up the device data structure */
    if( unregister )
    {
        A_FREE(ar);
        ar6000_devices[devIndex] = NULL;
    }
    AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("-ar6000_destroy \n"));
}

/* connect to a service */
static A_STATUS ar6000_connect_raw_service(AR_SOFTC_T        *ar,
                                           HTC_RAW_STREAM_ID StreamID)
{
    A_STATUS                 status;
    HTC_SERVICE_CONNECT_RESP response;
    A_UINT8                  streamNo;
    HTC_SERVICE_CONNECT_REQ  connect;
    
    do
	{            
        A_MEMZERO(&connect,sizeof(connect));
            /* meta data*/
		streamNo = (A_UINT8)StreamID;
        connect.pMetaData = &streamNo;
        connect.MetaDataLength = sizeof(A_UINT8);
            /* these fields are the same for all service endpoints */
        connect.EpCallbacks.pContext = ar;
        connect.EpCallbacks.EpTxCompleteMultiple = ar6000_tx_complete_multiple;
        connect.EpCallbacks.EpRecv = ar6000_rx;
        connect.EpCallbacks.EpRecvRefill = ar6000_rx_refill;
        connect.EpCallbacks.EpSendFull = ar6000_tx_queue_full;
            /* connect to the raw streams service, we may be able to get 1 or more
             * connections, depending on WHAT is running on the target */
        connect.ServiceID = HTC_RAW_STREAMS_SVC;
            /* set the max queue depth so that our ar6000_tx_queue_full handler gets called.
             * Linux has the peculiarity of not providing flow control between the
             * NIC and the network stack. There is no API to indicate that a TX packet
             * was sent which could provide some back pressure to the network stack.
             * Under linux you would have to wait till the network stack consumed all sk_buffs
             * before any back-flow kicked in. Which isn't very friendly.
             * So we have to manage this ourselves */
        connect.MaxSendQueueDepth = 32;
        //connect.RecvRefillWaterMark = AR6000_MAX_RX_BUFFERS / 4; /* set to 25 % */
        //if (0 == connect.RecvRefillWaterMark) {
        //    connect.RecvRefillWaterMark++;        
        //} 
      
        A_MEMZERO(&response,sizeof(response));
        
            /* try to connect to the raw stream, it is okay if this fails with 
             * status HTC_SERVICE_NO_MORE_EP */
        status = HTCConnectService(ar->arHtcTarget, 
                                   &connect,
                                   &response);
        
        if (A_FAILED(status)) {
            if (response.ConnectRespCode == HTC_SERVICE_NO_MORE_EP) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HTC RAW , No more streams allowed \n"));
                status = A_OK;    
            }
            break;    
        }

            /* set endpoint mapping for the RAW HTC streams */
        arSetRawStream2EndpointIDMap(ar,StreamID,response.Endpoint);

        AR_DEBUG_PRINTF(ATH_DEBUG_HTC_RAW,("HTC RAW : stream ID: %d, endpoint: %d\n", 
                        StreamID, arRawStream2EndpointID(ar,StreamID)));
        
    } while (FALSE);
    
    return status;
}


static A_STATUS HCIReadMessagePending(void *pContext, A_UINT8 LookAheadBytes[], int ValidBytes)
{
    GMBOX_PROTO_HCI_UART        *pProt = (GMBOX_PROTO_HCI_UART *)pContext;
    int                         totalRecvLength = 0;
    A_STATUS                    status = A_OK;
    HTC_PACKET                  szhtc_pack;
    HTC_PACKET                  *pPacket = &szhtc_pack;

	//printf ("--- HCIUartMessagePending\n");


   do {
    
        if (ValidBytes < 3) {
                /* not enough for ACL or event header */
            break;    
        }    
        
        //if ((LookAheadBytes[0] == HCI_UART_ACL_PKT) && (ValidBytes < 5)) {
        if ((LookAheadBytes[0] == 0x02) && (ValidBytes < 5)) {
                /* not enough for ACL data header */
            break;    
        }
                
        switch (LookAheadBytes[0]) {       
		case 0x04://HCI_UART_EVENT_PKT:
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI Event: %d param length: %d \n",
                        LookAheadBytes[1], LookAheadBytes[2]));
                totalRecvLength = LookAheadBytes[2];
                totalRecvLength += 3; /* add type + event code + length field */
                //pktType = HCI_EVENT_TYPE;      
                break;
		case 0x02://HCI_UART_ACL_PKT:                
                totalRecvLength = (LookAheadBytes[4] << 8) | LookAheadBytes[3];                
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI ACL: conn:0x%X length: %d \n",
                        ((LookAheadBytes[2] & 0xF0) << 8) | LookAheadBytes[1], totalRecvLength));
                totalRecvLength += 5; /* add type + connection handle + length field */
                //pktType = HCI_ACL_TYPE;           
                break;        
            default:
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**Invalid HCI packet type: %d \n",LookAheadBytes[0]));
                status = A_EPROTO;
                break;
        }
        
        if (A_FAILED(status)) {
            break;    
        }
          
    } while (FALSE);


    do {
        
        if (A_FAILED(status) || (NULL == pPacket)) {
            break;    
        } 
        
            /* do this synchronously, we don't need to be fast here */
        pPacket->Completion = NULL;
        pPacket-> pBuffer = hci_rxBuffer;
        AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI : getting recv packet len:%d hci-uart-type: %s \n",
                totalRecvLength, (LookAheadBytes[0] == HCI_UART_EVENT_PKT) ? "EVENT" : "ACL"));
           
			
        status = DevGMboxRead(pProt->pDev, pPacket, totalRecvLength);     
        
        if (A_FAILED(status)) {
            break;    
        }
        
        if (pPacket->pBuffer[0] != LookAheadBytes[0]) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI buffer does not contain expected packet type: %d ! \n",
                        pPacket->pBuffer[0]));
            status = A_EPROTO;
            break;   
        }
#if 0      
        if (pPacket->pBuffer[0] == HCI_UART_EVENT_PKT) {
                /* validate event header fields */
            if ((pPacket->pBuffer[1] != LookAheadBytes[1]) ||
                (pPacket->pBuffer[2] != LookAheadBytes[2])) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI buffer does not match lookahead! \n"));
                DebugDumpBytes(LookAheadBytes, 3, "Expected HCI-UART Header");  
                DebugDumpBytes(pPacket->pBuffer, 3, "** Bad HCI-UART Header");  
                status = A_EPROTO;
                break;       
            }   
        } else if (pPacket->pBuffer[0] == HCI_UART_ACL_PKT) {
                /* validate acl header fields */
            if ((pPacket->pBuffer[1] != LookAheadBytes[1]) ||
                (pPacket->pBuffer[2] != LookAheadBytes[2]) ||
                (pPacket->pBuffer[3] != LookAheadBytes[3]) ||
                (pPacket->pBuffer[4] != LookAheadBytes[4])) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI buffer does not match lookahead! \n"));
                DebugDumpBytes(LookAheadBytes, 5, "Expected HCI-UART Header");  
                DebugDumpBytes(pPacket->pBuffer, 5, "** Bad HCI-UART Header");  
                status = A_EPROTO;
                break;       
            }   
        }
#endif     

        
  
        
    } while (FALSE);

	A_MEMCPY( &rxBuffer[eRxBufIndex],pPacket-> pBuffer, totalRecvLength);
	
	eRxBufIndex += totalRecvLength;
	if (eRxBufIndex > MAX_RX_BUF_SIZE)
    {
        eRxBufIndex= MAX_RX_BUF_SIZE;
	}
	
	SetEvent(rxEvent);
	return A_OK;
}

/* This function does one time initialization for the lifetime of the device */
static A_STATUS ar6000_init(int devIndex)
{
    AR_SOFTC_T *ar;
    A_STATUS    status;
    HTC_SERVICE_ID servicepriority;
    HTC_TARGET *target;
    int streamID, endPt;

    if((ar = ar6000_devices[devIndex]) == NULL)
    {
        return A_ERROR;
    }

    /* Do we need to finish the BMI phase */
    if(BMIDone(ar->arHifDevice) != A_OK)
    {
        return A_ERROR;
    }

    /* the reason we have to wait for the target here is that the driver layer
     * has to init BMI in order to set the host block size,
     */
    status = HTCWaitTarget(ar->arHtcTarget);

    if (A_FAILED(status))
	{
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HTCWaitTarget returns error %d\n", status));
        return A_ERROR;
    }

    for (endPt = 0; endPt < ENDPOINT_MAX; endPt++)
	{
        ar->arEp2RawMapping[endPt] = HTC_RAW_STREAM_NOT_MAPPED;
    }
    
    // connect to 1 endpoint for normal TX/RX
    for (streamID = HTC_RAW_STREAM_0; streamID < HTC_RAW_STREAM_1; streamID++)
	{
            /* try to connect to the raw service */
        status = ar6000_connect_raw_service(ar,streamID);
        
        if (A_FAILED(status)) {
            break;    
        }
        
        if (arRawStream2EndpointID(ar,streamID) == 0) {
            break;    
        }
    }
    
    if (A_FAILED(status))
	{
        return A_ERROR;
    }

    /*
     * give our connected endpoints some buffers
     */

    //ar6000_rx_refill(ar, ar->arControlEp); Note: control buffers have been allocated in HTCCreate
    ar6000_rx_refill(ar, ar->arRaw2EpMapping[0]);

    //    /* setup credit distribution */
    servicepriority = HTC_RAW_STREAMS_SVC;  /* only 1 */
    HTCSetCreditDistribution(ar->arHtcTarget,
                             ar,
                             NULL,  /* use default */
                             NULL,  /* use default */
                             &servicepriority,
                             1);

    /* Since cookies are used for HTC transports, they should be */
    /* initialized prior to enabling HTC.                        */
    ar6000_cookie_init(ar);

	/* start HTC */
    status = HTCStart(ar->arHtcTarget);

    if (status != A_OK) {
        ar6000_cookie_cleanup(ar);
		printf ("--- Error in HTCStart\n");
        return A_ERROR;
    }

    ar->arNumDataEndPts = 1;
	ar->arConnected = TRUE;

    dk_htc_start = 1;

    target = GET_HTC_TARGET_FROM_HANDLE(ar->arHtcTarget);
    status = DevGMboxIRQAction(&target->Device, GMBOX_ERRORS_IRQ_ENABLE, PROC_IO_SYNC);   
        
    if (A_FAILED(status)) {
        printf ("--- Error in DevGMboxIRQAction -- GMBOX_ERRORS_IRQ_ENABLE\n");
	return A_ERROR;   
    } 
        /* enable recv */   
    status = DevGMboxIRQAction(&target->Device, GMBOX_RECV_IRQ_ENABLE, PROC_IO_SYNC);
        
    if (A_FAILED(status)) {
        printf ("--- Error in DevGMboxIRQAction -- GMBOX_RECV_IRQ_ENABLE\n");
	return A_ERROR;  
    } 
        /* signal bridge side to power up BT */
        //status = DevGMboxSetTargetInterrupt(ar->arHtcTarget, MBOX_SIG_HCI_BRIDGE_BT_ON, BTON_TIMEOUT_MS);
    status = DevGMboxSetTargetInterrupt(&target->Device, 0, 500);
    if (A_FAILED(status)) {
            //AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HCI_TransportStart : Failed to trigger BT ON \n"));
	printf ("DevGMboxSetTargetInterrupt : Failed to trigger BT ON \n");
        return A_ERROR;   
    } 
		
		target->Device.GMboxInfo.pMessagePendingCallBack = HCIReadMessagePending;
    return A_OK;
}

#ifdef DEBUG
static void ar6000_dump_skb(A_PBUF_T *pbuf)
{
   u_char *ch;
   for (ch = A_NETBUF_DATA(pbuf);
        (A_UINT32)ch < ((A_UINT32)A_NETBUF_DATA(pbuf) +
        A_NETBUF_LEN(pbuf)); ch++)
    {
         AR_DEBUG_PRINTF(ATH_DEBUG_WARN,("%2.2x ", *ch));
    }
    AR_DEBUG_PRINTF(ATH_DEBUG_WARN,("\n"));
}
#endif

#ifdef HTC_TEST_SEND_PKTS
static void DoHTCSendPktsTest(AR_SOFTC_T *ar, int MapNo, HTC_ENDPOINT_ID eid, A_PBUF_T *pbuf);
#endif

static int
ar6000_data_tx(A_PBUF_T *pbuf, int devIndex)
{
    AR_SOFTC_T        *ar = ar6000_devices[devIndex];
    HTC_ENDPOINT_ID    eid = ENDPOINT_UNUSED;
    A_UINT32          mapNo = 0;
    struct ar_cookie *cookie;
    A_BOOL            bMoreData = FALSE; 
    HTC_TX_TAG        htc_tag = AR6K_DATA_PKT_TAG;

    AR_DEBUG_PRINTF(ATH_DEBUG_WLAN_TX,("ar6000_data_tx start - pbuf=0x%x, data=0x%x, len=0x%x\n",
                     (A_UINT32)pbuf, (A_UINT32)A_NETBUF_DATA(pbuf),
                     A_NETBUF_LEN(pbuf)));

    /* If target is not associated */
    if(!ar->arConnected) 
	{
        return 0;
    }

    cookie = NULL;

        /* take the lock to protect driver data */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    do {

        eid = ar->arRaw2EpMapping[0];

            /* validate that the endpoint is connected */
        if (eid == 0 || eid == ENDPOINT_UNUSED ) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,(" eid %d is NOT mapped!\n", eid));
            break;
        }
            /* allocate resource for this packet */
        cookie = ar6000_alloc_cookie(ar);

        if (cookie != NULL) {
                /* update counts while the lock is held */
            ar->arTxPending[eid]++;
            ar->arTotalTxDataPending++;
        }

    } while (FALSE);

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    if (cookie != NULL) {
        cookie->arc_bp[0] = (A_UINT32)pbuf;
        cookie->arc_bp[1] = mapNo;
        SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                               cookie,
                               A_NETBUF_DATA(pbuf),
                               A_NETBUF_LEN(pbuf),
                               eid,
                               htc_tag);

#ifdef DEBUG
        if (debugdriver >= 3) {
            ar6000_dump_skb(pbuf);
        }
#endif
#ifdef HTC_TEST_SEND_PKTS
        DoHTCSendPktsTest(ar,mapNo,eid,pbuf);
#endif        
            /* HTC interface is asynchronous, if this fails, cleanup will happen in
             * the ar6000_tx_complete callback */
        HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
    } else {
            /* no packet to send, cleanup */
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);
    }

    return 0;
}

static HTC_SEND_FULL_ACTION ar6000_tx_queue_full(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T     *ar = (AR_SOFTC_T *)Context;
    HTC_SEND_FULL_ACTION    action = HTC_SEND_FULL_KEEP;
    HTC_ENDPOINT_ID         Endpoint = HTC_GET_ENDPOINT_FROM_PKT(pPacket);

    do {

        if (Endpoint == ar->arControlEp) {
                /* under normal WMI if this is getting full, then something is running rampant
                 * the host should not be exhausting the WMI queue with too many commands
                 * the only exception to this is during testing using endpointping */
            AR6000_SPIN_LOCK(&ar->arLock, 0);
                /* set flag to handle subsequent messages */
            AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("WMI Control Endpoint is FULL!!! \n"));
            break;
        }

        /* if we get here, we are dealing with data endpoints getting full */

        if (HTC_GET_TAG_FROM_PKT(pPacket) == AR6K_CONTROL_PKT_TAG) {
            /* don't drop control packets issued on ANY data endpoint */
            break;
        }
    } while (FALSE);

    return action;
}

static void
ar6000_tx_complete_multiple(void *Context, HTC_PACKET_QUEUE *pPacketQueue)
{
    AR_SOFTC_T          *ar = (AR_SOFTC_T *)Context;
    A_UINT32            mapNo = 0;
    A_STATUS            status;
    struct ar_cookie    *ar_cookie;
    HTC_ENDPOINT_ID     eid;
    //A_BOOL          wakeEvent = FALSE;
    A_NETBUF_QUEUE_T    skb_queue;
    HTC_PACKET          *pPacket;
    A_PBUF_T            *pktSkb;
    A_BOOL              flushing = FALSE;

    A_NETBUF_QUEUE_INIT(&skb_queue);
  
        /* lock the driver as we update internal state */
    AR6000_SPIN_LOCK(&ar->arLock, 0);
    
        /* reap completed packets */    
    while (!HTC_QUEUE_EMPTY(pPacketQueue)) {
        
        pPacket = HTC_PACKET_DEQUEUE(pPacketQueue);
        
        ar_cookie = (struct ar_cookie *)pPacket->pPktContext;
        A_ASSERT(ar_cookie); 
          
        status = pPacket->Status;
        pktSkb = (A_PBUF_T *)ar_cookie->arc_bp[0];
        eid = pPacket->Endpoint;
        mapNo = ar_cookie->arc_bp[1];
        
        A_ASSERT(pktSkb);
        A_ASSERT(pPacket->pBuffer == A_NETBUF_DATA(pktSkb));
        
            /* add this to the list, use faster non-lock API */
        A_NETBUF_ENQUEUE(&skb_queue,pktSkb);
        
        if (A_SUCCESS(status)) {
            A_ASSERT(pPacket->ActualLength == A_NETBUF_LEN(pktSkb));
        }
    
        AR_DEBUG_PRINTF(ATH_DEBUG_WLAN_TX,("ar6000_tx_complete pbuf=0x%x data=0x%x len=0x%x eid=%d ",
                         (A_UINT32)pktSkb, (A_UINT32)pPacket->pBuffer,
                         pPacket->ActualLength,
                         eid));
    
        ar->arTxPending[eid]--;
    
        if ((eid  != ar->arControlEp) || bypasswmi) {
            ar->arTotalTxDataPending--;
        }
           
        if (A_FAILED(status)) {
            if (status == A_ECANCELED) {
                    /* a packet was flushed  */
                flushing = TRUE;    
            }            
            AR6000_STAT_INC(ar, tx_errors);
            if (status != A_NO_RESOURCE) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s() -TX ERROR, status: 0x%x\n", __FUNCTION__, status));
            }
        } else {
            AR_DEBUG_PRINTF(ATH_DEBUG_WLAN_TX,("OK\n"));
            flushing = FALSE;
            AR6000_STAT_INC(ar, tx_packets);
            ar->arNetStats.tx_bytes += A_NETBUF_LEN(pktSkb);
        }
    
        ar6000_free_cookie(ar, ar_cookie);
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    /* lock is released, we can freely call other kernel APIs */

        /* free all skbs in our local list */
    while (A_NETBUF_QUEUE_SIZE(&skb_queue) > 0)
    {
            /* use non-lock version */
        pktSkb = A_NETBUF_DEQUEUE(&skb_queue);
        //printf("--- ar6000_tx_complete_multiple - free pbuf=0x%x data=0x%x len=0x%x eid=%d\n",
        //                 (A_UINT32)pktSkb, (A_UINT32)pPacket->pBuffer,
        //                 pPacket->ActualLength,
        //                 eid);
        A_NETBUF_FREE(pktSkb);
    }
}

/*
 * Receive event handler.  This is called by HTC when a packet is received
 */
int pktcount;
static void
ar6000_rx(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)Context;
    A_PBUF_T *pbuf = (A_PBUF_T *)pPacket->pPktContext;
    A_STATUS        status = pPacket->Status;
    HTC_ENDPOINT_ID   ept = pPacket->Endpoint;

    A_ASSERT((status != A_OK) ||
             (pPacket->pBuffer == ((A_UINT8 *)A_NETBUF_DATA(pbuf) + HTC_HEADER_LEN)));

    AR_DEBUG_PRINTF(ATH_DEBUG_WLAN_RX,("ar6000_rx ar=0x%x eid=%d, pbuf=0x%x, data=0x%x, len=0x%x status:%d",
                    (A_UINT32)ar, ept, (A_UINT32)pbuf, (A_UINT32)pPacket->pBuffer,
                    pPacket->ActualLength, status));
    if (status != A_OK) {
        if (status != A_ECANCELED) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("RX ERR (%d) \n",status));
        }
    } 

        /* take lock to protect buffer counts
         * and adaptive power throughput state */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if (A_SUCCESS(status)) {
        AR6000_STAT_INC(ar, rx_packets);
        ar->arNetStats.rx_bytes += pPacket->ActualLength;

        A_NETBUF_PUT(pbuf, pPacket->ActualLength +  HTC_HEADER_LEN);
        A_NETBUF_PULL(pbuf, HTC_HEADER_LEN);

#ifdef DEBUG
        if (debugdriver >= 2) {
            ar6000_dump_skb(pbuf);
        }
#endif /* DEBUG */
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    if (status != A_OK) {
        AR6000_STAT_INC(ar, rx_errors);
        A_NETBUF_FREE(pbuf);
    } else {
        ar6000_deliver_frames_to_nw_stack(ept, (void *)pbuf);
    }

    return;
}

static void
ar6000_deliver_frames_to_nw_stack(HTC_ENDPOINT_ID eid, void *osbuf)
{   
    A_PBUF_T *pbuf = (A_PBUF_T *)osbuf;
    unsigned int length = A_NETBUF_LEN(pbuf);

    if (eRxBufIndex >= MAX_RX_BUF_SIZE)
    {
         return;
    }

	A_MEMCPY( &rxBuffer[eRxBufIndex], A_NETBUF_DATA(pbuf), length);
	eRxBufIndex += length;

	if (eRxBufIndex > MAX_RX_BUF_SIZE)
    {
        eRxBufIndex= MAX_RX_BUF_SIZE;
	}
    A_NETBUF_FREE(osbuf);
    rRxBuffers[eid]--;
	SetEvent(rxEvent);
}


static void
ar6000_rx_refill(void *Context, HTC_ENDPOINT_ID Endpoint)
{
    AR_SOFTC_T  *ar = (AR_SOFTC_T *)Context;
    void        *osBuf;
    int         RxBuffers;
    int         buffersToRefill;
    HTC_PACKET  *pPacket;
    HTC_PACKET_QUEUE queue;

    buffersToRefill = (int)AR6000_MAX_RX_BUFFERS -
                                    HTCGetNumRecvBuffers(ar->arHtcTarget, Endpoint);

    if (buffersToRefill <= 0) {
            /* fast return, nothing to fill */
        return;
    }

    INIT_HTC_PACKET_QUEUE(&queue);
    
    AR_DEBUG_PRINTF(ATH_DEBUG_WLAN_RX,("ar6000_rx_refill: providing htc with %d buffers at eid=%d\n",
                    buffersToRefill, Endpoint));

    for (RxBuffers = 0; RxBuffers < buffersToRefill; RxBuffers++) {
        osBuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE);
        if (NULL == osBuf) {
            break;
        }
            /* the HTC packet wrapper is at the head of the reserved area
             * in the pbuf */
        pPacket = (HTC_PACKET *)(A_NETBUF_HEAD(osBuf));
            /* set re-fill info */
        SET_HTC_PACKET_INFO_RX_REFILL(pPacket,osBuf,A_NETBUF_DATA(osBuf),AR6000_BUFFER_SIZE,Endpoint);
            /* add to queue */
        HTC_PACKET_ENQUEUE(&queue,pPacket);
    }
    
    if (!HTC_QUEUE_EMPTY(&queue)) {
            /* add packets */
        HTCAddReceivePktMultiple(ar->arHtcTarget, &queue);
    }

}

/* Init cookie queue */
static void
ar6000_cookie_init(AR_SOFTC_T *ar)
{
    A_UINT32    i;

    ar->arCookieList = NULL;
    A_MEMZERO(s_ar_cookie_mem, sizeof(s_ar_cookie_mem));

    for (i = 0; i < MAX_COOKIE_NUM; i++) {
        ar6000_free_cookie(ar, &s_ar_cookie_mem[i]);
    }
}

/* cleanup cookie queue */
static void
ar6000_cookie_cleanup(AR_SOFTC_T *ar)
{
    /* It is gone .... */
    ar->arCookieList = NULL;
}

/* Init cookie queue */
static void
ar6000_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie)
{
    /* Insert first */
    A_ASSERT(ar != NULL);
    A_ASSERT(cookie != NULL);
    cookie->arc_list_next = ar->arCookieList;
    ar->arCookieList = cookie;
}

/* cleanup cookie queue */
static struct ar_cookie *
ar6000_alloc_cookie(AR_SOFTC_T  *ar)
{
    struct ar_cookie   *cookie;

    cookie = ar->arCookieList;
    if(cookie != NULL)
    {
        ar->arCookieList = cookie->arc_list_next;
    }

    return cookie;
}

#ifdef SEND_EVENT_TO_APP
/*
 * This function is used to send event which come from taget to
 * the application. The buf which send to application is include
 * the event ID and event content.
 */
#define EVENT_ID_LEN   2
void ar6000_send_event_to_app(AR_SOFTC_T *ar, A_UINT16 eventId,
                              A_UINT8 *datap, int len)
{

#if (WIRELESS_EXT >= 15)

/* note: IWEVCUSTOM only exists in wireless extensions after version 15 */

    char *buf;
    A_UINT16 size;
    union iwreq_data wrqu;

    size = len + EVENT_ID_LEN;

    if (size > IW_CUSTOM_MAX) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("WMI event ID : 0x%4.4X, len = %d too big for IWEVCUSTOM (max=%d) \n",
                eventId, size, IW_CUSTOM_MAX));
        return;
    }

    buf = A_MALLOC_NOWAIT(size);
    A_MEMZERO(buf, size);
    A_MEMCPY(buf, &eventId, EVENT_ID_LEN);
    A_MEMCPY(buf+EVENT_ID_LEN, datap, len);

    //AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("event ID = %d,len = %d\n",*(A_UINT16*)buf, size));
    A_MEMZERO(&wrqu, sizeof(wrqu));
    wrqu.data.length = size;
    wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);

    A_FREE(buf);
#endif //(WIRELESS_EXT >= 15)


}
#endif //SEND_EVENT_TO_APP


DWORD HCI_DRG_Read( HANDLE pContext,  PUCHAR buf,
                     ULONG length,  PULONG pBytesRead)
{
    DWORD Retval = -1;
    unsigned int counter = 60000;
    int devIndex;
    AR_SOFTC_T *ar;
    HTC_TARGET *target;
	

    devIndex = GetDeviceIndex(pContext);
    ar = AR6000_GET(devIndex);
    target = GET_HTC_TARGET_FROM_HANDLE(ar->arHtcTarget);


    if ( ((eRxBufIndex - sRxBufIndex)  <(LONG) length) && ((eRxBufIndex - sRxBufIndex) == 0)) {
  
        //printf("dk_sdio_read:wait for rxq\n");
        
        do {
            counter--;
            if (0 == counter)
            {
                //printf("HCI_DRG_READ FAILED\n");
                //driver_failure = 1;
                return -1;
            }
	if (DevDsrHandler(&(target->Device)) != A_OK)
            {
                printf("htcDSRHandler FAILED in HCI_DRG_Read\n");
                return A_ERROR;
            }
            //milliSleep(1); //<--- don't need this. This slows DR_Read down
            Retval=WaitForSingleObject(rxEvent,0);          
        } while (Retval != WAIT_OBJECT_0);
        //printf("--- %s count = %d)\n", GetTimeStamp(), 1000 - counter);
        
        ResetEvent(rxEvent);           
    }

    length = eRxBufIndex - sRxBufIndex;
    memcpy(buf, (PUCHAR)&rxBuffer[sRxBufIndex], (A_UINT32)(length));
    sRxBufIndex += (length);
    if (sRxBufIndex > MAX_RX_BUF_SIZE)
    {
        sRxBufIndex= 0;
        eRxBufIndex= 0;
    }
	*pBytesRead = length;
    //printf("--- %s HCI_DRG_READ BytesRead=%d %f\n", GetTimeStamp(), *pBytesRead, GetTimeStampDouble() );
    return (length);
}

int driver_failure = 0;
DWORD SDIO_DRG_Read( HANDLE pContext,  PUCHAR buf,
                     ULONG length,  PULONG pBytesRead)
{
    DWORD Retval = -1;
    unsigned int counter = 60000;
	int devIndex = GetDeviceIndex(pContext);
    AR_SOFTC_T *ar = AR6000_GET(devIndex);
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(ar->arHtcTarget);
    //double StartTime = GetTimeStampDouble();

    if (driver_failure)
    {
        return -1;
    }

    if ( ((eRxBufIndex - sRxBufIndex)  <(LONG) length) && ((eRxBufIndex - sRxBufIndex) == 0)) {
  
        //printf("dk_sdio_read:wait for rxq\n");
        
        do {
            counter--;
            if (0 == counter)
            {
                printf("DRG_READ FAILED\n");
                driver_failure = 1;
                return -1;
            }
			if (DevDsrHandler(&(target->Device)) != A_OK)
            {
                printf("htcDSRHandler FAILED in DRG_Read\n");
                return A_ERROR;
            }
            //milliSleep(1); //<--- don't need this. This slows DR_Read down
            Retval=WaitForSingleObject(rxEvent,0);          
        } while (Retval != WAIT_OBJECT_0);
        //printf("--- %s count = %d)\n", GetTimeStamp(), 1000 - counter);
        
        ResetEvent(rxEvent);           
    }
 
    memcpy(buf, (PUCHAR)&rxBuffer[sRxBufIndex], length);
    sRxBufIndex += length;
    if (sRxBufIndex > MAX_RX_BUF_SIZE)
	{
        sRxBufIndex= 0;
        eRxBufIndex= 0;
    }
#ifdef DEBUG
    if (length >=8)
    {
        if ((113 == buf[0]) || (26 == buf[0]))
        {
            FILE * pFile;

            pFile= fopen("log\\debug_log.txt", "a");
			if (pFile)
			{
				fprintf (pFile,"read cmd %d, length %d\n",buf[0],length);
				fclose(pFile);
			}
        }
    }
#endif
    //printf("--- %s DRG_READ %f\n", GetTimeStamp(), GetTimeStampDouble() - StartTime);
    return length;
}

 
DWORD HCI_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length )
{
    A_STATUS status;
    int devIndex;
    AR_SOFTC_T *ar;
    HTC_TARGET *target;
    HTC_PACKET szhtc_pack;
	//unsigned char buff[100];

    sRxBufIndex = 0; 
    eRxBufIndex = 0;

    devIndex = GetDeviceIndex(COM_Write);
    ar = AR6000_GET(devIndex);
    target = GET_HTC_TARGET_FROM_HANDLE(ar->arHtcTarget);
    ResetEvent(rxEvent);
    //A_STATUS DevGMboxWrite(AR6K_DEVICE *pDev, HTC_PACKET *pPacket, A_UINT32 WriteLength) 
    //memcpy(szhtc_pack.pBuffer,buf,length);
    //memset(htc_txBuffer,0,sizeof(htc_txBuffer));
    //szhtc_pack.pBuffer = htc_txBuffer;
    //memcpy(szhtc_pack.pBuffer,buf,length); 
    szhtc_pack.pBuffer = buf;
    szhtc_pack.Completion=NULL;

    status = DevGMboxWrite(&target->Device,&szhtc_pack,length);
    if(status == A_OK)
    {
	return length;
    }
    else
    {
	return 0;
    }
}
DWORD SDIO_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length )
{
#ifdef DEBUG
    unsigned char cmdId = 0;
    FILE * pFile;
#endif
    A_PBUF_T *pbuf;
    A_STATUS status;
	int chunkIndex, chunkLen, bytes;
	int transferLength, maxLen;
    int devIndex;
    AR_SOFTC_T *ar;
    HTC_TARGET *target;
    //double StartTime = GetTimeStampDouble();

    if (driver_failure)
    {
        return -1;
    }
    sRxBufIndex = 0; 
    eRxBufIndex = 0;

    devIndex = GetDeviceIndex(COM_Write);
    ar = AR6000_GET(devIndex);

    if (!dk_htc_start)
    {
        dk_htc_start = 1;
        rxEvent= CreateEvent(NULL,TRUE,FALSE,NULL);
        if(rxEvent==NULL)
        {
            driver_failure=1;
            return 0;
        }

        if (ar6000_init(devIndex) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("DRG_Write: ar6000_init failed\n"));
            ar6000_destroy(devIndex, 0);
            driver_failure = 1;
            return A_ERROR;
        }
    
        HIFClaimDevice(ar->arHifDevice, ar);

        AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("DRG_Write: hifdevice=0x%x, dev(%d), ar=0x%x\n",
                        ar->arHifDevice, devIndex, ar));
    }

    ResetEvent(rxEvent);
#ifdef DEBUG
	if (length >=8)
	{
		if ((0x71 == buf[2]) || (26 == buf[2]))
		{
			cmdId = buf[2];
			pFile= fopen("log\\debug_log.txt", "a");
			if (pFile)
			{
				fprintf (pFile,"send cmd %d , length %d +\n", cmdId, length);
				fclose(pFile);
			}
		}
	}
#endif 

	bytes = length;
	target = GET_HTC_TARGET_FROM_HANDLE(ar->arHtcTarget);
	maxLen = target->TargetCreditSize * target->TargetCredits;
    transferLength = DEV_CALC_SEND_PADDED_LEN(&target->Device, maxLen);       
	chunkLen = maxLen - (transferLength - maxLen) - HTC_HDR_LENGTH - 16; // why -16???
	if (bytes < chunkLen)
	{
		chunkLen = bytes;
	}
	//printf("--- length = %d; chunkLen = %d; maxLen = %d; transferLength = %d\n", length, chunkLen, maxLen, transferLength);
	chunkIndex = 0;

    while (bytes > 0)
	{
	    pbuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE);
	    if (pbuf)
	    {
			memcpy (A_NETBUF_DATA(pbuf), (unsigned char *)&buf[chunkIndex], chunkLen);
			A_NETBUF_PUT(pbuf, chunkLen);

			status = ar6000_data_tx(pbuf, 0);           
			if (status != A_OK)
			{
				printf("***** (BAD) write:HTC Buffer send : fails");
				driver_failure = 1;
				A_NETBUF_FREE(pbuf);
				return A_ERROR;
			}
			chunkIndex += chunkLen;
			if ((bytes = bytes - chunkLen) < chunkLen)
			{
				chunkLen = bytes;
			}
		}
        else
        {
            driver_failure = 1;
            printf("*** (BAD) drg_write\n");
	        return A_ERROR;
        }
	    //A_NETBUF_FREE(pbuf);
    }
    
#ifdef DEBUG
    if (cmdId)
    {
        pFile= fopen("log\\debug_log.txt", "a");
		if (pFile)
		{
			fprintf (pFile,"send cmd %d -\n",cmdId);
			fclose(pFile);
		}
    }
#endif
    //printf("--- %s DRG_WRITE %f\n", GetTimeStamp(), GetTimeStampDouble() - StartTime);
    return length;
}

static TCHAR*  GetVidPidString(A_UINT32 subSystemID)
{
    switch(subSystemID){
        case 0x6041:
        case 0x6042:
        case 0x6043:
        case 0x6044:
        case 0x6045:
        case 0x6046:
        case 0x6047:
        case 0x6048:
        case 0x6049:
        case 0x6050:
        case 0x6051:
        case 0x6052:
        case 0x6053:
        case 0x6054:
        case 0x6099:
            return ((TCHAR*)psVID_PID_Mercury);
        case 0x6301:
        case 0x6302:
        case 0x6303:
        case 0x6304:
        case 0x6305:
        case 0x6306:
        case 0x6307:
        case 0x6308:
        case 0x6309:
        case 0x6310:
            return ((TCHAR*)psVID_PID_Venus);
        default:
            return ((TCHAR*)psVID_PID_Dragon);
    }
}

static A_BOOL
GetDevicePath(GUID Guid, PSP_DEVICE_INTERFACE_DETAIL_DATA *ppDeviceInterfaceDetail)
{
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    SP_DEVINFO_DATA DeviceInfoData;
    HDEVINFO hDevInfo;

    ULONG size;
    int count, i, index;
    BOOL status = TRUE;
    TCHAR *DeviceName = NULL;
    TCHAR *DeviceLocation = NULL;

    //
    //  Retreive the device information for all  devices.
    //
    hDevInfo = SetupDiGetClassDevs(&Guid,
                                   NULL,
                                   NULL,
                                   DIGCF_DEVICEINTERFACE |
                                   DIGCF_PRESENT/*DIGCF_ALLCLASSES*/);
                                    //Free ,

    //
    //  Initialize the SP_DEVICE_INTERFACE_DATA Structure.
    //
    DeviceInterfaceData.cbSize  = sizeof(SP_DEVICE_INTERFACE_DATA);

    //
    //  Determine how many devices are present.
    //
    count = 0;
    while(SetupDiEnumDeviceInterfaces(hDevInfo,
                                      NULL,
                                      &Guid,
                                      count++,  //Cycle through the available devices.
                                      &DeviceInterfaceData)
          );

    //
    // Since the last call fails when all devices have been enumerated,
    // decrement the count to get the true device count.
    //
    count--;

    //
    //  If the count is zero then there are no devices present.
    //
    if (count == 0) {
        printf("No devices are present and enabled in the system ---> %d.\n",GetLastError());
        return FALSE;
    }

    //
    //  Initialize the appropriate data structures in preparation for
    //  the SetupDi calls.
    //
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    //  Loop through the device list to allow user to choose
    //  a device.  If there is only one device, select it
    //  by default.
    //
    i = 0;
    while (SetupDiEnumDeviceInterfaces(hDevInfo,
                                       NULL,
                                       (LPGUID)&Guid,
                                       i,
                                       &DeviceInterfaceData)) {

        //
        // Determine the size required for the DeviceInterfaceData
        //
        SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &DeviceInterfaceData,
                                        NULL,
                                        0,
                                        &size,
                                        NULL);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("SetupDiGetDeviceInterfaceDetail failed, Error: %d", GetLastError());
            return FALSE;
        }

        *ppDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) A_MALLOC(size);

        if (!*ppDeviceInterfaceDetail) {
            printf("Insufficient memory.\n");
            return FALSE;
        }

        //
        // Initialize structure and retrieve data.
        //
        (*ppDeviceInterfaceDetail)->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        status = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                                 &DeviceInterfaceData,
                                                 *ppDeviceInterfaceDetail,
                                                 size,
                                                 NULL,
                                                 &DeviceInfoData);

        A_FREE(*ppDeviceInterfaceDetail);

        if (!status) {
            printf("SetupDiGetDeviceInterfaceDetail failed, Error: %d", GetLastError());
            return FALSE;
        }

        //
        //  Get the Device Name
        //  Calls to SetupDiGetDeviceRegistryProperty require two consecutive
        //  calls, first to get required buffer size and second to get
        //  the data.
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                        &DeviceInfoData,
                                        SPDRP_DEVICEDESC,
                                        NULL,
                                        (PBYTE)DeviceName,
                                        0,
                                        &size);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("SetupDiGetDeviceRegistryProperty failed, Error: %d", GetLastError());
            return FALSE;
        }

        DeviceName = (TCHAR*) A_MALLOC(size);
        if (!DeviceName) {
            printf("Insufficient memory.\n");
            return FALSE;
        }

        status = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                  &DeviceInfoData,
                                                  SPDRP_DEVICEDESC,
                                                  NULL,
                                                  (PBYTE)DeviceName,
                                                  size,
                                                  NULL);
        if (!status) {
            printf("SetupDiGetDeviceRegistryProperty failed, Error: %d",
                   GetLastError());
            A_FREE(DeviceName);
            return FALSE;
        }

        //
        //  Now retrieve the Device Location.
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DeviceInfoData,
                                         SPDRP_LOCATION_INFORMATION,
                                         NULL,
                                         (PBYTE)DeviceLocation,
                                         0,
                                         &size);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            DeviceLocation = (TCHAR*) A_MALLOC(size);

            if (DeviceLocation != NULL) {

                status = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                          &DeviceInfoData,
                                                          SPDRP_LOCATION_INFORMATION,
                                                          NULL,
                                                          (PBYTE)DeviceLocation,
                                                          size,
                                                          NULL);
                if (!status) {
                    A_FREE(DeviceLocation);
                    DeviceLocation = NULL;
                }
            }

        } else {
            DeviceLocation = NULL;
        }

        //
        // If there is more than one device print description.
        //
        if (count > 1 ) {
            printf("%d- ", i);
        }

        printf("%s\n", DeviceName);

        if (DeviceLocation) {
            printf("        %s\n", DeviceLocation);
        }

        A_FREE(DeviceName);
        DeviceName = NULL;

        if (DeviceLocation) {
            A_FREE(DeviceLocation);
            DeviceLocation = NULL;
        }

        i++; // Cycle through the available devices.
    }

    //
    //  Select device.
    //
    index = 0;
    if (count > 1) {
        printf("\nSelect Device: ");

        if (scanf("%d", &index) == 0) {
            return FALSE;
        }
    }

    //
    //  Get information for specific device.
    //
    status = SetupDiEnumDeviceInterfaces(hDevInfo,
                                            NULL,
                                            (LPGUID)&Guid,
                                            index,
                                            &DeviceInterfaceData);

    if (!status) {
        printf("SetupDiEnumDeviceInterfaces failed, Error: %d", GetLastError());
        return FALSE;
    }

    //
    // Determine the size required for the DeviceInterfaceData
    //
    SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                    &DeviceInterfaceData,
                                    NULL,
                                    0,
                                    &size,
                                    NULL);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        printf("SetupDiGetDeviceInterfaceDetail failed, Error: %d", GetLastError());
        return FALSE;
    }

    *ppDeviceInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) A_MALLOC(size);

    if (!*ppDeviceInterfaceDetail) {
        printf("Insufficient memory.\n");
        return FALSE;
    }

    //
    // Initialize structure and retrieve data.
    //
    (*ppDeviceInterfaceDetail)->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    status = SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                             &DeviceInterfaceData,
                                             *ppDeviceInterfaceDetail,
                                             size,
                                             NULL,
                                             &DeviceInfoData);   
    
    if (!status) {
        printf("SetupDiGetDeviceInterfaceDetail failed, Error: %d", GetLastError());
        return FALSE;
    }

    return TRUE;
}

static A_BOOL GetDeviceHandle(GUID Guid, HANDLE *phDevice)
{
    A_BOOL status = TRUE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetail=NULL;

    if (pDeviceInterfaceDetail == NULL) {
        status = GetDevicePath(Guid, &pDeviceInterfaceDetail);
    }
    if (pDeviceInterfaceDetail == NULL) {
        status = FALSE;
    }

    if (status == TRUE)
    {
        //
        //  Get handle to device.
        //
        *phDevice = CreateFile(pDeviceInterfaceDetail->DevicePath,
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (*phDevice  == INVALID_HANDLE_VALUE) {
            status = FALSE;
            printf("CreateFile failed.  Error:%d", GetLastError());
        }
        A_FREE(pDeviceInterfaceDetail);
    }

    return status;
}

int SDIO_DisableDragonSleep(HANDLE device)
{

    int status = A_OK;
    A_UINT32 address;
    A_UCHAR cmdCredits = 1;
    HIF_DEVICE *hifDevice = getHifDevice(device);

    address = 0x46a ;
    status = HIFReadWrite(hifDevice, address, &cmdCredits, sizeof(cmdCredits),
                            HIF_RD_SYNC_BYTE_INC, NULL);

    if (status != A_OK) {
        printf("Unable to read sleep regiter\n");
        return A_ERROR;
    }
    printf("read sleep regiter %d\n",cmdCredits);

    cmdCredits|=1;
    
    address = 0x46a ;
    status = HIFReadWrite(hifDevice, address, &cmdCredits, sizeof(cmdCredits),
                            HIF_WR_SYNC_BYTE_INC, NULL);
    if (status != A_OK) {
        printf("Unable to decrement the command credit count register\n");
        return A_ERROR;
    }

    address = 0x46a ;
    status = HIFReadWrite(hifDevice, address, &cmdCredits, sizeof(cmdCredits),
                            HIF_RD_SYNC_BYTE_INC, NULL);

    if (status != A_OK) {
        printf("Unable to read sleep regiter\n");
        return A_ERROR;
    }
    printf("read sleep regiter %d\n",cmdCredits);  
    return A_OK;
}

static A_BOOL DumpDeviceWithInfo(HDEVINFO Devs, PSP_DEVINFO_DATA DevInfo, LPCTSTR Info)
/*++

Routine Description:

    Write device instance & info to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
{
    TCHAR devID[MAX_DEVICE_ID_LEN];
	DWORD requiredSize;
    //LPTSTR desc;
    A_BOOL b = TRUE;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
			(SetupDiGetDeviceInstanceId(Devs, DevInfo, devID, MAX_DEVICE_ID_LEN, &requiredSize) == FALSE))
    {
        strcpy(devID, TEXT("?"));
        b = FALSE;
    }

    if(Info)
    {
        printf(TEXT("%-60s: %s\n"), devID, Info);
    }
    else
    {
        printf(TEXT("%s\n"), devID);
    }
    return b;
}

static A_BOOL AdapterOnOff(BOOL bEnable, TCHAR * pszGuid, TCHAR *pszDescription, GUID* FileGUID, DWORD RequiredSize)
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i,j;
    DWORD bufSize,dwCount;
    TCHAR szHardwareID[512], szMatchDID[512], szPCIClass[50];  
    SP_PROPCHANGE_PARAMS pcp;
    A_BOOL bRet = FALSE,bFound = FALSE;
    SP_DEVINSTALL_PARAMS devParams;
    HKEY hKey;
    A_BOOL bRetVal = FALSE;
       
#if 0
    sprintf(szPCIClass, "sd\\vid_0271&pid_010b");  //dragon
#endif
    _tcscpy (szPCIClass, pszGuid);


    for(j = 0; j < RequiredSize; j ++)
    {
        //  printf("[yy]: j is %d\n",j);
        // Create a Device Information Set with all present devices.
        DeviceInfoSet = SetupDiGetClassDevs(&FileGUID[j], // All Classes
                                            NULL,
                                            NULL, 
                                            DIGCF_PRESENT);                
        if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        {   
            printf("[yy]: DeviceInfoSet is INVALID_HANDLE_VALUE!!!\n");
            break;
        }

        //
        //  Enumerate through all Devices.
        //
    
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        for (i=0; SetupDiEnumDeviceInfo(DeviceInfoSet,i,&DeviceInfoData);i++)
        {
            bufSize =sizeof(szHardwareID);  
            //Get required buffer size to Alloc the nessicary space.    
                                                        
            #if 1
            {
                hKey = SetupDiOpenDevRegKey(DeviceInfoSet,&DeviceInfoData,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_ALL_ACCESS);
                if(hKey==INVALID_HANDLE_VALUE)
                {
                    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,&DeviceInfoData,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_READ);
                }
                if(hKey!=INVALID_HANDLE_VALUE)
                {
                    bufSize = sizeof(szHardwareID); //reset the size
                    if(RegQueryValueEx(hKey,TEXT("MatchingDeviceId"),0,NULL,(LPBYTE)szHardwareID,&bufSize) != ERROR_SUCCESS)
                    {
                        printf("[yy]: Can't read value from MatchingDeviceId!!!\n");
                        RegCloseKey(hKey);
                        continue;
                    }

                    RegCloseKey(hKey);

                    //printf("MatchingDeviceId is %s, pszGuid is %s!!!\n",szHardwareID,pszGuid);

                    if(_tcsstr(_tcslwr(szHardwareID),pszGuid) != NULL) //&yw-110403 find device 
                    {
                        bFound = TRUE;
                    }
                    else
                    {//&yw-091106
                        _tcscpy(szMatchDID, szHardwareID);
                        bufSize =sizeof(szHardwareID); //reset the size
                        if(SetupDiGetDeviceInstanceId(DeviceInfoSet, &DeviceInfoData, szHardwareID, bufSize, &dwCount))
                        {
                            _tprintf(TEXT("InstanceId is %s!!!\n"),szHardwareID);
                            if((_tcsstr(_tcslwr(szHardwareID),pszGuid)!= NULL)&&(_tcsstr(_tcslwr(szMatchDID),szPCIClass)!=NULL))
                            {
                                bFound = TRUE;
                            }
                            /* 
                             * ADD for multiple id of Dragon SDIO Card,
                             * may be introduce some problem, to be tested ...
                             */
                            if( _tcsstr(_tcslwr(szHardwareID), _tcslwr(szMatchDID))!= NULL)
                            {
                                bFound = TRUE;
                            }
                        }
                    }
                }
                else
                {
                    printf("[yy]: hKey is INVALID_HANDLE_VALUE!!!\n");
                }
            }
            #else
            {
                if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                     &DeviceInfoData,
                                                     SPDRP_DEVICEDESC,
                                                     NULL,
                                                     (PBYTE)szHardwareID,
                                                     bufSize,
                                                     NULL))
                {
                    if(_tcsstr(szHardwareID,pszDescription)) //find device
                    {
                        bFound = TRUE;
                        hKey = SetupDiOpenDevRegKey(DeviceInfoSet,&DeviceInfoData,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_READ);
                        if(hKey!=INVALID_HANDLE_VALUE)
                        {
                            bufSize = sizeof(szHardwareID); //reset the size
                            if(RegQueryValueEx(hKey,TEXT("MatchingDeviceId"),0,NULL,(LPBYTE)szHardwareID,&bufSize) == ERROR_SUCCESS)
                            {
                                RegCloseKey(hKey);
                                SetWin9xConfigFlag(szHardwareID,pszDescription, bEnable);
                            }
                            else
                            {
                                RegCloseKey(hKey);
                            }
                        }
                    }
                }
            }
            #endif
                                                
            if(bFound == TRUE)
            {
                printf("[yy]: Found the device!!!\n");

                //Sleep(3000);

                if(NULL!=g_handle)
                {
                    printf("[yy]: bEnable is %d......\n",bEnable);
                    //TONY: crash debug build while(CloseHandle(g_handle));
                    CloseHandle(g_handle);
                    g_handle = NULL;
                }
                else
                {
                    printf("[yy]: g_handle is NULL!bEnable is %d......\n",bEnable);
                }

                pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
                if(bEnable)
                {
                    pcp.StateChange = DICS_ENABLE;
                }
                else
                {
                    pcp.StateChange = DICS_DISABLE;
                }

                pcp.Scope =DICS_FLAG_CONFIGSPECIFIC  ;//DICS_FLAG_CONFIGSPECIFIC;
                pcp.HwProfile = 0;

                printf("[yy]: test spec1\n");
                if(!SetupDiSetClassInstallParams(DeviceInfoSet,
                    &DeviceInfoData,&pcp.ClassInstallHeader,sizeof(pcp)) ||
                    !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,DeviceInfoSet,&DeviceInfoData))
                {
                    //    
                    // failed to invoke DIF_PROPERTYCHANGE
                    //
                    //DumpDeviceWithInfo(DeviceInfoSet,&DeviceInfoData,pControlContext->strFail);
                    printf("[yy]: AT LAST FAIL!!!\n");
                } 
                else
                {
                    DumpDeviceWithInfo(DeviceInfoSet,&DeviceInfoData,"fffffffff");
                    devParams.cbSize = sizeof(devParams);
                    if(SetupDiGetDeviceInstallParams(DeviceInfoSet,&DeviceInfoData,&devParams) &&
                        (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT)))
                    {
                        printf("[yy]:yes,need rebbot!!!\n");
                        //devParams.Flags &= ~(DI_NEEDRESTART|DI_NEEDREBOOT);
                        //SetupDiSetDeviceInstallParams(DeviceInfoSet,&DeviceInfoData,&devParams);
                        //SetupDiChangeState(DeviceInfoSet,&DeviceInfoData);
                    }
                }
            }
        }

        if(bFound == TRUE)
        {
            //CM_Reenumerate_DevNode(DeviceInfoData.DevInst, 0);
        }

        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        if(bFound == TRUE) break;
    }           
    Sleep(1000);
    return bRet;
}

A_BOOL  SDIO_loadDriver_ENE(A_BOOL bOnOff, A_UINT32 subSystemID)
{    
    GUID guid[3];
    TCHAR* vidStr;

    guid[0] = GUID_DEVINTERFACE_SDRAW;
    guid[1] = GUID_DEVINTERFACE_SDRAW2;

    vidStr = GetVidPidString(subSystemID);
    arSubSystemID = subSystemID;
    AdapterOnOff(bOnOff, vidStr, NULL, guid, 2);  //for both mercury and dragon 
	return TRUE;
}

A_BOOL  SDIO_closehandle(void)
{
    if(NULL!=g_handle)
    {
        while(CloseHandle(g_handle));
        g_handle = NULL;
    }
    printf("Devdll: to close handle--->\n");
    return TRUE;
}

static HANDLE GetArDeviceHandle(int devIndex)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)ar6000_devices[devIndex];
    return (ar->arDeviceHandle);
}

static int GetDeviceIndex (HANDLE hDevice)
{
    AR_SOFTC_T *ar;
    int i;

    for (i = 0; i < MAX_AR6000; ++i)
    {
        ar = (AR_SOFTC_T *)ar6000_devices[i];
        if (ar->arDeviceHandle == hDevice)
        {
            return i;
        }
    }
    return -1;
}

int  SDIO_BMIWriteSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 param)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIWriteSOCRegister(device, address, param);
}

int SDIO_BMIReadSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 *param)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIReadSOCRegister(device, address, param);
}

int SDIO_BMIWriteMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIWriteMemory(device, address, buffer, length);
}

int SDIO_BMIReadMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIReadMemory(device, address, buffer, length);
}

int SDIO_BMISetAppStart_win(HANDLE handle, A_UINT32 address)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMISetAppStart(device, address);
}

int SDIO_BMIDone_win(HANDLE handle)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIDone(device);
}

int SDIO_BMIFastDownload_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIFastDownload(device, address, buffer, length);
}

int SDIO_BMIExecute_win(HANDLE handle, A_UINT32 address, A_UINT32 *param)
{
    HIF_DEVICE *device = getHifDevice(handle);
	return BMIExecute(device, address, param);
}

int SDIO_BMITransferEepromFile_win(HANDLE handle, A_UCHAR *eeprom, A_UINT32 length)
{
    A_UINT32 board_data_addr;
    A_UINT32 board_data_size;
    A_UINT32 board_ext_data_size;
    A_UINT32 one = 1;
    AR_SOFTC_T *ar = AR6000_GET(GetDeviceIndex(handle));
    HIF_DEVICE *device = getHifDevice(handle);

    board_data_size = (((ar)->arTargetType == TARGET_TYPE_AR6002) ? AR6002_BOARD_DATA_SZ : \
                          (((ar)->arTargetType == TARGET_TYPE_AR6003) ? AR6003_BOARD_DATA_SZ : \
                           (((ar)->arTargetType == TARGET_TYPE_MCKINLEY) ? MCKINLEY_BOARD_DATA_SZ : 0)));

    board_ext_data_size = (((ar)->arTargetType == TARGET_TYPE_AR6002) ? AR6002_BOARD_EXT_DATA_SZ : \
                               (((ar)->arTargetType == TARGET_TYPE_AR6003) ? AR6003_BOARD_EXT_DATA_SZ : \
                               (((ar)->arTargetType == TARGET_TYPE_MCKINLEY) ? MCKINLEY_BOARD_EXT_DATA_SZ : 0)));
    
    if (length != board_data_size + board_ext_data_size)
    {
        if (length == board_data_size)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - WARNING mismatch in length of eeprom file (%d); expexted (%d)\n",
                                            length, board_data_size + board_ext_data_size));
            printf("BMITransferEepromFile_win - WARNING mismatch in length of eeprom file (%d); expexted (%d)\n",
                                            length, board_data_size + board_ext_data_size);
        }
        else
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - invalid eeprom length \n"));
            return A_ERROR;
        }
    }    
    // Determine where in Target RAM to write Board Data
    if (BMIReadMemory(device, HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_data), (A_UCHAR*)&board_data_addr, sizeof(board_data_addr)) != A_OK)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - get hi_board_data failed \n"));
        return A_ERROR;
    }
    if (board_data_addr == 0)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("hi_board_data is zero\n"));
        return A_ERROR;
    }

    // If length of the eeprom data > 1024, the 1st 1024 bytes will be written to hi_board_data;
    // the rest will be written to hi_board_ext_data
    printf ("BMITransferEepromFile_win - %d bytes to Target RAM at 0x%08x\n", board_data_size, board_data_addr);
    
    // Write eeprom
    if (BMIWriteMemory(device, board_data_addr, eeprom, board_data_size) != A_OK)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - write eeprom failed \n"));
        return A_ERROR;
    }

    // Record the fact that Board Data IS initialized */
    if (BMIWriteMemory(device, HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_data_initialized),
                        (A_UCHAR *)&one, sizeof(A_UINT32)) != A_OK)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - write hi_board_data_initialized failed \n"));
        return A_ERROR;
    }

    if (length > board_data_size)
    {
        if (BMIReadMemory(device, HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_ext_data), (A_UCHAR*)&board_data_addr, sizeof(board_data_addr)) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - get hi_board_ext_data failed \n"));
            return A_ERROR;
        }
        if (board_data_addr == 0)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("hi_board_ext_data is zero\n"));
            return A_ERROR;
        }
        
        printf ("BMITransferEepromFile_win - %d bytes to ext Target RAM at 0x%08x\n", board_ext_data_size, board_data_addr);
    
        // Write eeprom
        if (BMIWriteMemory(device, board_data_addr, eeprom + board_data_size, board_ext_data_size) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - write eeprom to ext data failed \n"));
            return A_ERROR;
        }
        /* Record the fact that Board Data IS initialized */
        one = (board_ext_data_size << 16) | 1;
        if (BMIWriteMemory(device, HOST_INTEREST_ITEM_ADDRESS(ar->arTargetType, hi_board_ext_data_config),
                        (A_UCHAR *)&one, sizeof(A_UINT32)) != A_OK)
        {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("BMITransferEepromFile_win - write hi_board_ext_data_config failed \n"));
            return A_ERROR;
        }
    }

    return A_OK;
}


