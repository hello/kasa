/*******************************************************************************
 * runtime_config_xml.h
 *
 * History:
 *  2013/05/24- [Zhi He] create file
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

#ifndef __RUNTIME_CONFIG_XML_H__
#define __RUNTIME_CONFIG_XML_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#if 0
#define DMEDIA_CONFIGFILE_DEFAULT_NAME "media.config"
#define DLOG_CONFIGFILE_DEFAULT_NAME "log.config"

#define DMEDIA_CONFIGFILE_ROOT_NAME "MediaConfig"
#define DLOG_CONFIGFILE_ROOT_NAME "LogConfig"
#endif

class CXMLRunTimeConfigAPI: public CObject, public IRunTimeConfigAPI
{
public:
    CXMLRunTimeConfigAPI();
    virtual ~CXMLRunTimeConfigAPI();

public:
    virtual ERunTimeConfigType QueryConfigType() const;
    virtual EECode OpenConfigFile(const TChar *config_file_name, TU8 read_or_write = 0, TU32 read_once = 0);
    virtual EECode GetContentValue(const TChar *content_path, TChar *content_value);
    virtual EECode SetContentValue(const TChar *content_path, TChar *content_value);
    virtual EECode SaveConfigFile(const TChar *config_file_name);
    virtual EECode CloseConfigFile();

    virtual EECode NewFile(const TChar *root_name, void *&root_node);
    virtual void *AddContent(void *p_parent, const TChar *content_name);
    virtual void *AddContentChild(void *p_parent, const TChar *child_name, const TChar *value);
    virtual EECode FinalizeFile(const TChar *file_name);

private:
    xmlDocPtr mpDoc;
    xmlNodePtr mpRootNode;
    xmlXPathContextPtr mpContext;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif//__RUNTIME_CONFIG_XML_H__
