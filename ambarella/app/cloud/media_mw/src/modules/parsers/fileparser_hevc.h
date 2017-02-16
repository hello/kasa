/**
 * fileparser_hevc.h
 *
 * History:
 *  2014/12/12 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __FILEPARSER_HEVC_H__
#define __FILEPARSER_HEVC_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CHEVCFileParser: public IMediaFileParser
{
public:
    static CHEVCFileParser *Create();
    virtual void Delete();

protected:
    CHEVCFileParser();
    ~CHEVCFileParser();
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

    TU8 mbRemoveBuggyFileBytes;
    TU8 mRemoveBuggyFileByteNumber;
    TU8 mReserved0;
    TU8 mReserved1;

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

