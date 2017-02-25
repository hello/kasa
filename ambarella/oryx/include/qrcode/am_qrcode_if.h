/******************************************************************************
 *
 * am_qrcode.h.cpp
 *
 * History:
 *   2014/12/17 - [Long Li] created file
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

#ifndef __AM_QRCODE_IF_H__
#define __AM_QRCODE_IF_H__
#include <string>
/* Instruction:
 * 1. This class is an interface exposing to users. So don't let other class
 * inherit form it.
 * 2. When using this class, make sure that camera must be in preview or
 * encoding mode.
 * 3. This class is implemented by using zbar lib.
 * 4. Text format that qrcode_parse can recognize:
 * config_name1:key_name1:key_value1;key_name2:key_value2;;config_name2...
 */
class AMIQrcode
{
  public:
    static AMIQrcode *create();
    virtual ~AMIQrcode(){};
    virtual void destroy() = 0;
    /* set file path to store configurations */
    virtual bool set_config_path(const std::string &config_path) = 0;
    /* read qrcode from camera and store it to config_file,
     * timeout is in second
     */
    virtual bool qrcode_read(const int32_t buf_index, const uint32_t timeout) = 0;
    /* parse config_string from config_file which qrcode_read() generates */
    virtual bool qrcode_parse() = 0;
    virtual void print_qrcode_config() = 0;
    /* config_name and key_word are case insensitive */
    virtual const std::string get_key_value(const std::string &config_name,
                                            const std::string &key_word) = 0;
    virtual bool print_config_name_list() = 0;
    /* print key name list under config name */
    virtual bool print_key_name_list(const std::string &config_name) = 0;
};

#endif


