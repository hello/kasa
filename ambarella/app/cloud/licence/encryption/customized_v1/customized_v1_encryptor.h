/**
 * customized_v1_encryption.h
 *
 * History:
 *  2014/03/08 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __CUSTOMIZED_V1_ENCRYPTION_H__
#define __CUSTOMIZED_V1_ENCRYPTION_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CCustomizedV1Encryptor
//
//-----------------------------------------------------------------------

class CCustomizedV1Encryptor: public ILicenceEncryptor
{
public:
    static CCustomizedV1Encryptor *Create(TU32 seed1, TU32 seed2);

protected:
    EECode Construct(TU32 seed1, TU32 seed2);

    CCustomizedV1Encryptor();
    virtual ~CCustomizedV1Encryptor();

public:
    virtual EECode Encryption(TU8 *p_encryption, TMemSize &encryption_size, TU8 *p_decryption, TMemSize decryption_size);
    virtual EECode Decryption(TU8 *p_encryption, TMemSize encryption_size, TU8 *p_decryption, TMemSize &decryption_size);

public:
    virtual void Destroy();

};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

