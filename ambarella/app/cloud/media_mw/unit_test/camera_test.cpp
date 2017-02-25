/*******************************************************************************
 * camera_test.cpp
 *
 * History:
 *    2014/01/07 - [Zhi He] create file
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

#define DCameraTestStringMaxLength 256

typedef struct {
    TChar cloud_server_addr[DCameraTestStringMaxLength];
    TChar cloud_tag[DCameraTestStringMaxLength];

    TU16 server_port;
    TU8 b_play_raw;
    TU8 audio_format;// 1: aac, 2, customized adpcm

    TU8 loglevel;
    TU8 preset_loglevel;
    TU8 preferpulseaudio;
    TU8 asdemo;

    TU8 enablewsd;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
} SCameraTestContext;

void _camera_test_print_version()
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

void _camera_test_print_usage()
{
    printf("\ncamera_test options:\n");
    printf("\t'--connect [server_addr]': specify cloud server addr\n");
    printf("\t'--port [server_port]': specify cloud server port\n");
    printf("\t'--cloudtag [cloud_tag]': specify cloud tag name\n");
    printf("\t'--loglevel [%%d]': set log level\n");
    printf("\t'--playraw': play raw pcm\n");
    printf("\t'--playaac': play aac\n");
    printf("\t'--playadpcm': play adpcm\n");
    printf("\t'--asdemo': run as demo");
    printf("\t'--enablewsd': enable wsdObserver");
}

void _camera_test_print_runtime_usage()
{
    printf("\ncamera_test runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
}

TInt camera_test_init_params(SCameraTestContext *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;

    if (argc < 2) {
        _camera_test_print_version();
        _camera_test_print_usage();
        _camera_test_print_runtime_usage();
        return 1;
    }

    //default settings
    p_content->server_port = DDefaultSACPServerPort;
    p_content->b_play_raw = 0;
    p_content->audio_format = 2;
    p_content->preferpulseaudio = 1;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--connect", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->cloud_server_addr[0], DCameraTestStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--connect', should follow with server url(%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--cloudtag", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->cloud_tag[0], DCameraTestStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--cloudtag', should follow with cloudtag(%%s), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        } else if (!strcmp("--port", argv[i])) {
            if ((i + 1) < argc) {
                if (1 == sscanf(argv[i + 1], "%d", &ret)) {
                    p_content->server_port = ret;
                }
            } else {
                test_loge("[input argument error]: '--port', should follow with server_port(%%d), argc %d, i %d.\n", argc, i);
                return (-3);
            }
            i ++;
        } else if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > ELogLevel_Verbose) || (ret < ELogLevel_Fatal)) {
                    test_loge("[input argument error]: '--loglevel', ret(%d) should between %d to %d.\n", ret, ELogLevel_Fatal, ELogLevel_Verbose);
                    return (-9);
                } else {
                    p_content->preset_loglevel = 1;
                    p_content->loglevel = ret;
                    const TChar *p_log_level_string = NULL;
                    gfGetLogLevelString((ELogLevel)p_content->loglevel, p_log_level_string);
                    test_log("[input argument]: '--loglevel': (%d) (%s).\n", p_content->loglevel, p_log_level_string);
                }
            } else {
                test_loge("[input argument error]: '--loglevel', should follow with integer(loglevel), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--playraw", argv[i])) {
            test_log("[input argument]: '--playraw', will play raw pcm.\n");
            p_content->b_play_raw = 1;
        } else if (!strcmp("--playaac", argv[i])) {
            test_log("[input argument]: '--playaac', will play aac.\n");
            p_content->b_play_raw = 0;
            p_content->audio_format = 1;
        } else if (!strcmp("--playadpcm", argv[i])) {
            test_log("[input argument]: '--playdpcm', will play customized adpcm.\n");
            p_content->b_play_raw = 0;
            p_content->audio_format = 2;
        } else if (!strcmp("--prefer-pulseaudio-renderer", argv[i])) {
            test_log("[input argument]: '--prefer-pulseaudio-renderer', will use pulse audio to render audio.\n");
            p_content->preferpulseaudio = 1;
        } else if (!strcmp("--asdemo", argv[i])) {
            test_log("[input argument]: '--asdemo', run camera_test as one demo.\n");
            p_content->asdemo = 1;
        } else if (!strcmp("--enablewsd", argv[i])) {
            test_log("[input argument]: '--enablewsd', will enable wsd Observer.\n");
            p_content->enablewsd = 1;
        } else {
            test_loge("unknown option[%d]: %s\n", i, argv[i]);
            return (-255);
        }
    }

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

TInt camera_test(SCameraTestContext *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TInt running = 1;

    IGenericEngineControlV2 *p_api = gfMediaEngineFactoryV2(EFactory_MediaEngineType_Generic);

    if (DUnlikely(!p_api)) {
        test_loge("gfMediaEngineFactoryV2() fail\n");
        return (-1);
    }

    TGenericID source_id = 0;
    TGenericID audio_decoder_id = 0;
    TGenericID audio_renderer_id = 0;
    TGenericID connection_id = 0;
    TGenericID playback_pipeline_id = 0;

    if (1 == p_context->preset_loglevel) {
        err = p_api->SetEngineLogLevel((ELogLevel)p_context->loglevel);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetEngineLogLevel(%d), err %d, %s\n", p_context->loglevel, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //default settings
    SGenericPreConfigure pre_config;

    if (1 == p_context->audio_format) {
        memset(&pre_config, 0x0, sizeof(SGenericPreConfigure));
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC;
        pre_config.number = 0;
        err = p_api->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else if (2 == p_context->audio_format) {
        memset(&pre_config, 0x0, sizeof(SGenericPreConfigure));
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM;
        pre_config.number = 0;
        err = p_api->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        DASSERT(0 == p_context->audio_format);
    }

    if (1 == p_context->preferpulseaudio) {
        memset(&pre_config, 0x0, sizeof(SGenericPreConfigure));
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioRenderer_PulseAudio;
        pre_config.number = 0;
        err = p_api->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioRenderer_PulseAudio, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioRenderer_PulseAudio), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    err = p_api->BeginConfigProcessPipeline();
    if (DUnlikely(EECode_OK != err)) {
        test_loge("BeginConfigProcessPipeline() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-2);
    }

    err = p_api->NewComponent(EGenericComponentType_CloudConnecterClientAgent, source_id);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("NewComponent(EGenericComponentType_CloudConnecterClientAgent) fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-3);
    }

    err = p_api->SetCloudClientTag(source_id, p_context->cloud_tag, p_context->cloud_server_addr, p_context->server_port);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("SetCloudClientTag() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-4);
    }

    if (!p_context->b_play_raw) {
        err = p_api->NewComponent(EGenericComponentType_AudioDecoder, audio_decoder_id);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("NewComponent(EGenericComponentType_AudioDecoder) fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return (-8);
        }

        err = p_api->NewComponent(EGenericComponentType_AudioRenderer, audio_renderer_id);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("NewComponent(EGenericComponentType_AudioRenderer) fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return (-5);
        }

        err = p_api->ConnectComponent(connection_id, source_id, audio_decoder_id,  StreamType_Audio);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("ConnectComponent() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return (-6);
        }

        err = p_api->ConnectComponent(connection_id, audio_decoder_id, audio_renderer_id,  StreamType_Audio);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("ConnectComponent() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return (-6);
        }
    } else {
        err = p_api->NewComponent(EGenericComponentType_AudioRenderer, audio_renderer_id);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("NewComponent(EGenericComponentType_AudioRenderer) fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return (-5);
        }

        err = p_api->ConnectComponent(connection_id, source_id, audio_renderer_id,  StreamType_Audio);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("ConnectComponent() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return (-6);
        }
    }

    err = p_api->SetupPlaybackPipeline(playback_pipeline_id, 0, source_id, 0, audio_decoder_id, 0, audio_renderer_id, 0, 1);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("SetupPlaybackPipeline() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-7);
    }

    err = p_api->FinalizeConfigProcessPipeline();
    if (DUnlikely(EECode_OK != err)) {
        test_loge("FinalizeConfigProcessPipeline() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-8);
    }

    err = p_api->StartProcess();
    if (DUnlikely(EECode_OK != err)) {
        test_loge("StartProcess() fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-9);
    }

    //process user cmd line
    while (running) {
        //add sleep to avoid affecting the performance
        usleep(500000);
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
                        p_api->PrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
                    }
                }
                break;

            case 'h':   // help
                _camera_test_print_version();
                _camera_test_print_usage();
                _camera_test_print_runtime_usage();
                break;

            default:
                break;
        }
    }

    err = p_api->StopProcess();
    if (DUnlikely(EECode_OK != err)) {
        test_loge("p_api->StopProcess() fail, err %d, %s\n", err, gfGetErrorCodeString(err));
    }

    p_api->Destroy();
    p_api = NULL;

    return 0;
}

static void __camera_test_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static TInt _camera_test_default_initial_setting(SCameraTestContext *p_content)
{
    DASSERT(p_content);

    FILE *file = fopen("/etc/camera_cloud.conf", "rt");

    if (!file) {
        test_log("no [/etc/camera_cloud.conf] file\n");
        return 0;
    }

    //default settings
    p_content->server_port = DDefaultSACPServerPort;
    p_content->b_play_raw = 0;
    p_content->audio_format = 2;
    p_content->preferpulseaudio = 1;

    TUint length = 0;
    fgets(&p_content->cloud_server_addr[0], DCameraTestStringMaxLength, file);
    length = strlen(p_content->cloud_server_addr);
    DASSERT((length + 1) < DCameraTestStringMaxLength);
    if (p_content->cloud_server_addr[length - 1] == '\n') {
        p_content->cloud_server_addr[length - 1] = 0;
    } else {
        test_log("%02x %02x", '\n', p_content->cloud_server_addr[length - 1]);
    }
    fgets(&p_content->cloud_tag[0], DCameraTestStringMaxLength, file);
    length = strlen(p_content->cloud_tag);
    DASSERT((length + 1) < DCameraTestStringMaxLength);
    if (p_content->cloud_tag[length - 1] == '\n') {
        p_content->cloud_tag[length - 1] = 0;
    } else {
        test_log("%02x %02x", '\n', p_content->cloud_tag[length - 1]);
    }
    p_content->server_port = DDefaultSACPServerPort;

    TChar playraw_string[64] = {0};

    fgets(&playraw_string[0], 64, file);
    length = 63;
    if (playraw_string[63] == '\n') {
        playraw_string[63] = 0;
    } else {
        test_log("%02x %02x", '\n', playraw_string[63]);
    }

    if (!strcmp(playraw_string, "audio play raw")) {
        p_content->b_play_raw = 1;
    } else {
        test_log("playraw_string %s", playraw_string);
        p_content->b_play_raw = 0;
    }

    return 1;
}

static TInt _camera_test_save_setting(SCameraTestContext *p_content)
{
    DASSERT(p_content);
    TChar byte = '\n';

    FILE *file = fopen("/etc/camera_cloud.conf", "wt");

    if (!file) {
        test_loge("can not open [/etc/camera_cloud.conf] file\n");
        return 0;
    }
    test_log("write cloud_server_addr: %s\n", p_content->cloud_server_addr);
    fputs(&p_content->cloud_server_addr[0], file);
    fwrite(&byte, 1, 1, file);
    test_log("write cloud_tag: %s\n", p_content->cloud_server_addr);
    fputs(&p_content->cloud_tag[0], file);
    fwrite(&byte, 1, 1, file);

    if (p_content->b_play_raw) {
        TChar playraw_string[64] = "audio play raw";
        test_log("write config, %s\n", playraw_string);
        fputs(playraw_string, file);
        fwrite(&byte, 1, 1, file);
    } else {
        TChar playraw_string[64] = "audio play aac";
        test_log("write config, %s\n", playraw_string);
        fputs(playraw_string, file);
        fwrite(&byte, 1, 1, file);
    }

    fclose(file);

    return 1;
}
#if 0
//------------------------------------------------------------------------------
//added for auto-discovery
//------------------------------------------------------------------------------
class DeviceObserver: public IWsdObserver
{
public:
    virtual void onDeviceChanged(char *uuid, char *ipaddr, char *stream_url) {
        printf("DeviceObserver::onDeviceChanged,uuid[%s],ipaddr[%s],stream_url[%s]\n", uuid, ipaddr ? ipaddr : "NULL", stream_url ? stream_url : "NULL");
        if (ipaddr) {
            int index;
            int ret = add_device(uuid, &index);
            if (ret < 0) {
                return;
            }
            if (!stream_url) {
                return;//stream_url not got yet.
            }
            if (ret) {
                printf("duplicate, DO remove_device first\n");
            }
            printf("DO add_device\n");
        } else {
            //delete source
            int index;
            int ret = remove_device(uuid, &index);
            if (ret < 0) { return; }
            printf("DO remove_device\n");
        }
    }
public:
    DeviceObserver(int max_num = 4): device_list_(NULL), device_num_(0), max_device_num_(max_num) {
        pthread_mutex_init(&device_mutex_, NULL);
    }
    ~DeviceObserver() {
        free_device_list();
        device_num_ = 0;
        pthread_mutex_destroy(&device_mutex_);
    }
private:
    int get_index(int mask) {
        int i;
        for (i = 0; i < max_device_num_; i++) {
            if (!(mask & (1 << i)))
            { return i; }
        }
        return -1;
    }
    int free_device_list() {
        pthread_mutex_lock(&device_mutex_);
        while (device_list_) {
            device_node_t *next = device_list_->next;
            delete device_list_;
            device_list_ = next;
        }
        pthread_mutex_unlock(&device_mutex_);
        return 0;
    }
    int add_device(char *uuid, int *index) {
        int index_mask = 0;
        pthread_mutex_lock(&device_mutex_);
        device_node_t *node = device_list_;
        while (node) {
            if (!strcmp(node->uuid, uuid)) {
                *index = node->index;
                pthread_mutex_unlock(&device_mutex_);
                return 1;//duplicate
            }
            index_mask |= 1 << node->index;
            node = node->next;
        }
        if (device_num_ >= max_device_num_) {
            printf("Too many devices, now max %d devices supported\n", max_device_num_);
            pthread_mutex_unlock(&device_mutex_);
            return -1;
        }
        node = new device_node_t;
        node->index = get_index(index_mask);
        snprintf(node->uuid, sizeof(node->uuid), "%s", uuid);
        node->next = NULL;
        *index = node->index;
        if (device_list_) {
            node->next = device_list_;
        }
        device_list_ = node;
        ++device_num_;
        pthread_mutex_unlock(&device_mutex_);
        return 0;
    }

    int remove_device(char *uuid, int *index) {
        pthread_mutex_lock(&device_mutex_);
        device_node_t *prev = NULL, *curr = device_list_;
        while (curr) {
            if (!strcmp(curr->uuid, uuid)) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    device_list_ = curr->next;
                }
                *index = curr->index;
                delete curr;
                --device_num_;
                pthread_mutex_unlock(&device_mutex_);
                return 0;
            }
            prev = curr;
            curr = curr->next;
        }
        pthread_mutex_unlock(&device_mutex_);
        return -1;
    }
private:
    struct device_node_t {
        char uuid[64];
        int index;
        struct device_node_t *next;
    };
    device_node_t *device_list_;
    int device_num_;
    int max_device_num_;
    pthread_mutex_t  device_mutex_;
};

class AutoDiscoveryExample
{
public:
    AutoDiscoveryExample() {
        observer = new DeviceObserver();
        WSDiscoveryClient::GetInstance()->registerObserver(observer);
        WSDiscoveryClient::GetInstance()->start();
    }
    ~AutoDiscoveryExample() {
        WSDiscoveryClient::GetInstance()->Delete();
        delete observer;
    }

public:
    static void DoDisplayDevice() {
        //WSDiscoveryClient::GetInstance()->dumpDeviceList();
        printf("WSDiscoveryClient::getDeviceList\n");
        WSDiscoveryClient::GetInstance()->getDeviceList(device_info, NULL);
        printf("WSDiscoveryClient::getDeviceList END\n");
    }
    static void device_info(void *usr, char *uuid, char *ipaddr, char *stream_url) {
        printf("\t device, uuid [%s], ipaddr[%s],stream_url[%s]\n", uuid, ipaddr, stream_url);
    }
private:
    DeviceObserver *observer;
};
//added for auto-discovery END --------------------------------------------------------------------------
#endif

TInt main(TInt argc, TChar **argv)
{
    TInt ret;
    TUint i = 0;
    SCameraTestContext *p_content = (SCameraTestContext *) malloc(sizeof(SCameraTestContext));

    test_log("[ut flow]: test start\n");

    if (!p_content) {
        test_loge("no memory,request size %ld\n", sizeof(SCameraTestContext));
        return (-7);
    }
    memset(p_content, 0x0, sizeof(SCameraTestContext));

    signal(SIGPIPE,  __camera_test_sig_pipe);

    //AutoDiscoveryExample exam;

    ret = _camera_test_default_initial_setting(p_content);

    if ((ret) && (argc < 2)) {
        test_log("[ut flow]: load from file\n");
        test_log("      cloud_server_addr: %s\n", p_content->cloud_server_addr);
        test_log("      cloud_tag: %s\n", p_content->cloud_server_addr);
    } else {
        if ((ret = camera_test_init_params(p_content, argc, argv)) < 0) {
            test_loge("[ut flow]: camera_test_init_params fail, return %d.\n", ret);
            goto camera_test_main_exit;
        } else if (ret > 0) {
            goto camera_test_main_exit;
        }
    }

    if (p_content->enablewsd) {
        //wsdObserver
    } else {
        do {
            ret = camera_test(p_content);
            if ((ret == (-9)) && p_content->asdemo) {
                sleep(5);
            }
        } while ((ret == (-9)) && p_content->asdemo);
    }

    test_log("[ut flow]: test end\n");

    ret = _camera_test_save_setting(p_content);
    if (0 == ret) {
        test_loge("write fail\n");
    }

camera_test_main_exit:

    if (p_content) {
        free(p_content);
        p_content = NULL;
    }

    return 0;
}

