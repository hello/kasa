/**
 * standalone_licence.h
 *
 * History:
 *  2014/03/06 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __STANDALONE_LICENCE_H__
#define __STANDALONE_LICENCE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CStandaloneLicence
//
//-----------------------------------------------------------------------

class CStandaloneLicence: public ILicenceClient
{
public:
    static CStandaloneLicence *Create();

protected:
    EECode Construct();

    CStandaloneLicence();
    virtual ~CStandaloneLicence();

public:
    virtual EECode Authenticate(TChar *native_file, const TChar *verified_string, TU32 verified_string_size, ELicenceEncryptionType type = ELicenceEncryptionType_CustomizedV1, TChar *server_url = NULL, TU32 port = 0);
    virtual EECode DailyCheck();
    virtual EECode GetCapability(TU32 &max_channel_number) const;

public:
    virtual void Destroy();

private:
    FILE *mpDeviceFile;

private:
    SDateTime mExpireDate;

private:
    TU32 mCapabilityMaxChannel;

private:
    ELicenceClientState msLicenceState;
    ELicenceEncryptionType mEncryptionType;

};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

