/*******************************************************************************
 * am_qrcode.h.cpp
 *
 * History:
 *   2014/12/16 - [Long Li] created file
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

#ifndef __AM_QRCODE_H__
#define __AM_QRCODE_H__
#include "am_qrcode_if.h"
#include <vector>

using namespace std;
typedef vector<pair<string, string> > AMQrcodeCfgDetail;
typedef vector<pair<string, vector<pair<string, string> > > > AMQrcodeConfig;

/* Instruction:
 * 1.Make sure that camera must be in preview or
 * encoding mode when use this class
 */
class AMQrcode: public AMIQrcode
{
  public:
    static AMQrcode* create();
    virtual void destroy() override;
    /* set file_path to store configurations */
    virtual bool set_config_path(const string &config_path) override;
    /* read qrcode from camera, timeout is in second */
    virtual bool qrcode_read(const int32_t buf_index, const uint32_t timeout) override;
    /* parse config file which qrcode_read() generates */
    virtual bool qrcode_parse() override;
    virtual void print_qrcode_config() override;
    virtual const string get_key_value(const string &config_name,
                                       const string &key_word) override;
    virtual bool print_config_name_list() override;
    /* print key name list under config name */
    virtual bool print_key_name_list(const string &config_name) override;

  private:
    AMQrcode();
    virtual ~AMQrcode();
    void remove_duplicate(AMQrcodeConfig &config_list, const string &config_name);
    bool parse_keyword(AMQrcodeCfgDetail &config_detail, string &entry);

  private:
    string m_config_path;
    AMQrcodeConfig m_config;
};

#endif
