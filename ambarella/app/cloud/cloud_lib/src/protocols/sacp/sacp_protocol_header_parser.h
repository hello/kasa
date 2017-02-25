/*******************************************************************************
 * sacp_protocol_header_parser.h
 *
 * History:
 *  2014/07/02 - [Zhi He] create file
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


#ifndef __SACP_PROTOCOL_HEADER_PARSER_H__
#define __SACP_PROTOCOL_HEADER_PARSER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CSACPProtocolHeaderParser: public IProtocolHeaderParser
{
public:
    static IProtocolHeaderParser *Create(EProtocolType type, EProtocolHeaderExtentionType extensiontype = EProtocolHeaderExtentionType_SACP_IM);
    virtual void Destroy();

protected:
    CSACPProtocolHeaderParser(EProtocolType type, EProtocolHeaderExtentionType extensiontype);
    virtual ~CSACPProtocolHeaderParser();

public:
    virtual TU32 GetFixedHeaderLength() const;
    virtual void Parse(TU8 *header, TU32 &payload_size, TU32 &ext_type, TU32 &ext_size, TU32 &type, TU32 &cat, TU32 &sub_type) const;
    virtual void Parse(TU8 *header, TU32 &payload_size, TU32 &reqres_bits, TU32 &cat, TU32 &sub_type) const;
    virtual void ParseWithTargetID(TU8 *header, TU32 &total_size, TUniqueID &target_id) const;
    virtual void ParseTargetID(TU8 *header, TUniqueID &target_id) const;
    virtual void FillSourceID(TU8 *header, TUniqueID id);
    virtual void SetFlag(TU8 *header, TULong flag);
    virtual void SetReqResBits(TU8 *header, TU32 bits);
    virtual void SetPayloadSize(TU8 *header, TU32 size);

private:
    EProtocolType mType;
    EProtocolHeaderExtentionType mExtensionType;
    TU32 mnHeaderLength;
    TU32 mnExtensionLength;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif