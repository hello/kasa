/*******************************************************************************
 * am_image_quality_data.h
 *
 * History:
 *   Aug 9, 2016 - [zfgong] created file
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
#ifndef AM_IMAGE_QUALITY_DATA_IF_H_
#define AM_IMAGE_QUALITY_DATA_IF_H_

struct AMImageAEConfig
{
    /*!
     * @sa AM_AE_CONFIG_BIT
     */
    uint32_t enable_bits;

    /*! percentage of ae target to use, [25,400], default value is 100
     * - < 100 (get darker picture)
     * - > 100 (get brighter picture)
     */
    uint32_t ae_target_ratio;

    /*! minimum sensor shutter time */
    uint32_t sensor_shutter_min;

    /*! maximum sensor shutter time */
    uint32_t sensor_shutter_max;

    /*!
     * image sensor analog gain, in dB unit;
     * this field is used to get current sensor gain,
     * or manually set sensor gain when AE is disabled.
     */
    uint32_t sensor_gain;

    /*!
     * image sensor shutter time, 1/x seconds unit;
     * this field is used to get current sensor shutter time,
     * or manually set sensor shutter time when AE is disabled.
     */
    uint32_t sensor_shutter;

    /*!
     *  - luma_value[0]RGB luma
     *  - lumavalue[1]CFA luma
     */
    uint32_t luma_value[2];

    /*!
     * - 0: spot
     * - 1: center(default)
     * - 2: average
     * - 3: custom
     */
    uint8_t ae_metering_mode;

    /*!
     * - 0: day mode, colorful(default)
     * - 1: night mode, black&white
     */
    uint8_t day_night_mode;

    /*!
     * - 0: off,
     * - 1: auto(default)
     */
    uint8_t slow_shutter_mode;

    /*!
     * - 0: 50Hz (China, EU)
     * - 1: 60Hz (US, TW)(default)
     */
    uint8_t anti_flicker_mode;

    /*!
     * - 0: off(default)
     * - 1: on
     */
    uint8_t backlight_comp_enable;

    /*!
     * - 0: off(default)
     * - 1: weak
     * - 2: normal
     * - 3: strong
     */
    uint8_t local_exposure;

    /*!
     * - 0: disabled(default)
     * - 1: enabled
     */
    uint8_t dc_iris_enable;

    /*!
     * maximum sensor analog gain, minimum sensor analog gain usually is 0 dB
     */
    uint8_t sensor_gain_max;

    /*!
     * - 0: off
     * - 1: on
     * - 2: auto(default)
     */
    uint8_t ir_led_mode;

    /*!
     * - 0: disable AE
     * - 1: enable AE(default)
     */
    uint8_t ae_enable;
};

struct AMImageAFConfig
{
    /*!
     * @sa AM_AF_CONFIG_AF_MODE
     */
    uint32_t enable_bits;

    /*! - 0: default
     */
    uint32_t af_mode;
};

struct AMImageAWBConfig
{
    /*!
     * @sa AM_AWB_CONFIG_WB_MODE
     */
    uint32_t enable_bits;

    /*!
     * - 0: auto(default)
     * - 1: 2800K
     * - 2: D4000
     * - 3: D5000
     * - 4: SUNNY(6500K)
     * - 5: CLOUD(7500K)
     * - 6: FLASH
     * - 7: FLUORESCENT
     * - 8: FLUORESCENT_H
     * - 9: UNDERWATER
     * - 10: CUSTOM
     */
    uint32_t wb_mode;
};

struct AMImage3AInfo
{
    AMImageAEConfig ae;
    AMImageAFConfig af;
    AMImageAWBConfig awb;
};


#endif /* AM_IMAGE_QUALITY_DATA_IF_H_ */
