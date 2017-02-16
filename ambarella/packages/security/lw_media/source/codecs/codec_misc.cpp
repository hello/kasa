/**
 * codec_misc.cpp
 *
 * History:
 *  2014/08/26 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "codec_interface.h"

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

TU8 *gfGenerateAACExtraData(TU32 samplerate, TU32 channel_number, TU32 &size)
{
    SSimpleAudioSpecificConfig *p_simple_header = NULL;

    size = 2;
    p_simple_header = (SSimpleAudioSpecificConfig *) DDBG_MALLOC((size + 3) & (~3), "GAAE");
    p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
    switch (samplerate) {
        case 44100:
            samplerate = eSamplingFrequencyIndex_44100;
            break;
        case 48000:
            samplerate = eSamplingFrequencyIndex_48000;
            break;
        case 24000:
            samplerate = eSamplingFrequencyIndex_24000;
            break;
        case 16000:
            samplerate = eSamplingFrequencyIndex_16000;
            break;
        case 8000:
            samplerate = eSamplingFrequencyIndex_8000;
            break;
        case 12000:
            samplerate = eSamplingFrequencyIndex_12000;
            break;
        case 32000:
            samplerate = eSamplingFrequencyIndex_32000;
            break;
        case 22050:
            samplerate = eSamplingFrequencyIndex_22050;
            break;
        case 11025:
            samplerate = eSamplingFrequencyIndex_11025;
            break;
        default:
            LOG_ERROR("NOT support sample rate (%d) here.\n", samplerate);
            break;
    }
    p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
    p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
    p_simple_header->channelConfiguration = channel_number;
    p_simple_header->bitLeft = 0;

    return (TU8 *)p_simple_header;
}

EECode gfGetH264Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size)
{
    TU8 has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    p_extradata = NULL;
    extradata_size = 0;

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = ptr[4] & 0x1F;
                        if (ENalType_IDR == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(has_pps);
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        } else if (ENalType_SPS == nal_type) {
                            has_sps = 1;
                            p_extradata = ptr;
                            ptr += 3;
                        } else if (ENalType_PPS == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(p_extradata);
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_sps && has_pps) {
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = ptr[3] & 0x1F;
                    if (ENalType_IDR == nal_type) {
                        DASSERT(has_sps);
                        DASSERT(has_pps);
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    } else if (ENalType_SPS == nal_type) {
                        has_sps = 1;
                        p_extradata = ptr;
                        ptr += 2;
                    } else if (ENalType_PPS == nal_type) {
                        DASSERT(has_sps);
                        DASSERT(p_extradata);
                        has_pps = 1;
                        ptr += 2;
                    } else if (has_sps && has_pps) {
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (p_extradata) {
        extradata_size = (TU32)(data_base + data_size - p_extradata);
        return EECode_OK;
    }

    return EECode_NotExist;
}

EECode gfGetH264SPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size)
{
    TU8 has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = ptr[4] & 0x1F;
                        if (ENalType_SPS == nal_type) {
                            has_sps = 1;
                            p_sps = ptr;
                            ptr += 3;
                        } else if (ENalType_PPS == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(p_sps);
                            p_pps = ptr;
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_sps && has_pps) {
                            sps_size = (TU32)(p_pps - p_sps);
                            pps_size = (TU32)(ptr - p_pps);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = ptr[3] & 0x1F;
                    if (ENalType_SPS == nal_type) {
                        has_sps = 1;
                        p_sps = ptr;
                        ptr += 2;
                    } else if (ENalType_PPS == nal_type) {
                        DASSERT(has_sps);
                        has_pps = 1;
                        p_pps = ptr;
                        ptr += 2;
                    } else if (has_sps && has_pps) {
                        sps_size = (TU32)(p_pps - p_sps);
                        pps_size = (TU32)(ptr - p_pps);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (has_sps && has_pps) {
        sps_size = (TU32)(p_pps - p_sps);
        pps_size = (TU32)(ptr - p_pps);
        return EECode_OK;
    }

    return EECode_NotExist;
}

EECode gfGetH265Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size)
{
    TU8 has_vps = 0, has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    p_extradata = NULL;
    extradata_size = 0;

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = (ptr[4] >> 1) & 0x3F;
                        if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
                            DASSERT(has_vps);
                            DASSERT(has_sps);
                            DASSERT(has_pps);
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        } else if (EHEVCNalType_VPS == nal_type) {
                            has_vps = 1;
                            p_extradata = ptr;
                            ptr += 3;
                        } else if (EHEVCNalType_SPS == nal_type) {
                            DASSERT(has_vps);
                            DASSERT(p_extradata);
                            has_sps = 1;
                            ptr += 3;
                        } else if (EHEVCNalType_PPS == nal_type) {
                            DASSERT(has_vps);
                            DASSERT(has_sps);
                            DASSERT(p_extradata);
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_vps && has_sps && has_pps) {
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = (ptr[3] >> 1) & 0x3F;
                    if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
                        DASSERT(has_vps);
                        DASSERT(has_sps);
                        DASSERT(has_pps);
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    } else if (EHEVCNalType_VPS == nal_type) {
                        has_vps = 1;
                        p_extradata = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_SPS == nal_type) {
                        DASSERT(has_vps);
                        has_sps = 1;
                        p_extradata = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_PPS == nal_type) {
                        DASSERT(has_vps);
                        DASSERT(has_sps);
                        DASSERT(p_extradata);
                        has_pps = 1;
                        ptr += 2;
                    } else if (has_vps && has_sps && has_pps) {
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (p_extradata) {
        extradata_size = (TU32)(data_base + data_size - p_extradata);
        return EECode_OK;
    }

    return EECode_NotExist;
}

EECode gfGetH265VPSSPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_vps, TU32 &vps_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size)
{
    TU8 has_vps = 0, has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = (ptr[4] >> 1) & 0x3F;
                        if (EHEVCNalType_VPS == nal_type) {
                            has_vps = 1;
                            p_vps = ptr;
                            ptr += 3;
                        } else if (EHEVCNalType_SPS == nal_type) {
                            DASSERT(has_vps);
                            p_sps = ptr;
                            has_sps = 1;
                            ptr += 3;
                        } else if (EHEVCNalType_PPS == nal_type) {
                            DASSERT(has_vps);
                            DASSERT(has_sps);
                            p_pps = ptr;
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_vps && has_sps && has_pps) {
                            vps_size = (TU32)(p_sps - p_vps);
                            sps_size = (TU32)(p_pps - p_sps);
                            pps_size = (TU32)(ptr - p_pps);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = (ptr[3] >> 1) & 0x3F;
                    if (EHEVCNalType_VPS == nal_type) {
                        has_vps = 1;
                        p_vps = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_SPS == nal_type) {
                        DASSERT(has_vps);
                        has_sps = 1;
                        p_sps = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_PPS == nal_type) {
                        DASSERT(has_vps);
                        DASSERT(has_sps);
                        has_pps = 1;
                        p_pps = ptr;
                        ptr += 2;
                    } else if (has_vps && has_sps && has_pps) {
                        vps_size = (TU32)(p_sps - p_vps);
                        sps_size = (TU32)(p_pps - p_sps);
                        pps_size = (TU32)(ptr - p_pps);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (has_vps && has_sps && has_pps) {
        vps_size = (TU32)(p_sps - p_vps);
        sps_size = (TU32)(p_pps - p_sps);
        pps_size = (TU32)(ptr - p_pps);
        return EECode_OK;
    }

    return EECode_NotExist;
}

TU8 *gfNALUFindFirstAVCSliceHeader(TU8 *p, TU32 len)
{
    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        if (((*(p + 4)) & 0x1F) <= ENalType_IDR) {
                            return p;
                        }
                    }
                } else if (*(p + 2) == 0x01) {
                    if (((*(p + 3)) & 0x1F) <= ENalType_IDR) {
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

TU8 *gfNALUFindFirstAVCSliceHeaderType(TU8 *p, TU32 len, TU8 &nal_type)
{
    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        if (((*(p + 4)) & 0x1F) <= ENalType_IDR) {
                            nal_type = (*(p + 4)) & 0x1F;
                            return p;
                        }
                    }
                } else if (*(p + 2) == 0x01) {
                    if (((*(p + 3)) & 0x1F) <= ENalType_IDR) {
                        nal_type = (*(p + 3)) & 0x1F;
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

