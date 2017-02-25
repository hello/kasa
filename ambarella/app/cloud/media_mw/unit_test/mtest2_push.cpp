/*******************************************************************************
 * mtest2_push.cpp
 *
 * History:
 *    2014/03/15 - [Zhi He] create file
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

#include "push_cloud_simple_api.h"

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

static TInt running = 1;

static TInt _mtest2_pushmode_add_url_into_context(SSimpleAPIContxtInPushMode *p_content, TChar *url, TULong type)
{
    TULong current_count = 0;
    TChar *ptmp = NULL;

    DASSERT(url);
    DASSERT(ETestUrlType_Tot_Count > type);

    if ((!p_content) || (!url) || (ETestUrlType_Tot_Count <= type)) {
        test_loge("BAD input params in _mtest2_add_url_into_context: p_content %p, url %p, type %ld\n", p_content, url, type);
        return (-1);
    }

    if (ETestUrlType_Source == type) {
        current_count = p_content->group_url.cloud_tag_number;
        if (DLikely(current_count < DSYSTEM_MAX_CHANNEL_NUM)) {
            snprintf(&p_content->group_url.cloud_agent_tag[current_count][0], DMaxUrlLen, "%s", url);
            p_content->group_url.cloud_tag_number = current_count + 1;
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, DSYSTEM_MAX_CHANNEL_NUM);
            return (-3);
        }
    } else if (ETestUrlType_Sink == type) {
        current_count = p_content->group_url.sink_url_number;
        if (DLikely(current_count < DSYSTEM_MAX_CHANNEL_NUM)) {
            snprintf(&p_content->group_url.sink_url[current_count][0], DMaxUrlLen, "%s", url);
            p_content->group_url.sink_url_number = current_count + 1;
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, DSYSTEM_MAX_CHANNEL_NUM);
            return (-5);
        }
    } else if (ETestUrlType_Streaming == type) {
        current_count = p_content->group_url.streaming_url_number;
        if (DLikely(current_count < DSYSTEM_MAX_CHANNEL_NUM)) {
            snprintf(&p_content->group_url.streaming_url[current_count][0], DMaxUrlLen, "%s", url);
            test_log("streaming url[%ld] %s\n", current_count, p_content->group_url.streaming_url[current_count]);
            p_content->group_url.streaming_url_number = current_count + 1;
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, DSYSTEM_MAX_CHANNEL_NUM);
            return (-7);
        }
    }
#if 0
    else if (ETestUrlType_CloudTag == type) {
        current_count = p_content->group_url.remote_control_tag_number;
        if (DLikely(current_count < DSYSTEM_MAX_CHANNEL_NUM)) {
            snprintf(&p_content->group_url.remote_control_agent_tag[current_count][0], DMaxUrlLen, "%s", url);
            p_content->group_url.remote_control_tag_number = current_count + 1;
        } else {
            test_loge("Excced: current count %lu, max count %d\n", current_count, DSYSTEM_MAX_CHANNEL_NUM);
            return (-9);
        }
    }
#endif
    else {
        test_loge("Must not comes here, type %ld\n", type);
    }

    return 0;
}

static void _mtest2_pushmode_print_version()
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

static void _mtest2_pushmode_print_usage()
{
    printf("\nmtest2_push options:\n");
    printf("\t'-f [filename]': '-f' specify input file name/url, for demuxer\n");
    printf("\t'-r [filename]': '-r' specify output file name/url, for muxer\n");
    printf("\t'-s [streaming_url]': '-s' specify streaming tag name, for streaming\n");
    printf("\t'-c [agent_tag]': '-c' specify cloud agent tag name, for cloud connetion\n");

    printf("\t'--maxchannel [%%d]': specify max channel number\n");
    printf("\t'--tcpreceiver [%%d]': specify tcp receiver number\n");
    printf("\t'--tcpsender [%%d]': specify tcp sender number\n");
    printf("\t'--remotenumber [%%d]': specify max remote channel number\n");

    printf("\t'--udec': will choose udec mode\n");
    printf("\t'--mudec': will choose multiple window udec mode\n");
    printf("\t'--genericnvr': will choose generic nvr, recording + streaming, not local playback\n");
    printf("\t'--voutmask [mask]': '--voutmask' specify output device's mask, LCD's bit is 0x01, HDMI's bit is 0x02\n");
    printf("\t'--saveall': save all files (.ts format)\n");
    printf("\t'--streamingport [%%d]': rtsp streaming server port\n");
    printf("\t'--cloudport [%%d]': sacp cloud server port\n");
    printf("\t'--enable-compensatejitter': try to monitor delay and compensate it from dsp side\n");
    printf("\t'--disable-compensatejitter': disable the monitor delay and compensate it from dsp side\n");
    printf("\t'--help': print help\n");
    printf("\t'--showversion': show current version\n");

    printf("\n\tdebug only options:\n");
    printf("\t'--loglevel [level]': '--loglevel' specify log level, regardless the log.config\n");
    printf("\t'--enable-audio': will enable audio path\n");
    printf("\t'--enable-video': will enable video path\n");
    printf("\t'--disable-audio': will disable audio path\n");
    printf("\t'--disable-video': will disable video path\n");
    printf("\t'--prefetch [%%d]': arm will prefetch some data(%%d frames) before start playback\n");
    printf("\t'--timeout [%%d]': set timeout threashold for streaming, %%d seconds, if timeout occur, will reconnect server\n");
    printf("\t'--reconnectinterval [%%d]': auto reconnect server interval, seconds\n");
    printf("\t'--nopts': not feed pts to decoder\n");
    printf("\t'--pts': feed pts to decoder\n");
    printf("\t'--fps': specify fps\n");
    printf("\t'--debug-parse-multiple-access-unit': a debug option: try parse acc multiple access unit in demuxer\n");
    printf("\t'--debug-not-parse-multiple-access-unit': a debug option: not parse acc multiple access unit in demuxer, down stream filters will parse it\n");

    printf("\t'--prefer-ffmpeg-muxer': prefer ffmpeg muxer\n");
    printf("\t'--prefer-ts-muxer': prefer private ts muxer\n");
    printf("\t'--prefer-ffmpeg-audio-decoder': prefer ffmpeg audio decoder\n");
    printf("\t'--prefer-aac-audio-decoder': prefer aac audio decoder\n");
    printf("\t'--prefer-aac-audio-encoder': prefer aac audio encoder\n");
    printf("\t'--prefer-customized-adpcm-encoder': prefer adpcm audio encoder\n");
    printf("\t'--prefer-customized-adpcm-decoder': prefer adpcm audio decoder\n");
}

static void _mtest2_pushmode_print_runtime_usage()
{
    printf("\nmtest2_push runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
    printf("\t'pall': will print all components' status\n");
}

static TInt mtest2_pushmode_load_config(SSimpleAPIContxtInPushMode *p_content, const TChar *configfile, const TChar *urlfile)
{
    if (DUnlikely(!p_content || !configfile)) {
        test_loge("[input argument error]: mtest2_pushmode_load_config NULL params %p, %p.\n", p_content, configfile);
        return (-1);
    }

    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, DSYSTEM_MAX_CHANNEL_NUM);
    if (DUnlikely(!api)) {
        test_loge("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return (-2);
    }

    EECode err = api->OpenConfigFile((const TChar *)configfile, 0, 1, 1);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("OpenConfigFile(%s) fail, ret %d, %s.\n", configfile, err, gfGetErrorCodeString(err));
        return (-3);
    }

    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TU32 val = 0;

    err = api->GetContentValue("streamingport", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(streamingport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.rtsp_server_port = val;
        test_log("streamingport:%s, %d\n", read_string, p_content->setting.rtsp_server_port);
    }

    err = api->GetContentValue("cloudport", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(cloudport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.cloud_server_port = val;
        test_log("cloudport:%s, %d\n", read_string, p_content->setting.cloud_server_port);
    }

    err = api->GetContentValue("loglevel", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(loglevel) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.force_log_level = val;
        test_log("loglevel:%s, %d\n", read_string, p_content->setting.force_log_level);
        if (p_content->setting.force_log_level) {
            p_content->setting.enable_force_log_level = 1;
        }
    }

    err = api->GetContentValue("maxchannel", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(maxchannel) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.max_push_channel_number = val;
        test_log("maxchannel:%s, %d\n", read_string, p_content->setting.max_push_channel_number);
    }

    err = api->GetContentValue("sender", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(sender) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.max_streaming_transmiter_number = val;
        test_log("sender:%s, %d\n", read_string, p_content->setting.max_streaming_transmiter_number);
    }

    err = api->GetContentValue("receiver", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(receiver) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.net_receiver_tcp_scheduler_number = val;
        test_log("receiver:%s, %d\n", read_string, p_content->setting.net_receiver_tcp_scheduler_number);
    }

#if 0
    err = api->GetContentValue("pulsesender", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(pulsesender) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.pulse_sender_number = val;
        test_log("pulse_sender_number:%s, %d\n", read_string, p_content->setting.pulse_sender_number);
    }

    err = api->GetContentValue("pulsesender_framecount", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(pulsesender_framecount) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.pulse_sender_framecount = val;
        test_log("pulsesender_framecount:%s, %d\n", read_string, p_content->setting.pulse_sender_framecount);
    }

    err = api->GetContentValue("pulsesender_memsize", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(pulsesender_memsize) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->setting.pulse_sender_memsize = val;
        test_log("pulsesender_memsize:%s, %d\n", read_string, p_content->setting.pulse_sender_memsize);
    }
#endif

    err = api->GetContentValue("daemon", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(daemon) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->daemon = val;
        test_log("daemon:%s, %d\n", read_string, p_content->daemon);
    }

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    err = api->OpenConfigFile((const TChar *)urlfile, 0, 1, 2);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("OpenConfigFile(%s) fail, ret %d, %s.\n", configfile, err, gfGetErrorCodeString(err));
        return (-3);
    }

    TU32 i = 0, max = p_content->setting.max_push_channel_number;

    for (i = 0; i < max; i ++) {
        err = api->GetContentValue("url", (TChar *)read_string, (TChar *)read_string1);
        if (DUnlikely(EECode_OK != err)) {
            test_log("GetContentValue(url(%d)) fail, end of file? ret %d, %s.\n", i, err, gfGetErrorCodeString(err));
            break;
        } else {
            TInt ret = _mtest2_pushmode_add_url_into_context(p_content, read_string, ETestUrlType_Source);
            test_log("add_url(ETestUrlType_Source[%d]): %s\n", i, read_string);
            DASSERT(!ret);
            ret = _mtest2_pushmode_add_url_into_context(p_content, read_string1, ETestUrlType_Streaming);
            test_log("add_url(ETestUrlType_Streaming[%d]): %s\n", i, read_string1);
            DASSERT(!ret);
        }
    }

    api->Destroy();

    return 0;
}

static TInt mtest2_pushmode_init_params(SSimpleAPIContxtInPushMode *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;
    TU8 total_group_number = 0;

    if (argc < 2) {
        //_mtest_print_version();
        _mtest2_pushmode_print_usage();
        _mtest2_pushmode_print_runtime_usage();
        return 1;
    }

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("-f", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_pushmode_add_url_into_context(p_content, argv[i + 1], ETestUrlType_Source);
                test_log("_mtest2_pushmode_add_url_into_context, argv[i + 1]: %s\n", argv[i + 1]);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_pushmode_add_url_into_context ret(%d).\n", ret);
                    return (-3);
                }
            } else {
                test_loge("[input argument error]: '-f', should follow with source url(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("-r", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_pushmode_add_url_into_context(p_content, argv[i + 1], ETestUrlType_Sink);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_pushmode_add_url_into_context ret(%d).\n", ret);
                    return (-5);
                }
            } else {
                test_loge("[input argument error]: '-r', should follow with sink url(%%s), argc %d, i %d.\n", argc, i);
                return (-6);
            }
            i ++;
        } else if (!strcmp("-s", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_pushmode_add_url_into_context(p_content, argv[i + 1], ETestUrlType_Streaming);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_pushmode_add_url_into_context ret(%d).\n", ret);
                    return (-7);
                }
            } else {
                test_loge("[input argument error]: '-F', should follow with streaming url(%%s), argc %d, i %d.\n", argc, i);
                return (-8);
            }
            i ++;
        } else if (!strcmp("-c", argv[i])) {
            if ((i + 1) < argc) {
                ret = _mtest2_pushmode_add_url_into_context(p_content, argv[i + 1], ETestUrlType_CloudTag);
                test_log("_mtest2_pushmode_add_url_into_context, argv[i + 1]: %s\n", argv[i + 1]);
                if (ret < 0) {
                    test_loge("[input argument error]: _mtest2_pushmode_add_url_into_context ret(%d).\n", ret);
                    return (-7);
                }
            } else {
                test_loge("[input argument error]: '-F', should follow with cloud agent tag(%%s), argc %d, i %d.\n", argc, i);
                return (-8);
            }
            i ++;
        } else if (!strcmp("--maxchannel", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.max_push_channel_number = ret;
                test_log("[input argument]: '--maxchannel': (%d).\n", p_content->setting.max_push_channel_number);
            } else {
                test_loge("[input argument error]: '--maxchannel', should follow with integer(channel number), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--remotenumber", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.max_remote_control_channel_number = ret;
                test_log("[input argument]: '--remotenumber': (%d).\n", p_content->setting.max_remote_control_channel_number);
            } else {
                test_loge("[input argument error]: '--remotenumber', should follow with integer(remote number), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--tcpreceiver", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.net_receiver_tcp_scheduler_number = ret;
                test_log("[input argument]: '--tcpreceiver': (%d).\n", p_content->setting.net_receiver_tcp_scheduler_number);
            } else {
                test_loge("[input argument error]: '--tcpreceiver', should follow with integer(tcp receiver), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--guard", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->gd_port = ret;
                test_log("[input argument]: '--guard': (%d).\n", p_content->gd_port);
            } else {
                test_loge("[input argument error]: '--guard', should follow with integer(gd port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--streamingport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.rtsp_server_port = ret;
                test_log("[input argument]: '--streamingport': (%d).\n", p_content->setting.rtsp_server_port);
            } else {
                test_loge("[input argument error]: '--streamingport', should follow with integer(rtsp streaming server port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--cloudport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.cloud_server_port = ret;
                test_log("[input argument]: '--cloudport': (%d).\n", p_content->setting.cloud_server_port);
            } else {
                test_loge("[input argument error]: '--cloudport', should follow with integer(tcp receiver), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--tcpsender", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.max_streaming_transmiter_number = ret;
                test_log("[input argument]: '--tcpsender': (%d).\n", p_content->setting.max_streaming_transmiter_number);
            } else {
                test_loge("[input argument error]: '--tcpsender', should follow with integer(tcp sender), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--daemon", argv[i])) {
            test_log("'--daemon': daemon\n");
            p_content->daemon = 1;
        } else if (!strcmp("--saveall", argv[i])) {
            test_log("'--saveall': save all streams\n");
            p_content->setting.enable_recording = 1;
        } else if (!strcmp("--showversion", argv[i])) {
            _mtest2_pushmode_print_version();
            return (18);
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
        } else if (!strcmp("--rtspserverport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.rtsp_server_port = ret;
                test_log("[input argument]: '--rtspserverport': (%d).\n", p_content->setting.rtsp_server_port);
            } else {
                test_loge("[input argument error]: '--rtspserverport', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--cloudserverport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->setting.cloud_server_port = ret;
                test_log("[input argument]: '--cloudserverport': (%d).\n", p_content->setting.cloud_server_port);
            } else {
                test_loge("[input argument error]: '--cloudserverport', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-10);
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
            _mtest2_pushmode_print_version();
            _mtest2_pushmode_print_usage();
            _mtest2_pushmode_print_runtime_usage();
            return (3);
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

static TInt relay_test(SSimpleAPIContxtInPushMode *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;

    CSimpleAPIInPushMode *p_api = CSimpleAPIInPushMode::Create(1);
    if (DUnlikely(!p_api)) {
        test_loge("CSimpleAPIInPushMode::Create(1) fail\n");
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

    if (!p_context->daemon) {

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
                            p_engine_control->PrintCurrentStatus(0, DPrintFlagBit_dataPipeline);
                        } else if (p_buffer[1] == 'p') {
                            TU32 index = 0;
                            if (p_buffer[2] == 's') {
                                if (1 == sscanf(p_buffer, "pps %d", &index)) {
                                    if (DUnlikely(index >= p_context->setting.max_push_channel_number)) {
                                        test_loge("pps index %d excced %d\n", index, p_context->setting.max_push_channel_number);
                                        break;
                                    }
                                    if (DUnlikely(!p_context->gragh_component.streaming_pipeline_id[index])) {
                                        test_loge("pps p_context->gragh_component.streaming_pipeline_id[%d] is zero? id 0x%x\n", index, p_context->gragh_component.streaming_pipeline_id[index]);
                                        break;
                                    }
                                } else {
                                    test_loge("pps should follow by a integer(streaming pipeline index)\n");
                                    break;
                                }
                                test_log("print streaming pipeline %d, id %08x\n", index, p_context->gragh_component.streaming_pipeline_id[index]);
                                p_engine_control->PrintCurrentStatus(p_context->gragh_component.streaming_pipeline_id[index], DPrintFlagBit_dataPipeline);
                            } else if (p_buffer[2] == 'c') {
                                if (1 == sscanf(p_buffer, "ppc %d", &index)) {
                                    if (DUnlikely(index >= p_context->setting.max_push_channel_number)) {
                                        test_loge("ppc index %d excced %d\n", index, p_context->setting.max_push_channel_number);
                                        break;
                                    }
                                    if (DUnlikely(!p_context->gragh_component.cloud_pipeline_id[index])) {
                                        test_loge("ppc p_context->gragh_component.cloud_pipeline_id[%d] is zero? id 0x%x\n", index, p_context->gragh_component.cloud_pipeline_id[index]);
                                        break;
                                    }
                                } else {
                                    test_loge("ppc should follow by a integer(cloud pipeline index)\n");
                                    break;
                                }
                                test_log("print cloud pipeline %d, id %08x\n", index, p_context->gragh_component.cloud_pipeline_id[index]);
                                p_engine_control->PrintCurrentStatus(p_context->gragh_component.cloud_pipeline_id[index], DPrintFlagBit_dataPipeline);
                            } else if (p_buffer[2] == 'r') {
                                if (1 == sscanf(p_buffer, "ppr %d", &index)) {
                                    if (DUnlikely(index >= p_context->setting.max_push_channel_number)) {
                                        test_loge("ppr index %d excced %d\n", index, p_context->setting.max_push_channel_number);
                                        break;
                                    }
                                    if (DUnlikely(!p_context->gragh_component.recording_pipeline_id[index])) {
                                        test_loge("ppr p_context->gragh_component.recording_pipeline_id[%d] is zero? id 0x%x\n", index, p_context->gragh_component.recording_pipeline_id[index]);
                                        break;
                                    }
                                } else {
                                    test_loge("ppr should follow by a integer(recording pipeline index)\n");
                                    break;
                                }
                                test_log("print recording pipeline %d, id %08x\n", index, p_context->gragh_component.recording_pipeline_id[index]);
                                p_engine_control->PrintCurrentStatus(p_context->gragh_component.recording_pipeline_id[index], DPrintFlagBit_dataPipeline);
                            }
                        }
                    }
                    break;

                case 'l': {
                        if ((p_buffer[1] == 'o') && (p_buffer[2] == 'g')) {
                            TU32 id = 0, level = 0, option = 0, output = 0, index = 0;
                            if ((p_buffer[3] == 's') && (p_buffer[4] == ' ')) {
                                if (1 == sscanf(p_buffer + 5, "%d,%08x,%08x,%08x", &index, &level, &option, &output)) {
                                    if (DUnlikely(index >= p_context->setting.max_push_channel_number)) {
                                        test_loge("logs index %d %d\n", index, p_context->setting.max_push_channel_number);
                                        break;
                                    }
                                    if (DUnlikely(!p_context->gragh_component.streaming_pipeline_id[index])) {
                                        test_loge("logs p_context->gragh_component.streaming_pipeline_id[%d] is zero? id 0x%x\n", index, p_context->gragh_component.streaming_pipeline_id[index]);
                                        break;
                                    }
                                } else {
                                    test_loge("logs should follow by a integer(streaming pipeline index), and log configs\n");
                                    break;
                                }
                                test_log("set log streaming pipeline %d, id %08x, %08x %08x %08x\n", index, p_context->gragh_component.streaming_pipeline_id[index], level, option, output);
                                p_engine_control->SetRuntimeLogConfig(p_context->gragh_component.streaming_pipeline_id[index], level, option, output);
                            }
                        } else if ((p_buffer[1] == 'o') && (p_buffer[2] == 'g')) {
                            TU32 id = 0, level = 0, option = 0, output = 0, index = 0;
                            if ((p_buffer[3] == 'c') && (p_buffer[4] == ' ')) {
                                if (1 == sscanf(p_buffer + 5, "%d,%08x,%08x,%08x", &index, &level, &option, &output)) {
                                    if (DUnlikely(index >= p_context->setting.max_push_channel_number)) {
                                        test_loge("logc index %d %d\n", index, p_context->setting.max_push_channel_number);
                                        break;
                                    }
                                    if (DUnlikely(!p_context->gragh_component.cloud_pipeline_id[index])) {
                                        test_loge("logc p_context->gragh_component.cloud_pipeline_id[%d] is zero? id 0x%x\n", index, p_context->gragh_component.cloud_pipeline_id[index]);
                                        break;
                                    }
                                } else {
                                    test_loge("logc should follow by a integer(cloud pipeline index), and log configs\n");
                                    break;
                                }
                                test_log("set log cloud pipeline %d, id %08x, %08x %08x %08x\n", index, p_context->gragh_component.cloud_pipeline_id[index], level, option, output);
                                p_engine_control->SetRuntimeLogConfig(p_context->gragh_component.cloud_pipeline_id[index], level, option, output);
                            }
                        }
                    }
                    break;

                case 'h':   // help
                    _mtest2_pushmode_print_version();
                    _mtest2_pushmode_print_usage();
                    _mtest2_pushmode_print_runtime_usage();
                    break;

                default:
                    break;
            }
        }

    } else {
        while (running) {
            usleep(5000000);
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

static void __mtest2_pushmode_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static void _mtest2_pushmode_default_initial_setting(SSimpleAPIContxtInPushMode *p_content)
{
    p_content->setting.voutmask = DCAL_BITMASK(EDisplayDevice_HDMI);

    p_content->setting.net_receiver_tcp_scheduler_number = 2;

    p_content->setting.max_push_channel_number = 100;
    p_content->setting.channel_number_per_transmiter = 4;
    p_content->setting.max_streaming_transmiter_number = 25;

    p_content->setting.enable_streaming_server = 1;
    p_content->setting.enable_cloud_server = 1;

    p_content->setting.disable_audio_path = 1;
    p_content->setting.disable_dsp_related = 1;

    p_content->setting.max_remote_control_channel_number = 4;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret;
    TUint i = 0;

    SSimpleAPIContxtInPushMode *p_content = (SSimpleAPIContxtInPushMode *) malloc(sizeof(SSimpleAPIContxtInPushMode));
    if (DUnlikely(!p_content)) {
        test_loge("can not malloc mem(SSimpleAPIContxtInPushMode), request size %ld\n", sizeof(SSimpleAPIContxtInPushMode));
        goto mtest2_pushmode_main_exit;
    }
    memset(p_content, 0x0, sizeof(SSimpleAPIContxtInPushMode));
    _mtest2_pushmode_default_initial_setting(p_content);

    signal(SIGPIPE,  __mtest2_pushmode_sig_pipe);

    ret = mtest2_pushmode_load_config(p_content, "relayserver_conf.ini", "relayserver_urllist.ini");
    if (0 > ret) {
        LOG_ERROR("load configfile fail, ret %d\n", ret);
        goto mtest2_pushmode_main_exit;
    }
    if ((ret = mtest2_pushmode_init_params(p_content, argc, argv)) < 0) {
        test_loge("mtest2_pushmode_init_params fail, return %d.\n", ret);
        goto mtest2_pushmode_main_exit;
    }

    if (p_content->gd_port) {
        p_content->gd = gfCreateGuardianAgent(EGuardianType_socket, p_content->gd_port);
    }

    if (p_content->daemon) {
        daemon(1, 0);
    }

    if (p_content->setting.max_streaming_transmiter_number && p_content->setting.max_push_channel_number) {
        p_content->setting.channel_number_per_transmiter = (p_content->setting.max_streaming_transmiter_number + p_content->setting.max_push_channel_number - 1) / (p_content->setting.max_streaming_transmiter_number);
    }
    DASSERT(p_content->setting.channel_number_per_transmiter);

    p_content->setting.work_mode = EMediaDeviceWorkingMode_NVR_RTSP_Streaming;
    switch (p_content->setting.work_mode) {

        case EMediaDeviceWorkingMode_NVR_RTSP_Streaming:
            p_content->setting.enable_playback = 0;
            p_content->setting.enable_recording = 0;
            p_content->setting.enable_streaming_server = 1;
            p_content->setting.enable_cloud_server = 1;
            p_content->setting.disable_dsp_related = 1;

            test_log("[ut flow]: relay_test() start\n");
            ret = relay_test(p_content);
            test_log("[ut flow]: relay_test end, ret %d\n", ret);
            break;

        default:
            test_loge("BAD workmode %d\n", p_content->setting.work_mode);
            goto mtest2_pushmode_main_exit;
    }

    test_log("[ut flow]: test end\n");

mtest2_pushmode_main_exit:

    if (p_content) {
        free(p_content);
        p_content = NULL;
    }

    return 0;
}

