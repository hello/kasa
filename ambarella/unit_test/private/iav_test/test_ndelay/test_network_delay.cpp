/*
 * test_network_delay.cpp
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pthread.h"
#include "semaphore.h"

#include "simple_log.h"
#include "network_utils.h"

#include "simple_queue.h"

#ifndef BUILD_OS_WINDOWS
#include <sys/time.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include "basetypes.h"
#include "iav_ioctl.h"
#endif

#define D_HEADER_LENGTH 16
#define D_ECHO_SERVER_PORT 5427
#define D_DELAY_SERVER_PORT 5429

#define D_MAX_PACKET_PAYLOAD_SIZE 1408

static int run_flag = 1;
static long long frame_gap = 16666;
static unsigned long idr_interval = 30;
static unsigned int idr_size = 26880;
static unsigned int p_size = 1280;
static unsigned char send_buf[D_HEADER_LENGTH + D_MAX_PACKET_PAYLOAD_SIZE];
static char echo_server_url[64] = "127.0.0.1";
static int start_echo_server = 0;
static int start_delay_test = 0;
static int verbose = 0;
static int nodelaysocket = 1;
static int queue_data = 1;
static int test_read_iav = 0;

#ifdef BUILD_OS_WINDOWS
static void gettimeofday(struct timeval* tp, void* tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);

    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon     = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min     = wtm.wMinute;
    tm.tm_sec     = wtm.wSecond;
    tm.tm_isdst    = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long) clock;
    tp->tv_usec = (long) (wtm.wMilliseconds * 1000);

    return;
}
#else

static unsigned int stream_id = 0;

typedef struct {
    void *base;
    unsigned int size;
} s_iav_map_bsb;

typedef struct {
    unsigned int stream_id;
    unsigned int offset;
    unsigned int size;
    unsigned long long pts;

    unsigned int video_width;
    unsigned int video_height;
} s_iav_read_bitstream;

static int __iav_map_bsb(int iav_fd, s_iav_map_bsb* map_bsb)
{
    int ret = 0;
    struct iav_querybuf querybuf;

    querybuf.buf = IAV_BUFFER_BSB;
    ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
    if (0 > ret) {
        perror("IAV_IOC_QUERY_BUF");
        LOG_ERROR("IAV_IOC_QUERY_BUF fail\n");
        return ret;
    }

    map_bsb->size = querybuf.length;
    map_bsb->base = mmap(NULL, querybuf.length * 2, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);

    if (map_bsb->base == MAP_FAILED) {
        perror("mmap");
        LOG_ERROR("mmap fail\n");
        return -1;
    }

    LOG_PRINTF("bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    return 0;
}

static int __iav_read_bitstream(int iav_fd, s_iav_read_bitstream* bitstream)
{
    struct iav_querydesc query_desc;
    struct iav_framedesc *frame_desc;
    unsigned int retry_count = 100;

    while (retry_count) {

        memset(&query_desc, 0, sizeof(query_desc));
        frame_desc = &query_desc.arg.frame;
        query_desc.qid = IAV_DESC_FRAME;
        frame_desc->id = -1;

        if (ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
            perror("IAV_IOC_QUERY_DESC");
            LOG_ERROR("IAV_IOC_QUERY_DESC error\n");
            return (-1);
        }

        if (bitstream->stream_id == frame_desc->id) {
            bitstream->offset = frame_desc->data_addr_offset;
            bitstream->size = frame_desc->size;
            bitstream->pts = frame_desc->arm_pts;
            bitstream->video_width = frame_desc->reso.width;
            bitstream->video_height = frame_desc->reso.height;
            return 0;
        }

        retry_count --;
    }

    LOG_ERROR("no frame about stream id %d\n", bitstream->stream_id);
    return (-2);
}

static int __iav_open_handle()
{
    int fd = open("/dev/iav", O_RDWR, 0);
    if (0 > fd) {
        LOG_FATAL("open iav fail, %d.\n", fd);
    }
    return fd;
}

static void __iav_close_handle(int fd)
{
    if (0 > fd) {
        LOG_FATAL("bad fd %d\n", fd);
        return;
    }
    close(fd);
    return;
}

#endif

static void __delay_us(unsigned int us)
{
#ifdef BUILD_OS_WINDOWS
    if (us > 2000) {
        Sleep(2);
    } else {
        Sleep(1);
    }
#else
    usleep(us);
#endif
}

static void __delay_500ms()
{
#ifdef BUILD_OS_WINDOWS
    Sleep(500);
#else
    usleep(500000);
#endif
}

static void __get_time(unsigned int* second, unsigned int* usecond)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    second[0] = tv.tv_sec;
    usecond[0] = tv.tv_usec;
}

static long long __get_time_0(unsigned int* second, unsigned int* usecond)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    second[0] = tv.tv_sec;
    usecond[0] = tv.tv_usec;

    return (long long)(((long long)tv.tv_sec * (long long)1000000) + ((long long)tv.tv_usec));
}

static int __send_fake_frame(TSocketHandler fd, unsigned int time_sec, unsigned int time_usec, unsigned int idr)
{
    unsigned int remain_size = 0;
    unsigned int send_size = 0;
    int ret = 0;

    if (idr) {
        remain_size = idr_size;
    } else {
        remain_size = p_size;
    }

    if (remain_size > D_MAX_PACKET_PAYLOAD_SIZE) {
        send_size = D_MAX_PACKET_PAYLOAD_SIZE;
    } else {
        send_size = remain_size;
    }
    send_buf[0] = 0;
    send_buf[1] = (send_size >> 16) & 0xff;
    send_buf[2] = (send_size >> 8) & 0xff;
    send_buf[3] = (send_size) & 0xff;
    *((unsigned int*) (send_buf + 4)) = time_sec;
    *((unsigned int*) (send_buf + 8)) = time_usec;
    send_buf[12] = (remain_size >> 24) & 0xff;
    send_buf[13] = (remain_size >> 16) & 0xff;
    send_buf[14] = (remain_size >> 8) & 0xff;
    send_buf[15] = (remain_size) & 0xff;
    ret = net_send(fd, send_buf, send_size + D_HEADER_LENGTH, 0);
    if ((unsigned int)ret != (send_size + D_HEADER_LENGTH)) {
        LOG_ERROR("send fail, %d\n", ret);
        return (-1);
    }
    remain_size -= send_size;

    while (remain_size) {
        if (remain_size > D_MAX_PACKET_PAYLOAD_SIZE) {
            send_size = D_MAX_PACKET_PAYLOAD_SIZE;
        } else {
            send_size = remain_size;
        }
        send_buf[0] = 1;
        send_buf[1] = (send_size >> 16) & 0xff;
        send_buf[2] = (send_size >> 8) & 0xff;
        send_buf[3] = (send_size) & 0xff;
        ret = net_send(fd, send_buf, send_size + D_HEADER_LENGTH, 0);
        if ((unsigned int)ret != (send_size + D_HEADER_LENGTH)) {
            LOG_ERROR("send fail, %d\n", ret);
            return (-1);
        }
        remain_size -= send_size;
    }

    return 0;
}

#ifndef BUILD_OS_WINDOWS
static int __next_frame_start_code(unsigned char* p, unsigned int len, unsigned char* nal_type)
{
    unsigned int state = 0;

    while (len) {
        switch (state) {
            case 0:
                if (!(*p)) {
                    state = 1;
                }
                break;

            case 1: //0
                if (!(*p)) {
                    state = 2;
                } else {
                    state = 0;
                }
                break;

            case 2: //0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    if ((p[1] & 0x1f) < 6) {
                        nal_type[0] = p[1] & 0x1f;
                        return 0;
                    }
                    state = 0;
                } else {
                    state = 0;
                }
                break;

            case 3: //0 0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    if ((p[1] & 0x1f) < 6) {
                        nal_type[0] = p[1] & 0x1f;
                        return 0;
                    }
                    state = 0;
                } else {
                    state = 0;
                }
                break;

            default:
                LOG_FATAL("impossible to comes here\n");
                break;

        }
        p++;
        len --;
    }

    LOG_FATAL("data corruption, not found\n");
    return (-1);
}

static int __send_iav_frame(TSocketHandler fd, unsigned int time_sec, unsigned int time_usec, unsigned int size)
{
    unsigned int remain_size = size;
    unsigned int send_size = 0;
    int ret = 0;

    if (remain_size > D_MAX_PACKET_PAYLOAD_SIZE) {
        send_size = D_MAX_PACKET_PAYLOAD_SIZE;
    } else {
        send_size = remain_size;
    }
    send_buf[0] = 0;
    send_buf[1] = (send_size >> 16) & 0xff;
    send_buf[2] = (send_size >> 8) & 0xff;
    send_buf[3] = (send_size) & 0xff;
    *((unsigned int*) (send_buf + 4)) = time_sec;
    *((unsigned int*) (send_buf + 8)) = time_usec;
    send_buf[12] = (remain_size >> 24) & 0xff;
    send_buf[13] = (remain_size >> 16) & 0xff;
    send_buf[14] = (remain_size >> 8) & 0xff;
    send_buf[15] = (remain_size) & 0xff;
    ret = net_send(fd, send_buf, send_size + D_HEADER_LENGTH, 0);
    if ((unsigned int)ret != (send_size + D_HEADER_LENGTH)) {
        LOG_ERROR("send fail, %d\n", ret);
        return (-1);
    }
    remain_size -= send_size;

    while (remain_size) {
        if (remain_size > D_MAX_PACKET_PAYLOAD_SIZE) {
            send_size = D_MAX_PACKET_PAYLOAD_SIZE;
        } else {
            send_size = remain_size;
        }
        send_buf[0] = 1;
        send_buf[1] = (send_size >> 16) & 0xff;
        send_buf[2] = (send_size >> 8) & 0xff;
        send_buf[3] = (send_size) & 0xff;
        ret = net_send(fd, send_buf, send_size + D_HEADER_LENGTH, 0);
        if ((unsigned int)ret != (send_size + D_HEADER_LENGTH)) {
            LOG_ERROR("send fail, %d\n", ret);
            return (-1);
        }
        remain_size -= send_size;
    }

    return 0;
}
#endif

typedef struct {
    TSocketHandler server_socket;
    TSocketHandler pipe_fd;
} s_test_server;

typedef struct {
    TSocketHandler socket;

    TSocketHandler echo_socket;

    TSocketHandler pipe_fd;
    pthread_t thread_id;
} s_test_echo_client_receiver;

typedef struct {
    TSocketHandler echo_socket;

    ISimpleQueue* p_data_queue;
    ISimpleQueue* p_free_queue;
    IMemPool* p_mem_pool;

    pthread_t thread_id;
} s_test_echo_client_sender;

typedef struct {
    TSocketHandler socket;

    TSocketHandler pipe_fd;
    pthread_t thread_id;
} s_test_delay_client;

typedef struct {
    unsigned char* p_data;
    unsigned int size;
} s_data_piece;

static void* __echo_client_sender_thread(void* p)
{
    s_test_echo_client_sender* thiz = (s_test_echo_client_sender*) p;
    TSocketHandler socket = thiz->echo_socket;
    ISimpleQueue* p_data_queue = thiz->p_data_queue;
    ISimpleQueue* p_free_queue = thiz->p_free_queue;
    IMemPool* p_mem_pool = thiz->p_mem_pool;
    s_data_piece* p_piece = NULL;
    int ret = 0;
    int quit = 0;

    LOG_NOTICE("echo client sender loop begin\n");

    while (1) {

        p_piece = (s_data_piece*) p_data_queue->Dequeue();
        if (p_piece) {
            ret = net_send(socket, p_piece->p_data, p_piece->size, 0);

            if ((unsigned int) ret != (p_piece->size)) {
                LOG_ERROR("send packet fail, %d, %d\n", ret, p_piece->size);
                quit = 1;
            }

            p_mem_pool->ReleaseMemBlock(p_piece->size, p_piece->p_data);
            p_free_queue->Enqueue((unsigned long) p_piece);

            if (quit) {
                break;
            }
        } else {
            break;
        }
    }

    LOG_NOTICE("echo client sender loop end\n");

    return NULL;
}

static void* __echo_client_receiver_thread(void* p)
{
    s_test_echo_client_receiver* thiz = (s_test_echo_client_receiver*) p;
    TSocketHandler socket = thiz->socket;
    int ret = 0;
    int len = 0;
    unsigned int max_size = 8 * 1024;
    void* tp = NULL;

    ISimpleQueue* p_data_queue = gfCreateSimpleQueue(24);
    ISimpleQueue* p_free_queue = gfCreateSimpleQueue(24);
    IMemPool* p_mem_pool = gfCreateSimpleRingMemPool(1024 * 1024);
    s_test_echo_client_sender sender_context;
    s_data_piece data_pieces[24];
    s_data_piece* p_piece = NULL;

    if ((!p_data_queue) || (!p_free_queue) || (!p_mem_pool)) {
        pipe_write(thiz->pipe_fd, 'd');
        LOG_ERROR("create queue and pool fail\n");
        return NULL;
    }

    for (ret = 0; ret < 20; ret ++) {
        p_free_queue->Enqueue((unsigned long)&data_pieces[ret]);
    }

    sender_context.echo_socket = thiz->echo_socket;
    sender_context.p_data_queue = p_data_queue;
    sender_context.p_free_queue = p_free_queue;
    sender_context.p_mem_pool = p_mem_pool;

    pthread_create(&sender_context.thread_id, NULL, __echo_client_sender_thread, &sender_context);

    LOG_NOTICE("echo client receiver loop begin\n");

    while (run_flag) {

        p_piece = (s_data_piece*) p_free_queue->Dequeue();
        p_piece->p_data = p_mem_pool->RequestMemBlock(max_size);

        len = D_HEADER_LENGTH;
        ret = net_recv(socket, p_piece->p_data, len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (ret != len) {
            LOG_ERROR("read header fail\n");
            break;
        }

        len = (((unsigned int)p_piece->p_data[1] << 16) | ((unsigned int)p_piece->p_data[2] << 8) | ((unsigned int)p_piece->p_data[3]));
        ret = net_recv(socket, p_piece->p_data + D_HEADER_LENGTH, len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (ret != len) {
            LOG_ERROR("read packet fail\n");
            break;
        }

        p_piece->size = len + D_HEADER_LENGTH;
        p_mem_pool->ReturnBackMemBlock(max_size - p_piece->size, p_piece->p_data + p_piece->size);

        p_data_queue->Enqueue((unsigned long) p_piece);
    }

    p_data_queue->Enqueue(0);
    pthread_join(sender_context.thread_id, &tp);

    net_close_socket(thiz->socket);
    net_close_socket(thiz->echo_socket);

    pipe_write(thiz->pipe_fd, 'd');

    p_mem_pool->Destroy();
    p_data_queue->Destroy();
    p_free_queue->Destroy();

    free(thiz);

    LOG_NOTICE("echo client receiver loop end\n");

    return NULL;
}

static void* __echo_client_thread(void* p)
{
    s_test_echo_client_receiver* thiz = (s_test_echo_client_receiver*) p;
    TSocketHandler socket = thiz->socket;
    unsigned char read_buf[2048];
    int ret = 0;
    int len = 0;

    LOG_NOTICE("echo client loop begin\n");

    while (run_flag) {

        len = D_HEADER_LENGTH;
        ret = net_recv(socket, read_buf, len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (ret != len) {
            LOG_ERROR("read header fail\n");
            break;
        }

        len = (((unsigned int)read_buf[1] << 16) | ((unsigned int)read_buf[2] << 8) | ((unsigned int)read_buf[3]));
        ret = net_recv(socket, read_buf + D_HEADER_LENGTH, len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (ret != len) {
            LOG_ERROR("read packet fail\n");
            break;
        }

        ret = net_send(thiz->echo_socket, read_buf, len + D_HEADER_LENGTH, 0);
        if (ret != (len + D_HEADER_LENGTH)) {
            LOG_ERROR("send packet fail, %d, %d\n", ret, len + D_HEADER_LENGTH);
            break;
        }
    }

    net_close_socket(thiz->socket);
    net_close_socket(thiz->echo_socket);

    pipe_write(thiz->pipe_fd, 'd');

    free(thiz);

    LOG_NOTICE("echo client loop end\n");

    return NULL;
}

static void* __echo_server_thread(void* p)
{
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;
    TSocketHandler mServerFd;
    TSocketHandler mClientFd;

    int client_number = 0;
    fd_set mAllSet;
    fd_set mReadSet;
    int nready = 0;
    s_test_server* p_server = (s_test_server*) p;
    s_test_echo_client_receiver* p_client;
    int reconnect_count = 0;

    char char_buffer;

    int ret = pipe_create(mPipeFd);
    if (ret) {
        LOG_ERROR("pipe_create fail\n");
        return NULL;
    }

    mServerFd = p_server->server_socket;

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    FD_SET(mServerFd, &mAllSet);
    if (mServerFd < mPipeFd[0]) {
        mMaxFd = mPipeFd[0];
    } else {
        mMaxFd = mServerFd;
    }

    p_server->pipe_fd = mPipeFd[1];

    while (run_flag) {
        mReadSet = mAllSet;

        nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
        if (0 > nready) {
            LOG_FATAL("select fail\n");
            break;
        } else if (nready == 0) {
            LOG_INFO("select return 0?\n");
            continue;
        }

        if (FD_ISSET(mPipeFd[0], &mReadSet)) {
            pipe_read(mPipeFd[0], &char_buffer);
            nready --;
            if ('q' == char_buffer) {
                LOG_NOTICE("quit.\n");
                break;
            } else if ('d' == char_buffer) {
                LOG_NOTICE("client decrease, %d\n", client_number);
                if (client_number > 0) {
                    client_number --;
                }
            } else {
                LOG_ERROR("not expected char %c\n", char_buffer);
                break;
            }

            if (nready <= 0) {
                LOG_INFO("read done.\n");
                continue;
            }
        }

        if (FD_ISSET(mServerFd, &mReadSet)) {
            if (client_number) {
                LOG_WARN("client connected.\n");
                continue;
            }

            //new client's request comes
            struct sockaddr_in client_addr;
            int ret_err = 0;
            TSocketSize clilen = sizeof(struct sockaddr_in);

            nready --;

do_retry:
            mClientFd = net_accept(mServerFd, (void *)&client_addr, (TSocketSize*)&clilen, &ret_err);
            if (mClientFd < 0) {
                if (1 == ret_err) {
                    LOG_WARN("non-blocking call?\n");
                    goto do_retry;
                }
                LOG_ERROR("accept fail, return %d.\n", mClientFd);
            }

            LOG_NOTICE("new client of echo server: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), mClientFd);
            if (nodelaysocket) {
                socket_set_no_delay(mClientFd);
            }
            socket_set_recv_buffer_size(mClientFd, 64 * 1024);
            socket_set_send_buffer_size(mClientFd, 8 * 1024);

            p_client = (s_test_echo_client_receiver*) malloc(sizeof(s_test_echo_client_receiver));
            if (p_client) {
                p_client->socket = mClientFd;
                reconnect_count = 0;

re_connect:
                p_client->echo_socket = net_connect_to(inet_ntoa(client_addr.sin_addr), D_DELAY_SERVER_PORT, SOCK_STREAM, IPPROTO_TCP);
                if (0 > p_client->echo_socket) {
                    if (reconnect_count < 8) {
                        reconnect_count ++;
                        __delay_500ms();
                        LOG_NOTICE("connect back fail, retry.\n");
                        goto re_connect;
                    } else {
                        LOG_NOTICE("connect back fail, retry.\n");
                        break;
                    }
                }
                LOG_NOTICE("connect back done, %d\n", p_client->echo_socket);
                if (nodelaysocket) {
                    socket_set_no_delay(p_client->echo_socket);
                }
                socket_set_recv_buffer_size(p_client->echo_socket, 8 * 1024);
                socket_set_send_buffer_size(p_client->echo_socket, 64 * 1024);

                p_client->pipe_fd = mPipeFd[1];
                if (queue_data) {
                    pthread_create(&p_client->thread_id, NULL, __echo_client_receiver_thread, (void*) p_client);
                } else {
                    pthread_create(&p_client->thread_id, NULL, __echo_client_thread, (void*) p_client);
                }
            } else {
                LOG_ERROR("no memory.\n");
                break;
            }

            client_number ++;
        }

    }

    p_server->pipe_fd = -1;

    return NULL;
}

static void* __delay_client_thread(void* p)
{
    s_test_delay_client* thiz = (s_test_delay_client*) p;
    TSocketHandler socket = thiz->socket;
    unsigned char read_buf[2048];
    unsigned char header[D_HEADER_LENGTH] = {0};
    int ret = 0;
    int len = 0;
    int frame_size = 0;
    int frame_count = 0;

    LOG_NOTICE("delay client loop begin\n");

    while (run_flag) {

        len = D_HEADER_LENGTH;
        ret = net_recv(socket, read_buf, len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (ret != len) {
            LOG_ERROR("read header fail\n");
            break;
        }

        len = (((unsigned int)read_buf[1] << 16) | ((unsigned int)read_buf[2] << 8) | ((unsigned int)read_buf[3]));
        ret = net_recv(socket, read_buf + D_HEADER_LENGTH, len, DNETUTILS_RECEIVE_FLAG_READ_ALL);
        if (ret != len) {
            LOG_ERROR("read packet fail\n");
            break;
        }

        if (!read_buf[0]) {
            if (frame_count) {
                unsigned int sec, usec, diff;
                __get_time(&sec, &usec);
                unsigned int* p_sec, *p_usec;
                p_sec = (unsigned int*)(header + 4);
                p_usec = (unsigned int*)(header + 8);
                if (p_sec[0] != sec) {
                    diff = (sec - p_sec[0]) * 1000000;
                    diff += usec;
                    diff -= p_usec[0];
                } else {
                    diff = usec - p_usec[0];
                }
                frame_size = (((unsigned int)header[12] << 24) | (unsigned int)header[13] << 16) | ((unsigned int)header[14] << 8) | ((unsigned int)header[15]);
                LOG_NOTICE("frame index %d, size %d, echo delay %d us\n", frame_count - 1, frame_size, diff);
                if (verbose) {
                    LOG_INFO("\tcur sec %d, usec %d, send sec %d, usec %d\n", sec, usec, p_sec[0], p_usec[0]);
                }
            }
            memcpy(header, read_buf, D_HEADER_LENGTH);
            frame_count ++;
        }

    }

    net_close_socket(thiz->socket);

    pipe_write(thiz->pipe_fd, 'd');

    free(thiz);

    LOG_NOTICE("delay client loop end\n");

    return NULL;
}

static void* __delay_server_thread(void* p)
{
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;
    TSocketHandler mServerFd;
    TSocketHandler mClientFd;

    int client_number = 0;
    fd_set mAllSet;
    fd_set mReadSet;
    int nready = 0;
    s_test_server* p_server = (s_test_server*) p;
    s_test_delay_client* p_client;

    char char_buffer;

    int ret = pipe_create(mPipeFd);
    if (ret) {
        LOG_ERROR("pipe_create fail\n");
        return NULL;
    }

    mServerFd = p_server->server_socket;

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    FD_SET(mServerFd, &mAllSet);
    if (mServerFd < mPipeFd[0]) {
        mMaxFd = mPipeFd[0];
    } else {
        mMaxFd = mServerFd;
    }

    LOG_NOTICE("mMaxFd %d, mServerFd %d, mPipeFd[0] %d\n", mMaxFd, mServerFd, mPipeFd[0]);

    p_server->pipe_fd = mPipeFd[1];

    while (run_flag) {
        mReadSet = mAllSet;

        nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);

        if (0 > nready) {
            LOG_FATAL("select fail\n");
            break;
        } else if (nready == 0) {
            LOG_INFO("select return 0?\n");
            continue;
        }

        if (FD_ISSET(mPipeFd[0], &mReadSet)) {
            pipe_read(mPipeFd[0], &char_buffer);
            nready --;
            if ('q' == char_buffer) {
                LOG_NOTICE("quit.\n");
                break;
            } else if ('d' == char_buffer) {
                LOG_NOTICE("client decrease, %d\n", client_number);
                if (client_number > 0) {
                    client_number --;
                }
            } else {
                LOG_ERROR("not expected char %c\n", char_buffer);
                break;
            }

            if (nready <= 0) {
                LOG_INFO("read done.\n");
                continue;
            }
        }

        if (FD_ISSET(mServerFd, &mReadSet)) {
            if (client_number > 2) {
                LOG_WARN("client connected.\n");
                continue;
            }

            //new client's request comes
            struct sockaddr_in client_addr;
            int ret_err = 0;
            TSocketSize clilen = sizeof(struct sockaddr_in);

            nready --;
            LOG_NOTICE("before accept\n");

do_retry:
            mClientFd = net_accept(mServerFd, (void *)&client_addr, (TSocketSize*)&clilen, &ret_err);
            if (mClientFd < 0) {
                if (1 == ret_err) {
                    LOG_WARN("non-blocking call?\n");
                    goto do_retry;
                }
                LOG_ERROR("accept fail, return %d.\n", mClientFd);
            }

            LOG_INFO("new client of delay server: %s, port %d, socket %d.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), mClientFd);

            if (nodelaysocket) {
                socket_set_no_delay(mClientFd);
            }
            socket_set_recv_buffer_size(mClientFd, 64 * 1024);
            socket_set_send_buffer_size(mClientFd, 8 * 1024);

            p_client = (s_test_delay_client*) malloc(sizeof(s_test_delay_client));
            if (p_client) {
                p_client->socket = mClientFd;
                p_client->pipe_fd = mPipeFd[1];
                pthread_create(&p_client->thread_id, NULL, __delay_client_thread, (void*) p_client);
            } else {
                LOG_ERROR("no memory.\n");
                break;
            }

            client_number ++;
        }

    }

    p_server->pipe_fd = -1;

    return NULL;
}

static int test_echo(int block)
{
    s_test_server context;
    pthread_t id;
    char c = 0;

    context.pipe_fd = -1;

    context.server_socket = net_setup_stream_socket(INADDR_ANY, D_ECHO_SERVER_PORT, 0);
    if (!DIsSocketHandlerValid(context.server_socket)) {
        LOG_ERROR("socket fail, %d\n", context.server_socket);
        return (-1);
    }

    LOG_NOTICE("echo server, setup socket done, %d\n", context.server_socket);

    pthread_create(&id, NULL, __echo_server_thread, &context);

    if (block) {
        while (run_flag) {
            printf("press 'q' to quit\n");
            scanf("%c", &c);

            if (c == 'q') {
                if (DIsSocketHandlerValid(context.pipe_fd)) {
                    pipe_write(context.pipe_fd, 'q');
                }
                run_flag = 0;
            }
        }
    }

    return 0;
}

static int test_delay()
{
    s_test_server context;
    pthread_t id;
    TSocketHandler send_fd = 0;
    long long cur_time = 0;
    unsigned int time_sec, time_usec;
    unsigned int frame_index = 0;
    int ret = 0;

    context.pipe_fd = -1;

    context.server_socket = net_setup_stream_socket(INADDR_ANY, D_DELAY_SERVER_PORT, 0);
    if (!DIsSocketHandlerValid(context.server_socket)) {
        LOG_ERROR("socket fail, %d\n", context.server_socket);
        return (-6);
    }

    LOG_NOTICE("delay server, setup socket done, %d\n", context.server_socket);

    pthread_create(&id, NULL, __delay_server_thread, &context);

    send_fd = net_connect_to(echo_server_url, D_ECHO_SERVER_PORT, SOCK_STREAM, IPPROTO_TCP);
    if (!DIsSocketHandlerValid(send_fd)) {
        LOG_ERROR("connect echo server(%s) fail\n", echo_server_url);
        if (DIsSocketHandlerValid(context.pipe_fd)) {
            pipe_write(context.pipe_fd, 'q');
        }
        run_flag = 0;
        return (-1);
    }
    if (nodelaysocket) {
        socket_set_no_delay(send_fd);
    }
    socket_set_recv_buffer_size(send_fd, 8 * 1024);
    socket_set_send_buffer_size(send_fd, 64 * 1024);

    LOG_NOTICE("connect done, %d\n", send_fd);

    __delay_500ms();

#ifdef BUILD_OS_WINDOWS
    test_read_iav= 0;
#endif

    if (!test_read_iav) {
        long long target_time = 0;
        unsigned int idr_cooldown = 0;

        cur_time = __get_time_0(&time_sec, &time_usec);
        target_time = cur_time + frame_gap;

        while (run_flag) {
            cur_time = __get_time_0(&time_sec, &time_usec);
            if ((cur_time + 500) > target_time) {
                ret = __send_fake_frame(send_fd, time_sec, time_usec, !idr_cooldown);
                if (ret) {
                    break;
                }
                if (verbose) {
                    LOG_INFO("send frame %d, sec %d, usec %d, curtime %lld, target %lld, diff %lld\n", frame_index, time_sec, time_usec, cur_time, target_time, cur_time - target_time);
                } else {
                    LOG_INFO("send frame %d\n", frame_index);
                }
                frame_index ++;
                target_time = target_time + frame_gap;
                if ((idr_cooldown + 2) > idr_interval) {
                    idr_cooldown = 0;
                } else {
                    idr_cooldown ++;
                }
            } else {
                if (target_time > (cur_time + 1000)) {
                    __delay_us(1000);
                } else {
                    __delay_us(target_time - cur_time - 500);
                }
            }
        }
    } else {
#ifndef BUILD_OS_WINDOWS
        int iav_fd = __iav_open_handle();
        s_iav_map_bsb mapbsb;
        s_iav_read_bitstream readbs;
        int till_idr = 0;

        if (0 > iav_fd) {
            LOG_ERROR("__iav_open_handle fail, ret %d\n", iav_fd);
            return (-1);
        }

        ret = __iav_map_bsb(iav_fd, &mapbsb);
        if (ret) {
            LOG_ERROR("__iav_map_bsb fail, ret %d\n", ret);
            __iav_close_handle(iav_fd);
            return (-1);
        }

        while (run_flag) {
            readbs.stream_id = stream_id;
            ret = __iav_read_bitstream(iav_fd, &readbs);
            if (ret) {
                LOG_ERROR("read bitstream fail\n");
                break;
            }
            if (!till_idr) {
                unsigned char nal_type = 0xff;
                __next_frame_start_code(((unsigned char*)mapbsb.base) + readbs.offset, 512, &nal_type);
                if (5 != nal_type) {
                    //LOG_INFO("skip before first idr\n");
                    continue;
                }
                till_idr = 1;
            }
            cur_time = __get_time_0(&time_sec, &time_usec);
            ret = __send_iav_frame(send_fd, time_sec, time_usec, readbs.size);
            if (ret) {
                break;
            }
            if (verbose) {
                LOG_INFO("send frame %d, sec %d, usec %d, curtime %lld\n", frame_index, time_sec, time_usec, cur_time);
            } else {
                LOG_INFO("send frame %d\n", frame_index);
            }
            frame_index ++;
        }

        __iav_close_handle(iav_fd);
#endif
    }

    if (DIsSocketHandlerValid(context.pipe_fd)) {
        pipe_write(context.pipe_fd, 'q');
    }
    run_flag = 0;

    return 0;
}

static void __print_ut_options()
{
    printf("test_network_delay options:\n");

    printf("\t'--echo': start echo server\n");
    printf("\t'--delay [echo server url]': start delay server, and specify echo server url\n");

    printf("\t'--idrsize [%%d]': specify key frame size\n");
    printf("\t'--psize [%%d]': specify non-key frame size\n");
    printf("\t'--idrinterval [%%d]': specify idr interval\n");
    printf("\t'--framegap [%%d]': specify framegap (us)\n");
    printf("\t'--verbose': show detailed time info\n");
    printf("\t'--notusenodelaysocket': do not use socket option 'no delay'\n");
    printf("\t'--usenodelaysocket': use socket option 'no delay'\n");
    printf("\t'--queuedata': use one thread receive, one thread send\n");
    printf("\t'--notqueuedata': use one thread receive and send\n");
    printf("\t'--readiav': simulate read iav data\n");
    printf("\t'--notreadiav': use fake data\n");
    printf("\t'--help': show help\n");

    printf("\n\t'example': start echo server:\n");
    printf("\t\tstart echo server: 'test_network_delay --echo'\n");
    printf("\t\tstart delay test: 'test_network_delay --delay 10.0.0.1'\n");

    printf("\t\t\tdelay test with parameters: 'test_network_delay --delay 10.0.0.1 --idrsize %d --psize %d --idrinterval %ld --framegap(us) %lld'\n", idr_size, p_size, idr_interval, frame_gap);
}

static void __print_ut_cmds()
{
    printf("test_network_delay runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit\n");
}

static int init_test_network_delay_params(int argc, char **argv)
{
    int i = 0;
    int v = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp("--delay", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(echo_server_url, 64, "%s", argv[i + 1]);
                start_delay_test = 1;
                i ++;
                printf("[input argument] --delay, echo server: %s.\n", echo_server_url);
            } else {
                printf("[input argument] --delay: should follow echo server url.\n");
                return (-2);
            }
        } else if(!strcmp("--idrsize", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &v);
                if ((80000000 < v) || (100 > v)) {
                    printf("not valid idrsize %d\n", v);
                    return (-3);
                }
                i ++;
                printf("[input argument] --idrsize %d.\n", v);
                idr_size = v;
            } else {
                printf("[input argument] --idrsize: should follow idr size\n");
                return (-2);
            }
        } else if(!strcmp("--psize", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &v);
                if ((8000000 < v) || (4 > v)) {
                    printf("not valid psize %d\n", v);
                    return (-3);
                }
                i ++;
                printf("[input argument] --psize %d.\n", v);
                p_size = v;
            } else {
                printf("[input argument] --psize: should follow p size\n");
                return (-2);
            }
        } else if(!strcmp("--idrinterval", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &v);
                if ((4096 < v) || (1 > v)) {
                    printf("not valid idr interval %d\n", v);
                    return (-3);
                }
                i ++;
                printf("[input argument] --idrinterval %d.\n", v);
                idr_interval = v;
            } else {
                printf("[input argument] --idrinterval: should follow idrinterval\n");
                return (-2);
            }
        } else if(!strcmp("--framegap", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &v);
                if ((2000000 < v) || (2000 > v)) {
                    printf("not valid framegap %d\n", v);
                    return (-3);
                }
                i ++;
                printf("[input argument] --framegap %d.\n", v);
                frame_gap = v;
            } else {
                printf("[input argument] --framegap: should follow framegap(us)\n");
                return (-2);
            }
        } else if (!strcmp("--echo", argv[i])) {
            start_echo_server = 1;
        } else if (!strcmp("--notusenodelaysocket", argv[i])) {
            nodelaysocket = 0;
            printf("[input argument] --notusenodelaysocket.\n");
        } else if (!strcmp("--usenodelaysocket", argv[i])) {
            nodelaysocket = 1;
            printf("[input argument] --usenodelaysocket.\n");
        } else if (!strcmp("--queuedata", argv[i])) {
            queue_data = 1;
        } else if (!strcmp("--notqueuedata", argv[i])) {
            queue_data = 0;
        } else if (!strcmp("--verbose", argv[i])) {
            verbose = 1;
        } else if (!strcmp("--readiav", argv[i])) {
            test_read_iav = 1;
        } else if (!strcmp("--notreadiav", argv[i])) {
            test_read_iav = 0;
        } else if (!strcmp("--help", argv[i])) {
            __print_ut_options();
            __print_ut_cmds();
        } else {
            printf("error: NOT processed option(%s).\n", argv[i]);
            __print_ut_options();
            __print_ut_cmds();
            return (-1);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;

    if (argc == 1) {
        __print_ut_options();
        __print_ut_cmds();
        printf("not sepcify options, try default: as echo server\n");
        start_echo_server = 1;
        start_delay_test = 0;
    } else {
        if ((ret = init_test_network_delay_params(argc, argv)) < 0) {
            printf("[error]: init_test_network_delay_params() fail.\n");
            return (-2);
        }
    }

    network_init();

    if (start_echo_server) {
        if (!start_delay_test) {
            test_echo(1);
        } else {
            test_echo(0);
        }
    }

    if (start_delay_test) {
        test_delay();
    }

    network_deinit();

    return 0;
}

