/*******************************************************************************
 * test_event_plugin.c
 *
 * Histroy:
 *  2014-11-19  [Dongge Wu] created file
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

#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_file.h"
#include "am_event_types.h"
#include "am_jpeg_encoder_if.h"
#include "am_event_monitor_if.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"

static bool g_run = true;

static int maxfd = -1;
fd_set allset;
fd_set fdset;

static int ctrl[2];
#define CTRL_R ctrl[0]
#define CTRL_W ctrl[1]

#define JPEG_PATH               ((const char *)"/tmp/md_jpeg")
#define MAX_FILES_NUM           (10)
#define MAX_FILENAME_LEN        (256)

typedef struct file_table {
    int id;
    int used;
    char name[MAX_FILENAME_LEN];
} file_table_t;

static struct file_table *g_file_table = NULL;
static int g_file_index = 0;
static pthread_cond_t g_cond;
static pthread_mutex_t g_mutex;
static int g_md_status = 0;
static float g_fps = 15.0;
static bool g_thread_run = true;


AMIEventMonitorPtr event_instance;
AMIVideoReaderPtr m_video_reader;
AMIVideoAddressPtr m_video_address;
AMIJpegEncoderPtr m_jpeg_encoder;

const char *event_type_str[EV_ALL_MODULE_NUM] =
{"Audio Alert", "Audio Analysis", "Face Detect", "Key Input", };
static int32_t show_plugin_menu(void)
{
  PRINTF("\n===============================\n");
  for (int i = 0; i < EV_ALL_MODULE_NUM; i ++) {
    PRINTF("    %d -- %s\n", i, event_type_str[i]);
  }
  PRINTF("    q -- Quit");
  PRINTF("\n===============================\n\n");
  PRINTF("> ");
  return 0;
}

static int32_t show_menu(void)
{
  PRINTF("\n===============================\n");
  PRINTF("    r -- Run event plugin\n");
  PRINTF("    s -- Stop event plugin\n");
  PRINTF("    d -- Destroy event plugin\n");
  PRINTF("    q -- Quit");
  PRINTF("\n===============================\n\n");
  PRINTF("> ");
  return 0;
}

static bool init_dir()
{
  bool res = true;
  struct file_table *ft = NULL;
  do {
    if (false == AMFile::create_path(JPEG_PATH)) {
      ERROR("AMFile::create_path failed!\n");
      res = false;
      break;
    }
    g_file_table = (struct file_table *)calloc(1, MAX_FILES_NUM * sizeof(struct file_table));
    if (!g_file_table) {
      ERROR("malloc file_table failed!\n");
      res = false;
      break;
    }
    for (int i = 0; i < MAX_FILES_NUM; ++i) {
      ft = g_file_table + i;
      ft->id = i;
    }
  } while (0);
  return res;
}

static void deinit_dir()
{
  if (g_file_table) {
    free(g_file_table);
  }
}

static int get_file_name(char *str, int len)
{
  char date_fmt[20];
  char date_ms[4];
  struct timeval tv;
  struct tm now_tm;
  int now_ms;
  time_t now_sec;
  int res = AM_RESULT_OK;
  do {
    if (str == NULL || len <= 0) {
      ERROR("invalid paraments!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    gettimeofday(&tv, NULL);
    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec/1000;
    localtime_r(&now_sec, &now_tm);
    strftime(date_fmt, 20, "%Y%m%d_%H%M%S", &now_tm);
    snprintf(date_ms, sizeof(date_ms), "%03d", now_ms);
    snprintf(str, len, "%s/%s_%s.jpeg", JPEG_PATH, date_fmt, date_ms);
  } while (0);
  return res;
}

static int trim_jpeg_files()
{
  struct file_table *ft = NULL;
  int res = AM_RESULT_OK;
  do {
    ++g_file_index;
    if (g_file_index >= MAX_FILES_NUM) {
      g_file_index = 0;
    }
    ft = g_file_table + g_file_index;
    if (ft->used) {
      if (-1 == remove(ft->name)) {
        ERROR("remove %s failed: %d\n", ft->name, errno);
        res = AM_RESULT_ERR_INVALID;
        break;
      }
    }
    ft->used = 1;
    get_file_name(ft->name, MAX_FILENAME_LEN);
  } while (0);
  return res;
}

static int save_jpeg_in_file(char *filename, void *data, size_t size)
{
  int res = AM_RESULT_OK;
  AMFile f(filename);
  do {
    if (!f.open(AMFile::AM_FILE_CREATE)) {
      PERROR("Failed to open file");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (f.write_reliable(data, size) < 0) {
      PERROR("Failed to sava data into file\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);
  f.close();
  return res;
}

static void free_yuv_buffer(AMYUVData *yuv)
{
  if (yuv) {
    free(yuv->y.iov_base);
    free(yuv->uv.iov_base);
    free(yuv);
  }
}

static AMYUVData *capture_yuv_buffer(int buf_id)
{
  AMQueryFrameDesc frame_desc;
  AMYUVData *yuv = NULL;
  AMAddress yaddr = {0};
  AMAddress uvaddr = {0};
  AM_RESULT result = AM_RESULT_OK;
  uint8_t *y_tmp;
  int32_t uv_height;
  int32_t i;

  do {
    yuv = (AMYUVData *)calloc(1, sizeof(AMYUVData));
    if (!yuv) {
      ERROR("malloc yuv failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_video_reader->query_yuv_frame(frame_desc,
                                AM_SOURCE_BUFFER_ID(buf_id), false);
    if (result != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (AM_RESULT_OK != m_video_address->yuv_y_addr_get(frame_desc, yaddr)) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Failed to get y address!");
      break;
    }
    if (AM_RESULT_OK != m_video_address->yuv_uv_addr_get(frame_desc, uvaddr)) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Failed to get uv address!");
      break;
    }

    yuv->width = frame_desc.yuv.width;
    yuv->height = frame_desc.yuv.height;
    yuv->pitch = frame_desc.yuv.pitch;
    yuv->format = frame_desc.yuv.format;

    if (frame_desc.yuv.format == AM_CHROMA_FORMAT_YUV420) {
      uv_height = yuv->height / 2;
    } else if (frame_desc.yuv.format == AM_CHROMA_FORMAT_YUV422) {
      uv_height = yuv->height;
    } else {
      ERROR("not supported chroma format in YUV dump\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //copy y buffer
    yuv->y.iov_len = yuv->width * yuv->height;
    yuv->y.iov_base = (uint8_t *)calloc(1, yuv->y.iov_len);
    if (!yuv->y.iov_base) {
      ERROR("malloc y buffer failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    y_tmp = (uint8_t *)yuv->y.iov_base;
    if (yuv->pitch == yuv->width) {
      memcpy(yuv->y.iov_base, yaddr.data, yuv->width * yuv->height);
    } else {
      for (i = 0; i < yuv->height ; i++) {
        memcpy(y_tmp, uvaddr.data + i * yuv->pitch, yuv->width);
        y_tmp += yuv->width;
      }
    }

    //copy uv buffer
    yuv->uv.iov_len = yuv->pitch * uv_height;//XXX: pitch
    yuv->uv.iov_base = (uint8_t *)calloc(1, yuv->uv.iov_len);
    if (!yuv->uv.iov_base) {
      ERROR("malloc uv buffer failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memcpy(yuv->uv.iov_base, uvaddr.data, yuv->uv.iov_len);
  } while (0);

  if (result != AM_RESULT_OK) {
    free_yuv_buffer(yuv);
    yuv = NULL;
  }

  return yuv;
}

static int encode_yuv_to_jpeg()
{
  AM_RESULT res = AM_RESULT_OK;
  char filename_jpeg[MAX_FILENAME_LEN];
  struct file_table *ft = NULL;
  AMYUVData *yuv = NULL;
  AMJpegData *jpeg = NULL;

  do {
    yuv = capture_yuv_buffer(0);
    if (!yuv) {
      ERROR("capture_yuv_buffer failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }

    jpeg = m_jpeg_encoder->new_jpeg_data(yuv->width, yuv->height);
    if (!jpeg) {
      ERROR("new_jpeg_buf failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (0 != m_jpeg_encoder->encode_yuv(yuv, jpeg)) {
      ERROR("jpeg encode failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (AM_RESULT_OK != trim_jpeg_files()) {
      ERROR("trim_jpeg_files failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (AM_RESULT_OK != get_file_name(filename_jpeg, MAX_FILENAME_LEN)) {
      ERROR("get_file_name failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    ft = g_file_table + g_file_index;
    save_jpeg_in_file(ft->name, (uint8_t *)jpeg->data.iov_base, jpeg->data.iov_len);
    INFO("Save JPEG file [%s] OK.\n", filename_jpeg);

    m_jpeg_encoder->free_jpeg_data(jpeg);
  } while (0);

  free_yuv_buffer(yuv);

  return res;
}

static void stop_encode_jpeg_thread()
{
  g_thread_run = false;
  pthread_mutex_lock(&g_mutex);
  pthread_cond_signal(&g_cond);
  pthread_mutex_unlock(&g_mutex);

}

static void *encode_jpeg_thread(void *arg)
{
  int ms_encode = 0;
  int ms_sleep = 0;
  struct timeval delay;
  struct timeval pre = {0, 0}, curr = {0, 0};

  while (g_thread_run) {
    if (g_md_status == 0) {
      pthread_mutex_lock(&g_mutex);
      pthread_cond_wait(&g_cond, &g_mutex);
      pthread_mutex_unlock(&g_mutex);
      continue;
    } else {
      gettimeofday(&curr, NULL);
      pre = curr;
      encode_yuv_to_jpeg();
      gettimeofday(&curr, NULL);
      ms_encode = ((1000000 * (curr.tv_sec - pre.tv_sec)) + (curr.tv_usec - pre.tv_usec)) / 1000;
      PRINTF("encode_yuv_to_jpeg take [%06d ms]", ms_encode);
      ms_sleep = (1.0/g_fps)*1000 - ms_encode;
      ms_sleep = (ms_sleep < 0) ? 0 : ms_sleep;
      delay.tv_sec = 0;
      delay.tv_usec = ms_sleep * 1000;
      select(0, NULL, NULL, NULL, &delay);
    }
  }
  return NULL;
}

static int32_t event_callback(AM_EVENT_MESSAGE *msg)
{
  PRINTF("event type: %s, pts: %llu, sequence number: %llu\n",
         event_type_str[msg->event_type],
         msg->pts,
         msg->seq_num);

  switch (msg->event_type) {
    case EV_AUDIO_ALERT_DECT:
      break;
    case EV_FACE_DECT:
      break;
    case EV_KEY_INPUT_DECT:
      PRINTF("key value: %d, event type: %s\n",
             msg->key_event.key_value,
             msg->key_event.key_state == AM_KEY_CLICKED ? "Short pressed" :
                                                          "Long pressed");
      break;
    default:
      ERROR("Unknown event type!\n");
      break;
  }

  return 0;
}

static void sigstop(int signo)
{
  write(CTRL_W, "e", 1);
  g_run = false;
  stop_encode_jpeg_thread();
}

static char get_cmd()
{
  char ch = '\n';
  fdset = allset;

  if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
    if (AM_LIKELY(FD_ISSET(STDIN_FILENO, &fdset))) {
      if (AM_LIKELY(read(STDIN_FILENO, &ch, 1)) < 0) {
        ERROR("Failed to read command from user input, quit this program!");
        ch = 'q';
      }
    } else if (AM_LIKELY(FD_ISSET(CTRL_R, &fdset))) {
      char cmd[1] =
      { 0 };
      if (AM_LIKELY(read(CTRL_R, cmd, sizeof(cmd)) < 0)) {
        ERROR("Failed to read command, quit this program!");
        ch = 'q';
      } else if (AM_LIKELY(cmd[0] == 'e')) {
        ch = 'q';
      }
    }
  } else {
    if (AM_LIKELY(errno != EINTR)) {
      PERROR("select");
      ch = 'q';
    }
  }

  return ch;
}

int main()
{
  int32_t ch;
  int32_t value = 1;
  int32_t error_opt = 0;
  EVENT_MODULE_CONFIG module_config;
  EVENT_MODULE_ID module_id;

  if (AM_UNLIKELY(pipe(ctrl) < 0)) {
    PERROR("pipe");
    return -1;
  }

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  FD_ZERO(&allset);
  FD_ZERO(&fdset);
  FD_SET(STDIN_FILENO, &allset);
  FD_SET(CTRL_R, &allset);
  maxfd = (STDIN_FILENO > CTRL_R) ? STDIN_FILENO : CTRL_R;

  memset(&module_config, 0, sizeof(EVENT_MODULE_CONFIG));
  event_instance = AMIEventMonitor::get_instance();
  if (!event_instance) {
    ERROR("event_instance is NULL!");
    return -1;
  }

  m_video_reader = AMIVideoReader::get_instance();
  if (!m_video_reader) {
    ERROR("m_video_reader is NULL!\n");
    return -1;
  }

  m_video_address = AMIVideoAddress::get_instance();
  if (!m_video_address) {
    ERROR("Failed to get instance of VideoAddress!");
    return -1;
  }

  m_jpeg_encoder = AMIJpegEncoder::get_instance();
  if (!m_jpeg_encoder) {
    ERROR("m_jpeg_encoder is NULL!\n");
    return -1;
  }
  if (m_jpeg_encoder->create() != 0) {
    ERROR("m_jpeg_encoder create failed!\n");
    return -1;
  }

  if (false == init_dir()) {
    ERROR("init dir failed!\n");
    return -1;
  }
  pthread_t tid;
  pthread_cond_init(&g_cond, NULL);
  pthread_mutex_init(&g_mutex, NULL);
  pthread_create(&tid, NULL, encode_jpeg_thread, NULL);

  while (g_run) {
    if (error_opt == 0) {
      show_menu();
    }

    error_opt = 0;
    ch = get_cmd();
    switch (ch) {
      case 'r':
        show_plugin_menu();
        while ((ch = get_cmd()) != 'q') {
          if (ch >= '0' && ch < '0' + EV_ALL_MODULE_NUM) {
            module_id = EVENT_MODULE_ID(ch - '0');
            if (event_instance->start_event_monitor(module_id)) {
              switch (module_id) {
                case EV_AUDIO_ALERT_DECT:
                  module_config.key = AM_ALERT_CALLBACK;
                  module_config.value = (void *) event_callback;
                  event_instance->set_monitor_config(module_id, &module_config);
                  module_config.key = AM_ALERT_ENABLE;
                  module_config.value = &value;
                  event_instance->set_monitor_config(module_id, &module_config);
                  break;
                case EV_KEY_INPUT_DECT:
                  AM_KEY_INPUT_CALLBACK key_callback;
                  key_callback.key_value = 116;
                  key_callback.callback = event_callback;
                  module_config.key = AM_KEY_CALLBACK;
                  module_config.value = (void *) &key_callback;
                  event_instance->set_monitor_config(module_id, &module_config);
                  key_callback.key_value = 238;
                  event_instance->set_monitor_config(module_id, &module_config);
                  break;
                default:
                  ERROR("Unknown module ID\n");
                  break;
              }
            }
            break;
          }
        }
        break;
      case 's':
        show_plugin_menu();
        while ((ch = get_cmd()) != 'q') {
          if (ch >= '0' && ch < '0' + EV_ALL_MODULE_NUM) {
            event_instance->stop_event_monitor(EVENT_MODULE_ID(ch - '0'));
            break;
          }
        }
        break;
      case 'd':
        show_plugin_menu();
        while ((ch = get_cmd()) != 'q') {
          if (ch >= '0' && ch < '0' + EV_ALL_MODULE_NUM) {
            event_instance->destroy_event_monitor(EVENT_MODULE_ID(ch - '0'));
            break;
          }
        }
        break;
      case 'q':
        goto done;
        break;
      default:
        error_opt = 1;
        break;
    }
  }

  done: close(CTRL_R);
  close(CTRL_W);
  event_instance->destroy_all_event_monitor();

  pthread_cond_destroy(&g_cond);
  pthread_mutex_destroy(&g_mutex);
  deinit_dir();

  return 0;
}
