/*******************************************************************************
 * am_file_demuxer_object.h
 *
 * History:
 *   2014-9-5 - [ypchang] created file
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
#ifndef AM_FILE_DEMUXER_OBJECT_H_
#define AM_FILE_DEMUXER_OBJECT_H_

#include <map>

class AMPlugin;
class AMIDemuxerCodec;
class AMFileDemuxerObject
{
    typedef std::map<uint32_t, AMIDemuxerCodec*> DemuxerCodecMap;
    friend class AMIDemuxerCodec;
    friend class AMFileDemuxer;
  public:
    AMFileDemuxerObject(const std::string& demuxer);
    virtual ~AMFileDemuxerObject();

  public:
    bool open();
    void close();
    AMIDemuxerCodec* get_demuxer(uint32_t stream_id);

  private:
    AMPlugin       *m_plugin;
    DemuxerCodecNew get_demuxer_codec;
    AM_DEMUXER_TYPE m_type;
    std::string     m_demuxer;
    DemuxerCodecMap m_codec_map;
};

#endif /* AM_FILE_DEMUXER_OBJECT_H_ */
