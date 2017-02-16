/**
 * runtime_config_xml.h
 *
 * History:
 *  2013/05/24- [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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
