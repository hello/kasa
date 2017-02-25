/*******************************************************************************
 * am_sensor.h
 *
 * History:
 *   May 7, 2015 - [ypchang] created file
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
#ifndef ORYX_VIDEO_INCLUDE_AM_SENSOR_H_
#define ORYX_VIDEO_INCLUDE_AM_SENSOR_H_

#include <vector>
#include <string>
#include "am_platform_if.h"

class AMSensor
{
  public:
    /*! @struct AMSensorCapability
     *  Defines sensor capability
     */
    struct AMSensorCapability
    {
        AM_SENSOR_TYPE       type;
        AM_VIDEO_FPS         fps;
        AM_HDR_TYPE          hdr;
        AM_VIN_MODE          mode;
        AM_VIN_BITS          bits;
        AMResolution         res;
        AMSensorCapability() :
          type(AM_SENSOR_TYPE_NONE),
          fps(AM_VIDEO_FPS_AUTO),
          hdr(AM_HDR_SINGLE_EXPOSURE),
          mode(AM_VIN_MODE_AUTO),
          bits(AM_VIN_BITS_AUTO)
        {
          res = {0};
        }
        AMSensorCapability(const AMSensorCapability &cap) :
          type(cap.type),
          fps(cap.fps),
          hdr(cap.hdr),
          mode(cap.mode),
          bits(cap.bits)
        {
          res.width  = cap.res.width;
          res.height = cap.res.height;
        }
    };

    /*!
     * @typedef AMSensorCapList
     * Sensor capability list type
     */
    typedef std::vector<AMSensorCapability> AMSensorCapList;

    /*! @struct AMSensor
     *  Defines sensor structure
     */
    struct AMSensorParameter
    {
        uint32_t        id;     //!< Sensor ID
        uint32_t        index;  //!< Default capability index
        std::string     name;   //!< Sensor name string
        AMSensorCapList cap;    //!< Sensor capability list
    };

  public:
    static AMSensor* create(AM_VIN_ID id);
    void destroy();

  public:
    /*! Get sensor parameter structure
     * @return Pointer to AMSensor on success or NULL if failed.
     */
    AMSensorParameter* get_sensor_driver_parameters();
    AMSensorParameter* get_sensor_config_parameters();

  private:
    AMIPlatformPtr     m_platform;
  protected:
    AMSensorParameter *m_sensor_drv_parameter;
    AMSensorParameter *m_sensor_cfg_parameter;
    AM_VIN_ID          m_source_id;

  private:
    AMSensor();
    virtual ~AMSensor();
    AM_RESULT init(AM_VIN_ID id);
};

#endif /* ORYX_VIDEO_INCLUDE_AM_SENSOR_H_ */
