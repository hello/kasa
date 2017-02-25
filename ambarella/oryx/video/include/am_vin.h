/**
 * am_vin.h
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
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
#ifndef ORYX_VIDEO_INCLUDE_AM_VIN_H_
#define ORYX_VIDEO_INCLUDE_AM_VIN_H_

#include "am_sensor.h"

#include <semaphore.h>
#include <atomic>

class AMThread;
class AMVin
{
  public:
    static AMVin* create(AM_VIN_ID id);
    virtual void destroy();

    virtual bool is_close_vin_needed();
    virtual AM_RESULT load_config();
    virtual AM_RESULT get_param(AMVinParamMap &param);
    virtual AM_RESULT set_param(const AMVinParamMap &param);

    virtual AM_RESULT start();
    virtual AM_RESULT stop();

    virtual AM_RESULT size_get(AMResolution &size);
    virtual AM_VIN_ID id_get();

    virtual AM_RESULT wait_frame_sync();
    virtual AM_RESULT get_agc(AMVinAGC &agc);

  protected:
    virtual AM_RESULT set_mode();
    virtual AM_RESULT set_fps();
    virtual AM_RESULT set_flip();

  protected:
    AMVin();
    virtual ~AMVin();
    AM_RESULT init(AM_VIN_ID id);

  protected:
    static void static_vin_sync(void *data);
    void vin_sync();

  protected:
    AMSensor                     *m_sensor;
    AMSensor::AMSensorParameter  *m_sensor_param;
    AMThread                     *m_vin_sync_thread;
    sem_t                         m_vin_sync_sem;
    AM_VIN_ID                     m_id;
    AM_VIN_STATE                  m_state;
    AM_HDR_TYPE                   m_hdr_type;
    bool                          m_hdr_type_changed;
    bool                          m_vin_sync_run;
    bool                          m_vin_sync_sem_inited;
    std::atomic<int>              m_vin_sync_waiters_num;
    AMVinConfigPtr                m_config;
    AMIPlatformPtr                m_platform;
    AMVinParamMap                 m_param;
    AMResolution                  m_size;
};

class AMRGBSensorVin: public AMVin
{
  public:
    static AMRGBSensorVin* create(AM_VIN_ID id);
    virtual void destroy();

  public:
    AM_RESULT set_mode();

  private:
    AMRGBSensorVin();
    virtual ~AMRGBSensorVin();
    AM_RESULT init(AM_VIN_ID id);
};

class AMYUVSensorVin: public AMVin
{
  public:
    static AMYUVSensorVin* create(AM_VIN_ID id);
    virtual void destroy();

  private:
    AMYUVSensorVin();
    virtual ~AMYUVSensorVin();
    AM_RESULT init(AM_VIN_ID id);
};

class AMYUVGenericVin: public AMVin
{
  public:
    static AMYUVGenericVin* create(AM_VIN_ID id);
    virtual void destroy();

  private:
    AMYUVGenericVin();
    virtual ~AMYUVGenericVin();
    AM_RESULT init(AM_VIN_ID id);

  private:
    uint32_t m_vin_source; //only YUV generic has concept of "source"
};

#endif /* ORYX_VIDEO_INCLUDE_AM_VIN_H_ */
