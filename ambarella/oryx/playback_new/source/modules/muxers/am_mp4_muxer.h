/*******************************************************************************
 * am_mp4_muxer.h
 *
 * History:
 *    2014/12/03 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef __AM_MP4_MUXER_H__
#define __AM_MP4_MUXER_H__

//-----------------------------------------------------------------------
//
// CMP4Muxer
//
//-----------------------------------------------------------------------
#define DVIDEO_FRAME_COUNT 108000
#define DAUDIO_FRAME_COUNT 169000
#define DVIDEO_COMPRESSORNAME "Ambarella Smart AVC"
#define DAUDIO_COMPRESSORNAME "Ambarella AAC"

class CMP4Muxer: public CObject, public IMuxer
{
  typedef CObject inherited;

public:
  static IMuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual void Destroy();

protected:
  CMP4Muxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  EECode Construct();
  virtual ~CMP4Muxer();

public:
  virtual void Delete();

  virtual EECode SetupContext();
  virtual EECode DestroyContext();

  virtual EECode SetSpecifiedInfo(SRecordSpecifiedInfo *info);
  virtual EECode GetSpecifiedInfo(SRecordSpecifiedInfo *info);

  virtual EECode SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size);
  virtual EECode SetPrivateDataDurationSeconds(void *p_data, TUint data_size);
  virtual EECode SetPrivateDataChannelName(void *p_data, TUint data_size);

  virtual EECode InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type = ContainerType_AUTO, TChar *thumbnailname = NULL, TTime start_pts = 0, TTime start_dts = 0);
  virtual EECode FinalizeFile(SMuxerFileInfo *p_file_info);

  virtual EECode WriteVideoBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);
  virtual EECode WriteAudioBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);
  virtual EECode WritePridataBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);

  virtual EECode SetState(TU8 b_suspend);

  virtual void PrintStatus();

private:
  void defaultMP4FileSetting();

private:
  void fillFileTypeBox(SISOMFileTypeBox *p_box);
  void fillMediaDataBox(SISOMMediaDataBox *p_box);
  void fillMovieHeaderBox(SISOMMovieHeaderBox *p_box);
  void fillMovieVideoTrackHeaderBox(SISOMTrackHeader *p_box);
  void fillMovieAudioTrackHeaderBox(SISOMTrackHeader *p_box);
  void fillMediaHeaderBox(SISOMMediaHeaderBox *p_box);
  void fillHanderReferenceBox(SISOMHandlerReferenceBox *p_box, const TChar *handler, const TChar *name);
  void fillChunkOffsetBox(SISOMChunkOffsetBox *p_box);
  void fillSampleToChunkBox(SISOMSampleToChunkBox *p_box);
  void fillSampleSizeBox(SISOMSampleSizeBox *p_box);
  void fillAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box);
  void fillHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box);
  void fillVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box);
  void fillAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box);
  void fillAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box);
  void fillVideoDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box);
  void fillAudioDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box);
  void fillVideoSampleTableBox(SISOMSampleTableBox *p_box);
  void fillSoundSampleTableBox(SISOMSampleTableBox *p_box);
  void fillDataReferenceBox(SISOMDataReferenceBox *p_box);
  void fillDataInformationBox(SISOMDataInformationBox *p_box);
  void fillVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box);
  void fillSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box);
  void fillVideoMediaInformationBox(SISOMMediaInformationBox *p_box);
  void fillAudioMediaInformationBox(SISOMMediaInformationBox *p_box);
  void fillVideoMediaBox(SISOMMediaBox *p_box);
  void fillAudioMediaBox(SISOMMediaBox *p_box);
  void fillMovieVideoTrackBox(SISOMTrackBox *p_box);
  void fillMovieAudioTrackBox(SISOMTrackBox *p_box);
  void fillMovieBox(SISOMMovieBox *p_box);

private:
  void updateChunkOffsetBoxSize(SISOMChunkOffsetBox *p_box);
  void updateSampleToChunkBoxSize(SISOMSampleToChunkBox *p_box);
  void updateSampleSizeBoxSize(SISOMSampleSizeBox *p_box);
  void updateDecodingTimeToSampleBoxSize(SISOMDecodingTimeToSampleBox *p_box);
  void updateVideoSampleTableBoxSize(SISOMSampleTableBox *p_box);
  void updateSoundSampleTableBoxSize(SISOMSampleTableBox *p_box);
  void updateVideoMediaInformationBoxSize(SISOMMediaInformationBox *p_box);
  void updateAudioMediaInformationBoxSize(SISOMMediaInformationBox *p_box);
  void updateVideoMediaBoxSize(SISOMMediaBox *p_box);
  void updateAudioMediaBoxSize(SISOMMediaBox *p_box);
  void updateMovieVideoTrackBoxSize(SISOMTrackBox *p_box);
  void updateMovieAudioTrackBoxSize(SISOMTrackBox *p_box);
  void updateMovieBoxSize(SISOMMovieBox *p_box);

private:
  void writeBaseBox(SISOMBox *p_box);
  void writeFullBox(SISOMFullBox *p_box);
  void writeFileTypeBox(SISOMFileTypeBox *p_box);
  void writeMediaDataBox(SISOMMediaDataBox *p_box);
  void writeMovieHeaderBox(SISOMMovieHeaderBox *p_box);
  void writeMovieTrackHeaderBox(SISOMTrackHeader *p_box);
  void writeMediaHeaderBox(SISOMMediaHeaderBox *p_box);
  void writeHanderReferenceBox(SISOMHandlerReferenceBox *p_box);
  void writeChunkOffsetBox(SISOMChunkOffsetBox *p_box);
  void writeSampleToChunkBox(SISOMSampleToChunkBox *p_box);
  void writeSampleSizeBox(SISOMSampleSizeBox *p_box);
  void writeAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box);
  void writeHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box);
  void writeVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box);
  void writeAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box);
  void writeAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box);
  void writeDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box);
  void writeVideoSampleTableBox(SISOMSampleTableBox *p_box);
  void writeSoundSampleTableBox(SISOMSampleTableBox *p_box);
  void writeDataReferenceBox(SISOMDataReferenceBox *p_box);
  void writeDataInformationBox(SISOMDataInformationBox *p_box);
  void writeVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box);
  void writeSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box);
  void writeVideoMediaInformationBox(SISOMMediaInformationBox *p_box);
  void writeAudioMediaInformationBox(SISOMMediaInformationBox *p_box);
  void writeVideoMediaBox(SISOMMediaBox *p_box);
  void writeAudioMediaBox(SISOMMediaBox *p_box);
  void writeMovieVideoTrackBox(SISOMTrackBox *p_box);
  void writeMovieAudioTrackBox(SISOMTrackBox *p_box);
  void writeMovieBox(SISOMMovieBox *p_box);

private:
  void insertVideoChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU64 offset);
  void insertAudioChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU64 offset);
  void insertVideoSampleSizeBox(SISOMSampleSizeBox *p_box, TU32 size);
  void insertAudioSampleSizeBox(SISOMSampleSizeBox *p_box, TU32 size);
  EECode writeStrangeSize(TU32 size);
  EECode writeData(TU8 *pdata, TU64 size);
  void writeCachedData();
  void writeVideoFrame(TU64 offset, TU64 size);
  void writeAudioFrame(TU64 offset, TU64 size);

  void writeU8(TU8 value);
  EECode writeBE16(TU16 value);
  EECode writeBE24(TU32 value);
  EECode writeBE32(TU32 value);
  EECode writeBE64(TU64 value);

private:
  void updateMeidaBoxSize();
  void writeHeader();
  void writeTail();
  void beginFile();
  void finalizeFile();

private:
  void processAVCExtradata(TU8 *pdata, TU32 len);
  void processHEVCExtradata(TU8 *pdata, TU32 len);
  void parseADTSHeader(TU8 *pdata);

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  IMsgSink *mpMsgSink;

private:
  SISOMFileTypeBox mFileTypeBox;
  SISOMMediaDataBox mMediaDataBox;
  SISOMMovieBox mMovieBox;

private:
  StreamFormat mVideoFormat;
  StreamFormat mAudioFormat;

private:
  TU64 mOverallVideoDataLen;
  TU64 mOverallAudioDataLen;
  TU64 mOverallMediaDataLen;

  TU64 mVideoDuration;
  TU64 mAudioDuration;

  TU64 mCreationTime;
  TU64 mModificationTime;
  TU64 mTimeScale;
  TU64 mDuration;

  TU32 mVideoFrameTick;
  TU32 mAudioFrameTick;
  TU32 mVideoFrameNumber;
  TU32 mAudioFrameNumber;

  TU32 mRate;
  TU16 mVolume;

  TU32 mMatrix[9];

private:
  TU8 mUsedVersion;
  TU8 mbVideoEnabled;
  TU8 mbAudioEnabled;
  TU8 mbConstFrameRate;

  TU8 mbHaveBframe;
  TU8 mbIOOpend;
  TU8 mbGetVideoExtradata;
  TU8 mbGetAudioExtradata;

  TU8 mVideoTrackID;
  TU8 mAudioTrackID;
  TU8 mNextTrackID;
  TU8 mbGetAudioParameters;

  TU32 mFlags;

private:
  TU32 mVideoWidth;
  TU32 mVideoHeight;
  const TChar *mpVideoCodecName;
  const TChar *mpAudioCodecName;

  TU8 *mpVideoVPS;
  TU32 mVideoVPSLength;

  TU8 *mpVideoSPS;
  TU32 mVideoSPSLength;

  TU8 *mpVideoPPS;
  TU32 mVideoPPSLength;

  TU8 mbVideoKeyFrameComes;
  TU8 mbWriteMediaDataStarted;
  TU8 mbSuspend;
  TU8 mbCorrupted;

private:
  TU16 mAudioChannelNumber;
  TU16 mAudioSampleSize;
  TU32 mAudioSampleRate;
  TU32 mAudioMaxBitrate;
  TU32 mAudioAvgBitrate;

  TU8 *mpAudioExtraData;
  TU32 mAudioExtraDataLength;
  TU32 mAudioExtraDataBufferLength;

private:
  TChar mVideoCompressorName[32];
  TChar mAudioCompressorName[32];

private:
  TU8 *mpCacheBufferBase;
  TU8 *mpCacheBufferBaseAligned;
  TU8 *mpCacheBufferEnd;
  TU8 *mpCurrentPosInCache;
  TU64 mCacheBufferSize;

  IIO *mpIO;
  TU64 mCurFileOffset;
};

#endif


