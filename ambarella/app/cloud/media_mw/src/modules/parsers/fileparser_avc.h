/**
 * fileparser_avc.h
 *
 * History:
 *  2014/12/08 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __FILEPARSER_AVC_H__
#define __FILEPARSER_AVC_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CAVCFileParser: public IMediaFileParser
{
public:
    static CAVCFileParser *Create();
    virtual void Delete();

protected:
    CAVCFileParser();
    ~CAVCFileParser();
    EECode Construct();

public:
    virtual EECode OpenFile(TChar *name);
    virtual EECode CloseFile();

    virtual EECode SeekTo(TIOSize offset);
    virtual EECode ReadPacket(SMediaPacket *packet);
    virtual EECode GetMediaInfo(SMediaInfo *info);

private:
    EECode getMediaInfo();
    EECode readDataFromFile();

private:
    IIO *mpIO;
    TU8 *mpReadBuffer;
    TIOSize mReadBufferSize;
    TIOSize mReadBlockSize;
    TIOSize mReadNextThreashold;

    TU8 *mpCurrentPtr;
    TIOSize mBufRemaingSize;

    TIOSize mFileTotalSize;
    TIOSize mFileCurrentOffset;

private:
    TU8 mbFileOpend;
    TU8 mbFileEOF;
    TU8 mbParseMediaInfo;
    TU8 mbStartReadFile;

private:
    TU32 mProfileIndicator;
    TU32 mLevelIndicator;

    TU32 mVideoWidth;
    TU32 mVideoHeight;

    TU32 mVideoFramerateNum;
    TU32 mVideoFramerateDen;
};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

