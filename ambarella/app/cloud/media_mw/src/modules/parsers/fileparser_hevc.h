/*******************************************************************************
 * fileparser_hevc.h
 *
 * History:
 *  2014/12/12 - [Zhi He] create file
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

