/**
 * runtime_config_xml.h
 *
 * History:
 *  2014/03/23 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __RUNTIME_CONFIG_SIMPLE_INI_H__
#define __RUNTIME_CONFIG_SIMPLE_INI_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef struct {
    TChar content[DMaxConfStringLength];
    TChar value[DMaxConfStringLength];
    TChar value1[DMaxConfStringLength];

    TU8 read;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SSimpleINIContent;

class CSimpleINIRunTimeConfigAPI: public CObject, public IRunTimeConfigAPI
{
public:
    CSimpleINIRunTimeConfigAPI(TU32 max);
    virtual ~CSimpleINIRunTimeConfigAPI();

public:
    virtual void Destroy();
    virtual void Delete();

public:
    virtual ERunTimeConfigType QueryConfigType() const;
    virtual EECode OpenConfigFile(const TChar *config_file_name, TU8 read_or_write = 0, TU8 read_once = 0, TU8 number = 1);
    virtual EECode GetContentValue(const TChar *content_path, TChar *content_value, TChar *content_value1 = NULL);
    virtual EECode SetContentValue(const TChar *content_path, TChar *content_value);
    virtual EECode SaveConfigFile(const TChar *config_file_name);
    virtual EECode CloseConfigFile();

    virtual EECode NewFile(const TChar *root_name, void *&root_node);
    virtual void *AddContent(void *p_parent, const TChar *content_name);
    virtual void *AddContentChild(void *p_parent, const TChar *child_name, const TChar *value);
    virtual EECode FinalizeFile(const TChar *file_name);

private:
    TU32 mMaxLine;
    TU32 mCurrentCount;
    SSimpleINIContent *mpContentList;
    FILE *mpFile;

private:
    TU8 mbReadOnce;
    TU8 mNumber;
    TU8 mReserved1;
    TU8 mReserved2;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

