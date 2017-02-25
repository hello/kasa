
#ifndef SDA_H
#define SDA_H

/* samsung direct audio prototype */

typedef struct SDA_Descriptor
{
	unsigned char m_ReadyToCopy;
	
	unsigned int m_PayloadSize;
	
	unsigned int m_TimeStamp;
}__attribute__((packed)) SDA_Descriptor_t;


typedef struct SDA_HeadRoom
{
	unsigned char m_DestAddress[6];
	
	unsigned char m_SourceAddress[6];
	
	unsigned short m_PacketType;
}__attribute__((packed)) SDA_HeadRoom_t;


typedef struct
{
	SDA_Descriptor_t   m_Descriptor;
	
	unsigned char m_Dummy[50 - sizeof(struct SDA_HeadRoom)];
	
	SDA_HeadRoom_t  m_Headroom;
}__attribute__((packed)) SDA_Header_t;



/**
 * @brief tsf를 wifi module로 부터 얻음
 * @remarks
 * @param pTsf : tsf 
 * @return none 
 * @see 
 */
void SDA_GetTSF(unsigned int *pTsf);




/**
 * @brief wifi module에 tx를 위해서 사용할 buffer 정보를 알려줌.
 * @brief send the buffer info to wifi module for tx.
 * @remarks
 * @param BufferId direct audio system에서 audio input buffer는 최대 5개로 가정함. buffer의 id임. 
 * @param BufferId is the mazimum 5. 
 * @param pBufferTotal : buffer address
 * @param BufferTotalSize : buffer size
 * @param BufferUnitSize : each unit size (descriptor + headroom + audio data size)
 * @param HeadroomSize : head room size
 * @return none 
 * @see 
 */
void SDA_setSharedMemory4Send(unsigned int BufferId, unsigned char *pBufferTotal, unsigned int BufferTotalSize, unsigned int BufferUnitSize, unsigned int HeadroomSize);





/**
 * @brief wifi module에 rx를 위해서 사용할 buffer 정보를 알려줌.
 * @brief send the buffer info to wifi module for rx.
 * @remarks
 * @param pBufferTotal : buffer address
 * @param BufferTotalSize : buffer size
 * @param BufferUnitSize : each unit size (descriptor + headroom + audio data size)
 * @param HeadroomSize : head room size
 * @return none 
 * @see 
 */
void SDA_setSharedMemory4Recv(unsigned char *pBufferTotal, unsigned int BufferTotalSize, unsigned int BufferUnitSize, unsigned int HeadroomSize);






/**
 * @brief 전송할 data를 wifi module에 전송함. 
 * @send the brief data data to wifi module (dsp --> wifi module)
 * @remarks
 * @param BufferId direct audio system에서 audio input buffer는 최대 5개로 가정함. buffer의 id임. 
 * @param on BufferId direct audio system, the maximum of audio input buffer is 5. buffer의 id임. 
 * @param pBuffer : buffer address
 * @param BufferSize : buffer size
 * @return none 
 * @see 
 */
void SDA_function4Send(unsigned int BufferId, unsigned char *pBuffer, unsigned int BufferSize);




/**
 * @brief wifi module에서 data를 전송한 후에 callback호출 (ack)
 * @brief after sending data on wifi module, call the callback(ack) (wifi module --> dsp)
 * @remarks
 * @param BufferId direct audio system에서 audio input buffer는 최대 5개로 가정함. buffer의 id임. 
 * @param on BufferId direct audio system, the maximum of audio input buffer is 5.
 * @param pBuffer : buffer address
 * @param BufferSize : buffer size
 * @return none 
 * @see 
 */
typedef void (*SDA_SendDoneCallBack)(unsigned int BufferId, unsigned char *pBuffer, unsigned int BufferSize);

void SDA_registerCallback4SendDone(SDA_SendDoneCallBack pCallback);







/**
 * @brief wifi module에서 전송 받은 데이터를 soc에 전송함. 
 * @brief on wifi module, send the receiving data to soc (wifi module --> dsp)
 * @remarks
 * @param pBuffer : buffer address
 * @param BufferSize : buffer size
 * @return none 
 * @see 
 */
typedef void (*SDA_RecvCallBack)(unsigned char *pBuffer, unsigned int BufferSize);

void SDA_registerCallback4Recv(SDA_RecvCallBack pCallback);





/**
 * @brief soc에서 전송 받은 데이터를 처리후에 wifi module에 알림. (ack)
 * @brief after finishing to received data on soc, notifying to wifi module(ack)
 * @remarks
 * @param pBuffer : buffer address
 * @param BufferSize : buffer size
 * @return none 
 * @see 
 */
void SDA_function4RecvDone(unsigned char *pBuffer, unsigned int BufferSize);


	

#endif // SDA_H
