/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BLS_LIB_RTMP_H
#define BLS_LIB_RTMP_H


//#include "../src/libs/srs_librtmp.h"
#include <bls_flv.h>
/*
 * the bls is the abbreviation fo "baidu live server"
 * bls-librtmp is a librtmp like library,
 * used to play/publish rtmp stream from/to rtmp server.
 * socket: use sync and block socket to connect/recv/send data with server.
 * depends: no need other libraries; depends on ssl if use .
 * thread-safe: no
 */

#ifdef __cplusplus
extern "C"{
#endif

// the RTMP handler.
typedef void* bls_rtmp_t;

/*
 * create/destroy a rtmp protocol stack.
 * @url is rtmp url, for example:
 *         rtmp://127.0.0.1/live/livestream
 * @accesstoken,devid is the identity verify information when you regisering
 * the baidu PCS platform.
 * @sessiontoken, get from baidu pcs, for security considerration whitch has only 10s validate duration time
 * @extjson the extended information that may be needed in the futrue,
 * you can just set it as null string right now.
 * @return a rtmp handler, or NULL if error occured.
 */
bls_rtmp_t bls_rtmp_create(const char *url, const char *devid,
                           const char *accesstoken, const char *sessiontoken, const char *extjson);
/*
 * close and destroy the rtmp stack.
 * @remark, user should use it at last.
 */
void bls_rtmp_destroy(bls_rtmp_t rtmp);
/*
 * connect and handshake with server,get the server timestamp
 * category: publish/play
 * previous: rtmp-create
 * next: play or publish
 * @return:if success,return 0 and set the timestamp variable as the time of the rtmpserver; otherwise, failed.
 */
int bls_connect_app(bls_rtmp_t rtmp, u_int32_t *timestamp);
/*
 * the method to judge if the tcp connection is still valid
 * @return:if valid ,return 1 else return 0
 * @remark: if the client or socket close the socket initiatively or passively, the invalid connection can be detected right now
 * but if the host is unreachable(such like physical link is broken) the connection invalidation would been detected
 * in a delay no more then 30s
 */
int bls_isconnected(bls_rtmp_t rtmp);

/*
 * play a live/vod stream.
 * @play_stream_name, the id returned when registering in the Baidu-cam Platform
 *     for example:“78c1f9ba124611e4815aac853dd1c804”
 * category: play
 * previous: connect-app
 * next: destroy
 * @return: if success return 0 and set the corresponded stream_id to pla_stream_id; otherwise, failed.
 */
int bls_play_stream(bls_rtmp_t rtmp, const char *play_stream_name, int *play_stream_id);
/*
 * stopplay a live/vod stream.
 * category: stopplay
 * previous: play
 * @return 0, success; otherwise, failed.
 */
int bls_stopplay_stream(bls_rtmp_t rtmp, int play_stream_id);

/*
 * publish a live stream.
 * category: publish
 * previous: connect
 * next: unpublish
 * @return 0, success; otherwise, failed.
 */
int bls_publish_stream(bls_rtmp_t rtmp, const char *publish_stream_name, int *publish_stream_id);
/*
 * unpublish a live stream.
 * category: unpublish
 * previous: publish
 * next: destroy
 * @return 0, success; otherwise, failed.
 */
int bls_unpublish_stream(bls_rtmp_t rtmp, int publish_stream_id);

/*
 * set the property of the stream
 * notify the server that the property have changed
 * and the server should handle the change event
 * @return 0, success; otherwise, failed.
 */
int bls_set_streamproperty(bls_rtmp_t rtmp, double flag);
/*
 * E.4.1 FLV Tag, page 75
 */
// 8 = audio
#define BLS_RTMP_TYPE_AUDIO 8
// 9 = video
#define BLS_RTMP_TYPE_VIDEO 9
// 18 = script data
#define BLS_RTMP_TYPE_SCRIPT 18
// 20 = user command
#define BLS_RTMP_TYPE_COMMAND 20
/*
 * convert the flv tag type to string.
 *     SRS_RTMP_TYPE_AUDIO to "Audio"
 *     SRS_RTMP_TYPE_VIDEO to "Video"
 *     SRS_RTMP_TYPE_SCRIPT to "Data"
 *     SRS_RTMP_TYPE_COMMAND to "Command"
 *     otherwise, "Unknown"
 * @remark user should never free the returned char*,
 */
const char* bls_type2string(int type);
/*
 * read a audio/video/script-data/command packet from rtmp stream.
 * @stream_id, indicating reading packet from whitch stream
 * @return the error code. 0 for success; otherwise, error.
 * @remark, if sucess, if there are packets for that stream, set the packet pointer as the real data packet pointer, if there not, the packet variable are set NULL
 *
 */
int bls_read_packet(bls_rtmp_t rtmp, struct bls_packet **packet, int stream_id);

/*
 * methods to get the properties of the packet you have read from a stream
 */
int bls_packet_type(struct bls_packet *packet);
int bls_packet_len(struct bls_packet *packet);
u_int32_t bls_packet_timestamp(struct bls_packet *packet);
/*
 * if the type is video,audio or meta, return the raw data the server transferd
 * if the type is command ,this function return the stream name if the commandname is "startPlay",
 * return the command content expressed by json format if the commandname is "userCmd"
 */
char *bls_packet_data(struct bls_packet *packet);
/*
 * if the packet type is command,
 * this function are used to get the command name of the packet
 * often these command names are:"_result","error","startPlay"or "userCmd" and so on.
 * but the applycation only need to handle the "startPlay" and "userCmd" currently
 */
char *bls_packet_commandname(struct bls_packet *packet);
/*
 * the packet you have read should be freed after been used
 */
void bls_free_packet(struct bls_packet *packet);
/*
 * write methods for raw packets
 * or the specified packets
 * @return 0, success; otherwise, failed.
 * @remark: if the physical link is broken, the kernel can not detect such condition right now,
 * and if the socket buffer is not full, the write methods will write the data to socket buffer and return 0,
 * if full, the write methods will block until the 5s timeout
 */
int bls_write_audio(bls_rtmp_t rtmp, u_int32_t timestamp, char* data, int size, int streamid);
int bls_write_video(bls_rtmp_t rtmp, u_int32_t timestamp, char* data, int size, int streamid);
int bls_write_meta(bls_rtmp_t rtmp, char* data, int size, int streamid);
/*
 * write the result for the usercmd_packet you have get
 * @cmd_packet, indicating whitch command packet you want to response
 * @data, the content you want to response
 * @return 0, success; otherwise, failed.
 */
int bls_write_usercmd_result(bls_rtmp_t rtmp, char* data, struct bls_packet *cmd_packet);

/*
 * get protocol stack version
 */
int bls_version_major();
int bls_version_minor();
int bls_version_revision();

/*
 * utilities
 */
int64_t bls_get_time_ms();
int64_t bls_get_nsend_bytes(bls_rtmp_t rtmp);
int64_t bls_get_nrecv_bytes(bls_rtmp_t rtmp);

#ifdef __cplusplus
}
#endif

#endif
