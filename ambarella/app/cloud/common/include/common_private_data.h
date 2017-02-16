/**
 * common_private_data.h
 *
 * History:
 *  2014/05/22 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_PRIVATE_DATA_H__
#define __COMMON_PRIVATE_DATA_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DPESPakStartCodeSize (3)    //0x000001
#define DPESPakStreamIDSize (1)     //0xBF
#define DPESPakLengthSize (2)       //two bytes packet length

//NOTE: all structs below defined as big endian
typedef struct {
    TU8 start_code_prefix_23to16;
    TU8 start_code_prefix_15to8;
    TU8 start_code_prefix_7to0;
    TU8 stream_id;
    TU8 packet_len_15to8;
    TU8 packet_len_7to0;
} STSPrivateDataPesPacketHeader;

typedef struct {
    TU8 data_type_4cc_31to24;
    TU8 data_type_4cc_23to16;
    TU8 data_type_4cc_15to8;
    TU8 data_type_4cc_7to0;
    TU8 data_sub_type_15to8;
    TU8 data_sub_type_7to0;
    TU8 data_length_15to8;
    TU8 data_length_7to0;
} STSPrivateDataHeader;


typedef enum {
    ETSPrivateDataType_StartTime,
    ETSPrivateDataType_Duration,
    ETSPrivateDataType_ChannelName,
    ETSPrivateDataType_TSPakIdx4GopStart,
    ETSPrivateDataType_MotionEvent,
    ETSPrivateDataType_Num
} ETSPrivateDataType;

typedef struct {
    TU8 year_15to8;
    TU8 year_7to0;
    TU8 month;
    TU8 day;
    TU8 hour;
    TU8 minute;
    TU8 seconds;
} STSPrivateDataStartTime;

typedef struct {
    TU8 file_duration_15to8;
    TU8 file_duration_7to0;
} STSPrivateDataDuration;

typedef struct {
    TU8 idx_31to24;
    TU8 idx_23to16;
    TU8 idx_15to8;
    TU8 idx_7to0;
} STSPrivateDataTSPakIdx4GopStart;

/*
<Input>
TU8* dest: pointer to the mem of STSPrivateDataHeader
void* src: pointer to the mem of original struct of private data, such as SDateTime, etc.
ETSPrivateDataType data_type: data type, to transform the pointer of src to specified struct and fill according FourCC in STSPrivateDataHeader
TU16 sub_type: sub type needed in STSPrivateDataHeader
TU16 data_len: length of original struct of private data
TU16 max_data_len: max appended data len in ONE TS packet
<Output>
TU8* dest: pointer to the mem after appending one private data unit, could be the start pointer of the next STSPrivateDataHeader
                if the length of TS packet will be out of range after this appending operation, return NULL
<Input&Output>
TU16& consumed_data_len: the consumed data length according to the input data length, should not large than the input data length
*/
TU8 *gfAppendTSPriData(TU8 *dest, void *src, ETSPrivateDataType data_type, TU16 sub_type, TU16 data_len, TU16 max_data_len, TU16 &consumed_data_len);

/*
<Input>
void* dest: pointer to the mem of original struct of private data, such as SDateTime, etc.
                should conform to data_type inputed.
TU16 dest_buf_len: dest buffer length
                            size should conform to data_type inputed
TU8* src: pointer to the mem of STSPrivateDataHeader
TU16 src_len: source mem total length
ETSPrivateDataType data_type: data type wanted
TU16 sub_type: sub type wanted
<Output>
EECode ret: if such type private data found and dest buffer is large enough to contain it, return EECode_OK, otherwise return error code
*/
EECode gfRetrieveTSPriData(void *dest, TU16 dest_buf_len, TU8 *src, TU16 src_len, ETSPrivateDataType data_type, TU16 sub_type);



DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
