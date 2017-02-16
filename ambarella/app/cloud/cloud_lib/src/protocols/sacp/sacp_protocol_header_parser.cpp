/**
 * sacp_protocol_header_parser.cpp
 *
 * History:
 *  2014/07/02 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_network_utils.h"

#include "cloud_lib_if.h"

#include "sacp_types.h"
#include "sacp_protocol_header_parser.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IProtocolHeaderParser *gfCreateSACPProtocolHeaderParser(EProtocolType type, EProtocolHeaderExtentionType extensiontype)
{
    return CSACPProtocolHeaderParser::Create(type, extensiontype);
}

//-----------------------------------------------------------------------
//
// CSACPProtocolHeaderParser
//
//-----------------------------------------------------------------------
IProtocolHeaderParser *CSACPProtocolHeaderParser::Create(EProtocolType type, EProtocolHeaderExtentionType extensiontype)
{
    CSACPProtocolHeaderParser *result = new CSACPProtocolHeaderParser(type, extensiontype);
    return result;
}

CSACPProtocolHeaderParser::CSACPProtocolHeaderParser(EProtocolType type, EProtocolHeaderExtentionType extensiontype)
    : mType(type)
    , mExtensionType(extensiontype)
    , mnHeaderLength(sizeof(SSACPHeader))
    , mnExtensionLength(0)
{
    if (mExtensionType == EProtocolHeaderExtentionType_SACP_IM) {
        mnExtensionLength = sizeof(TUniqueID);
    } else if (mExtensionType == EProtocolHeaderExtentionType_SACP_ADMIN) {
        mnExtensionLength = 0;
    }
}

CSACPProtocolHeaderParser::~CSACPProtocolHeaderParser()
{
}

void CSACPProtocolHeaderParser::Destroy()
{
    delete this;
}

TU32 CSACPProtocolHeaderParser::GetFixedHeaderLength() const
{
    return (mnHeaderLength + mnExtensionLength);
}

void CSACPProtocolHeaderParser::Parse(TU8 *header, TU32 &payload_size, TU32 &ext_type, TU32 &ext_size, TU32 &type, TU32 &cat, TU32 &sub_type) const
{
    SSACPHeader *p_header = (SSACPHeader *)header;

    payload_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | ((TU32)p_header->size_2);
    ext_type = p_header->header_ext_type;
    ext_size = (p_header->header_ext_size_1 << 8) | (p_header->header_ext_size_2);
    type = (p_header->type_1 << 24) | (p_header->type_2 << 16) | (p_header->type_3 << 8) | (p_header->type_4);
    cat = p_header->type_2;
    sub_type = (p_header->type_3 << 8) | (p_header->type_4);

    return;
}

void CSACPProtocolHeaderParser::Parse(TU8 *header, TU32 &payload_size, TU32 &reqres_bits, TU32 &cat, TU32 &sub_type) const
{
    SSACPHeader *p_header = (SSACPHeader *)header;

    payload_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | ((TU32)p_header->size_2);
    reqres_bits = p_header->type_1 << 24;
    cat = p_header->type_2;
    sub_type = (p_header->type_3 << 8) | (p_header->type_4);

    return;
}

void CSACPProtocolHeaderParser::ParseWithTargetID(TU8 *header, TU32 &total_size, TUniqueID &target_id) const
{
    SSACPHeader *p_header = (SSACPHeader *)header;

    TU32 payload_size = ((TU32)p_header->size_0 << 16) | ((TU32)p_header->size_1 << 8) | ((TU32)p_header->size_2);
    total_size = payload_size + mnHeaderLength + mnExtensionLength;

    TU8 *p_ext = &header[mnHeaderLength];
    TU32 shift = mnExtensionLength - 1;
    target_id = 0;
    for (TU32 i = 0; i < mnExtensionLength; i++, shift--) {
        target_id += ((TUniqueID)p_ext[i] << shift * 8);
    }

    return;
}

void CSACPProtocolHeaderParser::ParseTargetID(TU8 *header, TUniqueID &target_id) const
{
    TU8 *p_ext = &header[mnHeaderLength];
    TU32 shift = mnExtensionLength - 1;
    target_id = 0;
    for (TU32 i = 0; i < mnExtensionLength; i++, shift--) {
        target_id += ((TUniqueID)p_ext[i] << shift * 8);
    }

    return;
}

void CSACPProtocolHeaderParser::FillSourceID(TU8 *header, TUniqueID id)
{
    TU8 *p_ext = &header[mnHeaderLength];
    TU32 shift = mnExtensionLength - 1;
    for (TU32 i = 0; i < mnExtensionLength; i++, shift--) {
        p_ext[i] = (id >> shift * 8) & 0xff;
    }

    return;
}

void CSACPProtocolHeaderParser::SetFlag(TU8 *header, TULong flag)
{
    DASSERT(header);

    SSACPHeader *p_header = (SSACPHeader *)header;
    p_header->flags = flag;

    return;
}

void CSACPProtocolHeaderParser::SetReqResBits(TU8 *header, TU32 bits)
{
    DASSERT(header);

    SSACPHeader *p_header = (SSACPHeader *)header;
    p_header->type_1 = (bits >> 24) & 0xff;

    return;
}

void CSACPProtocolHeaderParser::SetPayloadSize(TU8 *header, TU32 size)
{
    DASSERT(header);

    SSACPHeader *p_header = (SSACPHeader *)header;
    p_header->size_0 = (size >> 16) & 0xff;
    p_header->size_1 = (size >> 8) & 0xff;
    p_header->size_2 = size & 0xff;

    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
