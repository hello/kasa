/*******************************************************************************
 * licence_lib_if.h
 *
 * History:
 *  2014/03/06 - [Zhi He] create file
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

#ifndef __LICENCE_LIB_IF_H__
#define __LICENCE_LIB_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef enum {
    ELicenceType_Invalid = 0,
    ELicenceType_Remote = 0x01,
    ELicenceType_StandAlone = 0x02,
} ELicenceType;

typedef enum {
    ELicenceEncryptionType_Invalid = 0,
    ELicenceEncryptionType_CustomizedV1 = 0x01,
} ELicenceEncryptionType;

typedef enum {
    EDeviceIDType_Invalid = 0,
    EDeviceIDType_LicenceClient = 0x01,
} EDeviceIDType;

typedef enum {
    ELicenceClientState_Invalid = 0,

    ELicenceClientState_LicenceFileOpend = 0x01,
    ELicenceClientState_LicenceFileVerified = 0x02,
    ELicenceClientState_LicenceMatchWithLibrary = 0x03,
    ELicenceClientState_IDFileOpend = 0x04,
    ELicenceClientState_IDFileVerified = 0x05,
    ELicenceClientState_IDFileDeviceMatched = 0x06,
    ELicenceClientState_IDFileLicenceMatched = 0x07,
    ELicenceClientState_LicenceAuthenticated = 0x08,

    ELicenceClientState_LicenceFileOpenFail = 0x041,
    ELicenceClientState_LicenceFileVerifiedFail = 0x042,
    ELicenceClientState_LicenceFileNotMatchWithLibrary = 0x043,
    ELicenceClientState_IDFileOpenFail = 0x044,
    ELicenceClientState_IDFileVerifiedFail = 0x045,
    ELicenceClientState_IDFileNotMatchWithDevice = 0x046,
    ELicenceClientState_IDFileLicenceNotMatched = 0x47,
    ELicenceClientState_LicenceTimeExpired = 0x48,
} ELicenceClientState;

class ILicenceClient
{
public:
    virtual EECode Authenticate(TChar *native_file, const TChar *verified_string, TU32 verified_string_size, ELicenceEncryptionType type = ELicenceEncryptionType_CustomizedV1, TChar *server_url = NULL, TU32 port = 0) = 0;
    virtual EECode DailyCheck() = 0;
    virtual EECode GetCapability(TU32 &max_channel_number) const = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ILicenceClient() {}
};


#define DMAX_DEVICE_ID_INFO_LENGTH 256

typedef struct {
    TChar company_name[DMAX_DEVICE_ID_INFO_LENGTH];
    TChar project_name[DMAX_DEVICE_ID_INFO_LENGTH];
    TChar server_name[DMAX_DEVICE_ID_INFO_LENGTH];
    TU32 channel_index;

    TU8 device_tag[8];
    TU8 time_tag[16];
} SDeviceIDInformation;

class IDeviceIDGenerator
{
public:
    virtual EECode EncodeID(SDeviceIDInformation *info, TU8 *&p_out_buffer, TMemSize &out_size) = 0;
    virtual EECode DecodeID(SDeviceIDInformation *out_info, TU8 *p_in_buffer, TMemSize in_size) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IDeviceIDGenerator() {}
};

#define DFLAGS_LOOPBACK 0x00000001

typedef struct {
    TU32 flags;

    TChar mac_address[32];
    TChar ip_address[32];
    TChar ipv6_address[32];
    TChar name[32];
} SNetworkDevice;

typedef struct {
    TU8 number;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    SNetworkDevice device[4];
} SNetworkDevices;

ILicenceClient *gfCreateLicenceClient(ELicenceType type);
IDeviceIDGenerator *gfDeviceIDGenerator(EDeviceIDType type);

EECode gfGetNetworkDeviceInfo(SNetworkDevices *devices);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

