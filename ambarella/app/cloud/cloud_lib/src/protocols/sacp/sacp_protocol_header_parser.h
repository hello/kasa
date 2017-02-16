/**
 * sacp_protocol_header_parser.h
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