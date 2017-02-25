/*******************************************************************************
 * am_jitter_buffer.h
 *
 * History:
 *   Oct 21, 2016 - [smdong] created file
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

#ifndef ORYX_PROTOCOL_SIP_AM_JITTER_BUFFER_H_
#define ORYX_PROTOCOL_SIP_AM_JITTER_BUFFER_H_

#define AM_JB_INTMAX 2147483647L
#define AM_JB_INTMIN (-AM_JB_INTMAX - 1L)
#define AM_JB_TARGET_EXTRA 40
#define AM_JB_ADJUST_DELAY 40
#define AM_JB_HISTORY_SZ   500
#define AM_JB_HISTORY_DROPPCT  3
#define AM_JB_HISTORY_DROPPCT_MAX  4
#define AM_JB_HISTORY_MAXBUF_SZ  \
        (AM_JB_HISTORY_SZ * AM_JB_HISTORY_DROPPCT_MAX / 100)

enum AM_JB_STATE
{
  AM_JB_OK,
  AM_JB_ERROR,
  AM_JB_EMPTY,
  AM_JB_NOFRAME,
  AM_JB_INTERP,
  AM_JB_DROP,
  AM_JB_SCHED
};

enum AM_JB_FRAME_TYPE
{
  AM_JB_TYPE_CONTROL,
  AM_JB_TYPE_VOICE,
  AM_JB_TYPE_VIDEO,
  AM_JB_TYPE_SILENCE
};

struct AM_JB_CONFIG
{
  int max_jitterbuf;
  int resync_threshold;
  int max_contig_interp;
  int target_extra;
};

struct AM_JB_INFO
{
  int frames_in;
  int frames_out;
  int frames_late;
  int frames_lost;
  int frames_dropped;
  int frames_ooo;
  int frames_cur;
  int jitter;
  int min;
  int current;
  int target;
  int losspct;
  int next_voice_ts;
  int last_voice_ms;
  int silence_begin_ts;
  int last_adjustment;
  int last_delay;
  int cnt_delay_discont;
  int resync_offset;
  int cnt_contig_interp;
  AM_JB_CONFIG jb_config;
};

struct AM_JB_FRAME
{
  void *data;
  int ts;
  int ms;
  AM_JB_FRAME *prev;
  AM_JB_FRAME *next;
  AM_JB_FRAME_TYPE type;
};

struct AM_JITTER_BUF
{
  AM_JB_FRAME *frames;
  AM_JB_FRAME *free;
  AM_JB_INFO info;

  int history[AM_JB_HISTORY_SZ];
  int hist_ptr;
  int hist_maxbuf[AM_JB_HISTORY_MAXBUF_SZ];
  int hist_minbuf[AM_JB_HISTORY_MAXBUF_SZ];
  int  hist_maxbuf_valid;
  bool drop_flag;
};

class AMJitterBuffer
{
  public:
    AM_JB_STATE jb_create();
    void jb_destroy();
    void jb_reset();
    void jb_setconf(AM_JB_CONFIG *conf);
    AM_JB_STATE jb_put(void *data, AM_JB_FRAME_TYPE type,
                       uint32_t ms, uint32_t ts, uint32_t now);
    AM_JB_STATE jb_get(AM_JB_FRAME *frameout, uint32_t now, uint32_t interpl);
    AM_JB_STATE jb_getall(AM_JB_FRAME *frameout);
    int jb_next();
    bool jb_ready_to_get();
    AM_JB_FRAME* jb_get_frame();

    AMJitterBuffer(uint16_t frames_remain);
    ~AMJitterBuffer() {};

  private:
    AM_JB_STATE jb_check_resync(int ts, int now, int ms, AM_JB_FRAME_TYPE type,
                                int *delay);
    void jb_history_put(int ts, int now, int ms, int delay);
    void jb_history_get();
    void jb_history_calc_maxbuf();
    int jb_queue_put(void *data, AM_JB_FRAME_TYPE type, int ms, int ts);
    AM_JB_FRAME* jb_queue_get(int ts, bool all);

  private:
    AM_JITTER_BUF *m_jitterbuf;
    uint16_t       m_frames_remain;

};


#endif /* ORYX_PROTOCOL_SIP_AM_JITTER_BUFFER_H_ */
