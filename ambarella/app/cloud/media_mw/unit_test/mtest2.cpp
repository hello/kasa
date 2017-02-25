/*******************************************************************************
 * mtest2.cpp
 *
 * History:
 *    2014/01/02 - [Zhi He] create file
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

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <fcntl.h>
#include <poll.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

#include "media_mw_if.h"

#include "simple_api.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

//unit test self log
#if 1
//log related
enum {
    e_log_level_always = 0,

    e_log_level_error = 1,
    e_log_level_warning,
    e_log_level_notice,
    e_log_level_debug,

    e_log_level_all,

    e_log_level_total_count,
};
static TInt g_log_level = e_log_level_notice;
static TChar g_log_tags[e_log_level_total_count][32] = {
    {"      [Info]: "},
    {"  [Error]: "},
    {"    [Warnning]: "},
    {"      [Notice]: "},
    {"        [Debug]: "},
    {"          [Verbose]: "},
};

#define test_log_level_trace(level, format, args...)  do { \
        if (level <= g_log_level) { \
            fprintf(stdout, "%s", g_log_tags[level]); \
            fprintf(stdout, format,##args); \
            fprintf(stdout,"[log location]: file %s, function %s, line %d.\n", __FILE__, __FUNCTION__, __LINE__); \
        } \
    } while (0)

#define test_log_level(level, format, args...)  do { \
        if (level <= g_log_level) { \
            fprintf(stdout, "%s", g_log_tags[level]); \
            fprintf(stdout, format,##args); \
        } \
    } while (0)

#define test_logv(format, args...) test_log_level(e_log_level_all, format, ##args)
#define test_logd(format, args...) test_log_level(e_log_level_debug, format, ##args)
#define test_logn(format, args...) test_log_level_trace(e_log_level_notice, format, ##args)
#define test_logw(format, args...) test_log_level_trace(e_log_level_warning, format, ##args)
#define test_loge(format, args...) test_log_level_trace(e_log_level_error, format, ##args)

#define test_log(format, args...) test_log_level(e_log_level_always, format, ##args)
#else

#define test_logv LOG_VERBOSE
#define test_logd LOG_DEBUG
#define test_logn LOG_NOTICE
#define test_logw LOG_WARN
#define test_loge LOG_ERROR

#define test_log LOG_PRINTF
#endif

enum {
    ETestUrlType_Source = 0,
    ETestUrlType_Sink,
    ETestUrlType_Streaming,
    ETestUrlType_CloudTag,

    ETestUrlType_Tot_Count,
};

#if 0
//-----------------------------------------------------------------
//added for auto-discovery test
//-----------------------------------------------------------------
#include "ws_discovery.h"
class ClientObserver: public IWsdClientObserver
{
public:
    ClientObserver(): client_num(0), client_list(NULL) {
        pthread_mutex_init(&client_mutex, NULL);
    }
    ~ClientObserver() {
        free_client_list();
        pthread_mutex_destroy(&client_mutex);
    }
    virtual void onClientChanged(char *uuid, char *ipaddr, char *stream_url) {
        printf("ClientObserver::onClientChanged,uuid[%s],ipaddr[%s],stream_url[%s]\n", uuid, ipaddr ? ipaddr : "NULL", stream_url ? stream_url : "NULL");
        if (ipaddr) {
            add_client(uuid, ipaddr, stream_url);
        } else {
            remove_client(uuid);
        }
    }
    int get_client_url(int client, char *url, int size_url) {
        pthread_mutex_lock(&client_mutex);
        if (client >= client_num) {
            pthread_mutex_unlock(&client_mutex);
            return -1;
        }
        //TODO, now the head return
        snprintf(url, size_url, "%s", client_list->stream_url);
        pthread_mutex_unlock(&client_mutex);
        return 0;
    }
private:
    int free_client_list() {
        pthread_mutex_lock(&client_mutex);
        while (client_list) {
            client_node_t *next = client_list->next;
            delete client_list;
            client_list = next;
        }
        client_num = 0;
        pthread_mutex_unlock(&client_mutex);
        return 0;
    }
    int add_client(char *uuid, char *ipaddr, char *stream_url) {
        pthread_mutex_lock(&client_mutex);
        client_node_t *node = client_list;
        while (node) {
            if (!strcmp(node->uuid, uuid)) {
                snprintf(node->ipaddr, sizeof(node->ipaddr), "%s", ipaddr);
                snprintf(node->stream_url, sizeof(node->stream_url), "%s", stream_url);
                pthread_mutex_unlock(&client_mutex);
                return 1;//duplicate
            }
            node = node->next;
        }
        node = new client_node_t;
        snprintf(node->uuid, sizeof(node->uuid), "%s", uuid);
        snprintf(node->ipaddr, sizeof(node->ipaddr), "%s", ipaddr);
        snprintf(node->stream_url, sizeof(node->stream_url), "%s", stream_url);
        node->next = NULL;
        if (client_list) {
            node->next = client_list;
        }
        client_list = node;
        ++client_num;
        pthread_mutex_unlock(&client_mutex);
        return 0;
    }
    int remove_client(char *uuid) {
        pthread_mutex_lock(&client_mutex);
        client_node_t *prev = NULL, *curr = client_list;
        while (curr) {
            if (!strcmp(curr->uuid, uuid)) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    client_list = curr->next;
                }
                delete curr;
                --client_num;
                pthread_mutex_unlock(&client_mutex);
                return 0;
            }
            prev = curr;
            curr = curr->next;
        }
        pthread_mutex_unlock(&client_mutex);
        return -1;
    }
private:
    int client_num;
    struct client_node_t {
        char uuid[64];
        char ipaddr[64];
        char stream_url[1024];
        struct client_node_t *next;
    } *client_list;
    pthread_mutex_t client_mutex;
};

class AutoDiscoveryExample
{
public:
    AutoDiscoveryExample(int rtsp_port = 0, char *url = NULL) {
        if (rtsp_port != 0  && url != NULL) {
            //for audio-backchannel
            WSDiscoveryService::GetInstance()->setRtspInfo(rtsp_port, (const char *)url);
        }
        observer_ = new ClientObserver;
        WSDiscoveryService::GetInstance()->registerObserver(observer_);
        WSDiscoveryService::GetInstance()->start();
        printf("WSDiscovery start OK, port %d, url %s\n", rtsp_port, url);
    }
    ~AutoDiscoveryExample() {
        WSDiscoveryService::GetInstance()->Delete();
        delete observer_;
        observer_ = NULL;
    }
public:
    static void client_info(void *usr, char *uuid, char *ipaddr, char *stream_url) {
        printf("\t client, uuid [%s], ipaddr[%s],stream_url[%s]\n", uuid, ipaddr, stream_url);
    }
    static void DoDisplayClient() {
        printf("WSDiscoveryService::getClientList\n");
        WSDiscoveryService::GetInstance()->getClientList(client_info, NULL);
        printf("WSDiscoveryClient::getDeviceList END\n");
    }
private:
    IWsdClientObserver *observer_;
};
//added for auto-discovery END --------------------------------------------------------------------------
#endif

TInt _mtest2_add_url_into_context(SSimpleAPIContxt *p_content, TChar *url, TULong type, TULong group, TULong is_hd)
{
    TULong current_count = 0;
    TChar *ptmp = NULL;

    DASSERT(url);
    DASSERT(ETestUrlType_Tot_Count > type);

    if ((!p_content) || (!url) || (ETestUrlType_Tot_Count <= type)) {
        test_loge("BAD input params in _mtest2_add_url_into_context: p_content %p, url %p, type %ld\n", p_content, url, type);
        return (-1);
    }

    DASSERT(DMaxGroupNumber > group);
    if (DMaxGroupNumber <= group) {
        test_loge("BAD input params in _mtest2_add_url_into_context: group %ld\n", group);
        return (-2);
    }

    SGroupUrl *p_group_url = &p_content->group[group];

    if (ETestUrlType_Source == type) {

        if (!is_hd) {
            current_count = p_group_url->source_2rd_channel_number;
            if (DLikely(current_count < DMaxChannelNumberInGroup)) {
                snprintf(&p_group_url->source_url_2rd[current_count][0], DMaxUrlLen, "%s", url);
                p_group_url->source_2rd_channel_number ++;
            } else {
                test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
                return (-3);
            }
        } else {
            current_count = p_group_url->source_channel_number;
            if (DLikely(current_count < DMaxChannelNumberInGroup)) {
                snprintf(&p_group_url->source_url[current_count][0], DMaxUrlLen, "%s", url);
                p_group_url->source_channel_number ++;
            } else {
                test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
                return (-4);
            }
        }

    } else if (ETestUrlType_Sink == type) {

        if (!is_hd) {
            current_count = p_group_url->sink_2rd_channel_number;
            if (DLikely(current_count < DMaxChannelNumberInGroup)) {
                snprintf(&p_group_url->sink_url_2rd[current_count][0], DMaxUrlLen, "%s", url);
                p_group_url->sink_2rd_channel_number ++;
            } else {
                test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
                return (-5);
            }
        } else {
            current_count = p_group_url->sink_channel_number;
            if (DLikely(current_count < DMaxChannelNumberInGroup)) {
                snprintf(&p_group_url->sink_url[current_count][0], DMaxUrlLen, "%s", url);
                p_group_url->sink_channel_number ++;
            } else {
                test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
                return (-6);
            }
        }

    } else if (ETestUrlType_Streaming == type) {

        if (!is_hd) {
            current_count = p_group_url->streaming_2rd_channel_number;
            if (DLikely(current_count < DMaxChannelNumberInGroup)) {
                snprintf(&p_group_url->streaming_url_2rd[current_count][0], DMaxUrlLen, "%s", url);
                p_group_url->streaming_2rd_channel_number ++;
            } else {
                test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
                return (-7);
            }
        } else {
            current_count = p_group_url->streaming_channel_number;
            if (DLikely(current_count < DMaxChannelNumberInGroup)) {
                snprintf(&p_group_url->streaming_url[current_count][0], DMaxUrlLen, "%s", url);
                p_group_url->streaming_channel_number ++;
            } else {
                test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
                return (-8);
            }
        }

    } else if (ETestUrlType_CloudTag == type) {

        current_count = p_group_url->cloud_channel_number;
        if (DLikely(current_count < DMaxChannelNumberInGroup)) {
            snprintf(&p_group_url->cloud_agent_tag[current_count][0], DMaxUrlLen, "%s", url);
            p_group_url->cloud_channel_number ++;
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, DMaxChannelNumberInGroup);
            return (-7);
        }

    } else {
        test_loge("Must not comes here, type %ld\n", type);
    }

    return 0;
}

void _mtest2_print_version()
{
    TU32 major, minor, patch;
    TU32 year;
    TU32 month, day;

    printf("\ncurrent version:\n");
    gfGetCommonVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_common version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetCloudLibVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_cloud_lib version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetMediaMWVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_media_mw version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
}

void _mtest2_print_usage()
{
    printf("\nmtest2 options:\n");
    printf("\t'-f [filename]': '-f' specify input file name/url, for demuxer\n");
    printf("\t'-r [filename]': '-r' specify output file name/url, for muxer\n");
    printf("\t'-s [streaming_url]': '-s' specify streaming tag name, for streaming\n");
    printf("\t'-c [agent_tag]': '-s' specify cloud agent tag name, for cloud connetion\n");

    printf("\t'--udec': will choose udec mode\n");
    printf("\t'--mudec': will choose multiple window udec mode\n");
    printf("\t'--genericnvr': will choose generic nvr, recording + streaming, not local playback\n");
    printf("\t'--voutmask [mask]': '--voutmask' specify output device's mask, LCD's bit is 0x01, HDMI's bit is 0x02\n");
    printf("\t'--tilemode [%%d]': '--tilemode' specify tilemode in DSP, 0:disable tilemode, 5:enable tilemodes\n");
    printf("\t'--saveall': save all files (.ts format)\n");
    printf("\t'--smoothswitch': will try do better switch(multiple view to single view only now)\n");
    printf("\t'--bgcolor [%%d:%%d:%%d]': specify Y, Cb, Cr in background color, if without parameters, Y Cb Cr will set to green color(all zero). if without this option, the bg color is black\n");
    printf("\t'--enable-compensatejitter': try to monitor delay and compensate it from dsp side\n");
    printf("\t'--disable-compensatejitter': disable the monitor delay and compensate it from dsp side\n");
    printf("\t'--help': print help\n");
    printf("\t'--showversion': show current version\n");

    printf("\t'--framerate': specify transcode framerate\n");
    printf("\t'--bitrate' [%%d Kbps]: specify transcode bitrate\n");

    printf("\n\tblow will show playback mode in multiple window UDEC\n");
    printf("\t\t'--1x1080p': [mudec mode], 1x1080p\n");
    printf("\t\t'--4+1': [mudec mode], 1x1080p + 4xD1\n");
    printf("\t\t'--6+1': [mudec mode], 1x1080p + 6xD1\n");
    printf("\t\t'--8+1': [mudec mode], 1x1080p + 8xD1\n");
    printf("\t\t'--9+1': [mudec mode], 1x1080p + 9xD1\n");
    printf("\t\t'--4x720p': [mudec mode], 4x720p\n");
    printf("\t\t'--4xD1': [mudec mode], 4xD1\n");
    printf("\t\t'--6xD1': [mudec mode], 6xD1\n");
    printf("\t\t'--8xD1': [mudec mode], 8xD1\n");
    printf("\t\t'--9xD1': [mudec mode], 9xD1\n");
    printf("\t\t'--1x3M': [mudec mode], 1x3M\n");
    printf("\t\t'--4+1x3M': [mudec mode], 1x3M + 4xD1\n");
    printf("\t\t'--6+1x3M': [mudec mode], 1x3M + 6xD1\n");
    printf("\t\t'--8+1x3M': [mudec mode], 1x3M + 8xD1\n");
    printf("\t\t'--9+1x3M': [mudec mode], 1x3M + 9xD1\n");

    printf("\n\tdebug only options:\n");
    printf("\t'--loglevel [level]': '--loglevel' specify log level, regardless the log.config\n");
    printf("\t'--enable-audio': will enable audio path\n");
    printf("\t'--enable-video': will enable video path\n");
    printf("\t'--disable-audio': will disable audio path\n");
    printf("\t'--disable-video': will disable video path\n");
    printf("\t'--sharedemuxer': debug only, will share demuxer\n");
    printf("\t'--maxbuffernumber [%%d]': will set buffer number of each udec instance (dsp), value should be in 7~20\n");
    printf("\t'--prebuffer [%%d]': will set prebuffer at dsp side, value should be in 1~{maxbuffernumber - 5}\n");
    printf("\t'--prefetch [%%d]': arm will prefetch some data(%%d frames) before start playback\n");
    printf("\t'--timeout [%%d]': set timeout threashold for streaming, %%d seconds, if timeout occur, will reconnect server\n");
    printf("\t'--reconnectinterval [%%d]': auto reconnect server interval, seconds\n");
    printf("\t'--nousequpes': not feed udec wrapper\n");
    printf("\t'--usequpes': feed udec wrapper\n");
    printf("\t'--nopts': not feed pts to decoder\n");
    printf("\t'--pts': feed pts to decoder\n");
    printf("\t'--fps': specify fps\n");
    printf("\t'--debug-parse-multiple-access-unit': a debug option: try parse acc multiple access unit in demuxer\n");
    printf("\t'--debug-not-parse-multiple-access-unit': a debug option: not parse acc multiple access unit in demuxer, down stream filters will parse it\n");
    printf("\t'--debug-stop0': a debug option: try stop(0) for udec\n");
    printf("\t'--debug-stop1': a debug option: try stop(1) for udec\n");

    printf("\t'--prefer-ffmpeg-muxer': prefer ffmpeg muxer\n");
    printf("\t'--prefer-ts-muxer': prefer private ts muxer\n");
    printf("\t'--prefer-ffmpeg-audio-decoder': prefer ffmpeg audio decoder\n");
    printf("\t'--prefer-aac-audio-decoder': prefer aac audio decoder\n");
    printf("\t'--prefer-aac-audio-encoder': prefer aac audio encoder\n");
    printf("\t'--prefer-customized-adpcm-encoder': prefer adpcm audio encoder\n");
    printf("\t'--prefer-customized-adpcm-decoder': prefer adpcm audio decoder\n");

    printf("\t'--enable-nativeaudiopush': enable native audio capture and push, 'NVR <--> camera' two way speech\n");
    printf("\t'--disable-nativeaudiopush': disable native audio capture and push, 'NVR <--> camera' two way speech\n");
    printf("\t'--enable-remoteaudio': enable remote audio push(forwarding), 'remote controller <--> camera' two way speech\n");
    printf("\t'--disable-remoteaudio': disable remote audio push(forwarding), 'remote controller <--> camera' two way speech\n");

    printf("\t'--pushraw': push audio format as raw PCM\n");
    printf("\t'--pushcomp': push audio as compressed\n");
    printf("\t'--pushaac': push audio as AAC\n");
    printf("\t'--pushadpcm': push audio as ADPCM\n");

    printf("\t'--speak': send audio to camera\n");
    printf("\t'--listen': receive audio from camera\n");

    printf("\t'--upload': upload es to cloud\n");
}

void _mtest2_print_runtime_usage()
{
    printf("\nmtest2 runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
    printf("\t'pall': will print all components' status\n");
    printf("\t'pdsp:[%%d]': will print udec(%%d)'s status\n");

    printf("\ttrickplay category\n");
    printf("\t\t' %%d', pause/resume, %%d is udec_id\n");
    printf("\t\t's%%d', step play, %%d is udec_id\n");

    printf("\t'cc': playback capture category:\n");
    printf("\t\t'ccjpeg:%%x %%x', udec_id %%x(if udec_id == 0xff, means screen shot), capture jpeg file index %%x.\n");

    printf("\t: playback zoom category:\n");
    printf("\t\t'--zoomin' or '--zoomin %%d', zoomin.\n");
    printf("\t\t'--zoomout' or '--zoomout %%d', zoomout.\n");
    printf("\t\t'--up' or '--up %%d', move zoomed window up.\n");
    printf("\t\t'--down' or '--down %%d', move zoomed window down.\n");
    printf("\t\t'--left' or '--left %%d', move zoomed window left.\n");
    printf("\t\t'--right' or '--right %%d', move zoomed window right.\n");

    printf("\t: display related\n");
    printf("\t\t'--layout %%d %%d', switch to display layout, %%d: 1 means 'MxN table view', 2 means tele-presence view', '3 means 'highlighten view', '4 means 'single window view', %%d: focus index\n");
    printf("\t\t'--showothers %%d', show other windows\n");
    printf("\t\t'--hideothers %%d', hide other windows\n");

    printf("\t: encode related\n");
    printf("\t\t'cbitrate:%%d', bitrate %%d K.\n");
    printf("\t\t'cframerate:%%d', framerate %%d fps.\n");

    printf("\t\t'--groupnum %%u, specify total group number, if set to 0, will use 1 as default\n");
    printf("\t\t'--switchgroup %%u, switch group\n");
    printf("\t\t'--addgroupch4 %%u -f %%s -f  %%s -f  %%s -f  %%s --hd  -f  %%s -f  %%s -f  %%s -f  %%s, add one group with channel number as 4\n");

    printf("\t: two way speech related\n");
    printf("\t\t'--speak:', start speak to\n");
    printf("\t\t'--listen', start listen from.\n");
}

TInt mtest2_init_params(SSimpleAPIContxt *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;
    TU8 total_group_number = 0;

    if (argc < 2) {
        //_mtest_print_version();
        _mtest2_print_usage();
        _mtest2_print_runtime_usage();
        return 1;
    }

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("-f", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_add_url_into_context(p_content, argv[i + 1], ETestUrlType_Source, cur_group, cur_source_is_hd);
                test_log("add_url_into_context, argv[i + 1]: %s, is_hd %ld\n", argv[i + 1], cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: add_url_into_context ret(%d).\n", ret);
                    return (-3);
                }
            } else {
                test_loge("[input argument error]: '-f', should follow with source url(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--hdsource", argv[i])) {
            test_log("'--hdsource': add_url_into_context, hd source start...\n");
            cur_source_is_hd = 1;
        } else if (!strcmp("--sdsource", argv[i])) {
            test_log("'--sdsource': add_url_into_context, sd source start...\n");
            cur_source_is_hd = 0;
        } else if (!strcmp("--nextgroup", argv[i])) {
            test_log("'--nextgroup': next group ...\n");
            if ((cur_group + 1) > DMaxGroupNumber) {
                test_loge("'--nextgroup': too many group %ld, max DMaxGroupNumber %d\n", cur_group, DMaxGroupNumber);
            } else {
                cur_group ++;
                cur_source_is_hd = 0;
            }
        } else if (!strcmp("--groupnum", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                total_group_number = ret;
                test_log("'--groupnum %d'\n", total_group_number);
                i ++;
            } else {
                test_log("'--groupnum, should followed by specified total group number'\n");
                return (-10);
            }
        } else if (!strcmp("--saveall", argv[i])) {
            test_log("'--saveall': save all streams\n");
            p_content->setting.enable_recording = 1;
        } else if (!strcmp("--tilemode", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((0 == ret) || (5 == ret)) {
                    test_log("'--tilemode %d'\n", ret);
                    p_content->setting.dsp_tilemode = ret;
                } else {
                    test_loge("'--tilemode %d, not expected, should be 0 or 5'\n", ret);
                    p_content->setting.dsp_tilemode = 5;
                }
                i ++;
            } else {
                test_log("'--tilemode, use default 5'\n");
                p_content->setting.dsp_tilemode = 5;
            }
        } else if (!strcmp("--bgcolor", argv[i])) {
            TU32 color_y = 0x0, color_cb = 0x0, color_cr = 0x0;
            if (((i + 1) < argc) && (3 == sscanf(argv[i + 1], "%d:%d:%d", &color_y, &color_cb, &color_cr))) {
                test_log("'--bgcolor Y %d Vb %d Cr %d'\n", color_y, color_cb, color_cr);
                p_content->setting.bg_color_y = color_y;
                p_content->setting.bg_color_cb = color_cb;
                p_content->setting.bg_color_cr = color_cr;
                p_content->setting.set_bg_color = 1;
                i ++;
            } else {
                test_loge("'--bgcolor, need followed by a color('%%d:%%d:%%d') Y:Cb:Cr\n");
                p_content->setting.bg_color_y = color_y;
                p_content->setting.bg_color_cb = color_cb;
                p_content->setting.bg_color_cr = color_cr;
                p_content->setting.set_bg_color = 1;
            }
        } else if (!strcmp("--showversion", argv[i])) {
            _mtest2_print_version();
            return (18);
        } else if (!strcmp("--sharedemuxer", argv[i])) {
            test_log("[input argument]: '--sharedemuxer': share demuxer\n");
            p_content->setting.debug_share_demuxer = 1;
        } else if (!strcmp("--smoothswitch", argv[i])) {
            test_log("[input argument]: '--smoothswitch': will try get better user experience when stream switch\n");
            p_content->setting.b_smooth_stream_switch = 1;
        } else if (!strcmp("--debug-parse-multiple-access-unit", argv[i])) {
            test_log("[input argument]: '--debug-parse-multiple-access-unit': demuxer will parse multiple access unit in one RTP packet's case\n");
            p_content->setting.parse_multiple_access_unit = 1;
        } else if (!strcmp("--debug-parse-multiple-access-unit", argv[i])) {
            test_log("[input argument]: '--debug-not-parse-multiple-access-unit': demuxer will not parse multiple access unit in one RTP packet's case, down stream filters will handle it\n");
            p_content->setting.parse_multiple_access_unit = 0;
        } else if (!strcmp("--prefer-ffmpeg-muxer", argv[i])) {
            test_log("[input argument]: '--prefer-ffmpeg-muxer': prefer ffmpeg muxer\n");
            p_content->setting.debug_prefer_ffmpeg_muxer = 1;
        } else if (!strcmp("--prefer-ts-muxer", argv[i])) {
            test_log("[input argument]: '--prefer-ts-muxer': prefer private ts muxer\n");
            p_content->setting.debug_prefer_private_ts_muxer = 1;
        } else if (!strcmp("--prefer-ffmpeg-audio-decoder", argv[i])) {
            test_log("[input argument]: '--prefer-ffmpeg-audio-decoder': prefer ffmpeg audio decoder\n");
            p_content->setting.debug_prefer_ffmpeg_audio_decoder = 1;
        } else if (!strcmp("--prefer-aac-audio-decoder", argv[i])) {
            test_log("[input argument]: '--prefer-aac-audio-decoder': prefer aac audio decoder\n");
            p_content->setting.debug_prefer_aac_audio_decoder = 1;
        } else if (!strcmp("--prefer-aac-audio-encoder", argv[i])) {
            test_log("[input argument]: '--prefer-aac-audio-encoder': prefer aac audio encoder\n");
            p_content->setting.debug_prefer_aac_audio_encoder = 1;
        } else if (!strcmp("--prefer-customized-adpcm-decoder", argv[i])) {
            test_log("[input argument]: '--prefer-customized-adpcm-decoder': prefer customized-adpcm decoder\n");
            p_content->setting.debug_prefer_customized_adpcm_decoder = 1;
        } else if (!strcmp("--prefer-customized-adpcm-encoder", argv[i])) {
            test_log("[input argument]: '--prefer-customized-adpcm-encoder': prefer customized-adpcm encoder\n");
            p_content->setting.debug_prefer_customized_adpcm_encoder = 1;
        } else if (!strcmp("--tcpmode", argv[i])) {
            test_log("[input argument]: '--tcpmode': let rtsp client try tcp mode first\n");
            p_content->setting.debug_try_rtsp_tcp_mode_first = 1;
        } else if (!strcmp("--rtspserverport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.rtsp_server_port = ret;
                test_log("[input argument]: '--rtspserverport': (%d).\n", p_content->setting.rtsp_server_port);
            } else {
                test_loge("[input argument error]: '--rtspserverport', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("-r", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_add_url_into_context(p_content, argv[i + 1], ETestUrlType_Sink, cur_group, cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_add_url_into_context ret(%d).\n", ret);
                    return (-5);
                }
            } else {
                test_loge("[input argument error]: '-r', should follow with sink url(%%s), argc %d, i %d.\n", argc, i);
                return (-6);
            }
            i ++;
        } else if (!strcmp("-s", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_add_url_into_context(p_content, argv[i + 1], ETestUrlType_Streaming, cur_group, cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_add_url_into_context ret(%d).\n", ret);
                    return (-7);
                }
            } else {
                test_loge("[input argument error]: '-F', should follow with streaming url(%%s), argc %d, i %d.\n", argc, i);
                return (-8);
            }
            i ++;
        } else if (!strcmp("-c", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_add_url_into_context(p_content, argv[i + 1], ETestUrlType_CloudTag, cur_group, cur_source_is_hd);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_add_url_into_context ret(%d).\n", ret);
                    return (-7);
                }
            } else {
                test_loge("[input argument error]: '-F', should follow with cloud agent tag(%%s), argc %d, i %d.\n", argc, i);
                return (-8);
            }
            i ++;
        } else if (!strcmp("--voutmask", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 3) || (ret < 0)) {
                    test_loge("[input argument error]: '--voutmask', ret(%d) should between 0 to 3.\n", ret);
                    return (-9);
                } else {
                    p_content->setting.voutmask = ret;
                    test_log("[input argument]: '--voutmask': (0x%x).\n", p_content->setting.voutmask);
                }
            } else {
                test_loge("[input argument error]: '--voutmask', should follow with integer(voutmask), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--loglevel", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > ELogLevel_Verbose) || (ret < ELogLevel_None)) {
                    test_loge("[input argument error]: '--loglevel', ret(%d) should between %d to %d.\n", ret, ELogLevel_None, ELogLevel_Verbose);
                    return (-9);
                } else {
                    p_content->setting.force_log_level = ret;
                    p_content->setting.enable_force_log_level = 1;
                    const TChar *p_log_level_string = NULL;
                    gfGetLogLevelString((ELogLevel)p_content->setting.force_log_level, p_log_level_string);
                    test_log("[input argument]: '--loglevel': (%d) (%s).\n", p_content->setting.force_log_level, p_log_level_string);
                }
            } else {
                test_loge("[input argument error]: '--loglevel', should follow with integer(loglevel), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--framerate", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.udec_specify_transcode_framerate = 1;
                p_content->setting.transcode_framerate = ret;
                test_log("[input argument]: '--framerate': (%d)\n", p_content->setting.transcode_framerate);
            } else {
                test_loge("[input argument error]: '--framerate', should follow with integer(framerate), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--bitrate", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.udec_specify_transcode_bitrate = 1;
                p_content->setting.transcode_bitrate = ret * 1024;
                test_log("[input argument]: '--bitrate': (%d)\n", p_content->setting.transcode_bitrate);
            } else {
                test_loge("[input argument error]: '--bitrate', should follow with integer(bitrate Kbps), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--pbgroup", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 4) || (ret < 0)) {
                    test_loge("[input argument error]: '--pbgroup', ret(%d) should between %d to %d.\n", ret, 0, 4);
                    return (-9);
                } else {
                    p_content->setting.playback_group_number = ret;
                    test_log("[input argument]: '--pbgroup': (%d) .\n", p_content->setting.playback_group_number);
                }
            } else {
                test_loge("[input argument error]: '--pbgroup', should follow with integer(pbgroup), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--recgroup", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 4) || (ret < 0)) {
                    test_loge("[input argument error]: '--recgroup', ret(%d) should between %d to %d.\n", ret, 0, 4);
                    return (-9);
                } else {
                    p_content->setting.recording_group_number = ret;
                    test_log("[input argument]: '--recgroup': (%d) .\n", p_content->setting.recording_group_number);
                }
            } else {
                test_loge("[input argument error]: '--recgroup', should follow with integer(recgroup), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--streaminggroup", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 4) || (ret < 0)) {
                    test_loge("[input argument error]: '--streaminggroup', ret(%d) should between %d to %d.\n", ret, 0, 4);
                    return (-9);
                } else {
                    p_content->setting.streaming_group_number = ret;
                    test_log("[input argument]: '--streaminggroup': (%d) .\n", p_content->setting.streaming_group_number);
                }
            } else {
                test_loge("[input argument error]: '--streaminggroup', should follow with integer(streaminggroup), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--cloudgroup", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 4) || (ret < 0)) {
                    test_loge("[input argument error]: '--cloudgroup', ret(%d) should between %d to %d.\n", ret, 0, 4);
                    return (-9);
                } else {
                    p_content->setting.cloud_group_number = ret;
                    test_log("[input argument]: '--cloudgroup': (%d) .\n", p_content->setting.cloud_group_number);
                }
            } else {
                test_loge("[input argument error]: '--cloudgroup', should follow with integer(cloudgroup), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--fullgragh", argv[i])) {
            p_content->setting.debug_not_build_full_gragh = 0;
        } else if (!strcmp("--notfullgraph", argv[i])) {
            p_content->setting.debug_not_build_full_gragh = 1;
        } else if (!strcmp("--enable-nativeaudiopush", argv[i])) {
            test_log("[input argument]: '--enable-nativeaudiopush'\n");
            p_content->setting.enable_audio_capture = 1;
        } else if (!strcmp("--disable-nativeaudiopush", argv[i])) {
            test_log("[input argument]: '--disable-nativeaudiopush'\n");
            p_content->setting.enable_audio_capture = 0;
        } else if (!strcmp("--enable-remoteaudiopush", argv[i])) {
            test_log("[input argument]: '--enable-remoteaudiopush'\n");
            p_content->setting.enable_remote_audio_forwarding = 1;
        } else if (!strcmp("--disable-remoteaudiopush", argv[i])) {
            test_log("[input argument]: '--disable-remoteaudiopush'\n");
            p_content->setting.enable_remote_audio_forwarding = 0;
        } else if (!strcmp("--pushraw", argv[i])) {
            p_content->setting.push_audio_type = 0;
        } else if (!strcmp("--pushcomp", argv[i])) {
            p_content->setting.push_audio_type = 1;
        } else if (!strcmp("--pushaac", argv[i])) {
            p_content->setting.push_audio_type = 1;
            p_content->setting.debug_prefer_aac_audio_encoder = 1;
        } else if (!strcmp("--pushadpcm", argv[i])) {
            p_content->setting.debug_prefer_customized_adpcm_encoder = 1;
            p_content->setting.push_audio_type = 1;
        } else if (!strcmp("--maxbuffernumber", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 20) || (ret < 7)) {
                    test_loge("[input argument error]: '--maxbuffernumber', (%d) should between %d to %d.\n", ret, 7, 20);
                    return (-25);
                } else {
                    p_content->setting.dsp_decoder_total_buffer_count = ret;
                    test_log("[input argument]: '--maxbuffernumber': (%d).\n", p_content->setting.dsp_decoder_total_buffer_count);
                }
            } else {
                test_loge("[input argument error]: '--maxbuffernumber', should follow with integer(maxbuffernumber), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--prefetch", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 30) || (ret < 1)) {
                    test_loge("[input argument error]: '--prefetch', (%d) should between %d to %d.\n", ret, 1, 30);
                    return (-9);
                } else {
                    p_content->setting.decoder_prefetch_count = ret;
                    test_log("[input argument]: '--prefetch': (%d).\n", p_content->setting.decoder_prefetch_count);
                }
            } else {
                test_loge("[input argument error]: '--prefetch', should follow with integer(prefetch count), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--timeout", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 30) || (ret < 5)) {
                    test_loge("[input argument error]: '--timeout', (%d) should between %d to %d.\n", ret, 5, 30);
                    return (-9);
                } else {
                    p_content->setting.streaming_timeout_threashold = ret;
                    test_log("[input argument]: '--timeout': (%d).\n", p_content->setting.streaming_timeout_threashold);
                }
            } else {
                test_loge("[input argument error]: '--timeout', should follow with integer(timeout), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--reconnectinterval", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > 30) || (ret < 2)) {
                    test_loge("[input argument error]: '--reconnectinterval', (%d) should between %d to %d.\n", ret, 2, 30);
                    return (-9);
                } else {
                    p_content->setting.streaming_retry_interval = ret;
                    test_log("[input argument]: '--reconnectinterval': (%d).\n", p_content->setting.streaming_retry_interval);
                }
            } else {
                test_loge("[input argument error]: '--reconnectinterval', should follow with integer(reconnectinterval), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--disable-audio", argv[i])) {
            test_log("[input argument]: '--disable-audio', disable audio.\n");
            p_content->setting.disable_audio_path = 1;
        } else if (!strcmp("--enable-audio", argv[i])) {
            test_log("[input argument]: '--enable-audio', enable audio.\n");
            p_content->setting.disable_audio_path = 0;
        } else if (!strcmp("--disable-video", argv[i])) {
            test_log("[input argument]: '--disable-audio', disable video.\n");
            p_content->setting.disable_video_path = 0;
        } else if (!strcmp("--enable-video", argv[i])) {
            test_log("[input argument]: '--enable-audio', enable video.\n");
            p_content->setting.disable_video_path = 1;
        } else if (!strcmp("--nousequpes", argv[i])) {
            test_log("[input argument]: '--nousequpes', not add udec warpper.\n");
            p_content->setting.udec_not_feed_usequpes = 1;
        } else if (!strcmp("--usequpes", argv[i])) {
            test_log("[input argument]: '--usequpes', add udec wrapper.\n");
            p_content->setting.udec_not_feed_usequpes = 0;
        } else if (!strcmp("--nopts", argv[i])) {
            test_log("[input argument]: '--nopts', not feed udec pts.\n");
            p_content->setting.udec_not_feed_pts = 1;
        } else if (!strcmp("--pts", argv[i])) {
            test_log("[input argument]: '--pts', feed udec pts.\n");
            p_content->setting.udec_not_feed_pts = 0;
        } else if (!strcmp("--fps", argv[i])) {
            TInt fps = 0;
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &fps))) {
                p_content->setting.udec_specify_fps = fps;
            } else {
                test_loge("[input argument error]: '--fps', should follow with integer, argc %d, i %d.\n", argc, i);
            }
            i ++;
            test_log("[input argument]: '--fps', specify fps %d.\n", p_content->setting.udec_specify_fps);
        } else if (!strcmp("--enable-compensatejitter", argv[i])) {
            test_log("[input argument]: '--enable-compensatejitter', try to compensate delay from jitter.\n");
            p_content->setting.compensate_delay_from_jitter = 1;
        } else if (!strcmp("--disable-compensatejitter", argv[i])) {
            test_log("[input argument]: '--disable-compensatejitter', disable compensate delay from jitter.\n");
            p_content->setting.compensate_delay_from_jitter = 0;
        } else if (!strcmp("--netreceiver-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--netreceiver-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    p_content->setting.net_receiver_scheduler_number = ret;

                    test_log("[input argument]: '--netreceiver-scheduler': (%d).\n", p_content->setting.net_receiver_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--netreceiver-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--netsender-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--netsender-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    p_content->setting.net_sender_scheduler_number = ret;

                    test_log("[input argument]: '--netsender-scheduler': (%d).\n", p_content->setting.net_sender_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--netsender-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--fileioreader-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--fileioreader-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    p_content->setting.fileio_reader_scheduler_number = ret;

                    test_log("[input argument]: '--fileioreader-scheduler': (%d).\n", p_content->setting.fileio_reader_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--fileioreader-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--fileiowriter-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--fileiowriter-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    p_content->setting.fileio_writer_scheduler_number = ret;

                    test_log("[input argument]: '--fileiowriter-scheduler': (%d).\n", p_content->setting.fileio_writer_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--fileiowriter-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--videodecoder-scheduler", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > DMaxSchedulerGroupNumber) || (ret < 0)) {
                    test_loge("[input argument error]: '--videodecoder-scheduler', number(%d) should between 0 to %d.\n", ret, DMaxSchedulerGroupNumber);
                    return (-11);
                } else {
                    p_content->setting.video_decoder_scheduler_number = ret;

                    test_log("[input argument]: '--videodecoder-scheduler': (%d).\n", p_content->setting.video_decoder_scheduler_number);
                }
            } else {
                test_loge("[input argument error]: '--videodecoder-scheduler', should follow with integer(number), argc %d, i %d.\n", argc, i);
            }
            i ++;
        } else if (!strcmp("--udec", argv[i])) {
            p_content->setting.work_mode = EMediaDeviceWorkingMode_SingleInstancePlayback;
            p_content->setting.playback_mode = EPlaybackMode_1x1080p;
            test_log("[input argument]: '--udec'\n");
        } else if (!strcmp("--mudec", argv[i])) {
            p_content->setting.work_mode = EMediaDeviceWorkingMode_MultipleInstancePlayback;
            test_log("[input argument]: '--mudec'\n");
        } else if (!strcmp("--mudec-transcode", argv[i])) {
            p_content->setting.work_mode = EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding;
            test_log("[input argument]: '--mudec-transcode'\n");
        } else if (!strcmp("--genericnvr", argv[i])) {
            p_content->setting.work_mode = EMediaDeviceWorkingMode_NVR_RTSP_Streaming;
            test_log("[input argument]: '--genericnvr'\n");
        } else if (!strcmp("--streaming-rtsp", argv[i])) {
            p_content->setting.enable_streaming_server = 1;
        } else if (!strcmp("--help", argv[i])) {
            _mtest2_print_version();
            _mtest2_print_usage();
            _mtest2_print_runtime_usage();
            return (3);
        } else if (!strcmp("--1x1080p", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x1080p;
            test_log("[input argument]: '--1x1080p'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 0;
        } else if (!strcmp("--4+1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x1080p_4xD1;
            test_log("[input argument]: '--4+1'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 4;
        } else if (!strcmp("--6+1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x1080p_6xD1;
            test_log("[input argument]: '--6+1'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 6;
        } else if (!strcmp("--8+1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x1080p_8xD1;
            test_log("[input argument]: '--8+1'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 8;
        } else if (!strcmp("--9+1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x1080p_9xD1;
            test_log("[input argument]: '--9+1'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 9;
        } else if (!strcmp("--1x3M", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x3M;
            test_log("[input argument]: '--1x3M'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 0;
        } else if (!strcmp("--4+1x3M", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x3M_4xD1;
            test_log("[input argument]: '--4+1x3M'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 4;
        } else if (!strcmp("--6+1x3M", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x3M_6xD1;
            test_log("[input argument]: '--6+1x3M'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 6;
        } else if (!strcmp("--8+1x3M", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x3M_8xD1;
            test_log("[input argument]: '--8+1x3M'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 8;
        } else if (!strcmp("--9+1x3M", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_1x3M_9xD1;
            test_log("[input argument]: '--9+1x3M'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 9;
        } else if (!strcmp("--4x720p", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_4x720p;
            test_log("[input argument]: '--4x720p'\n");
            p_content->setting.udec_instance_number = 0;
            p_content->setting.udec_2rd_instance_number = 4;
        } else if (!strcmp("--4xD1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_4xD1;
            test_log("[input argument]: '--4xD1'\n");
            p_content->setting.udec_instance_number = 0;
            p_content->setting.udec_2rd_instance_number = 4;
        } else if (!strcmp("--6xD1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_6xD1;
            test_log("[input argument]: '--6xD1'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 6;
        } else if (!strcmp("--8xD1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_8xD1;
            test_log("[input argument]: '--8xD1'\n");
            p_content->setting.udec_instance_number = 1;
            p_content->setting.udec_2rd_instance_number = 8;
        } else if (!strcmp("--9xD1", argv[i])) {
            p_content->setting.playback_mode = EPlaybackMode_9xD1;
            test_log("[input argument]: '--9xD1'\n");
            p_content->setting.udec_instance_number = 0;
            p_content->setting.udec_2rd_instance_number = 9;
        } else if (!strcmp("--debug-stop0", argv[i])) {
            test_log("[input argument]: '--debug-stop0', prefer stop(0) for udec.\n");
            p_content->setting.debug_prefer_udec_stop_flag = 0;
        } else if (!strcmp("--debug-stop1", argv[i])) {
            test_log("[input argument]: '--debug-stop1', prefer stop(1) for udec.\n");
            p_content->setting.debug_prefer_udec_stop_flag = 1;
        } else if (!strcmp("--upload", argv[i])) {
            test_log("[input argument]: '--upload', upload es to cloud\n");
            if ((i + 1) < argc) {
                snprintf(&p_content->setting.cloud_upload_server_addr[0], DMaxUrlLen, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--cloudserver', should follow with server url(%%s), argc %d, i %d.\n", argc, i);
            }
            i++;
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.cloud_upload_server_port = ret;
            }
            i++;
            if ((i + 1) < argc) {
                snprintf(&p_content->setting.cloud_upload_tag[0], DMaxUrlLen, "%s", argv[i + 1]);
            }
            i++;
            p_content->setting.enable_cloud_upload = 1;
        } else {
            test_loge("unknown option[%d]: %s\n", i, argv[i]);
            return (-255);
        }
    }

    p_content->group_number = total_group_number ? total_group_number : (cur_group + 1);
    return 0;
}

static TInt __replace_enter(TChar *p_buffer, TUint size)
{
    while (size) {
        if (('\n') == (*p_buffer)) {
            *p_buffer = 0x0;
            return 0;
        }
        p_buffer ++;
        size --;
    }

    test_loge("no enter found, should be error\n");

    return 1;
}

void full_gragh(SSimpleAPIContxt *p_context, TUint channel_number)
{
    if (p_context && channel_number) {
        if (channel_number > DMaxChannelNumberInGroup) {
            LOG_ERROR("bad channel number %d\n", channel_number);
            return;
        }

        TUint index = 0;
        for (index = 0; index < DMaxGroupNumber; index ++) {
            p_context->group[index].source_2rd_channel_number = channel_number;
            p_context->group[index].source_channel_number = channel_number;
            p_context->group[index].sink_2rd_channel_number = channel_number;
            p_context->group[index].sink_channel_number = channel_number;
            p_context->group[index].streaming_2rd_channel_number = channel_number;
            p_context->group[index].streaming_channel_number = channel_number;
            p_context->group[index].cloud_channel_number = channel_number;
        }
    }
}

TInt mtest2_mudec_test(SSimpleAPIContxt *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TInt running = 1;

    CSimpleAPI *p_api = CSimpleAPI::Create(1);
    if (DUnlikely(!p_api)) {
        test_loge("CSimpleAPI::Create(1) fail\n");
        return (-11);
    }

    err = p_api->AssignContext(p_context);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("p_api->AssignContext() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-12);
    }

    err = p_api->StartRunning();
    if (DUnlikely(EECode_OK != err)) {
        test_loge("p_api->StartRunning() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-14);
    }

    //debug use
    IGenericEngineControlV2 *p_engine_control = p_api->QueryEngineControl();
    if (DUnlikely(NULL == p_engine_control)) {
        test_loge("p_api->QueryEngineControl() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-15);
    }

    //process user cmd line
    while (running) {
        //add sleep to avoid affecting the performance
        usleep(100000);
        //memset(buffer, 0x0, sizeof(buffer));
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
            TInt errcode = errno;
            if (errcode == EAGAIN) {
                continue;
            }
            test_loge("read STDIN_FILENO fail, errcode %d\n", errcode);
            continue;
        }

        test_log("[user cmd debug]: read input '%s'\n", buffer);

        if (buffer[0] == '\n') {
            p_buffer = buffer_old;
            test_log("repeat last cmd:\n\t\t%s\n", buffer_old);
        } else if (buffer[0] == 'l') {
            test_log("show last cmd:\n\t\t%s\n", buffer_old);
            continue;
        } else {

            ret = __replace_enter(buffer, (128 - 1));
            DASSERT(0 == ret);
            if (ret) {
                test_loge("no enter found\n");
                continue;
            }
            p_buffer = buffer;

            //record last cmd
            strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
            buffer_old[sizeof(buffer_old) - 1] = 0x0;
        }

        test_log("[user cmd debug]: '%s'\n", p_buffer);

        switch (p_buffer[0]) {

            case 'q':   // exit
                test_log("[user cmd]: 'q', Quit\n");
                running = 0;
                break;

            case 'p': {
                    if (!strncmp("pall", p_buffer, strlen("pall"))) {
                        test_log("print all\n");
                        p_engine_control->PrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
                    } else if (!strncmp("pdsp", p_buffer, strlen("pdsp"))) {
                        TU32 id = 0;
                        if (1 == sscanf(p_buffer, "pdsp:%u", &id)) {
                            if (id >= (p_context->setting.udec_instance_number + p_context->setting.udec_2rd_instance_number)) {
                                test_loge("dsp id(%d) exceed max value\n", id);
                                break;
                            }
                        } else {
                            id = 0;
                        }

                        err = p_engine_control->GenericControl(EGenericEngineQuery_VideoDSPStatus, (void *)&id);
                        DASSERT_OK(err);
                        if (EECode_OK != err) {
                            test_loge("EGenericEngineQuery_VideoDSPStatus id %d fail\n", id);
                            break;
                        }
                    }
                }
                break;

            case '-': {
                    TUint param1 = 0;
                    TUint param2 = 0;
                    if (!strncmp("--layout", p_buffer, strlen("--layout"))) {
                        if (2 == sscanf(p_buffer, "--layout %d %d", &param1, &param2)) {

                        } else if (1 == sscanf(p_buffer, "--layout %d", &param1)) {

                        } else {
                            break;
                        }

                        SUserParamType_UpdateDisplayLayout layout;

                        layout.check_field = EUserParamType_UpdateDisplayLayout;
                        layout.layout = param1;
                        layout.focus_stream_index = param2;

                        err = p_api->Control(EUserParamType_UpdateDisplayLayout, (void *)&layout);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_UpdateDisplayLayout(%d) fail, ret %d, %s\n", param1, err, gfGetErrorCodeString(err));
                            break;
                        }

                    } else if (!strncmp("--hideothers", p_buffer, strlen("--hideothers"))) {
                        SUserParamType_DisplayShowOtherWindows show;
                        show.check_field = EUserParamType_DisplayShowOtherWindows;
                        show.window_id = 0;
                        show.show_others = 0;

                        err = p_api->Control(EUserParamType_DisplayShowOtherWindows, (void *)&show);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayShowOtherWindows(hide) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--showothers", p_buffer, strlen("--showothers"))) {
                        SUserParamType_DisplayShowOtherWindows show;
                        show.check_field = EUserParamType_DisplayShowOtherWindows;
                        show.window_id = 0;
                        show.show_others = 1;

                        err = p_api->Control(EUserParamType_DisplayShowOtherWindows, (void *)&show);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayShowOtherWindows(show) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--switchgroup", p_buffer, strlen("--switchgroup"))) {
                        SUserParamType_SwitchGroup group;

                        if (1 == sscanf(p_buffer, "--switchgroup %d", &param1)) {
                        } else {
                            test_loge("--switchgroup should follow by a integer\n");
                            break;
                        }

                        group.check_field = EUserParamType_SwitchGroup;
                        group.group_id = param1;

                        err = p_api->Control(EUserParamType_SwitchGroup, (void *)&group);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_SwitchGroup(%d) fail, ret %d, %s\n", group.group_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--addgroupch4", p_buffer, strlen("--addgroupch4"))) {
                        SUserParamType_AddGroup group;
                        if (9 == sscanf(p_buffer, "--addgroupch4 %u -f %s -f  %s -f  %s -f  %s --hd  -f  %s -f  %s -f  %s -f  %s",
                                        &group.group_id,
                                        group.source_url_2rd[0],
                                        group.source_url_2rd[1],
                                        group.source_url_2rd[2],
                                        group.source_url_2rd[3],
                                        group.source_url[0],
                                        group.source_url[1],
                                        group.source_url[2],
                                        group.source_url[3])) {
                        } else {
                            test_loge("--addgroupch4 should followed by group_id and 8 source urls\n");
                            break;
                        }
                        group.source_channel_number = group.source_2rd_channel_number = 4;
                        group.check_field = EUserParamType_AddGroup;
                        err = p_api->Control(EUserParamType_AddGroup, (void *)&group);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_AddGroup fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        }
                        //tmp flow test
                        /*group.check_field = EUserParamType_SwitchGroup;
                        group.group_id = 1;

                        err = p_api->Control(EUserParamType_SwitchGroup, (void *)&group);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_SwitchGroup(%d) fail, ret %d, %s\n", group.group_id, err, gfGetErrorCodeString(err));
                            break;
                        }*/
                    } else if (!strncmp("--zoomin", p_buffer, strlen("--zoomin"))) {
                        SUserParamType_DisplayZoom zoom;

                        if (1 == sscanf(p_buffer, "--zoomin %d", &param1)) {
                        }

                        zoom.check_field = EUserParamType_DisplayZoom;
                        zoom.window_id = param1;

                        zoom.zoom_factor = 1.25;
                        zoom.gesture_offset_x = 0;
                        zoom.gesture_offset_y = 0;

                        err = p_api->Control(EUserParamType_DisplayZoom, (void *)&zoom);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayZoom(zoom in)(%d) fail, ret %d, %s\n", zoom.window_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--zoomout", p_buffer, strlen("--zoomout"))) {
                        SUserParamType_DisplayZoom zoom;

                        if (1 == sscanf(p_buffer, "--zoomout %d", &param1)) {
                        }

                        zoom.check_field = EUserParamType_DisplayZoom;
                        zoom.window_id = param1;

                        zoom.zoom_factor = 0.8;
                        zoom.gesture_offset_x = 0;
                        zoom.gesture_offset_y = 0;

                        err = p_api->Control(EUserParamType_DisplayZoom, (void *)&zoom);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayZoom(zoom out)(%d) fail, ret %d, %s\n", zoom.window_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--up", p_buffer, strlen("--up"))) {
                        SUserParamType_DisplayZoom zoom;

                        if (1 == sscanf(p_buffer, "--up %d", &param1)) {
                        }

                        zoom.check_field = EUserParamType_DisplayZoom;
                        zoom.window_id = param1;

                        zoom.zoom_factor = 1.0;
                        zoom.gesture_offset_x = 0;
                        zoom.gesture_offset_y = -0.2;

                        err = p_api->Control(EUserParamType_DisplayZoom, (void *)&zoom);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayZoom(up)(%d) fail, ret %d, %s\n", zoom.window_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--down", p_buffer, strlen("--down"))) {
                        SUserParamType_DisplayZoom zoom;

                        if (1 == sscanf(p_buffer, "--down %d", &param1)) {
                        }

                        zoom.check_field = EUserParamType_DisplayZoom;
                        zoom.window_id = param1;

                        zoom.zoom_factor = 1.0;
                        zoom.gesture_offset_x = 0;
                        zoom.gesture_offset_y = 0.2;

                        err = p_api->Control(EUserParamType_DisplayZoom, (void *)&zoom);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayZoom(down)(%d) fail, ret %d, %s\n", zoom.window_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--left", p_buffer, strlen("--left"))) {
                        SUserParamType_DisplayZoom zoom;

                        if (1 == sscanf(p_buffer, "--left %d", &param1)) {
                        }

                        zoom.check_field = EUserParamType_DisplayZoom;
                        zoom.window_id = param1;

                        zoom.zoom_factor = 1.0;
                        zoom.gesture_offset_x = -0.2;
                        zoom.gesture_offset_y = 0;

                        err = p_api->Control(EUserParamType_DisplayZoom, (void *)&zoom);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayZoom(left)(%d) fail, ret %d, %s\n", zoom.window_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--right", p_buffer, strlen("--right"))) {
                        SUserParamType_DisplayZoom zoom;

                        if (1 == sscanf(p_buffer, "--right %d", &param1)) {
                        }

                        zoom.check_field = EUserParamType_DisplayZoom;
                        zoom.window_id = param1;

                        zoom.zoom_factor = 1.0;
                        zoom.gesture_offset_x = 0.2;
                        zoom.gesture_offset_y = 0;

                        err = p_api->Control(EUserParamType_DisplayZoom, (void *)&zoom);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_DisplayZoom(right)(%d) fail, ret %d, %s\n", zoom.window_id, err, gfGetErrorCodeString(err));
                            break;
                        }
                    } else if (!strncmp("--speak", p_buffer, strlen("--speak"))) {
                        SUserParamType_AudioStopListenFrom stop;
                        stop.check_field = EUserParamType_AudioStopListenFrom;
                        err = p_api->Control(EUserParamType_AudioStopListenFrom, (void *)&stop);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_AudioStopListenFrom fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        }

                        SUserParamType_AudioStartTalkTo start;
                        start.check_field = EUserParamType_AudioStartTalkTo;
                        err = p_api->Control(EUserParamType_AudioStartTalkTo, (void *)&start);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_AudioStartTalkTo fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        }
                    } else if (!strncmp("--listen", p_buffer, strlen("--listen"))) {
                        SUserParamType_AudioStopTalkTo stop;
                        stop.check_field = EUserParamType_AudioStopTalkTo;
                        err = p_api->Control(EUserParamType_AudioStopTalkTo, (void *)&stop);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_AudioStopTalkTo fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        }

                        SUserParamType_AudioStartListenFrom start;
                        start.check_field = EUserParamType_AudioStartListenFrom;
                        err = p_api->Control(EUserParamType_AudioStartListenFrom, (void *)&start);
                        if (EECode_OK != err) {
                            test_loge("EUserParamType_AudioStartListenFrom fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        }
                    }
                }
                break;

            case 'c': {
                    TUint id = 0;
                    if (!strncmp("ccjpeg:", p_buffer, strlen("ccjpeg:"))) {
                        if (EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding == p_context->setting.work_mode) {
                            test_loge("Still capture not supported in transcoding mode.\n");
                            break;
                        }
                        TUint cap_index = 0;
                        SUserParamType_PlaybackCapture capture;
                        memset(&capture, 0x0, sizeof(capture));

                        if (2 == sscanf(p_buffer, "ccjpeg:%x %x", &id, &cap_index)) {
                            SUserParamType_QueryStatus query_status;

                            query_status.check_field = EUserParamType_QueryStatus;

                            err = p_api->Control(EUserParamType_QueryStatus, (void *)&query_status);
                            if (EECode_OK != err) {
                                test_loge("EUserParamType_QueryStatus fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                break;
                            }

                            //test_log("enc_w=%u, enc_h=%u, vout_w=%u, vout_h=%u, total_window_number=%u, current_display_window_number=%u\n", query_status.enc_w, query_status.enc_h, query_status.vout_w, query_status.vout_h, query_status.total_window_number, query_status.current_display_window_number);
                            if (0 == query_status.current_display_window_number) {
                                test_loge("EUserParamType_PlaybackCapture invalid current_display_window_number=%u\n", query_status.current_display_window_number);
                                break;
                            }
                            /*for (int i=0; i<query_status.current_display_window_number; i++) {
                                test_log("window[%d] udec_idx=%u, render_idx=%u, x=%u, y=%u, w=%u, h=%u, source_w=%u, source_h=%u\n", i, query_status.window[i].udec_index, query_status.window[i].render_index, query_status.window[i].window_pos_x, query_status.window[i].window_pos_y, query_status.window[i].window_width, query_status.window[i].window_height, query_status.window[i].source_width, query_status.window[i].source_height);
                            }*/

                            if (1 == query_status.current_display_window_number) { //single view - HD mode
                                if (id >= p_context->group[0].source_2rd_channel_number
                                        && id < p_context->group[0].source_2rd_channel_number + p_context->group[0].source_channel_number) {
                                    capture.check_field = EUserParamType_PlaybackCapture;

                                    capture.capture_screennail = 1;
                                    capture.screennail_width = query_status.window[0].window_width;
                                    capture.screennail_height = query_status.window[0].window_height;
                                    capture.capture_thumbnail = 1;
                                    capture.capture_coded = 1;
                                    capture.coded_width = query_status.window[0].source_width;
                                    capture.coded_height = query_status.window[0].source_height;
                                    capture.file_index = cap_index;
                                    capture.channel_id = query_status.window[0].udec_index;
                                    capture.source_id = id;
                                } else {
                                    test_loge("EUserParamType_PlaybackCapture invalid id=%u for HD mode.\n", id);
                                    break;
                                }
                            } else {//multi view - SD mode
                                if (0xff == id) {
                                    capture.check_field = EUserParamType_PlaybackCapture;

                                    capture.capture_screennail = 1;
                                    capture.screennail_width = query_status.vout_w;
                                    capture.screennail_height = query_status.vout_h;
                                    capture.capture_thumbnail = 1;
                                    capture.capture_coded = 1;
                                    capture.coded_width = query_status.enc_w;
                                    capture.coded_height = query_status.enc_h;
                                    capture.file_index = cap_index;
                                    capture.channel_id = id;
                                    capture.source_id = id;
                                } else if (id < p_context->group[0].source_2rd_channel_number) {
                                    capture.check_field = EUserParamType_PlaybackCapture;

                                    capture.capture_screennail = 1;
                                    capture.screennail_width = query_status.window[id].window_width;
                                    capture.screennail_height = query_status.window[id].window_height;
                                    capture.capture_thumbnail = 1;
                                    capture.capture_coded = 1;
                                    capture.coded_width = query_status.window[id].source_width;
                                    capture.coded_height = query_status.window[id].source_height;
                                    capture.file_index = cap_index;
                                    capture.channel_id = id;
                                    capture.source_id = id;
                                } else {
                                    test_loge("EUserParamType_PlaybackCapture invalid id=%u for SD mode\n", id);
                                    break;
                                }
                            }

                            err = p_api->Control(EUserParamType_PlaybackCapture, (void *)&capture);
                            if (EECode_OK != err) {
                                test_loge("EUserParamType_PlaybackCapture fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                break;
                            }

                        }
                    } else if (!strncmp("cbitrate:", p_buffer, strlen("cbitrate:"))) {
                        SUserParamType_VideoTranscoderUpdateBitrate bitrate;
                        if (1 == sscanf(p_buffer, "cbitrate:%d", &id)) {
                            bitrate.check_field = EUserParamType_VideoTranscoderUpdateBitrate;
                            bitrate.bitrate = id * 1024;
                            err = p_api->Control(EUserParamType_VideoTranscoderUpdateBitrate, &bitrate);
                            if (DUnlikely(EECode_OK != err)) {
                                test_loge("EUserParamType_VideoTranscoderUpdateBitrate(%d K) fail, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                                break;
                            }
                        } else {
                            test_loge("'cbitrate:%%d' should specify video bitrate %%d K\n");
                        }
                    } else if (!strncmp("cframerate:", p_buffer, strlen("cframerate:"))) {
                        SUserParamType_VideoTranscoderUpdateFramerate framerate;
                        if (1 == sscanf(p_buffer, "cframerate:%d", &id)) {
                            framerate.check_field = EUserParamType_VideoTranscoderUpdateFramerate;
                            framerate.framerate = id;
                            err = p_api->Control(EUserParamType_VideoTranscoderUpdateFramerate, &framerate);
                            if (DUnlikely(EECode_OK != err)) {
                                test_loge("EUserParamType_VideoTranscoderUpdateFramerate(%d) fail, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                                break;
                            }
                        } else {
                            test_loge("'cframerate:%%d' should specify video framerate %%d fps\n");
                        }
                    }
                }
                break;

            case ' ': {
                    TU32 param1 = 0;
                    if (1 == sscanf(p_buffer, " %d", &param1)) {
                    }

                    SUserParamType_PlaybackTrickplay trickplay;
                    memset(&trickplay, 0x0, sizeof(trickplay));

                    trickplay.check_field = EUserParamType_PlaybackTrickplay;
                    trickplay.trickplay = SUserParamType_PlaybackTrickplay_PauseResume;
                    trickplay.index = param1;

                    err = p_api->Control(EUserParamType_PlaybackTrickplay, &trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        test_loge("EUserParamType_PlaybackTrickplay(pause resume)(%d) fail, ret %d, %s\n", trickplay.index, err, gfGetErrorCodeString(err));
                        break;
                    }
                }
                break;

            case 's': {
                    TU32 param1 = 0;
                    if (1 == sscanf(p_buffer, "s%d", &param1)) {
                    }

                    SUserParamType_PlaybackTrickplay trickplay;
                    memset(&trickplay, 0x0, sizeof(trickplay));

                    trickplay.check_field = EUserParamType_PlaybackTrickplay;
                    trickplay.trickplay = SUserParamType_PlaybackTrickplay_Step;
                    trickplay.index = param1;

                    err = p_api->Control(EUserParamType_PlaybackTrickplay, &trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        test_loge("EUserParamType_PlaybackTrickplay(step)(%d) fail, ret %d, %s\n", trickplay.index, err, gfGetErrorCodeString(err));
                        break;
                    }
                }
                break;

            case 'h':   // help
                _mtest2_print_version();
                _mtest2_print_usage();
                _mtest2_print_runtime_usage();
                break;

            default:
                break;
        }
    }

    err = p_api->ExitRunning();
    if (DUnlikely(EECode_OK != err)) {
        test_loge("p_api->ExitRunning() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }

    p_api->Destroy();
    p_api = NULL;

    return 0;
}

static void __mtest2_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static void _mtest2_default_initial_setting(SSimpleAPIContxt *p_content)
{
    p_content->setting.voutmask = DCAL_BITMASK(EDisplayDevice_HDMI);

    p_content->current_group_index = 0;
    p_content->is_vod_enabled = 0;
    p_content->is_vod_ready = 0;

    p_content->setting.net_receiver_scheduler_number = 1;
    //p_content->setting.fileio_writer_scheduler_number = 1;

    p_content->setting.recording_group_number = 0;
    p_content->setting.streaming_group_number = 0;

    p_content->setting.dsp_tilemode = 5;

    p_content->setting.udec_not_feed_pts = 1;
    p_content->setting.debug_prefer_udec_stop_flag = 1;

    p_content->setting.enable_audio_capture = 0;
    p_content->setting.push_audio_type = 1;
    p_content->setting.enable_remote_audio_forwarding = 1;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret;
    TUint i = 0;

    //AutoDiscoveryExample example;

    SSimpleAPIContxt *p_content = (SSimpleAPIContxt *) malloc(sizeof(SSimpleAPIContxt));
    if (DUnlikely(!p_content)) {
        test_loge("can not malloc mem(SSimpleAPIContxt), request size %zu\n", sizeof(SSimpleAPIContxt));
        goto mtest2_main_exit;
    }
    memset(p_content, 0x0, sizeof(SSimpleAPIContxt));
    _mtest2_default_initial_setting(p_content);

    signal(SIGPIPE,  __mtest2_sig_pipe);

    if ((ret = mtest2_init_params(p_content, argc, argv)) < 0) {
        test_loge("mtest: mtest2_init_params fail, return %d.\n", ret);
        goto mtest2_main_exit;
    } else if (ret > 0) {
        goto mtest2_main_exit;
    }

    switch (p_content->setting.playback_mode) {

        case EPlaybackMode_1x1080p:
        case EPlaybackMode_1x3M:
            p_content->setting.udec_2rd_instance_number = 1;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 1;
            p_content->have_dual_stream = 0;
            break;

        case EPlaybackMode_1x1080p_4xD1:
        case EPlaybackMode_1x3M_4xD1:
            p_content->setting.udec_2rd_instance_number = 4;
            p_content->setting.udec_instance_number = 1;
            p_content->channel_number_per_group = 4;
            p_content->have_dual_stream = 1;
            break;

        case EPlaybackMode_1x1080p_6xD1:
        case EPlaybackMode_1x3M_6xD1:
            p_content->setting.udec_2rd_instance_number = 6;
            p_content->setting.udec_instance_number = 1;
            p_content->channel_number_per_group = 6;
            p_content->have_dual_stream = 1;
            break;

        case EPlaybackMode_1x1080p_8xD1:
        case EPlaybackMode_1x3M_8xD1:
            p_content->setting.udec_2rd_instance_number = 8;
            p_content->setting.udec_instance_number = 1;
            p_content->channel_number_per_group = 8;
            p_content->have_dual_stream = 1;
            break;

        case EPlaybackMode_1x1080p_9xD1:
        case EPlaybackMode_1x3M_9xD1:
            p_content->setting.udec_2rd_instance_number = 9;
            p_content->setting.udec_instance_number = 1;
            p_content->channel_number_per_group = 9;
            p_content->have_dual_stream = 1;
            break;

        case EPlaybackMode_4x720p:
            p_content->setting.udec_2rd_instance_number = 4;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 4;
            p_content->have_dual_stream = 0;
            break;

        case EPlaybackMode_4xD1:
            p_content->setting.udec_2rd_instance_number = 4;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 4;
            p_content->have_dual_stream = 0;
            break;

        case EPlaybackMode_6xD1:
            p_content->setting.udec_2rd_instance_number = 6;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 6;
            p_content->have_dual_stream = 0;
            break;

        case EPlaybackMode_8xD1:
            p_content->setting.udec_2rd_instance_number = 8;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 8;
            p_content->have_dual_stream = 0;
            break;

        case EPlaybackMode_9xD1:
            p_content->setting.udec_2rd_instance_number = 9;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 9;
            p_content->have_dual_stream = 0;
            break;

        case ENoPlaybackInstance:
            p_content->setting.udec_2rd_instance_number = 0;
            p_content->setting.udec_instance_number = 0;
            p_content->channel_number_per_group = 4;//hard code here
            p_content->have_dual_stream = 0;
            break;

        default:
            test_loge("[ut flow]: not set p_content->setting.playback_mode %d\n", p_content->setting.playback_mode);
            goto mtest2_main_exit;

    }

    //hard code here
    if (p_content && (!p_content->setting.debug_not_build_full_gragh) && p_content->setting.udec_2rd_instance_number) {
        full_gragh(p_content, p_content->setting.udec_2rd_instance_number);
    }

    switch (p_content->setting.work_mode) {

        case EMediaDeviceWorkingMode_MultipleInstancePlayback:
            p_content->setting.enable_playback = 1;
            //p_content->setting.enable_recording = 1;
            p_content->setting.enable_streaming_server = 1;
            p_content->setting.enable_cloud_server = 1;

            if (DLikely(p_content->group_number)) {
                p_content->setting.playback_group_number = p_content->group_number;
                p_content->setting.cloud_group_number = p_content->group_number;
                if (p_content->setting.enable_recording) {
                    if (!p_content->setting.recording_group_number) {
                        p_content->setting.recording_group_number = p_content->group_number;
                    }
                }

                for (i = 0; i < p_content->setting.playback_group_number; i++) {
                    p_content->group[i].enable_playback = 1;
                }
                for (i = 0; i < p_content->setting.recording_group_number; i++) {
                    p_content->group[i].enable_recording = 1;
                }
                for (i = 0; i < p_content->setting.streaming_group_number; i++) {
                    p_content->group[i].enable_streaming = 1;
                }
                for (i = 0; i < p_content->setting.cloud_group_number; i++) {
                    p_content->group[i].enable_cloud_agent = 1;
                }
            }

            test_log("[ut flow]: before mudec_test(), ret %d, [no transcoding]\n", ret);
            ret = mtest2_mudec_test(p_content);
            test_log("[ut flow]: mtest2_mudec_test end, ret %d\n", ret);
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding:
            p_content->setting.enable_playback = 1;
            //p_content->setting.enable_recording = 1;
            p_content->setting.enable_trasncoding = 1;
            p_content->setting.enable_streaming_server = 1;
            p_content->setting.enable_cloud_server = 1;
            if (p_content->setting.disable_audio_path) {
                p_content->setting.enable_audio_pinmuxer = 0;
            } else {
                p_content->setting.enable_audio_pinmuxer = 1;
            }

            if (DLikely(p_content->group_number)) {
                p_content->setting.playback_group_number = p_content->group_number;
                if (p_content->setting.enable_recording) {
                    if (!p_content->setting.recording_group_number) {
                        p_content->setting.recording_group_number = p_content->group_number;
                    }
                }
                //p_content->setting.streaming_group_number = p_content->group_number;
                p_content->setting.cloud_group_number = p_content->group_number;
                TUint i = 0;
                for (i = 0; i < p_content->setting.playback_group_number; i++) {
                    p_content->group[i].enable_playback = 1;
                }
                for (i = 0; i < p_content->setting.recording_group_number; i++) {
                    p_content->group[i].enable_recording = 1;
                }
                for (i = 0; i < p_content->setting.streaming_group_number; i++) {
                    p_content->group[i].enable_streaming = 1;
                }
                for (i = 0; i < p_content->setting.cloud_group_number; i++) {
                    p_content->group[i].enable_cloud_agent = 1;
                }
            }
            test_log("[ut flow]: before mudec_test(), ret %d, [enable trancoding]\n", ret);
            ret = mtest2_mudec_test(p_content);
            test_log("[ut flow]: mudec_test() end, ret %d, [enable trancoding]\n", ret);
            break;

        case EMediaDeviceWorkingMode_NVR_RTSP_Streaming:
            p_content->setting.enable_playback = 0;
            //p_content->setting.enable_recording = 1;
            p_content->setting.enable_streaming_server = 1;
            p_content->setting.enable_cloud_server = 1;
            p_content->setting.disable_dsp_related = 1;

            if (DLikely(p_content->group_number)) {
                p_content->setting.playback_group_number = p_content->group_number;
                if (p_content->setting.enable_recording) {
                    if (!p_content->setting.recording_group_number) {
                        p_content->setting.recording_group_number = p_content->group_number;
                    }
                }
                p_content->setting.streaming_group_number = p_content->group_number;
                p_content->setting.cloud_group_number = p_content->group_number;
                TUint i = 0;
                for (i = 0; i < p_content->setting.playback_group_number; i++) {
                    p_content->group[i].enable_playback = 1;
                }
                for (i = 0; i < p_content->setting.recording_group_number; i++) {
                    p_content->group[i].enable_recording = 1;
                }
                for (i = 0; i < p_content->setting.streaming_group_number; i++) {
                    p_content->group[i].enable_streaming = 1;
                    p_content->group[i].enable_streaming_relay = 1;
                }
                for (i = 0; i < p_content->setting.cloud_group_number; i++) {
                    p_content->group[i].enable_cloud_agent = 1;
                }
            }

            test_log("[ut flow]: before mudec_test(), [generic NVR], ret %d\n", ret);
            ret = mtest2_mudec_test(p_content);
            test_log("[ut flow]: mtest2_mudec_test end, ret %d\n", ret);
            break;

        default:
            test_loge("BAD workmode %d\n", p_content->setting.work_mode);
            goto mtest2_main_exit;
    }

    test_log("[ut flow]: test end\n");

mtest2_main_exit:

    if (p_content) {
        free(p_content);
        p_content = NULL;
    }

    return 0;
}

