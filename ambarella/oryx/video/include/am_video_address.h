/**
 * am_video_address.h
 *
 *  History:
 *    Aug 11, 2015 - [Shupeng Ren] created file
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
 */
#ifndef ORYX_VIDEO_INCLUDE_AM_VIDEO_ADDRESS_H_
#define ORYX_VIDEO_INCLUDE_AM_VIDEO_ADDRESS_H_

using std::map;
using std::recursive_mutex;
using std::atomic_int;

class AMVideoAddress: public AMIVideoAddress
{
    friend AMIVideoAddress;

  public:
    AM_RESULT addr_get(AM_DATA_FRAME_TYPE type,
                       uint32_t offset,
                       AMAddress &addr)                                override;

    uint8_t* pic_addr_get(uint32_t pic_data_offset)                    override;
    uint8_t* video_addr_get(uint32_t video_data_offset)                override;

    AM_RESULT video_addr_get(const AMQueryFrameDesc &desc,
                             AMAddress &addr)                          override;
    AM_RESULT yuv_y_addr_get(const AMQueryFrameDesc &desc,
                             AMAddress &addr)                          override;
    AM_RESULT yuv_uv_addr_get(const AMQueryFrameDesc &desc,
                              AMAddress &addr)                         override;
    AM_RESULT raw_addr_get(const AMQueryFrameDesc &desc,
                           AMAddress &addr)                            override;
    AM_RESULT me0_addr_get(const AMQueryFrameDesc &desc,
                           AMAddress &addr)                            override;
    AM_RESULT me1_addr_get(const AMQueryFrameDesc &desc,
                           AMAddress &addr)                            override;
    AM_RESULT usr_addr_get(AMAddress &addr)                            override;

    bool is_new_video_session(uint32_t session_id,
                              AM_STREAM_ID stream_id)                  override;
  private:
    AM_RESULT map_bsb();
    AM_RESULT unmap_bsb();
    AM_RESULT map_dsp();
    AM_RESULT unmap_dsp();
    AM_RESULT map_usr();
    AM_RESULT unmap_usr();


  private:
    static AMVideoAddress* get_instance();
    static AMVideoAddress* create();
    void inc_ref()                                                     override;
    void release()                                                     override;
    AMVideoAddress();
    virtual ~AMVideoAddress();
    AM_RESULT init();

    AMVideoAddress(AMVideoAddress const &copy) = delete;
    AMVideoAddress& operator=(AMVideoAddress const &copy) = delete;

  private:
    static AMVideoAddress      *m_instance;
    static recursive_mutex      m_mtx;
    atomic_int                  m_ref_cnt;
    AMIPlatformPtr              m_platform;
    AMMemMapInfo                m_bsb_mem;
    AMMemMapInfo                m_usr_mem;
    map<AM_STREAM_ID, uint32_t> m_stream_session_id;
};

#endif /* ORYX_VIDEO_INCLUDE_AM_VIDEO_ADDRESS_H_ */
