/*******************************************************************************
 * cloud_sacp_client_agent.cpp
 *
 * History:
 *  2013/11/28 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_network_utils.h"

#include "licence_lib_if.h"
#include "licence_encryption_if.h"

#include "standalone_licence.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static TChar *_find_string_s(TChar *buffer, TInt src_len, const TChar *target, TInt target_len)
{
    char *ptmp = buffer;
    DASSERT(buffer);
    DASSERT(target);
    DASSERT(target_len);

    while ((0x0 != ptmp) && (0 < src_len)) {
        if (!strncmp(ptmp, target, target_len)) {
            return ptmp;
        }
        src_len --;
        ptmp ++;
    }
    return NULL;
}

ILicenceClient *gfCreateStandaloneLicence()
{
    CStandaloneLicence *thiz = CStandaloneLicence::Create();
    return thiz;
}

//-----------------------------------------------------------------------
//
//  CStandaloneLicence
//
//-----------------------------------------------------------------------

CStandaloneLicence *CStandaloneLicence::Create()
{
    CStandaloneLicence *result = new CStandaloneLicence();
    if ((result) && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CStandaloneLicence::Construct()
{
    return EECode_OK;
}

CStandaloneLicence::CStandaloneLicence()
    : mpDeviceFile(NULL)
    , mCapabilityMaxChannel(0)
    , msLicenceState(ELicenceClientState_Invalid)
    , mEncryptionType(ELicenceEncryptionType_CustomizedV1)
{

}

CStandaloneLicence::~CStandaloneLicence()
{

}

EECode CStandaloneLicence::Authenticate(TChar *native_file, const TChar *verified_string, TU32 verified_string_size, ELicenceEncryptionType type, TChar *server_url, TU32 port)
{
    msLicenceState = ELicenceClientState_Invalid;

    if (mpDeviceFile) {
        fclose(mpDeviceFile);
        mpDeviceFile = NULL;
    }

    if (DUnlikely(!native_file)) {
        LOG_FATAL("NULL native_file\n");
        return EECode_BadParam;
    }

    if (DUnlikely((!verified_string) || (!verified_string_size))) {
        LOG_FATAL("NULL verified_string %p, or zero verified_string_size %d\n", verified_string, verified_string_size);
        return EECode_BadParam;
    }

    TU8 readable_licence_string[1024 + 64] = {0};
    TU8 encrypted_licence_string[512 + 32] = {0};
    TU8 decrypted_licence_string[512] = {0};
    TMemSize licence_string_length = 512;
    TUint read_size = 0;

    FILE *licence_file = NULL;

    licence_file = fopen(native_file, "rb");
    if (DLikely(licence_file)) {
        read_size = fread(readable_licence_string, 1, 1024 + 64, licence_file);
        LOG_NOTICE("readable_licence_string %s\n", readable_licence_string);
        if (DLikely(read_size)) {
            gfDecodingBase16(encrypted_licence_string, (const TU8 *)readable_licence_string, (TInt)read_size);
        } else {
            LOG_FATAL("fread(%s) fail\n", native_file);
            return EECode_BadParam;
        }
        read_size = read_size / 2;
    } else {
        LOG_FATAL("fopen(%s) fail\n", native_file);
        return EECode_BadParam;
    }

    msLicenceState = ELicenceClientState_LicenceFileOpend;

    ILicenceEncryptor *encryptor = gfCreateLicenceEncryptor(mEncryptionType, 17, 29);
    if (DUnlikely(!encryptor)) {
        LOG_ERROR("gfCreateLicenceEncryptor fail\n");
        return EECode_Error;
    }

    EECode err = encryptor->Decryption(encrypted_licence_string, (TMemSize)read_size, decrypted_licence_string, licence_string_length);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Decryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    } else {
        LOG_PRINTF("decrypted string %s, size %ld\n", decrypted_licence_string, licence_string_length);
    }

    msLicenceState = ELicenceClientState_LicenceFileVerified;

    TU8 licence_check_string[512 + 64] = {0};
    TMemSize licence_check_string_size = 512 + 64;

    //LOG_NOTICE("!!!before encryp check string, size %d, %02x %02x %02x %02x %02x %02x %02x %02x\n", read_size, encrypted_licence_string[0], encrypted_licence_string[1], encrypted_licence_string[2], encrypted_licence_string[3], encrypted_licence_string[4], encrypted_licence_string[5], encrypted_licence_string[6], encrypted_licence_string[7]);
    err = encryptor->Encryption((TU8 *)licence_check_string, licence_check_string_size, (TU8 *)encrypted_licence_string, (TMemSize)read_size);
    //LOG_NOTICE("licence_check_string_size %ld\n", licence_check_string_size);
    //LOG_NOTICE("!!!after encryp check string, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_check_string_size, licence_check_string[0], licence_check_string[1], licence_check_string[2], licence_check_string[3], licence_check_string[4], licence_check_string[5], licence_check_string[6], licence_check_string[7]);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Encryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    TU8 licence_check_readable_string[1024 + 128] = {0};
    //LOG_NOTICE("!!!gfEncodingBase16, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_check_string_size, licence_check_string[0], licence_check_string[1], licence_check_string[2], licence_check_string[3], licence_check_string[4], licence_check_string[5], licence_check_string[6], licence_check_string[7]);
    gfEncodingBase16((TChar *)licence_check_readable_string, (const TU8 *)licence_check_string, (TInt)licence_check_string_size);
    //LOG_NOTICE("!!!gfEncodingBase16, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_check_string_size, licence_check_readable_string[0], licence_check_readable_string[1], licence_check_readable_string[2], licence_check_readable_string[3], licence_check_readable_string[4], licence_check_readable_string[5], licence_check_readable_string[6], licence_check_readable_string[7]);
    licence_check_string_size = licence_check_string_size * 2;

    if (((TU32)licence_check_string_size) != verified_string_size) {
        LOG_ERROR("licence_check_string_size %ld, not equal to verified_string_size %d\n", licence_check_string_size, verified_string_size);
        return EECode_BadParam;
    }

    if (strcmp((TChar *)licence_check_readable_string, verified_string)) {
        LOG_ERROR("licence_check_readable_string {%s}, not equal with verified_string {%s}\n", licence_check_readable_string, verified_string);
        return EECode_BadParam;
    }

    msLicenceState = ELicenceClientState_LicenceMatchWithLibrary;

    TU32 device_id_index = 0;

    TChar *p_tmp = strchr((TChar *)decrypted_licence_string, '[');
    if (p_tmp) {
        TUint ret = sscanf(p_tmp, "[[devindex=%d", &device_id_index);
        if (1 == ret) {
            TChar device_filename[64] = {0};
            if (0 == device_id_index) {
                strcpy(device_filename, "/etc/deviceid.txt");
            } else {
                snprintf(device_filename, 64, "/etc/deviceid_%d.txt", device_id_index);
            }

            mpDeviceFile = fopen(device_filename, "rx");
            if (!mpDeviceFile) {
                LOG_ERROR("device file open fail {%s}\n", device_filename);
                return EECode_BadParam;
            }
        } else {
            LOG_ERROR("can not find '[[devindex=%%d' in {%s}\n", decrypted_licence_string);
            return EECode_BadParam;
        }
    } else {
        LOG_ERROR("can not fine '[' in {%s}\n", decrypted_licence_string);
        return EECode_BadParam;
    }

    msLicenceState = ELicenceClientState_IDFileOpend;

    TU8 device_encrypted_string[256 + 32] = {0};
    TMemSize device_encryption_size = 256 + 32;
    TChar device_readable_string[512 + 64] = {0};
    TU8 device_decrypted_string[256] = {0};
    TMemSize device_decryption_size = 256;

    device_encryption_size = fread(device_readable_string, 2, device_encryption_size, mpDeviceFile);
    gfDecodingBase16(device_encrypted_string, (const TU8 *)device_readable_string, (TInt)device_encryption_size * 2);

    err = encryptor->Decryption((TU8 *)device_encrypted_string, device_encryption_size, (TU8 *)device_decrypted_string, device_decryption_size);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Decryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    msLicenceState = ELicenceClientState_IDFileVerified;

    TChar *p_tmp2 = NULL;
    p_tmp2 = strchr(p_tmp, ',');
    if (DUnlikely(!p_tmp2)) {
        LOG_ERROR("not valid device string\n");
        return EECode_BadParam;
    }

    p_tmp2 += strlen(" hwaddr=") + 1;

    SNetworkDevices devices;
    err = gfGetNetworkDeviceInfo(&devices);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetNetworkDeviceInfo return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    if (strncmp(devices.device[0].mac_address, (const TChar *)p_tmp2, 11)) {
        LOG_ERROR("device string(%s) and device(%s) not match\n", p_tmp2, devices.device[0].mac_address);
        return EECode_BadParam;
    }

    msLicenceState = ELicenceClientState_IDFileDeviceMatched;

    p_tmp += 2;
    TInt comp = memcmp(p_tmp, device_decrypted_string, device_decryption_size);
    if (0 != comp) {
        LOG_ERROR("device file and licence file not match, device_decrypted_string %s, p_tmp %s\n", device_decrypted_string, p_tmp);
        return EECode_BadParam;
    }

    msLicenceState = ELicenceClientState_IDFileLicenceMatched;

    TU32 year, month, day, rettt = 0;
    p_tmp = _find_string_s((TChar *)decrypted_licence_string, (TInt)licence_string_length, " expire=", strlen(" expire="));
    if (!p_tmp) {
        LOG_ERROR("do not find expire string\n");
        return EECode_BadParam;
    }
    rettt = sscanf(p_tmp, " expire=%04d-%02d-%02d", &year, &month, &day);
    if (3 != rettt) {
        LOG_ERROR("do not find date string, %s\n", p_tmp);
        return EECode_BadParam;
    }

    p_tmp = _find_string_s((TChar *)decrypted_licence_string, (TInt)licence_string_length, " maxchannel=", strlen(" maxchannel="));
    if (!p_tmp) {
        LOG_ERROR("do not find maxchannel string\n");
        return EECode_BadParam;
    }
    rettt = sscanf(p_tmp, " maxchannel=%08x", &mCapabilityMaxChannel);
    if (1 != rettt) {
        LOG_ERROR("do not find max channel string, %s\n", p_tmp);
        return EECode_BadParam;
    }

    mExpireDate.year = year;
    mExpireDate.month = month;
    mExpireDate.day = day;

    SDateTime datetime;
    memset(&datetime, 0x0, sizeof(datetime));
    err = gfGetCurrentDateTime(&datetime);
    if (mExpireDate.year) {
        if (mExpireDate.year < datetime.year) {
            LOG_ERROR("expired year %d, %d\n", mExpireDate.year, datetime.year);
            return EECode_Error;
        } else if (mExpireDate.year == datetime.year) {
            if (mExpireDate.month < datetime.month) {
                LOG_ERROR("expired month %d, %d\n", mExpireDate.month, datetime.month);
                return EECode_Error;
            } else if (mExpireDate.month == datetime.month) {
                if (mExpireDate.day < datetime.day) {
                    LOG_ERROR("expired day, %d-%d-%d, %d-%d-%d\n", mExpireDate.year, mExpireDate.month, mExpireDate.day, datetime.year, datetime.month, datetime.day);
                    return EECode_Error;
                }
            }
        }
    }

    //msLicenceState = ELicenceClientState_LicenceNotExpired;

    msLicenceState = ELicenceClientState_LicenceAuthenticated;

    return EECode_OK;
}


EECode CStandaloneLicence::DailyCheck()
{
    SDateTime datetime;
    memset(&datetime, 0x0, sizeof(datetime));
    gfGetCurrentDateTime(&datetime);
    if (mExpireDate.year) {
        if (mExpireDate.year < datetime.year) {
            LOG_ERROR("expired year %d, %d\n", mExpireDate.year, datetime.year);
            return EECode_Error;
        } else if (mExpireDate.year == datetime.year) {
            if (mExpireDate.month < datetime.month) {
                LOG_ERROR("expired month %d, %d\n", mExpireDate.month, datetime.month);
                return EECode_Error;
            } else if (mExpireDate.month == datetime.month) {
                if (mExpireDate.day < datetime.day) {
                    LOG_ERROR("expired day, %d-%d-%d, %d-%d-%d\n", mExpireDate.year, mExpireDate.month, mExpireDate.day, datetime.year, datetime.month, datetime.day);
                    return EECode_Error;
                }
            }
        }
    }

    return EECode_OK;
}

EECode CStandaloneLicence::GetCapability(TU32 &max_channel_number) const
{
    max_channel_number = mCapabilityMaxChannel;
    return EECode_OK;
}

void CStandaloneLicence::Destroy()
{
    msLicenceState = ELicenceClientState_Invalid;

    if (mpDeviceFile) {
        fclose(mpDeviceFile);
        mpDeviceFile = NULL;
    }

    delete this;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

