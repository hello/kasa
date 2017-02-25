/*******************************************************************************
 * codec_misc.cpp
 *
 * History:
 *  2014/08/26 - [Zhi He] create file
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

#include "common_log.h"

#include "codec_misc.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TU8 *gfNALUFindNextStartCode(TU8 *p, TU32 len)
{
    TUint state = 0;

    while (len) {
        switch (state) {
            case 0:
                if (!(*p)) {
                    state = 1;
                }
                break;

            case 1: //0
                if (!(*p)) {
                    state = 2;
                } else {
                    state = 0;
                }
                break;

            case 2: //0 0
                if (!(*p)) {
                    state = 3;
                } else {
                    state = 0;
                }
                break;

            case 3: //0 0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    return (p + 1);
                } else {
                    state = 0;
                }
                break;

            default:
                LOG_FATAL("impossible to comes here\n");
                break;

        }
        p++;
        len --;
    }

    return NULL;
}

TU8 *gfNALUFindIDR(TU8 *p, TU32 len)
{
    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        if (((*(p + 4)) & 0x1F) == ENalType_IDR) {
                            return p;
                        }
                    }
                } else if (*(p + 2) == 0x01) {
                    if (((*(p + 3)) & 0x1F) == ENalType_IDR) {
                        return p;
                    }
                }
            }
        }
        ++ p;
        len --;
    }
    return NULL;
}

TU8 *gfNALUFindSPSHeader(TU8 *p, TU32 len)
{
    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        if (((*(p + 4)) & 0x1F) == ENalType_SPS) {
                            return p;
                        }
                    }
                }
            }
        }
        ++p;
        len --;
    }
    return NULL;
}

TU8 *gfNALUFindPPSEnd(TU8 *p, TU32 len)
{
    TU32 find_pps = 0;
    TU32 find_sps = 0;
    TU32 nal_type = 0;

    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        nal_type = p[4] & 0x1F;
                        if (ENalType_IDR == nal_type) {
                            DASSERT(find_sps);
                            DASSERT(find_pps);
                            return p;
                        } else if (ENalType_SPS == nal_type) {
                            find_sps = 1;
                        } else if (ENalType_PPS == nal_type) {
                            find_pps = 1;
                        } else {
                            if (find_pps) {
                                DASSERT(find_sps);
                                return p;
                            }
                        }
                    }
                }
            }
        }
        ++ p;
        len --;
    }
    return NULL;
}

void gfFindH264SpsPpsIdr(TU8 *data_base, TInt data_size, TU8 &has_sps, TU8 &has_pps, TU8 &has_idr, TU8 *&p_sps, TU8 *&p_pps, TU8 *&p_pps_end, TU8 *&p_idr)
{
    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return;
    }

    has_sps = 0;
    has_pps = 0;
    p_pps_end = 0;
    has_idr = 0;
    p_sps = NULL;
    p_pps = NULL;
    p_idr = NULL;

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = ptr[4] & 0x1F;
                        if (has_pps && (!p_pps_end)) {
                            p_pps_end = ptr;
                            ptr += 3;
                        }
                        if (ENalType_IDR == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(has_pps);
                            DASSERT(p_sps);
                            DASSERT(p_pps);
                            has_idr = 1;
                            p_idr = ptr;
                            return;
                        } else if (ENalType_SPS == nal_type) {
                            if (!has_sps) {
                                has_sps = 1;
                                p_sps = ptr;
                            }
                            ptr += 2;
                        } else if (ENalType_PPS == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(p_sps);
                            if (!has_pps) {
                                has_pps = 1;
                                p_pps = ptr;
                            }
                            ptr += 2;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = ptr[3] & 0x1F;
                    if (has_pps && (!p_pps_end)) {
                        p_pps_end = ptr;
                        ptr += 2;
                    }
                    if (ENalType_IDR == nal_type) {
                        DASSERT(has_sps);
                        DASSERT(has_pps);
                        DASSERT(p_sps);
                        DASSERT(p_pps);
                        has_idr = 1;
                        p_idr = ptr;
                        return;
                    } else if (ENalType_SPS == nal_type) {
                        if (!has_sps) {
                            has_sps = 1;
                            p_sps = ptr;
                        }
                        ptr += 2;
                    } else if (ENalType_PPS == nal_type) {
                        DASSERT(has_sps);
                        DASSERT(p_sps);
                        if (!has_pps) {
                            has_pps = 1;
                            p_pps = ptr;
                        }
                        ptr += 2;
                    }
                }
            }
        }
        ++ptr;
    }

    if (has_pps && (!p_pps_end)) {
        p_pps_end = data_base + data_size;
    }

    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

