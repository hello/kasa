/**
 * security_utils_if.h
 *
 * History:
 *  2014/06/30 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __SECURITY_UTILS_IF_H__
#define __SECURITY_UTILS_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

TU32 gfHashAlgorithmV3(TU8 *p_input, TU32 input_size);
TU32 gfHashAlgorithmV4(TU8 *p_input, TU32 input_size);
TU64 gfHashAlgorithmV5(TU8 *p_input, TU32 input_size);
TU32 gfHashAlgorithmV6(TU8 *p_input, TU32 input_size);
TU64 gfHashAlgorithmV7(TU8 *p_input, TU32 input_size);
TU32 gfHashAlgorithmV8(TU8 *p_input, TU32 input_size);

EECode gfCustomizedEncryptionV0(TU8 *p_encryption, TMemSize &encryption_size, TU8 *p_decryption, TMemSize decryption_size);
EECode gfCustomizedDecryptionV0(TU8 *p_encryption, TMemSize encryption_size, TU8 *p_decryption, TMemSize &decryption_size);

EECode gfParseProductionCode(TChar *p_production_code, TU32 length, TUniqueID &id);
TChar *gfGenerateProductionCode(TU32 &production_code_length, TUniqueID id, TChar *desc, TU32 desc_len);

//standard secure hash function
void gfStandardHashMD5Oneshot(TU8 *input, TU32 size, TChar *&output);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

