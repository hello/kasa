/*******************************************************************************
 * common_private_data.cpp
 *
 * History:
 *  2014/05/23 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

//#ifdef BUILD_WITH_UNDER_DEVELOP_COMPONENT

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"
#include "common_base.h"

#include "common_private_data.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TU8 *gfAppendTSPriData(TU8 *dest, void *src, ETSPrivateDataType data_type, TU16 sub_type, TU16 data_len, TU16 max_data_len, TU16 &consumed_data_len)
{
    TU8 *p_cur_dest = NULL;
    TU16 packing_data_len = 0;
    if (!dest || !src || !data_len) {
        LOG_ERROR("gfAppendTSPriData, invalid paras, dest=%p, src=%p, data_len=%hu\n", dest, src, data_len);
        return p_cur_dest;
    }
    switch (data_type) {
        case ETSPrivateDataType_StartTime: {
                if (sizeof(SDateTime) != data_len) {
                    LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_StartTime, data_len=%hu invalid(should be %lu)\n", data_len, (TULong)sizeof(SDateTime));
                    return p_cur_dest;
                }
                p_cur_dest = dest;
                packing_data_len = sizeof(STSPrivateDataStartTime);
                STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
                p_header->data_type_4cc_31to24 = 0x54;  //T
                p_header->data_type_4cc_23to16 = 0x53;   //S
                p_header->data_type_4cc_15to8 = 0x53;    //S
                p_header->data_type_4cc_7to0 = 0x54;     //T
                p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
                p_header->data_sub_type_7to0 = sub_type & 0xff;
                p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
                p_header->data_length_7to0 = packing_data_len & 0xff;
                p_cur_dest += sizeof(STSPrivateDataHeader);
                STSPrivateDataStartTime *p_start_time_dest = (STSPrivateDataStartTime *)p_cur_dest;
                SDateTime *p_start_time_src = (SDateTime *)src;
                p_start_time_dest->year_15to8 = (p_start_time_src->year >> 8) & 0xff;
                p_start_time_dest->year_7to0 = p_start_time_src->year & 0xff;
                p_start_time_dest->month = p_start_time_src->month;
                p_start_time_dest->day = p_start_time_src->day;
                p_start_time_dest->hour = p_start_time_src->hour;
                p_start_time_dest->minute = p_start_time_src->minute;
                p_start_time_dest->seconds = p_start_time_src->seconds;
                p_cur_dest += packing_data_len;
                consumed_data_len = data_len;
            }
            break;
        case ETSPrivateDataType_Duration: {
                if (sizeof(TU16) != data_len) {
                    LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_Duration, data_len=%hu invalid(should be %lu)\n", data_len, (TULong)sizeof(TU16));
                    return p_cur_dest;
                }
                p_cur_dest = dest;
                packing_data_len = sizeof(STSPrivateDataDuration);
                STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
                p_header->data_type_4cc_31to24 = 0x54;  //T
                p_header->data_type_4cc_23to16 = 0x53;   //S
                p_header->data_type_4cc_15to8 = 0x44;    //D
                p_header->data_type_4cc_7to0 = 0x55;     //U
                p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
                p_header->data_sub_type_7to0 = sub_type & 0xff;
                p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
                p_header->data_length_7to0 = packing_data_len & 0xff;
                p_cur_dest += sizeof(STSPrivateDataHeader);
                STSPrivateDataDuration *p_duration_dest = (STSPrivateDataDuration *)p_cur_dest;
                TU16 *p_duration_src = (TU16 *)src;
                p_duration_dest->file_duration_15to8 = ((*p_duration_src) >> 8) & 0xff;
                p_duration_dest->file_duration_7to0 = (*p_duration_src) & 0xff;
                p_cur_dest += packing_data_len;
                consumed_data_len = data_len;
            }
            break;
        case ETSPrivateDataType_ChannelName: {
                if (data_len > max_data_len) {
                    LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_ChannelName, data_len=%hu invalid(should <= %hu)\n", data_len, max_data_len);
                    return p_cur_dest;
                }
                p_cur_dest = dest;
                packing_data_len = data_len;
                STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
                p_header->data_type_4cc_31to24 = 0x54;  //T
                p_header->data_type_4cc_23to16 = 0x53;   //S
                p_header->data_type_4cc_15to8 = 0x43;    //C
                p_header->data_type_4cc_7to0 = 0x4E;     //N
                p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
                p_header->data_sub_type_7to0 = sub_type & 0xff;
                p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
                p_header->data_length_7to0 = packing_data_len & 0xff;
                p_cur_dest += sizeof(STSPrivateDataHeader);
                TU8 *p_chaname_src = (TU8 *)src;
                memcpy(p_cur_dest, p_chaname_src, packing_data_len);
                p_cur_dest += packing_data_len;
                consumed_data_len = data_len;
            }
            break;
        case ETSPrivateDataType_TSPakIdx4GopStart: {
                //TU16 max_data_len = MPEG_TS_TP_PACKET_SIZE-MPEG_TS_TP_PACKET_HEADER_SIZE-DPESPakStartCodeSize-DPESPakStreamIDSize-DPESPakLengthSize-sizeof(STSPrivateDataHeader);
                if (0 != data_len % sizeof(TU32)) {
                    LOG_ERROR("gfAppendTSPriData,  ETSPrivateDataType_TSPakIdx4GopStart, data_len=%hu invalid(should multiple of %lu bytes)\n", data_len, (TULong)sizeof(TU32));
                    return p_cur_dest;
                }
                p_cur_dest = dest;
                consumed_data_len = (data_len > max_data_len) ? max_data_len : data_len;
                packing_data_len = consumed_data_len;
                STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_dest;
                p_header->data_type_4cc_31to24 = 0x54;  //T
                p_header->data_type_4cc_23to16 = 0x53;   //S
                p_header->data_type_4cc_15to8 = 0x50;    //P
                p_header->data_type_4cc_7to0 = 0x49;     //I
                p_header->data_sub_type_15to8 = (sub_type >> 8) & 0xff;
                p_header->data_sub_type_7to0 = sub_type & 0xff;
                p_header->data_length_15to8 = (packing_data_len >> 8) & 0xff;
                p_header->data_length_7to0 = packing_data_len & 0xff;
                p_cur_dest += sizeof(STSPrivateDataHeader);
                TU32 *p_tspakidx_src = (TU32 *)src;
                for (TU32 i = 0; i < consumed_data_len / sizeof(TU32); i++) {
                    STSPrivateDataTSPakIdx4GopStart *p_tspakidx_dest = (STSPrivateDataTSPakIdx4GopStart *)p_cur_dest;
                    p_tspakidx_dest->idx_31to24 = ((*p_tspakidx_src) >> 24) & 0xff;
                    p_tspakidx_dest->idx_23to16 = ((*p_tspakidx_src) >> 16) & 0xff;
                    p_tspakidx_dest->idx_15to8 = ((*p_tspakidx_src) >> 8) & 0xff;
                    p_tspakidx_dest->idx_7to0 = (*p_tspakidx_src) & 0xff;
                    p_cur_dest += sizeof(STSPrivateDataTSPakIdx4GopStart);
                    p_tspakidx_src++;
                }
            }
            break;
        case ETSPrivateDataType_MotionEvent: {//0x54534D45;//TSME
                consumed_data_len = 0;
                LOG_WARN("gfAppendTSPriData, ETSPrivateDataType_MotionEvent NoImplement\n");
            }
            break;
        default: {
                consumed_data_len = 0;
                LOG_ERROR("gfAppendTSPriData, invalid paras, data_type=%d\n", data_type);
            }
            break;
    }

    return p_cur_dest;
}

EECode gfRetrieveTSPriData(void *dest, TU16 dest_buf_len, TU8 *src, TU16 src_len, ETSPrivateDataType data_type, TU16 sub_type)
{
    if (!dest || !dest_buf_len || !src || !src_len || data_type >= ETSPrivateDataType_Num) {
        LOG_ERROR("gfRetrieveTSPriData, invalid paras, dest=%p, dest_buf_len=%hu, src=%p, src_len=%hu, data_type=%d\n", dest, dest_buf_len, src, src_len, data_type);
        return EECode_BadParam;
    }

    //TU16 max_check_len = MPEG_TS_TP_PACKET_SIZE-MPEG_TS_TP_PACKET_HEADER_SIZE-DPESPakStartCodeSize-DPESPakStreamIDSize-DPESPakLengthSize;
    TU8 *p_cur_header = src;
    while ((p_cur_header - src) < src_len) {
        STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)p_cur_header;
        TU32 data_type_4cc = (p_header->data_type_4cc_31to24 << 24) | (p_header->data_type_4cc_23to16 << 16) | (p_header->data_type_4cc_15to8 << 8) | p_header->data_type_4cc_7to0;
        TU32 data_type_wanted_4cc = 0;
        TU16 data_sub_type = (p_header->data_sub_type_15to8 << 8) | p_header->data_sub_type_7to0;
        TU16 data_length = (p_header->data_length_15to8 << 8) | p_header->data_length_7to0;
        switch (data_type) {
            case ETSPrivateDataType_StartTime:
                data_type_wanted_4cc = 0x54535354;//TSST
                break;
            case ETSPrivateDataType_Duration:
                data_type_wanted_4cc = 0x54534455;//TSDU
                break;
            case ETSPrivateDataType_ChannelName:
                data_type_wanted_4cc = 0x5453434E;//TSCN
                break;
            case ETSPrivateDataType_TSPakIdx4GopStart:
                data_type_wanted_4cc = 0x54535049;//TSPI
                break;
            case ETSPrivateDataType_MotionEvent:
                data_type_wanted_4cc = 0x54534D45;//TSME
                break;
            default:
                LOG_ERROR("gfRetrieveTSPriData, invalid data_type=%d\n", data_type);
                return EECode_BadParam;
        }
        if ((data_type_wanted_4cc == data_type_4cc) && (sub_type == data_sub_type)) {
            if (data_length > dest_buf_len) {
                LOG_ERROR("gfRetrieveTSPriData, dest buffer is not large enough to contain data_type=%d\n", data_type);
                return EECode_InternalLogicalBug;
            }
            p_cur_header += sizeof(STSPrivateDataHeader);//now p_cur_header point to the beginning of data
            switch (data_type) {
                case ETSPrivateDataType_StartTime: {
                        SDateTime *p_start_time_dest = (SDateTime *)dest;
                        STSPrivateDataStartTime *p_start_time_src = (STSPrivateDataStartTime *)p_cur_header;
                        p_start_time_dest->year = (p_start_time_src->year_15to8 << 8) | p_start_time_src->year_7to0;
                        p_start_time_dest->month = p_start_time_src->month;
                        p_start_time_dest->day = p_start_time_src->day;
                        p_start_time_dest->hour = p_start_time_src->hour;
                        p_start_time_dest->minute = p_start_time_src->minute;
                        p_start_time_dest->seconds = p_start_time_src->seconds;
                    }
                    break;
                case ETSPrivateDataType_Duration: {
                        TU16 *p_duration_dest = (TU16 *)dest;
                        STSPrivateDataDuration *p_duration_src = (STSPrivateDataDuration *)p_cur_header;
                        *p_duration_dest = (p_duration_src->file_duration_15to8 << 8) | p_duration_src->file_duration_7to0;
                    }
                    break;
                case ETSPrivateDataType_ChannelName: {
                        TU8 *p_chaname_dest = (TU8 *)dest;
                        memcpy(p_chaname_dest, p_cur_header, data_length);
                    }
                    break;
                case ETSPrivateDataType_TSPakIdx4GopStart: {
                        TU32 *p_tspakidx_dest = (TU32 *)dest;
                        STSPrivateDataTSPakIdx4GopStart *p_tspakidx_src = (STSPrivateDataTSPakIdx4GopStart *)p_cur_header;
                        for (TU32 i = 0; i < data_length / sizeof(STSPrivateDataTSPakIdx4GopStart); i++) {
                            *p_tspakidx_dest = (p_tspakidx_src->idx_31to24 << 24) | (p_tspakidx_src->idx_23to16 << 16) | (p_tspakidx_src->idx_15to8 << 8) | p_tspakidx_src->idx_7to0;
                            p_tspakidx_dest++;
                            p_tspakidx_src++;
                        }
                    }
                    break;
                case ETSPrivateDataType_MotionEvent:
                    LOG_WARN("gfRetrieveTSPriData, ETSPrivateDataType_MotionEvent NoImplement\n");
                default:
                    LOG_ERROR("gfRetrieveTSPriData, invalid data_type=%d\n", data_type);
                    return EECode_BadParam;
            }
            return EECode_OK;
        } else {
            p_cur_header += sizeof(STSPrivateDataHeader) + data_length;
        }
    }
    LOG_WARN("gfRetrieveTSPriData, can not found data_type=%d sub_type=%hu in current source.\n", data_type, sub_type);
    return EECode_BadParam;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

//#endif

