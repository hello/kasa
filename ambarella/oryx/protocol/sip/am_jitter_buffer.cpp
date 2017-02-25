/*******************************************************************************
 * am_jitter_buffer.cpp
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
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_jitter_buffer.h"

static int jb_queue_next(AM_JITTER_BUF *jb)
{
  int ret = -1;
  if (AM_LIKELY(jb->frames)) {
    ret = jb->frames->ts;
  }
  return ret;
}

static int jb_queue_last(AM_JITTER_BUF *jb)
{
  int ret = -1;
  if (AM_LIKELY(jb->frames)) {
    ret = jb->frames->prev->ts;
  }
  return ret;
}

static void jb_increment_losspct(AM_JITTER_BUF *jb)
{
  jb->info.losspct = (100000 + 499 * jb->info.losspct)/500;
}

static void jb_decrement_losspct(AM_JITTER_BUF *jb)
{
  jb->info.losspct = (499 * jb->info.losspct)/500;
}

AM_JB_STATE AMJitterBuffer::jb_create()
{
  AM_JB_STATE state = AM_JB_OK;
  if (AM_UNLIKELY(nullptr == (m_jitterbuf = new AM_JITTER_BUF()))) {
    ERROR("Create jitter buffer error!");
    state = AM_JB_ERROR;
  } else {
    jb_reset();
  }

  return state;
}

void AMJitterBuffer::jb_destroy()
{
  AM_JB_FRAME *frame = nullptr;

  frame = m_jitterbuf->free;
  while (nullptr != frame) {
    AM_JB_FRAME *next = frame->next;
    delete frame;
    frame = next;
  }
  delete m_jitterbuf;
  m_jitterbuf = nullptr;
}

void AMJitterBuffer::jb_reset()
{
  AM_JB_CONFIG config = m_jitterbuf->info.jb_config;
  AM_JB_FRAME *free_list = m_jitterbuf->free;

  memset(m_jitterbuf, 0, sizeof(AM_JITTER_BUF));
  m_jitterbuf->info.jb_config = config;
  m_jitterbuf->free = free_list;

  m_jitterbuf->info.current = m_jitterbuf->info.target =
      m_jitterbuf->info.jb_config.target_extra = AM_JB_TARGET_EXTRA;
  m_jitterbuf->info.silence_begin_ts = -1;

}

void AMJitterBuffer::jb_setconf(AM_JB_CONFIG *conf)
{
  m_jitterbuf->info.jb_config.max_jitterbuf = conf->max_jitterbuf;
  m_jitterbuf->info.jb_config.resync_threshold = conf->resync_threshold;
  m_jitterbuf->info.jb_config.max_contig_interp = conf->max_contig_interp;

  m_jitterbuf->info.jb_config.target_extra = (conf->target_extra == -1) ?
      AM_JB_TARGET_EXTRA : conf->target_extra;

  m_jitterbuf->info.current = m_jitterbuf->info.jb_config.target_extra;
  m_jitterbuf->info.target = m_jitterbuf->info.jb_config.target_extra;

}

AM_JB_STATE AMJitterBuffer::jb_put(void *data, AM_JB_FRAME_TYPE type,
                                   uint32_t ms, uint32_t ts, uint32_t now)
{
  AM_JB_STATE state = AM_JB_OK;
  do {
    int delay = now - (ts - m_jitterbuf->info.resync_offset);

    if (AM_UNLIKELY((state = jb_check_resync(ts, now, ms, type, &delay))
                    != AM_JB_OK )) {
      break;
    }
    if (AM_LIKELY(AM_JB_TYPE_VOICE == type)) {
      jb_history_put(ts, now, ms, delay);
    }

    m_jitterbuf->info.frames_in ++;
    /* if put into head of queue, caller needs to reschedule */
    if (jb_queue_put(data, type, ms, ts)) {
      state = AM_JB_SCHED;
      break;
    }
  } while(0);

  return state;
}

AM_JB_STATE AMJitterBuffer::jb_get(AM_JB_FRAME *frameout, uint32_t now,
                                   uint32_t interpl)
{
  AM_JB_STATE state = AM_JB_OK;
  do {
    AM_JB_FRAME *frame = nullptr;
    int diff = 0;

    /* get jitter info */
    jb_history_get();

    /* target */
    m_jitterbuf->info.target = m_jitterbuf->info.jitter + m_jitterbuf->info.min
                               + m_jitterbuf->info.jb_config.target_extra;
    /* if a hard clamp was requested, use it */
    if ((m_jitterbuf->info.jb_config.max_jitterbuf) &&
        ((m_jitterbuf->info.target - m_jitterbuf->info.min) >
          m_jitterbuf->info.jb_config.max_jitterbuf)) {
      m_jitterbuf->info.target = m_jitterbuf->info.min +
                                 m_jitterbuf->info.jb_config.max_jitterbuf;
    }

    diff = m_jitterbuf->info.target - m_jitterbuf->info.current;
    if (!m_jitterbuf->info.silence_begin_ts) {
      /* we want to grow */
      if ((diff > 0) &&
        /* we haven't grown in the delay length */
          (((m_jitterbuf->info.last_adjustment + AM_JB_ADJUST_DELAY) < (int)now)
             /* we need to grow more than the "length" we have left */
         || (diff > jb_queue_last(m_jitterbuf) - jb_queue_next(m_jitterbuf)))) {
        /* grow by interp frame length */
        m_jitterbuf->info.current += interpl;
        m_jitterbuf->info.next_voice_ts += interpl;
        m_jitterbuf->info.last_voice_ms = interpl;
        m_jitterbuf->info.last_adjustment = now;
        m_jitterbuf->info.cnt_contig_interp++;
        if (m_jitterbuf->info.jb_config.max_contig_interp &&
            m_jitterbuf->info.cnt_contig_interp >=
            m_jitterbuf->info.jb_config.max_contig_interp) {
          m_jitterbuf->info.silence_begin_ts = m_jitterbuf->info.next_voice_ts
                                               - m_jitterbuf->info.current;
        }
        INFO(" grow by interp frame length");
        state = AM_JB_INTERP;
        break;
      }
      frame = jb_queue_get(m_jitterbuf->info.next_voice_ts -
                           m_jitterbuf->info.current, false);
      /* not a voice frame; just return it. */
      if (AM_UNLIKELY(frame && (frame->type != AM_JB_TYPE_VOICE))) {
        if (frame->type == AM_JB_TYPE_SILENCE) {
          m_jitterbuf->info.silence_begin_ts = frame->ts;
          m_jitterbuf->info.cnt_contig_interp = 0;
        }

        *frameout = *frame;
        m_jitterbuf->info.frames_out++;
        INFO("Not a voice frame");
        state = AM_JB_OK;
        break;
      }
      /* voice frame is later than expected */
      if (AM_UNLIKELY(frame && ((frame->ts + m_jitterbuf->info.current) <
                                m_jitterbuf->info.next_voice_ts))) {
        if (frame->ts + m_jitterbuf->info.current >
            m_jitterbuf->info.next_voice_ts - m_jitterbuf->info.last_voice_ms) {
          /* either we interpolated past this frame in the last jb_get */
          /* or the frame is still in order, but came a little too quick */
          *frameout = *frame;
          /* reset expectation for next frame */
          m_jitterbuf->info.next_voice_ts = frame->ts +
                       m_jitterbuf->info.current + frame->ms;
          m_jitterbuf->info.frames_out++;
          jb_decrement_losspct(m_jitterbuf);
          m_jitterbuf->info.cnt_contig_interp = 0;
          state = AM_JB_OK;
          break;
        } else {
          /* voice frame is late */
          *frameout = *frame;
          m_jitterbuf->info.frames_out++;
          jb_decrement_losspct(m_jitterbuf);
          m_jitterbuf->info.frames_late++;
          m_jitterbuf->info.frames_lost--;
          WARN("Voice frame is late!");
          state = AM_JB_DROP;
          break;
        }
      }
      /* keep track of frame sizes, to allow for variable sized-frames */
      if (frame && frame->ms > 0) {
        m_jitterbuf->info.last_voice_ms = frame->ms;
      }
      /* we want to shrink; shrink at 1 frame / 500ms */
      /* unless we don't have a frame, then shrink 1 frame */
      /* every 80ms (though perhaps we can shrink even faster */
      /* in this case) */
      if (diff < -m_jitterbuf->info.jb_config.target_extra &&
        ((!frame && m_jitterbuf->info.last_adjustment + 80 < (int)now) ||
        (m_jitterbuf->info.last_adjustment + 500 < (int)now))) {

        m_jitterbuf->info.last_adjustment = now;
        m_jitterbuf->info.cnt_contig_interp = 0;

        if (frame) {
          *frameout = *frame;
          /* shrink by frame size we're throwing out */
          m_jitterbuf->info.current -= frame->ms;
          m_jitterbuf->info.frames_out++;
          jb_decrement_losspct(m_jitterbuf);
          m_jitterbuf->info.frames_dropped++;
          INFO("shrink by frame size we're throwing out");
          state = AM_JB_DROP;
          break;
        } else {
          /* shrink by last_voice_ms */
          m_jitterbuf->info.current -= m_jitterbuf->info.last_voice_ms;
          m_jitterbuf->info.frames_lost++;
          jb_increment_losspct(m_jitterbuf);
          INFO("shrink by last_voice_ms");
          state = AM_JB_NOFRAME;
          break;
        }
      }
      /* lost frame */
      if (!frame) {
        m_jitterbuf->info.frames_lost++;
        jb_increment_losspct(m_jitterbuf);
        m_jitterbuf->info.next_voice_ts += interpl;
        m_jitterbuf->info.last_voice_ms = interpl;
        m_jitterbuf->info.cnt_contig_interp++;
        if (m_jitterbuf->info.jb_config.max_contig_interp &&
            m_jitterbuf->info.cnt_contig_interp >=
            m_jitterbuf->info.jb_config.max_contig_interp) {
          m_jitterbuf->info.silence_begin_ts = m_jitterbuf->info.next_voice_ts
                                               - m_jitterbuf->info.current;
        }
        state = AM_JB_INTERP;
        break;
      }
      /* normal case; return the frame, increment stuff */
      *frameout = *frame;
      m_jitterbuf->info.next_voice_ts += frame->ms;
      m_jitterbuf->info.frames_out++;
      m_jitterbuf->info.cnt_contig_interp = 0;
      jb_decrement_losspct(m_jitterbuf);
      state = AM_JB_OK;
      break;
    } else {
      /* shrink interpl len every 10ms during silence */
      if (diff < -m_jitterbuf->info.jb_config.target_extra &&
          m_jitterbuf->info.last_adjustment + 10 <= (int)now) {
        m_jitterbuf->info.current -= interpl;
        m_jitterbuf->info.last_adjustment = now;
      }

      frame = jb_queue_get(now - m_jitterbuf->info.current, false);
      if (AM_UNLIKELY(nullptr == frame)) {
        state = AM_JB_NOFRAME;
        break;
      } else if (AM_UNLIKELY(frame->type != AM_JB_TYPE_VOICE)) {
        /* normal case; in silent mode, got a non-voice frame */
        *frameout = *frame;
        m_jitterbuf->info.frames_out++;
        state = AM_JB_OK;
        break;
      }
      if (AM_UNLIKELY(frame->ts < m_jitterbuf->info.silence_begin_ts)) {
        /* voice frame is late */
        *frameout = *frame;
        m_jitterbuf->info.frames_out++;
        jb_decrement_losspct(m_jitterbuf);
        m_jitterbuf->info.frames_late++;
        m_jitterbuf->info.frames_lost--;
        WARN("Voice frame is late");
        state = AM_JB_DROP;
        break;
      } else {
        /* voice frame */
        /* try setting current to target right away here */
        m_jitterbuf->info.current = m_jitterbuf->info.target;
        m_jitterbuf->info.silence_begin_ts = 0;
        m_jitterbuf->info.next_voice_ts = frame->ts + m_jitterbuf->info.current
                                          + frame->ms;
        m_jitterbuf->info.last_voice_ms = frame->ms;
        m_jitterbuf->info.frames_out++;
        jb_decrement_losspct(m_jitterbuf);
        *frameout = *frame;
        state = AM_JB_OK;
        break;
      }
    }
  } while(0);

  if (AM_JB_INTERP == state) {
    frameout->ms = m_jitterbuf->info.last_voice_ms;
  }

  return state;
}

AM_JB_STATE AMJitterBuffer::jb_getall(AM_JB_FRAME *frameout)
{
  AM_JB_STATE state = AM_JB_OK;
  do {
    AM_JB_FRAME *frame = jb_queue_get(0, true);

    if (AM_UNLIKELY(nullptr == frame)) {
      state = AM_JB_NOFRAME;
      break;
    }
    *frameout = *frame;

  } while(0);

  return state;
}

int AMJitterBuffer::jb_next()
{
  int ret = 0;
  if (m_jitterbuf->info.silence_begin_ts) {
    if (m_jitterbuf->frames) {
      int next = jb_queue_next(m_jitterbuf);
      jb_history_get();
      /* shrink during silence */
      if (m_jitterbuf->info.target - m_jitterbuf->info.current <
          -m_jitterbuf->info.jb_config.target_extra) {
        ret = m_jitterbuf->info.last_adjustment + 10;
      } else {
        ret = next + m_jitterbuf->info.target;
      }
    } else {
      ret = AM_JB_INTMAX;
    }
  } else {
    ret = m_jitterbuf->info.next_voice_ts;
  }

  return ret;
}

bool AMJitterBuffer::jb_ready_to_get()
{
  return (m_jitterbuf != nullptr) ?
         (m_jitterbuf->info.frames_cur > m_frames_remain) : false;
}

AM_JB_FRAME* AMJitterBuffer::jb_get_frame()
{
  return (m_jitterbuf != nullptr) ? m_jitterbuf->frames : nullptr;
}

AMJitterBuffer::AMJitterBuffer(uint16_t frames_remain) :
    m_jitterbuf(nullptr),
    m_frames_remain(frames_remain)
{

}

AM_JB_STATE AMJitterBuffer::jb_check_resync(int ts, int now, int ms,
                                            AM_JB_FRAME_TYPE type, int *delay)
{
  AM_JB_STATE state = AM_JB_OK;
  do {
    int numts = 0;
    int threshold = 2 * m_jitterbuf->info.jitter +
                    m_jitterbuf->info.jb_config.resync_threshold;
    if (AM_LIKELY(m_jitterbuf->frames)) {
      numts = m_jitterbuf->frames->prev->ts - m_jitterbuf->frames->ts;
    }
    /* Check for overfill of the buffer */
    if (numts >= m_jitterbuf->info.jb_config.max_jitterbuf) {
      if (!m_jitterbuf->drop_flag) {
        m_jitterbuf->drop_flag = true;
      }
      m_jitterbuf->info.frames_dropped ++;
      WARN("The jitter buffer was overfilled!");
      state = AM_JB_DROP;
      break;
    } else {
      m_jitterbuf->drop_flag = false;
    }
    /* check for drastic change in delay */
    if (m_jitterbuf->info.jb_config.resync_threshold != -1) {
      if (AM_ABS(*delay - m_jitterbuf->info.last_delay) > threshold) {
        m_jitterbuf->info.cnt_delay_discont ++;
        /* resync the jitterbuffer on 3 consecutive discontinuities */
        if ((m_jitterbuf->info.cnt_delay_discont > 3) ||
            (type == AM_JB_TYPE_CONTROL)) {
          m_jitterbuf->info.cnt_delay_discont = 0;
          m_jitterbuf->hist_ptr = 0;
          m_jitterbuf->hist_maxbuf_valid = 0;
          WARN("Resyncing the jitter buffer.");
          m_jitterbuf->info.resync_offset = ts - now;
          m_jitterbuf->info.last_delay = *delay = 0;
        } else {
          m_jitterbuf->info.frames_dropped ++;
          state = AM_JB_DROP;
          break;
        }
      } else {
        m_jitterbuf->info.last_delay = *delay;
        m_jitterbuf->info.cnt_delay_discont = 0;
      }
    }
  } while(0);

  return state;
}

void AMJitterBuffer::jb_history_put(int ts, int now, int ms, int delay)
{
  int kicked = 0;
  do {
    /* don't add special/negative times to history */
    if (AM_UNLIKELY(ts < 0)) {
      WARN("negative times should not put to history");
      break;
    }
    kicked = m_jitterbuf->history[m_jitterbuf->hist_ptr % AM_JB_HISTORY_SZ];

    m_jitterbuf->history[(m_jitterbuf->hist_ptr ++) % AM_JB_HISTORY_SZ] = delay;

    if (!m_jitterbuf->hist_maxbuf_valid) {
      break;
    }
    if (m_jitterbuf->hist_ptr < AM_JB_HISTORY_SZ) {
      m_jitterbuf->hist_maxbuf_valid = 0;
      break;
    }
    /* if the new delay would go into min */
    if (delay < m_jitterbuf->hist_minbuf[AM_JB_HISTORY_MAXBUF_SZ - 1]) {
      m_jitterbuf->hist_maxbuf_valid = 0;
      break;
    }
    /* or max.. */
    if (delay > m_jitterbuf->hist_maxbuf[AM_JB_HISTORY_MAXBUF_SZ - 1]) {
      m_jitterbuf->hist_maxbuf_valid = 0;
      break;
    }
    /* or the kicked delay would be in min */
    if (kicked <= m_jitterbuf->hist_minbuf[AM_JB_HISTORY_MAXBUF_SZ - 1]) {
      m_jitterbuf->hist_maxbuf_valid = 0;
      break;
    }
    if (kicked >= m_jitterbuf->hist_maxbuf[AM_JB_HISTORY_MAXBUF_SZ - 1]) {
      m_jitterbuf->hist_maxbuf_valid = 0;
      break;
    }

  } while(0);

}

void AMJitterBuffer::jb_history_get()
{
  int max, min, jitter;
  int idx;
  int count;
  do {
    if (!m_jitterbuf->hist_maxbuf_valid) {
      jb_history_calc_maxbuf();
    }

    /* count is how many items in history we're examining */
    count = (m_jitterbuf->hist_ptr < AM_JB_HISTORY_SZ) ?
             m_jitterbuf->hist_ptr : AM_JB_HISTORY_SZ;

    /* idx is the "n"ths highest/lowest that we'll look for */
    idx = count * AM_JB_HISTORY_DROPPCT / 100;

    if (AM_UNLIKELY(idx > (AM_JB_HISTORY_MAXBUF_SZ - 1))) {
      idx = AM_JB_HISTORY_MAXBUF_SZ - 1;
    }

    if (AM_UNLIKELY(idx < 0)) {
      m_jitterbuf->info.min = 0;
      m_jitterbuf->info.jitter = 0;
      break;
    }

    max = m_jitterbuf->hist_maxbuf[idx];
    min = m_jitterbuf->hist_minbuf[idx];

    jitter = max - min;

    m_jitterbuf->info.min = min;
    m_jitterbuf->info.jitter = jitter;

  } while(0);

}

void AMJitterBuffer::jb_history_calc_maxbuf()
{
  do {
    if (AM_UNLIKELY(0 == m_jitterbuf->hist_ptr)) {
      break;
    }
    for (int i = 0; i < AM_JB_HISTORY_MAXBUF_SZ; i++) {
      m_jitterbuf->hist_maxbuf[i] = AM_JB_INTMIN;
      m_jitterbuf->hist_minbuf[i] = AM_JB_INTMAX;
    }

    int index = (m_jitterbuf->hist_ptr > AM_JB_HISTORY_SZ) ?
                (m_jitterbuf->hist_ptr - AM_JB_HISTORY_SZ) : 0;

    for (; index < m_jitterbuf->hist_ptr; index ++) {
      int toins = m_jitterbuf->history[index % AM_JB_HISTORY_SZ];

      /* if the maxbuf should get this */
      if (toins > m_jitterbuf->hist_maxbuf[AM_JB_HISTORY_MAXBUF_SZ - 1])  {
        /* insertion-sort it into the maxbuf */
        for (int i = 0; i < AM_JB_HISTORY_MAXBUF_SZ; i++) {
          /* found where it fits */
          if (toins > m_jitterbuf->hist_maxbuf[i]) {
            /* move over */
            if (i != AM_JB_HISTORY_MAXBUF_SZ - 1) {
              memmove(m_jitterbuf->hist_maxbuf + i + 1,
                      m_jitterbuf->hist_maxbuf + i,
                      (AM_JB_HISTORY_MAXBUF_SZ - (i + 1)) *
                      sizeof(m_jitterbuf->hist_maxbuf[0]));
            }
            /* insert */
            m_jitterbuf->hist_maxbuf[i] = toins;
            break;
          }
        }
      }
      /* if the minbuf should get this */
      if (toins < m_jitterbuf->hist_minbuf[AM_JB_HISTORY_MAXBUF_SZ - 1])  {
        /* insertion-sort it into the maxbuf */
        for (int i = 0; i < AM_JB_HISTORY_MAXBUF_SZ; i++) {
          /* found where it fits */
          if (toins < m_jitterbuf->hist_minbuf[i]) {
            /* move over */
            if (i != AM_JB_HISTORY_MAXBUF_SZ - 1) {
              memmove(m_jitterbuf->hist_minbuf + i + 1,
                      m_jitterbuf->hist_minbuf + i,
                      (AM_JB_HISTORY_MAXBUF_SZ - (i + 1)) *
                      sizeof(m_jitterbuf->hist_minbuf[0]));
            }
            /* insert */
            m_jitterbuf->hist_minbuf[i] = toins;
            break;
          }
        }
      }
    }
    m_jitterbuf->hist_maxbuf_valid = 1;

  } while(0);

}

int AMJitterBuffer::jb_queue_put(void *data, AM_JB_FRAME_TYPE type, int ms,
                                 int ts)
{
  /* returns 1 if frame was inserted into head of queue, 0 otherwise */
  int head = 0;
  AM_JB_FRAME *frame = nullptr;
  AM_JB_FRAME *tmp = nullptr;
  int resync_ts = ts - m_jitterbuf->info.resync_offset;

  do {
    if ((frame = m_jitterbuf->free)) {
      m_jitterbuf->free = frame->next;
    } else if (AM_UNLIKELY(nullptr == (frame = new AM_JB_FRAME()))) {
      ERROR("Failed to create frame");
      break;
    }

    m_jitterbuf->info.frames_cur++;

    frame->data = data;
    frame->ts = resync_ts;
    frame->ms = ms;
    frame->type = type;

    /* frames are a circular list, frames points to to the lowest ts,
     * frames->prev points to the highest ts
     */
    if (AM_UNLIKELY(nullptr == m_jitterbuf->frames)) {  /* queue is empty */
      m_jitterbuf->frames = frame;
      frame->next = frame;
      frame->prev = frame;
      head = 1;
    } else if (resync_ts < m_jitterbuf->frames->ts) {
      frame->next = m_jitterbuf->frames;
      frame->prev = m_jitterbuf->frames->prev;

      frame->next->prev = frame;
      frame->prev->next = frame;

      /* frame is out of order */
      m_jitterbuf->info.frames_ooo++;

      m_jitterbuf->frames = frame;
      head = 1;
    } else {
      tmp = m_jitterbuf->frames;

      /* frame is out of order */
      if (resync_ts < tmp->prev->ts) {
        m_jitterbuf->info.frames_ooo ++;
      }

      while ((resync_ts < tmp->prev->ts) &&
             (tmp->prev != m_jitterbuf->frames)) {
        tmp = tmp->prev;
      }

      frame->next = tmp;
      frame->prev = tmp->prev;

      frame->next->prev = frame;
      frame->prev->next = frame;
    }
  } while(0);

  return head;
}

AM_JB_FRAME* AMJitterBuffer::jb_queue_get(int ts, bool all)
{
  AM_JB_FRAME *frame = nullptr;
  frame = m_jitterbuf->frames;
  do {
    if (AM_UNLIKELY(nullptr == frame)) {
      NOTICE("frames list is empty!");
      break;
    }
    if (AM_LIKELY(all || (ts >= frame->ts))) {
      /* remove this frame */
      frame->prev->next = frame->next;
      frame->next->prev = frame->prev;

      if (frame->next == frame) {
        m_jitterbuf->frames = nullptr;
      } else {
        m_jitterbuf->frames = frame->next;
      }

      /* insert onto "free" single-linked list */
      frame->next = m_jitterbuf->free;
      m_jitterbuf->free = frame;

      m_jitterbuf->info.frames_cur--;

      /* we return the frame pointer, even though it's on free list,
       * but caller must copy data */
    } else {
      frame = nullptr;
    }

  } while(0);

  return frame;
}
