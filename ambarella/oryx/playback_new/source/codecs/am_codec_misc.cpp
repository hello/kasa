/**
 * am_codec_misc.cpp
 *
 * History:
 *  2014/08/26 - [Zhi He] create file
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
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_playback_new_if.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_codec_interface.h"

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
              ptr += 3;
            } else if (ENalType_PPS == nal_type) {
              DASSERT(has_sps);
              DASSERT(p_sps);
              if (!has_pps) {
                has_pps = 1;
                p_pps = ptr;
              }
              ptr += 3;
            }
          }
        } else if (*(ptr + 2) == 0x01) {
          nal_type = ptr[3] & 0x1F;
          if (has_pps && (!p_pps_end)) {
            p_pps_end = ptr;
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
            ptr += 3;
          } else if (ENalType_PPS == nal_type) {
            DASSERT(has_sps);
            DASSERT(p_sps);
            if (!has_pps) {
              has_pps = 1;
              p_pps = ptr;
            }
            ptr += 3;
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

void gfFindH265VpsSpsPpsIdr(TU8 *data_base, TInt data_size, TU8 &has_vps, TU8 &has_sps, TU8 &has_pps, TU8 &has_idr, TU8 *&p_vps, TU8 *&p_sps, TU8 *&p_pps, TU8 *&p_pps_end, TU8 *&p_idr)
{
  TU8 *ptr = data_base, *ptr_end = data_base + data_size;
  TUint nal_type = 0;
  if (DUnlikely((NULL == ptr) || (!data_size))) {
    LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
    return;
  }
  has_vps = 0;
  has_sps = 0;
  has_pps = 0;
  p_pps_end = 0;
  has_idr = 0;
  p_vps = NULL;
  p_sps = NULL;
  p_pps = NULL;
  p_idr = NULL;
  while ((ptr + 6) < ptr_end) {
    if (*ptr == 0x00) {
      if (*(ptr + 1) == 0x00) {
        if (*(ptr + 2) == 0x00) {
          if (*(ptr + 3) == 0x01) {
            nal_type = (ptr[4] >> 1) & 0x3F;
            if (has_pps && (!p_pps_end)) {
              p_pps_end = ptr;
            }
            if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
              DASSERT(has_sps);
              DASSERT(has_pps);
              DASSERT(p_sps);
              DASSERT(p_pps);
              has_idr = 1;
              p_idr = ptr;
              return;
            } else if (EHEVCNalType_VPS == nal_type) {
              if (!has_vps) {
                has_vps = 1;
                p_vps = ptr;
              }
              ptr += 3;
            } else if (EHEVCNalType_SPS == nal_type) {
              DASSERT(has_vps);
              DASSERT(p_vps);
              if (!has_sps) {
                has_sps = 1;
                p_sps = ptr;
              }
              ptr += 3;
            } else if (EHEVCNalType_PPS == nal_type) {
              DASSERT(has_vps);
              DASSERT(p_vps);
              DASSERT(has_sps);
              DASSERT(p_sps);
              if (!has_pps) {
                has_pps = 1;
                p_pps = ptr;
              }
              ptr += 3;
            }
          }
        } else if (*(ptr + 2) == 0x01) {
          nal_type = (ptr[3] >> 1) & 0x3F;
          if (has_pps && (!p_pps_end)) {
            p_pps_end = ptr;
          }
          if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
            DASSERT(has_sps);
            DASSERT(has_pps);
            DASSERT(p_sps);
            DASSERT(p_pps);
            has_idr = 1;
            p_idr = ptr;
            return;
          } else if (EHEVCNalType_VPS == nal_type) {
            if (!has_vps) {
              has_vps = 1;
              p_vps = ptr;
            }
            ptr += 3;
          } else if (EHEVCNalType_SPS == nal_type) {
            DASSERT(has_vps);
            DASSERT(p_vps);
            if (!has_sps) {
              has_sps = 1;
              p_sps = ptr;
            }
            ptr += 3;
          } else if (EHEVCNalType_PPS == nal_type) {
            DASSERT(has_vps);
            DASSERT(p_vps);
            DASSERT(has_sps);
            DASSERT(p_sps);
            if (!has_pps) {
              has_pps = 1;
              p_pps = ptr;
            }
            ptr += 3;
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

TU8 gfGetAACSamplingFrequencyIndex(TU32 samplerate)
{
  switch (samplerate) {
    case 96000:
      return eSamplingFrequencyIndex_96000;
      break;
    case 88200:
      return eSamplingFrequencyIndex_88200;
      break;
    case 64000:
      return eSamplingFrequencyIndex_64000;
      break;
    case 48000:
      return eSamplingFrequencyIndex_48000;
      break;
    case 44100:
      return eSamplingFrequencyIndex_44100;
      break;
    case 24000:
      return eSamplingFrequencyIndex_24000;
      break;
    case 16000:
      return eSamplingFrequencyIndex_16000;
      break;
    case 8000:
      return eSamplingFrequencyIndex_8000;
      break;
    case 12000:
      return eSamplingFrequencyIndex_12000;
      break;
    case 32000:
      return eSamplingFrequencyIndex_32000;
      break;
    case 22050:
      return eSamplingFrequencyIndex_22050;
      break;
    case 11025:
      return eSamplingFrequencyIndex_11025;
      break;
    default:
      LOG_ERROR("NOT support sample rate (%d) here.\n", samplerate);
      break;
  }
  return 0;
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
  while (len > 4) {
    if (*p == 0x00) {
      if (*(p + 1) == 0x00) {
        if (*(p + 2) == 0x00) {
          if (*(p + 3) == 0x01) {
            nal_type = (*(p + 4)) & 0x1F;
            if (nal_type <= ENalType_IDR) {
              return p;
            }
          }
        } else if (*(p + 2) == 0x01) {
          nal_type = (*(p + 3)) & 0x1F;
          if (nal_type <= ENalType_IDR) {
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

TU8 *gfNALUFindFirstHEVCSliceHeaderType(TU8 *p, TU32 len, TU8 &nal_type, TU8 &is_first_slice)
{
  if (DUnlikely(NULL == p)) {
    return NULL;
  }
  is_first_slice = 0;
  while (len > 5) {
    if (*p == 0x00) {
      if (*(p + 1) == 0x00) {
        if (*(p + 2) == 0x00) {
          if (*(p + 3) == 0x01) {
            nal_type = (((*(p + 4)) >> 1) & 0x3F);
            if (nal_type < EHEVCNalType_VPS) {
              if (p[6] & 0x80) {
                is_first_slice = 1;
              } else {
                is_first_slice = 0;
              }
              return p;
            }
          }
        } else if (*(p + 2) == 0x01) {
          nal_type = (((*(p + 3)) >> 1) & 0x3F);
          if (nal_type < EHEVCNalType_VPS) {
            if (p[5] & 0x80) {
              is_first_slice = 1;
            } else {
              is_first_slice = 0;
            }
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

TU8 *gfNALUFindFirstHEVCVPSSPSPPSAndSliceNalType(TU8 *p, TU32 len, TU8 &nal_type)
{
  if (DUnlikely(NULL == p)) {
    return NULL;
  }
  while (len > 5) {
    if (*p == 0x00) {
      if (*(p + 1) == 0x00) {
        if (*(p + 2) == 0x00) {
          if (*(p + 3) == 0x01) {
            nal_type = (((*(p + 4)) >> 1) & 0x3F);
            if (nal_type < EHEVCNalType_AUD) {
              return p;
            }
          }
        } else if (*(p + 2) == 0x01) {
          nal_type = (((*(p + 3)) >> 1) & 0x3F);
          if (nal_type < EHEVCNalType_AUD) {
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

TU8 *gfNALUFindFirstAVCNalType(TU8 *p, TU32 len, TU8 &nal_type)
{
  TU8 naltype = 0;
  if (DUnlikely(NULL == p)) {
    return NULL;
  }
  while (len) {
    if (*p == 0x00) {
      if (*(p + 1) == 0x00) {
        if (*(p + 2) == 0x00) {
          if (*(p + 3) == 0x01) {
            naltype = ((*(p + 4)) & 0x1F);
            if ((naltype <= ENalType_IDR) || (naltype == ENalType_SPS) || (naltype == ENalType_PPS)) {
              nal_type = naltype;
              return p;
            }
          }
        } else if (*(p + 2) == 0x01) {
          naltype = ((*(p + 3)) & 0x1F);
          if ((naltype <= ENalType_IDR) || (naltype == ENalType_SPS) || (naltype == ENalType_PPS)) {
            nal_type = naltype;
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

EECode gfParseJPEG(TU8 *p, TU32 size, SJPEGInfo *info)
{
  TU8 *p_cur = p, *p_limit = p + size;
  TU8 segment_type, start_image = 0, end_image = 0, start_scan = 0, start_frame = 0;
  TU32 segment_length = 0;
  SJPEGQtTable *p_cur_table = NULL;
  if (DUnlikely((!p) || (!size) || (!info))) {
    LOG_FATAL("NULL (%p) or zero size(%d) or NULL info (%p)\n", p, size, info);
    return EECode_BadParam;
  }
  if ((EJPEG_MarkerPrefix != p[0]) || (EJPEG_SOI != p[1]) || (EJPEG_MarkerPrefix != p[2])) {
    LOG_ERROR("bad jpeg header: %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3]);
    return EECode_DataCorruption;
  }
  info->width = 0;
  info->height = 0;
  info->type = 0;
  info->number_qt_tables = 0;
  info->precision = 0;
  memset(&info->qt_tables[0], 0x0, sizeof(info->qt_tables));
  info->total_tables_length = 0;
  info->p_jpeg_content = NULL;
  info->jpeg_content_length = 0;
  while (p_cur < p_limit) {
    if (EJPEG_MarkerPrefix != p_cur[0]) {
      LOG_ERROR("no marker prefix: %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3]);
      return EECode_DataCorruption;
    }
    segment_type = p_cur[1];
    if (EJPEG_SOI == segment_type) {
      start_image = 1;
      p_cur += 2;
      //LOG_PRINTF("SOI\n");
    } else if (EJPEG_EOI == segment_type) {
      LOG_FATAL("get EOI now?\n");
      return EECode_DataCorruption;
    } else if (EJPEG_SOS == segment_type) {
      start_scan = 1;
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      info->p_jpeg_content = p_cur;
      //LOG_PRINTF("SOS, %d\n", segment_length);
      TU8 *p_end = p_limit - 6;
      while ((p_end + 1) < p_limit) {
        if ((EJPEG_MarkerPrefix == p_end[0]) && (EJPEG_EOI == p_end[1])) {
          end_image = 1;
          DASSERT(info->p_jpeg_content);
          info->jpeg_content_length = (TU32)(p_end - info->p_jpeg_content);
          //LOG_PRINTF("data length %d\n", info->jpeg_content_length);
          break;
        }
        p_end ++;
      }
      break;
    } else if (EJPEG_DQT == segment_type) {
      if ((info->number_qt_tables + 1) >= DMAX_JPEG_QT_TABLE_NUMBER) {
        LOG_ERROR("too many tables? %d\n", info->number_qt_tables);
        return EECode_DataCorruption;
      }
      p_cur_table = &info->qt_tables[info->number_qt_tables];
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur_table->p_table = p_cur + 3;
      if (0 == (p_cur[2] >> 4)) {
        p_cur_table->length = 64;
      } else if (1 == (p_cur[2] >> 4)) {
        info->precision |= (1 << info->number_qt_tables);
        p_cur_table->length = 128;
      } else {
        LOG_FATAL("bad precise\n");
        return EECode_DataCorruption;
      }
      info->total_tables_length += p_cur_table->length;
      p_cur += segment_length;
      info->number_qt_tables ++;
      //LOG_PRINTF("dq table %d, length %d\n", info->number_qt_tables, segment_length);
    } else if ((EJPEG_APP_MIN <= segment_type) && (EJPEG_APP_MAX >= segment_type)) {
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      //LOG_PRINTF("skip EJPEG_APP, %02x, %d\n", segment_type, segment_length);
    } else if ((EJPEG_JPEG_MIN <= segment_type) && (EJPEG_JPEG_MAX >= segment_type)) {
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      //LOG_PRINTF("skip EJPEG_JPEG, %02x, %d\n", segment_type, segment_length);
    } else if ((EJPEG_REV_MIN <= segment_type) && (EJPEG_REV_MAX >= segment_type)) {
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      //LOG_PRINTF("skip EJPEG_REV, %02x, %d\n", segment_type, segment_length);
    } else {
      switch (segment_type) {
        case EJPEG_SOF0:
        case EJPEG_SOF1:
        case EJPEG_SOF2:
        case EJPEG_SOF3:
        case EJPEG_SOF5:
        case EJPEG_SOF6:
        case EJPEG_SOF7:
        case EJPEG_SOF8:
        case EJPEG_SOF9:
        case EJPEG_SOF10:
        case EJPEG_SOF11:
        case EJPEG_SOF13:
        case EJPEG_SOF14:
        case EJPEG_SOF15: {
            DASSERT(!start_frame);
            start_frame = 1;
            p_cur += 2;
            DBER16(segment_length, p_cur);
            TU8 *ptmp = p_cur + 3;
            DBER16(info->height, ptmp);
            ptmp += 2;
            DBER16(info->width, ptmp);
            ptmp += 2;
            TU32 ncomponents = ptmp[0];
            ptmp ++;
            TU32 sampling_factor_h[4] = {1};
            TU32 sampling_factor_v[4] = {1};
            if (3 == ncomponents) {
              LOG_PRINTF("jpeg: 3 components %02x %02x %02x.\n", ptmp[1], ptmp[4], ptmp[7]);
              sampling_factor_h[0] = (ptmp[1] >> 4) & 0xf;
              sampling_factor_v[0] = (ptmp[1]) & 0xf;
              sampling_factor_h[1] = (ptmp[4] >> 4) & 0xf;
              sampling_factor_v[1] = (ptmp[4]) & 0xf;
              sampling_factor_h[2] = (ptmp[7] >> 4) & 0xf;
              sampling_factor_v[2] = (ptmp[7]) & 0xf;
              if ((sampling_factor_h[1] == sampling_factor_h[2]) && ((sampling_factor_h[1] << 1) == sampling_factor_h[0])) {
                if (sampling_factor_v[1] == sampling_factor_v[2]) {
                  if ((sampling_factor_v[1] << 1) == sampling_factor_v[0]) {
                    info->type = EJpegTypeInFrameHeader_YUV420;
                  } else if (sampling_factor_v[1] == sampling_factor_v[0]) {
                    info->type = EJpegTypeInFrameHeader_YUV422;
                  } else {
                    LOG_ERROR("only support 420 and 422. %02x %02x %02x\n", ptmp[1], ptmp[4], ptmp[7]);
                    gfPrintMemory(ptmp, 16);
                    return EECode_BadFormat;
                  }
                } else {
                  LOG_ERROR("v sampling not expected. %02x %02x %02x\n", ptmp[1], ptmp[4], ptmp[7]);
                  gfPrintMemory(ptmp, 16);
                  return EECode_BadFormat;
                }
              } else {
                LOG_ERROR("h sampling not expected. %02x %02x %02x\n", ptmp[1], ptmp[4], ptmp[7]);
                gfPrintMemory(ptmp, 16);
                return EECode_BadFormat;
              }
            } else if (1 == ncomponents) {
              LOG_PRINTF("jpeg: 1 component %02x\n", ptmp[1]);
              info->type = EJpegTypeInFrameHeader_GREY8;
            } else {
              LOG_WARN("not support components %d\n", ncomponents);
            }
            p_cur += segment_length;
            //LOG_PRINTF("jpeg frame header, %02x, length %d, %dx%d, type %d\n", segment_type, segment_length, info->width, info->height, info->type);
          }
          break;
        case EJPEG_DHT:
        case EJPEG_DAT:
        case EJPEG_DNL:
        case EJPEG_DRI:
        case EJPEG_DHP:
        case EJPEG_EXP:
        case EJPEG_COMMENT:
          p_cur += 2;
          DBER16(segment_length, p_cur);
          p_cur += segment_length;
          //LOG_PRINTF("jpeg segment, %02x, %d\n", segment_type, segment_length);
          break;
        default:
          p_cur += 2;
          DBER16(segment_length, p_cur);
          p_cur += segment_length;
          LOG_WARN("unknown jpeg segment, %02x, %d\n", segment_type, segment_length);
          break;
      }
    }
  }
  if (!start_image) {
    LOG_ERROR("do not find soi\n");
    return EECode_DataMissing;
  }
  if (!start_frame) {
    LOG_ERROR("do not find sof\n");
    return EECode_DataMissing;
  }
  if (!start_scan) {
    LOG_ERROR("do not find sos\n");
    return EECode_DataMissing;
  }
  if (!end_image) {
    LOG_ERROR("do not find eoi\n");
    return EECode_DataMissing;
  }
  return EECode_OK;
}

void gfAmendVideoResolution(TU32 &width, TU32 &height)
{
  if ((1920 == width) && (1088 == height)) {
    height = 1080;
    LOG_CONFIGURATION("Amend height to 1080\n");
  } else if ((1920 == height) && (1088 == width)) {
    width = 1080;
    LOG_CONFIGURATION("Amend width to 1080\n");
  }
}

EECode gfGetH264SizeFromSPS(TU8 *p_sps, TU32 &width, TU32 &height)
{
  EECode err = EECode_OK;
  SCodecVideoCommon *p_video_parser = gfGetVideoCodecParser(p_sps + 5, 40, StreamFormat_H264, err);
  if (DUnlikely(!p_video_parser || EECode_OK != err)) {
    LOG_ERROR("gfGetVideoCodecParser failed, ret %d, %s\n", err, gfGetErrorCodeString(err));
    if (p_video_parser) {
      DDBG_FREE(p_video_parser, "GAVC");
    }
    return EECode_DataCorruption;
  } else {
    width = p_video_parser->max_width;
    height = p_video_parser->max_height;
  }
  if (p_video_parser) {
    DDBG_FREE(p_video_parser, "GAVC");
  }
  return err;
}

