#include "core.h"
#include "debug.h"
#include "sda.h"

#ifdef DIRECT_AUDIO_SUPPORT

#define SDA_DEBUG	1

#define DIRECT_AUDIO_LLC_TYPE 0x43FF
#if SDA_DEBUG
#define MAX_BUF_SIZE 1500 // bytes
#define MAX_SHARE_MEM_ITEM	32
#endif
#define MAX_TX_POOL_ID 5

struct Direct_Audio_Context {
	u32		last_tx_process_time;
	u32		max_tx_process_time;
	u32		last_rx_process_time;
	u32		max_rx_process_time;
	struct ath6kl *ar;
	u8		vif_id;
};

static struct Direct_Audio_Context	DA_context;
SDA_SendDoneCallBack SDA_TX_DONE_CB = NULL;
SDA_RecvCallBack SDA_RX_READY_CB = NULL;

struct Direct_Audio_Setting_str {
    u8  *pAddr;
	u32 sizeTotal;
	u32 lenUnit;
	u32 lenHeadroom;
	u8  *current_pAddr; 
}__packed;

static struct Direct_Audio_Setting_str global_TX_DA_Setting[MAX_TX_POOL_ID];
static struct Direct_Audio_Setting_str global_RX_DA_Setting;

#define TX_DA_INIT_ADDR(id)   global_TX_DA_Setting[id].pAddr
#define TX_DA_ADDR(id)        global_TX_DA_Setting[id].current_pAddr
#define TX_DA_POOL_SIZE(id)   global_TX_DA_Setting[id].sizeTotal
#define TX_DA_UNIT_LEN(id)    global_TX_DA_Setting[id].lenUnit
#define TX_DA_HEAD_ROOM(id)   global_TX_DA_Setting[id].lenHeadroom

static void Direct_Audio_Tx_Setting(u32 BufferId,u8 *pAddr, u32 sizeTotal, u16 lenUnit, u16 lenHeadroom)
{
	if(BufferId >= MAX_TX_POOL_ID)
	{
		ath6kl_err("%s: BufferId %d excess MAX_TX_POOL_ID %d\n", __func__, BufferId, MAX_TX_POOL_ID);
		return;
	}
    TX_DA_INIT_ADDR(BufferId) = pAddr;
	TX_DA_ADDR(BufferId) = pAddr;
	TX_DA_POOL_SIZE(BufferId) = sizeTotal;
	TX_DA_UNIT_LEN(BufferId) = lenUnit;
	TX_DA_HEAD_ROOM(BufferId) = lenHeadroom;
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]BufferId=%d,pAddr=0%p,sizeTotal=%d,lenUnit=%d,lenHeadroom=%d\r\n",
	        __func__, __LINE__, BufferId, pAddr, sizeTotal, lenUnit, lenHeadroom);	
}

#define RX_DA_INIT_ADDR   global_RX_DA_Setting.pAddr
#define RX_DA_ADDR        global_RX_DA_Setting.current_pAddr
#define RX_DA_POOL_SIZE   global_RX_DA_Setting.sizeTotal
#define RX_DA_UNIT_LEN    global_RX_DA_Setting.lenUnit
#define RX_DA_HEAD_ROOM   global_RX_DA_Setting.lenHeadroom

static void Direct_Audio_Rx_Setting(u8 *pAddr, u32 sizeTotal, u32 lenUnit, u32 lenHeadroom)
{
    RX_DA_INIT_ADDR = pAddr;
	RX_DA_ADDR = pAddr;
	RX_DA_POOL_SIZE = sizeTotal;
	RX_DA_UNIT_LEN = lenUnit;
	RX_DA_HEAD_ROOM = lenHeadroom;
}

//SDA_SendDoneCallBack  SDA_TX_DONE_CB = NULL;
#define SDA_BUF_READY(_desc)      	!!_desc->m_ReadyToCopy
#define TX_SDA_INIT_ADDR			orig_data_p
#define TX_SDA_ADDR 				descp_p
#define TX_SDA_POOL_SIZE			BufferSize
#define TX_SDA_UNIT_LEN(id)				TX_DA_UNIT_LEN(id)
#define CLR_SDA_BUF_READY(_desc)  	_desc->m_ReadyToCopy = 0
#define SET_SDA_BUF_READY(_desc)  	_desc->m_ReadyToCopy = 1
static void SDA_Tx_fun( struct net_device *dev, u32 BufferId, u8 *pBuffer, u32 BufferSize)
{
	u8 *orig_data_p = pBuffer;
	u8 *data_p = pBuffer;
	u8 *descp_p = pBuffer;
	struct SDA_Descriptor *tx_desc = (struct SDA_Descriptor *) data_p;
	if(BufferId >= MAX_TX_POOL_ID)
	{
		ath6kl_err("%s: BufferId excess %d\n", __func__, MAX_TX_POOL_ID);
		goto tx_fail;
	}
	if (!data_p || !SDA_BUF_READY(tx_desc)) {
		//Should not into here
		ath6kl_err("%s: descriptor show not ready\n", __func__);
		goto tx_fail;
	}
	/*Check descriptor*/
	do {
		struct sk_buff *skb;
		int send_len = 0;
		struct ethhdr *eth_hdr;
		int alloc_size = 0;

		/* point to RAW_Data
		* Assume offset include Descriptor & Headroom 
		*/
		data_p += sizeof(SDA_Header_t);
		/*Create skb, copy data, fill skb header*/
		alloc_size = tx_desc->m_PayloadSize+sizeof(struct ethhdr);
		skb = ath6kl_buf_alloc(alloc_size);
		if (skb == NULL) {
			//should not happened
			ath6kl_err("%s: Allocate skb failed\n", __func__);
			break;
		}

		skb_put(skb, alloc_size);
		memset(skb->data, 0, alloc_size);
		//Copy data from share buf to skb
		memcpy(skb->data, data_p-sizeof(struct ethhdr), tx_desc->m_PayloadSize+sizeof(struct ethhdr));
		send_len += tx_desc->m_PayloadSize+sizeof(struct ethhdr);
		
		
		eth_hdr = (struct ethhdr *)(skb->data);
		skb->protocol = ntohs(eth_hdr->h_proto);	
		skb->dev = dev;

		/*Adjust the length of data before send*/ 
		skb_trim(skb, send_len);
		ath6kl_dbg(ATH6KL_DBG_DA, "%s[%d]BufferId=%d,audio len = %d,data len = %d\n",
		        __func__, __LINE__, BufferId, tx_desc->m_PayloadSize, send_len);
		/*{
			int kkk;
			for(kkk = 0;kkk<send_len;kkk++) {
				if(kkk % 16 == 0)
					printk("\n\r");
				printk("%02x ",skb->data[kkk]);
			}
			printk("\n\r");
		}*/
		//Call normal tx path
		ath6kl_data_tx(skb, dev, true);

		//Clear share buffer Ready flag
		CLR_SDA_BUF_READY(tx_desc);
		//To-do: call SS complete routine

		//Next frame & check if need retrun pAddr of DA Share buff
		if(((TX_SDA_ADDR - TX_SDA_INIT_ADDR)+TX_SDA_UNIT_LEN(BufferId)) >= TX_SDA_POOL_SIZE ) {
			/*Return to Init buf addr*/
			TX_SDA_ADDR = TX_SDA_INIT_ADDR;
		} else {
			/*Go to Next buf*/
			TX_SDA_ADDR += TX_SDA_UNIT_LEN(BufferId);	   
		}
		data_p = TX_SDA_ADDR;
		tx_desc = (struct SDA_Descriptor *) data_p;
	   
	} while(SDA_BUF_READY(tx_desc));//Check if having next
tx_fail:
	if (SDA_TX_DONE_CB)
		SDA_TX_DONE_CB(BufferId,pBuffer,BufferSize);
}

/*Assume The RX Already to be Packet Mode*/
void SDA_Rx_fun( struct net_device *dev, struct sk_buff *skb)
{
    u32 tsf_ie=0, tsf_data=0;
    u8 *data_p = RX_DA_ADDR;
    struct SDA_Descriptor *rx_desc = (struct SDA_Descriptor *) data_p;

    if (skb == NULL) {
		ath6kl_err("%s: input buf should not be NULL\n", __func__);
		return;
	}
	if (!data_p) {
		ath6kl_err("%s: DSP dose not register rx pool\n", __func__);
		dev_kfree_skb(skb);
		return;
	}

	if(SDA_BUF_READY(rx_desc)) {
	   //the share buf not available. DO NOT PUT PRINT LOG HERE.
	   goto rx_fail;
	}
	
#define TSF_IE 0x1234fedc
    tsf_ie = le32_to_cpu(*((u32 *)(skb->data + skb->len - 2*sizeof(u32))));
    if (tsf_ie == TSF_IE){
      skb_trim(skb, (skb->len - 2*sizeof(u32)));

      tsf_data = le32_to_cpu(*((u32 *)(skb->data + skb->len + sizeof(u32))));
      //printk("~~current-tsf (%x) \n",tsf_data);
    }
	//Fill derscriptort
	rx_desc->m_PayloadSize = skb->len;//no ethernet header included and remove TSF_IE
    rx_desc->m_TimeStamp = 0 ;

	if (skb->len > (RX_DA_UNIT_LEN-sizeof(struct SDA_Descriptor)-RX_DA_HEAD_ROOM)) {
	    //Should not happen
		ath6kl_err("%s: Not having enough room for RX DATA\n", __func__);
		skb->len = RX_DA_UNIT_LEN-sizeof(struct SDA_Descriptor)-RX_DA_HEAD_ROOM;
	}
	//copy to share buf & Skip the length of "struct ethhdr"
	memcpy((data_p + sizeof(struct SDA_Descriptor) + RX_DA_HEAD_ROOM) - sizeof(struct ethhdr),
           skb->data -sizeof(struct ethhdr), 
		   skb->len + sizeof(struct ethhdr));
	
	// Set rx timestamp
	memcpy(data_p + sizeof(struct SDA_Descriptor) + RX_DA_HEAD_ROOM +sizeof(u32),
			&tsf_data, sizeof(u32));	

	//To-do: :Call SS Direct Audio RX routine
	ath6kl_dbg(ATH6KL_DBG_DA, "%s[%d]audio len=%d\n\r",__func__,__LINE__,rx_desc->m_PayloadSize);
	/*{
		int kkk;
		u8	*debug_ptr = data_p+sizeof(struct SDA_Descriptor) + RX_DA_HEAD_ROOM - sizeof(struct ethhdr);
		for(kkk = 0;kkk<(rx_desc->m_PayloadSize+sizeof(struct ethhdr));kkk++) {
			if(kkk % 16 == 0)
				printk("\n\r");
			printk("%02x ",debug_ptr[kkk]);
		}
		printk("\n\r");
	}*/	
	SET_SDA_BUF_READY(rx_desc);
	if (SDA_RX_READY_CB)
		SDA_RX_READY_CB((u8*)rx_desc,RX_DA_UNIT_LEN);

	//Next DA Share buff
	if(((RX_DA_ADDR - RX_DA_INIT_ADDR)+RX_DA_UNIT_LEN) >= RX_DA_POOL_SIZE ) {
	    /*Return to Init buf addr*/
	    RX_DA_ADDR = RX_DA_INIT_ADDR;
	} else {
	    /*Go to Next buf*/
	    RX_DA_ADDR += RX_DA_UNIT_LEN;	   
	}
rx_fail:	
    dev_kfree_skb(skb);
}

void SDA_function4Send(unsigned int BufferId, unsigned char *pBuffer, unsigned int BufferSize)
{
	struct ath6kl_vif *vif;
	u32	entry_time;
	if (DA_context.ar == NULL) {
		ath6kl_err("%s: DA_context.ar should not be NULL\n", __func__);
		return;
	}
	vif = ath6kl_get_vif_by_index(DA_context.ar, DA_context.vif_id);
	if (!vif) {
		ath6kl_err("%s: Failed to find vif for direct audio, DA_context.ar=%p, DA_context.vif_id=%d\n", __func__, DA_context.ar, DA_context.vif_id);
		return;
	}
	entry_time = jiffies;
	SDA_Tx_fun(vif->ndev,BufferId,pBuffer,BufferSize);
	DA_context.last_tx_process_time = jiffies - entry_time;
	
	if (DA_context.max_tx_process_time < DA_context.last_tx_process_time)
		DA_context.max_tx_process_time = DA_context.last_tx_process_time;
}

void SDA_function4RecvDone(unsigned char *pBuffer, unsigned int BufferSize)
{
	//what should we do in this API
	ath6kl_dbg(ATH6KL_DBG_DA, "%s[%d]pBuffer=0x%p,BufferSize=%d\n\r",__func__,__LINE__,pBuffer,BufferSize);
}

void SDA_setSharedMemory4Send(unsigned int BufferId, unsigned char *pBufferTotal, unsigned int BufferTotalSize, unsigned int BufferUnitSize, unsigned int HeadroomSize)
{
	Direct_Audio_Tx_Setting(BufferId,pBufferTotal, BufferTotalSize, BufferUnitSize, HeadroomSize);	
}

void SDA_setSharedMemory4Recv(unsigned char *pBufferTotal, unsigned int BufferTotalSize, unsigned int BufferUnitSize, unsigned int HeadroomSize)
{
	Direct_Audio_Rx_Setting(pBufferTotal, BufferTotalSize, BufferUnitSize, HeadroomSize);
}

//Register callback which Mck notify DSP that Mck has received the audio packet
void SDA_registerCallback4Recv(SDA_RecvCallBack pCallback)
{
	SDA_RX_READY_CB = pCallback;
}

//Register callback which Mck notify DSP that Mck has sent the audio packet
void SDA_registerCallback4SendDone(SDA_SendDoneCallBack pCallback)
{
	ath6kl_dbg(ATH6KL_DBG_DA, "%s[%d]\n\r",__func__,__LINE__);
	SDA_TX_DONE_CB = pCallback;
}

void Direct_Audio_debug_dump(void)
{
	ath6kl_dbg(ATH6KL_DBG_DA,"DA_context.ar = 0x%x\n\r",(int)DA_context.ar);
	ath6kl_dbg(ATH6KL_DBG_DA,"DA_context.last_tx_process_time = %u\n\r",DA_context.last_tx_process_time);
	ath6kl_dbg(ATH6KL_DBG_DA,"DA_context.max_tx_process_time = %u\n\r",DA_context.max_tx_process_time);
	ath6kl_dbg(ATH6KL_DBG_DA,"DA_context.last_rx_process_time = %u\n\r",DA_context.last_rx_process_time);
	ath6kl_dbg(ATH6KL_DBG_DA,"DA_context.max_rx_process_time = %u\n\r",DA_context.max_rx_process_time);
}

void Direct_Audio_init(struct ath6kl *ar)
{
	int i;
	
	memset(&global_RX_DA_Setting, 0x0, sizeof(global_RX_DA_Setting));
	for (i = 0; i < MAX_TX_POOL_ID; i++)
	    memset(&global_TX_DA_Setting[i], 0x0, sizeof(global_TX_DA_Setting));
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]\n\r",__func__,__LINE__);

	memset(&DA_context,0x00,sizeof(DA_context));

	DA_context.ar = ar;
	DA_context.vif_id = 1;//use p2p0 as default
}

void Direct_Audio_deinit(struct ath6kl *ar)
{
	#if SDA_DEBUG//only for debug,will remove after integrate SS DSP side
	if(RX_DA_INIT_ADDR)
		kfree(RX_DA_INIT_ADDR);
	#endif
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]\n\r",__func__,__LINE__);
}

void SDA_GetTSF(unsigned int *pTsf)
{
    struct ath6kl_vif *vif;
    u32 datah = 0, datal = 0;
#define TSF_TO_TU(_h,_l)    ((((u64)(_h)) << 32) | ((u64)(_l)))
#define REG_TSF_L 0x5054
#define REG_TSF_H 0x5058
	
	if (DA_context.ar == NULL) {
		ath6kl_err("%s: DA_context.ar should not be NULL\n", __func__);
		return;
	}
	vif = ath6kl_get_vif_by_index(DA_context.ar, DA_context.vif_id);
	if (!vif)
		return;
		
	ath6kl_diag_read32(vif->ar, REG_TSF_L, &datal);
	ath6kl_diag_read32(vif->ar, REG_TSF_H, &datah);
	*((u64 *)pTsf)  = TSF_TO_TU(datah, datal);
}

#if SDA_DEBUG//debug use
struct D_A_SHARE_MEM {
    SDA_Header_t	desc;
	u8				data[MAX_BUF_SIZE];
};

static void debug_tx_done_cb(unsigned int BufferId, unsigned char *pBuffer, unsigned int BufferSize)
{
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]BufferId=%d,pBuffer=0x%p,BufferSize=%d\n\r",
	        __func__, __LINE__, BufferId, pBuffer, BufferSize);
}

int Direct_Audio_TX_debug(void) 
{
	u8	*buf;
	u32 len,i,BufferSize;
	u8	*ptr,*eth_ptr;
	SDA_Descriptor_t *share_mem_ptr;
	struct ath6kl_vif *vif;
	u8	test_mymac[6] = {0x00,0x03,0x7f,0x20,0x52,0xa4};
	u8	test_peermac[6] = {0x84,0xc9,0xb2,0x11,0xf3,0x41};
	struct ethhdr *eth_hdr;

	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]Simulate TX behavior\n\r",__func__,__LINE__);
	SDA_registerCallback4SendDone(debug_tx_done_cb);

	len = (sizeof(SDA_Header_t)+MAX_BUF_SIZE)*MAX_SHARE_MEM_ITEM;
	buf = kmalloc(len, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;
	SDA_setSharedMemory4Send(0, buf, len, sizeof(SDA_Header_t)+MAX_BUF_SIZE, 50);		
	//SDA_registerIf(0);//use wlan0
	ptr	= buf;
	//construct tx share memory
	vif = ath6kl_get_vif_by_index(DA_context.ar, DA_context.vif_id);
	if(!vif)
		return -ENOMEM;	
	memcpy(test_peermac,vif->bssid,6);
	memcpy(test_mymac,vif->ndev->dev_addr,6);
	memset(buf,0x00,len);
	share_mem_ptr = (SDA_Descriptor_t *)buf;
	BufferSize = 0;
	
	for (i=0; i<MAX_SHARE_MEM_ITEM; i++) {
		//fill descript
		share_mem_ptr->m_ReadyToCopy = 1;
		share_mem_ptr->m_PayloadSize = 5*(i+1);
		share_mem_ptr->m_TimeStamp = 0;
		//fill ethernet header
		eth_ptr = ptr+sizeof(SDA_Header_t)-sizeof(struct ethhdr);
		eth_hdr = (struct ethhdr *)eth_ptr;
		memcpy(eth_hdr->h_dest,test_peermac,6);
		memcpy(eth_hdr->h_source,test_mymac,6);
		eth_hdr->h_proto = htons(DIRECT_AUDIO_LLC_TYPE);	
		
		memset(ptr+sizeof(SDA_Header_t),i,share_mem_ptr->m_PayloadSize);
		BufferSize += sizeof(SDA_Header_t) + MAX_BUF_SIZE;
		ptr = ptr + sizeof(SDA_Header_t) + MAX_BUF_SIZE;
		share_mem_ptr = (SDA_Descriptor_t *)ptr;
	}
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]BufferId=%d,pBuffer=0x%p,BufferSize=%d\n\r",
	        __func__, __LINE__, 0, buf, BufferSize);
	//notify Direct Audio module to send packet
	SDA_function4Send(0, buf, BufferSize);

	kfree(buf);
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]Simulate TX finish\n\r",__func__,__LINE__);
	return 0;
}

static void debug_rx_ready_cb(unsigned char *pBuffer, unsigned int BufferSize)
{
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]pBuffer=0x%p,BufferSize=%d\n\r",
	        __func__, __LINE__, pBuffer, BufferSize);
}

int Direct_Audio_RX_debug(void)
{
	u8	*buf;
	u32 len;
	
	if(RX_DA_INIT_ADDR) {
		ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]already rx debug\n\r",__func__,__LINE__);
		return -1;
	}
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]SimulateRX behavior\n\r",__func__,__LINE__);
	len = sizeof(struct D_A_SHARE_MEM)*MAX_SHARE_MEM_ITEM;
	buf = kmalloc(len, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;
	//construct rx share memory
	memset(buf,0x00,len);
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]buf=0x%p\n\r", __func__, __LINE__, buf);
	SDA_setSharedMemory4Recv(buf, len, sizeof(struct D_A_SHARE_MEM), 50);
	//Direct_Audio_RxReady_Notify_cb_Reg(debug_rx_ready_cb);
	SDA_registerCallback4Recv(debug_rx_ready_cb);
	ath6kl_dbg(ATH6KL_DBG_DA,"%s[%d]Simulate RX finish\n\r",__func__,__LINE__);
	return 0;
}
#endif //if SDA_DEBUG

EXPORT_SYMBOL(SDA_function4Send);
EXPORT_SYMBOL(SDA_function4RecvDone);
EXPORT_SYMBOL(SDA_setSharedMemory4Send);
EXPORT_SYMBOL(SDA_setSharedMemory4Recv);
EXPORT_SYMBOL(SDA_registerCallback4SendDone);
EXPORT_SYMBOL(SDA_registerCallback4Recv);
EXPORT_SYMBOL(SDA_GetTSF);
#endif
