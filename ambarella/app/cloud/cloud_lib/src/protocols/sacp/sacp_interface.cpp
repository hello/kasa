/*******************************************************************************
 * sacp_interface.cpp
 *
 * History:
 *  2013/12/09 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

#include "sacp_types.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TMemSize gfWriteSACPElement(TU8 *p_src, TMemSize max_size, ESACPElementType type, TU32 param0, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 param6, TU32 param7)
{
    if (DUnlikely(!p_src || (max_size < 5))) {
        LOG_FATAL("error parameters\n");
        return 0;
    }

    TU8 *ptmp = NULL;
    TMemSize total_size = 0;
    SSACPElementHeader *p_header = (SSACPElementHeader *) p_src;

    switch (type) {

        case ESACPElementType_EncodingParameters: {

                if (DUnlikely(max_size < 16)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBEW32(param0, ptmp);//width
                ptmp += 4;
                total_size += 4;

                DBEW32(param1, ptmp);//height
                ptmp += 4;
                total_size += 4;

                DBEW32(param2, ptmp);//bitrate
                ptmp += 4;
                total_size += 4;

                DBEW32(param3, ptmp);//framerate
                ptmp += 4;
                total_size += 4;

                p_header->type1 = (((TU32)ESACPElementType_EncodingParameters) >> 8) & 0xff;
                p_header->type2 = ((TU32)ESACPElementType_EncodingParameters) & 0xff;

                p_header->size1 = (((TU32)total_size) >> 8) & 0xff;
                p_header->size2 = ((TU32)total_size) & 0xff;

                total_size += sizeof(SSACPElementHeader);

                DASSERT((16 + sizeof(SSACPElementHeader)) == total_size);
                return total_size;
            }
            break;

        case ESACPElementType_DisplaylayoutInfo: {

                if (DUnlikely(max_size < 8)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBEW16(param0, ptmp);//layout
                ptmp += 2;
                total_size += 2;

                DBEW16(param1, ptmp);//hd index
                ptmp += 2;
                total_size += 2;

                DBEW16(param2, ptmp);//audio source
                ptmp += 2;
                total_size += 2;

                DBEW16(param3, ptmp);//audio target
                ptmp += 2;
                total_size += 2;

                p_header->type1 = (((TU32)ESACPElementType_DisplaylayoutInfo) >> 8) & 0xff;
                p_header->type2 = ((TU32)ESACPElementType_DisplaylayoutInfo) & 0xff;

                p_header->size1 = (((TU32)total_size) >> 8) & 0xff;
                p_header->size2 = ((TU32)total_size) & 0xff;

                total_size += sizeof(SSACPElementHeader);

                DASSERT((8 + sizeof(SSACPElementHeader)) == total_size);
                return total_size;
            }
            break;

        case ESACPElementType_DisplayWindowInfo: {

                if (DUnlikely(max_size < 20)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBEW32(param0, ptmp);//window_pos_x
                ptmp += 4;
                total_size += 4;

                DBEW32(param1, ptmp);//window_pos_y
                ptmp += 4;
                total_size += 4;

                DBEW32(param2, ptmp);//window_width
                ptmp += 4;
                total_size += 4;

                DBEW32(param3, ptmp);//window_height
                ptmp += 4;
                total_size += 4;

                DBEW16(param4, ptmp);//udec_index
                ptmp += 2;
                total_size += 2;

                *ptmp ++ = param5 & 0xff;//window index
                *ptmp ++ = param6 & 0xff;//display_disable
                total_size += 2;

                p_header->type1 = (((TU32)ESACPElementType_DisplayWindowInfo) >> 8) & 0xff;
                p_header->type2 = ((TU32)ESACPElementType_DisplayWindowInfo) & 0xff;

                p_header->size1 = (((TU32)total_size) >> 8) & 0xff;
                p_header->size2 = ((TU32)total_size) & 0xff;

                total_size += sizeof(SSACPElementHeader);

                DASSERT((20 + sizeof(SSACPElementHeader)) == total_size);
                return total_size;
            }
            break;

        case ESACPElementType_DisplayZoomInfo: {

                if (DUnlikely(max_size < 18)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBEW32(param0, ptmp);//zoom_size_x
                ptmp += 4;
                total_size += 4;

                DBEW32(param1, ptmp);//zoom_size_y
                ptmp += 4;
                total_size += 4;

                DBEW32(param2, ptmp);//zoom_input_center_x
                ptmp += 4;
                total_size += 4;

                DBEW32(param3, ptmp);//zoom_input_center_y
                ptmp += 4;
                total_size += 4;

                DBEW16(param4, ptmp);//udec_index
                ptmp += 2;
                total_size += 2;

                p_header->type1 = (((TU32)ESACPElementType_DisplayZoomInfo) >> 8) & 0xff;
                p_header->type2 = ((TU32)ESACPElementType_DisplayZoomInfo) & 0xff;

                p_header->size1 = (((TU32)total_size) >> 8) & 0xff;
                p_header->size2 = ((TU32)total_size) & 0xff;

                total_size += sizeof(SSACPElementHeader);

                DASSERT((18 + sizeof(SSACPElementHeader)) == total_size);
                return total_size;
            }
            break;

        case ESACPElementType_GlobalSetting: {

                if (DUnlikely(max_size < 10)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                *ptmp ++ = (param0 & 0xff);//channel_number_per_group
                *ptmp ++ = (param1 & 0xff);//total_group_number
                *ptmp ++ = (param2 & 0xff);//current_group_index
                *ptmp ++ = (param3 & 0xff);//have_dual_stream

                *ptmp ++ = (param4 & 0xff);//is_vod_enabled
                *ptmp ++ = (param5 & 0xff);//is_vod_ready
                total_size += 6;

                DBEW16(param6, ptmp);//total_window_number
                ptmp += 2;
                total_size += 2;

                DBEW16(param7, ptmp);//current_display_window_number
                ptmp += 2;
                total_size += 2;

                p_header->type1 = (((TU32)ESACPElementType_GlobalSetting) >> 8) & 0xff;
                p_header->type2 = ((TU32)ESACPElementType_GlobalSetting) & 0xff;

                p_header->size1 = (((TU32)total_size) >> 8) & 0xff;
                p_header->size2 = ((TU32)total_size) & 0xff;

                total_size += sizeof(SSACPElementHeader);

                DASSERT((10 + sizeof(SSACPElementHeader)) == total_size);
                return total_size;
            }
            break;

        case ESACPElementType_SyncFlags: {

                if (DUnlikely(max_size < 8)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                *ptmp ++ = (param0 & 0xff);//update_group_info_flag
                *ptmp ++ = (param1 & 0xff);//update_display_layout_flag
                *ptmp ++ = (param2 & 0xff);//update_source_flag
                *ptmp ++ = (param3 & 0xff);//update_display_flag

                *ptmp ++ = (param4 & 0xff);//update_audio_flag
                *ptmp ++ = (param5 & 0xff);//update_zoom_flag
                total_size += 6;

                total_size += 2;//align

                p_header->type1 = (((TU32)ESACPElementType_SyncFlags) >> 8) & 0xff;
                p_header->type2 = ((TU32)ESACPElementType_SyncFlags) & 0xff;

                p_header->size1 = (((TU32)total_size) >> 8) & 0xff;
                p_header->size2 = ((TU32)total_size) & 0xff;

                total_size += sizeof(SSACPElementHeader);

                DASSERT((8 + sizeof(SSACPElementHeader)) == total_size);
                return total_size;
            }
            break;

        default:
            LOG_FATAL("BAD ESACPElementType %d\n", type);
            break;
    }

    return 0;
}

TMemSize gfReadSACPElement(TU8 *p_src, TMemSize max_size, ESACPElementType &type, TU32 &param0, TU32 &param1, TU32 &param2, TU32 &param3, TU32 &param4, TU32 &param5, TU32 &param6, TU32 &param7)
{
    if (DUnlikely(!p_src || (max_size < 5))) {
        LOG_FATAL("error parameters\n");
        return 0;
    }

    TU8 *ptmp = NULL;
    TMemSize total_size = 0;
    SSACPElementHeader *p_header = (SSACPElementHeader *) p_src;

    total_size = (p_header->size1 << 8) | p_header->size2;
    type = (ESACPElementType)((p_header->type1 << 8) | p_header->type2);

    //LOG_PTINTF("gfReadSACPElement, type %d, size %ld, %02x %02x %02x %02x\n", type, total_size, p_src[0], p_src[1], p_src[2], p_src[3]);

    switch (type) {

        case ESACPElementType_EncodingParameters: {

                if (DUnlikely(max_size < 16)) {
                    LOG_FATAL("not enough space, bad or incomplete payload\n");
                    return 0;
                }

                if (DUnlikely(16 != total_size)) {
                    LOG_FATAL("bad payload element?\n");
                    return total_size;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBER32(param0, ptmp);//width
                ptmp += 4;

                DBER32(param1, ptmp);//height
                ptmp += 4;

                DBER32(param2, ptmp);//bitrate
                ptmp += 4;

                DBER32(param3, ptmp);//framerate
                ptmp += 4;

                total_size += sizeof(SSACPElementHeader);

                return total_size;
            }
            break;

        case ESACPElementType_DisplaylayoutInfo: {

                if (DUnlikely(max_size < 8)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                if (DUnlikely(8 != total_size)) {
                    LOG_FATAL("bad payload element?\n");
                    return total_size;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBER16(param0, ptmp);//layout
                ptmp += 2;

                DBER16(param1, ptmp);//hd index
                ptmp += 2;

                DBER16(param2, ptmp);//audio source
                ptmp += 2;

                DBER16(param3, ptmp);//audio target
                ptmp += 2;

                total_size += sizeof(SSACPElementHeader);
                return total_size;
            }
            break;

        case ESACPElementType_DisplayWindowInfo: {

                if (DUnlikely(max_size < 20)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                if (DUnlikely(20 != total_size)) {
                    LOG_FATAL("bad payload element?\n");
                    return total_size;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBER32(param0, ptmp);//window_pos_x
                ptmp += 4;

                DBER32(param1, ptmp);//window_pos_y
                ptmp += 4;

                DBER32(param2, ptmp);//window_width
                ptmp += 4;

                DBER32(param3, ptmp);//window_height
                ptmp += 4;

                DBER16(param4, ptmp);//udec_index
                ptmp += 2;

                param5 = *ptmp ++;//window index
                param6 = *ptmp ++;//display_disable

                total_size += sizeof(SSACPElementHeader);
                return total_size;
            }
            break;

        case ESACPElementType_DisplayZoomInfo: {

                if (DUnlikely(max_size < 18)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                if (DUnlikely(18 != total_size)) {
                    LOG_FATAL("bad payload element?\n");
                    return total_size;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                DBER32(param0, ptmp);//zoom_size_x
                ptmp += 4;

                DBER32(param1, ptmp);//zoom_size_y
                ptmp += 4;

                DBER32(param2, ptmp);//zoom_input_center_x
                ptmp += 4;

                DBER32(param3, ptmp);//zoom_input_center_y
                ptmp += 4;

                DBER16(param4, ptmp);//udec_index
                ptmp += 2;

                total_size += sizeof(SSACPElementHeader);
                return total_size;
            }
            break;

        case ESACPElementType_GlobalSetting: {

                if (DUnlikely(max_size < 10)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                if (DUnlikely(10 != total_size)) {
                    LOG_FATAL("bad payload element?\n");
                    return total_size;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                param0 = *ptmp ++;//channel_number_per_group
                param1 = *ptmp ++;//total_group_number
                param2 = *ptmp ++;//current_group_index
                param3 = *ptmp ++;//have_dual_stream

                param4 = *ptmp ++;//is_vod_enabled
                param5 = *ptmp ++;//is_vod_ready

                DBER16(param6, ptmp);//total_window_number
                ptmp += 2;

                DBER16(param7, ptmp);//current_display_window_number
                ptmp += 2;

                total_size += sizeof(SSACPElementHeader);
                return total_size;
            }
            break;

        case ESACPElementType_SyncFlags: {

                if (DUnlikely(max_size < 8)) {
                    LOG_FATAL("not enough space\n");
                    return 0;
                }

                if (DUnlikely(8 != total_size)) {
                    LOG_FATAL("bad payload element?\n");
                    return total_size;
                }

                ptmp = p_src + sizeof(SSACPElementHeader);

                param0 = *ptmp ++;//update_group_info_flag
                param1 = *ptmp ++;//update_display_layout_flag
                param2 = *ptmp ++;//update_source_flag
                param3 = *ptmp ++;//update_display_flag

                param4 = *ptmp ++;//update_audio_flag
                param5 = *ptmp ++;//update_zoom_flag

                total_size += sizeof(SSACPElementHeader);
                return total_size;
            }
            break;

        default:
            LOG_FATAL("BAD ESACPElementType %d\n", type);
            break;
    }

    return 0;
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

