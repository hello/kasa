/******************************************************************************
 *
 * am_qrcode.h.cpp
 *
 * History:
 *   2014/12/17 - [Long Li] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
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


