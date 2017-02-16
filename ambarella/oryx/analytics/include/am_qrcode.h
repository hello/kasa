/*******************************************************************************
 * am_qrcode.h.cpp
 *
 * History:
 *   2014/12/16 - [Long Li] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
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
