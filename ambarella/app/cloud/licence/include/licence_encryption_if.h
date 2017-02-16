/**
 * licence_encryption_if.h
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

#ifndef __LICENCE_ENCRYPTION_IF_H__
#define __LICENCE_ENCRYPTION_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class ILicenceEncryptor
{
public:
    virtual EECode Encryption(TU8 *p_encryption, TMemSize &encryption_size, TU8 *p_decryption, TMemSize decryption_size) = 0;
    virtual EECode Decryption(TU8 *p_encryption, TMemSize encryption_size, TU8 *p_decryption, TMemSize &decryption_size) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ILicenceEncryptor() {}
};

ILicenceEncryptor *gfCreateLicenceEncryptor(ELicenceEncryptionType type = ELicenceEncryptionType_CustomizedV1, TU32 seed1 = 17, TU32 seed2 = 29);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

