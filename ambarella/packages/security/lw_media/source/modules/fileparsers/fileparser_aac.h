/**
 * fileparser_aac.h
 *
 * History:
 *  2014/12/10 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __FILEPARSER_AAC_H__
#define __FILEPARSER_AAC_H__

class CAACFileParser: public IMediaFileParser
{
public:
    static CAACFileParser *Create();
    virtual void Delete();

protected:
    CAACFileParser();
    ~CAACFileParser();
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
    TU8 mAudioChannelNumber;
    TU8 mAudioSampleSize;
    TU8 mAudioProfile;
    TU8 mReserved1;

    TU32 mAudioSampleRate;
    TU32 mAudioFrameSize;

private:
    SADTSHeader mADTSHeader;
};

#endif

