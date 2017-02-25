/*
 * main.cpp
 *
 *  History:
 *    Apr 10, 2015 - [Shupeng Ren] created file
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
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_export_if.h"
#include <string>
#include "am_bls_rtmp.h"
#include "am_baiducam.h"

#define BLS_STREAM_URL    "rtmp://hz.cam.baidu.com:1935/live/8a3f4e31d45711e4b0c390b11c1db209?deviceid=ambacloudcam&sign=DTAES-CqnyQSm05YocyMV8Skl5IwA2-C%2F4T76uNORpCdigKBofG8Bo3lBU%3D&time=1427462332&expire=1427462352&liveid=142746233262867"
#define BLS_DEVICE_ID     "ambacloudcam"
#define BLS_ACCESS_TOKEN  "23.b916c02994a8ff652096fbe898801bd6.2592000.1430305965.1882242308-5529481"
#define BLS_STREAM_NAME   "8a3f4e31d45711e4b0c390b11c1db209"

using std::string;
static AMBaiduCam *g_baiducam;
static AMIExportClient *g_client;

int main(int argc, char **argv)
{
  int ret = -1;
  if (argc < 2) {
    PRINTF("Usage:    %s stream_id (0|1|2|3)", argv[0]);
    return ret;
  } else if (atoi(argv[1]) < 0 || atoi(argv[1]) > 3) {
    ERROR("Stream id: %d ERROR!", atoi(argv[1]));
    return ret;
  }

  AMExportPacket packet;
  AMExportConfig config = {1};
  std::string url(BLS_STREAM_URL);
  std::string devid(BLS_DEVICE_ID);
  std::string access_token(BLS_ACCESS_TOKEN);
  std::string stream_name(BLS_STREAM_NAME);
  std::string session_token("");

  do {
    if (!(g_baiducam = AMBaiduCam::create(atoi(argv[1])))) {
      break;
    }
    g_baiducam->set_auth_info(url, devid, access_token, stream_name);

    if (!(g_client =
        am_create_export_client(AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET,
                                &config))) {
      break;
    }

    if (!g_client->connect_server(DEXPORT_PATH)) {
      break;
    }

    if (!g_baiducam->connect_bls(session_token)) {
      break;
    }

    while (true) {
      if (!g_client->receive(&packet)) {
        break;
      }

      if (!g_baiducam->process_packet(&packet)) {
        break;
      }

      g_client->release(&packet);
    }
    ret = 0;
  } while (0);

  if (g_client) {
    g_client->destroy();
  }
  if (g_baiducam) {
    g_baiducam->disconnect_bls();
    g_baiducam->destroy();
  }

  return ret;
}
