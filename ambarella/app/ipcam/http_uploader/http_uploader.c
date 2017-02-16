/*
 * http_uploader.c -- get es data from dsp and uploading to http server
 *
 * History:
 *	2014/12/03 - [Chu Chen] create this file base on test_encode.c
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include <curl/curl.h>

static int fd_iav = -1;

static int current_stream = 0; //defalt stream A
static int duration = 300;// frame count
static int enable_uploading = 0;
#define MAX_LENGTH_HTTP_URL 128
char http_server_url[MAX_LENGTH_HTTP_URL] = {0};

u8 *bsb_mem = NULL;
u32 bsb_size = 0;
/////////////////////////////////////////////////////////////
void usage(void)
{
    printf("http_uploader usage:\n");
    printf("http_uploader [--duration total_frame_count] --httpurl http_server_url:port\n");
}

static int init_param(int argc, char **argv)
{
    int i =0;

    for (i = 1; i < argc; i++) {
        if (!strcmp("--httpurl", argv[i])) {
            snprintf(http_server_url, MAX_LENGTH_HTTP_URL, "http://%s", argv[i+1]);
            printf("http_server_url %s\n", http_server_url);
            enable_uploading = 1;
            i++;
        } else if (!strcmp("--duration", argv[i])) {
            sscanf(argv[i + 1], "%d", &duration);
            i++;
        }
    }

    printf("init_param done:\n");
    printf("http_server_url %s\n", http_server_url);
    printf("duration %d\n", duration);
    printf("current_stream %d\n", current_stream);

    return 0;
}


int map_bsb(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_BSB;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	bsb_size = querybuf.length;
	bsb_mem = mmap(NULL, bsb_size * 2, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
	if (bsb_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
	return 0;
}

static int do_write_video_frame(FILE* p_file)
{
    struct iav_stream_info stream_info;
    struct iav_querydesc query_desc;
    struct iav_framedesc *frame_desc;

    u32 total_frames = 0;

    stream_info.id = current_stream;
    printf("query stream(%d) status:\n", current_stream);
    if (ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &stream_info) < 0) {
        perror("IAV_IOC_GET_STREAM_INFO");
        return -1;
    }
    if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
        printf("current_stream: %d, status is %d, not IAV_STREAM_STATE_ENCODING!\n", current_stream, stream_info.state);
        //return -1;
    }
    printf("query stream(%d) status done\n", current_stream);

    unsigned int whole_pic_size=0;
    u32 pic_size = 0;

    int led_switch = 0;
    char cmd[128] = {0};
    while (1) {
#if 1
        memset(&query_desc, 0, sizeof(query_desc));
        frame_desc = &query_desc.arg.frame;
        query_desc.qid = IAV_DESC_FRAME;
        frame_desc->id = -1;
        if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
            perror("IAV_IOC_QUERY_DESC");
            return -1;
        }

        if (frame_desc->stream_end) {
            printf("do_write_video_frame: get stream END flag! total_frames %d!\n", total_frames);
            return 0;
        }

        pic_size = frame_desc->size;
        whole_pic_size  += (pic_size & (~(1<<23)));

        if (pic_size>>23) {
            //end of frame
            pic_size = pic_size & (~(1<<23));
            //padding some data to make whole picture to be 32 byte aligned
            pic_size += (((whole_pic_size + 31) & ~31)- whole_pic_size);
            //rewind whole pic size counter
            // printf("whole %d, pad %d \n", whole_pic_size, (((whole_pic_size + 31) & ~31)- whole_pic_size));
            whole_pic_size = 0;
        }

        fwrite((void*)(bsb_mem + frame_desc->data_addr_offset), pic_size, 1, p_file);
#else
        usleep(30000);
#endif

        total_frames += 1;

        if (total_frames >= duration) {
            printf("do_write_video_frame get es done, total frames %d\n", total_frames);
            return 0;
        }

        if (total_frames%15 == 0) {
            //printf("do_write_video_frame, set led status %d\n", led_switch%2);
            snprintf(cmd, 128, "echo %d > /sys/class/backlight/0.pwm_bl/brightness", led_switch%2);
            system((const char*)cmd);
            led_switch++;
        }
    }

    return 0;
}

size_t do_read_data(void *buffer, size_t size, size_t nmemb, void *instream)
{
    FILE *p_fd = (FILE*)instream;
    return fread(buffer, size, nmemb, p_fd);
}

static int do_uploading_video_frame()
{
    CURL* p_easy_curl = NULL;
    CURLcode ret;
    FILE* p_file = NULL;

    if ((p_file = fopen("/tmp/es.h264", "r")) == NULL) {
        printf("fopen es file fail\n");
        return -1;
    }

    p_easy_curl = curl_easy_init();
    if (p_easy_curl == NULL) {
        printf("curl_easy_init fail\n");
        return -1;
    }

    curl_easy_setopt(p_easy_curl, CURLOPT_READFUNCTION, do_read_data);
    curl_easy_setopt(p_easy_curl, CURLOPT_READDATA, p_file);

    struct curl_httppost* p_formpost = NULL;
    struct curl_httppost* p_lastptr  = NULL;

    curl_formadd(&p_formpost, &p_lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);
    curl_formadd(&p_formpost, &p_lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, "/tmp/es.h264", CURLFORM_END);

    curl_easy_setopt(p_easy_curl, CURLOPT_URL, http_server_url);
    curl_easy_setopt(p_easy_curl, CURLOPT_HTTPPOST, p_formpost);

    ret = curl_easy_perform(p_easy_curl);
    if (ret != CURLE_OK) {
        printf("curl_easy_perform fail, %d\n", ret);
        curl_easy_cleanup(p_easy_curl);
        p_easy_curl= NULL;
        return -1;
    }

    curl_easy_cleanup(p_easy_curl);
    p_easy_curl= NULL;
    return 0;
}

static int do_send_msg(const char* msg)
{
    CURL* p_easy_curl = NULL;
    CURLcode ret;

    p_easy_curl = curl_easy_init();
    if (p_easy_curl == NULL) {
        printf("curl_easy_init fail\n");
        return -1;
    }

    char get_url[MAX_LENGTH_HTTP_URL + MAX_LENGTH_HTTP_URL] = {0};
    snprintf(get_url, MAX_LENGTH_HTTP_URL + MAX_LENGTH_HTTP_URL, "%s?action=%s", http_server_url, msg);

    curl_easy_setopt(p_easy_curl, CURLOPT_URL, get_url);

    ret = curl_easy_perform(p_easy_curl);
    if (ret != CURLE_OK) {
        printf("curl_easy_perform fail, %d\n", ret);
        curl_easy_cleanup(p_easy_curl);
        p_easy_curl= NULL;
        return -1;
    }

    curl_easy_cleanup(p_easy_curl);
    p_easy_curl= NULL;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return -1;
    }

    if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("open /dev/iav");
        return -1;
    }

    if (init_param(argc, argv) < 0) {
        return -1;
    }

    if (map_bsb() < 0) {
        printf("map bsb failed\n");
        return -1;
    }

    FILE* p_fd = fopen("/tmp/es.h264", "w");
    if (NULL == p_fd) {
        printf("fopen fail\n");
        return -1;
    }

    if (enable_uploading && (do_send_msg("encode_start") < 0)) {
        printf("do_send_msg 'encode_start' fail\n");
    }

    if (do_write_video_frame(p_fd) != 0) {
        printf("do_write_video_frame fail\n");
        fclose(p_fd);
        return -1;
    }

    //turn on led
    system("echo 1 > /sys/class/backlight/0.pwm_bl/brightness");
    fclose(p_fd);
    p_fd = NULL;

    if (enable_uploading) {
        if (do_send_msg("encode_stop") < 0) {
            printf("do_send_msg 'encode_stop' fail\n");
            return -1;
        }

        if (do_uploading_video_frame() < 0) {
            printf("do_uploading_video_frame fail\n");
            return -1;
        }

        if (do_send_msg("uploading_done") < 0) {
            printf("do_send_msg 'uploading_done' fail\n");
            return -1;
        }
    }

    return 0;
}

