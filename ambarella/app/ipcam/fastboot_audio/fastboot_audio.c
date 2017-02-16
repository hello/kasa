/*
 * fastboot_audio.c --
*      get audio data (pcm)  by amboot_audiorecord and handle it.
 *
 * History:
 *	2014/12/12 - [Jian Liu] create this file
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>

#include "getopt_s.h" /* for local getopt()  */

typedef unsigned int u32;

static void usage(char *string) {
    printf("usage: %s  [-r <start_address>] [-s  <buffer_size>]  [-m]  [-d <duration>] -f <filePath>\n"
        "    or %s  -f <source filePath> -t  <type>   #resample pcm data\n"
        "where  \n"
        "    -r  <start_address>, must be hex data (\"0x\"), default, 0x07700000\n"
        "    -s  <buffer_size> must be hex data (\"0x\"), default, 0x400000\n"
        "    -m,  save pcm data of left channel only, mono, default mono\n "
        "    -d <duration>, seconds, default 30, no more than 300\n"
        "    -t <type>, S16, type: 0 -- 48K_mono to 8K_mono, 1 -- 48K_Stereo to 8K_mono\n",
	 string,string);
    exit(1);
}

static int stop_audio_encode(void){
    //echo 6 > /proc/ambarella/dma
    system("echo 6 > /proc/ambarella/dma");
    return 0;
}

static void setscheduler(void)
{
    struct sched_param sched_param;
    if (sched_getparam(0, &sched_param) < 0) {
        //printf("Scheduler getparam failed...\n");
        return;
    }
    sched_param.sched_priority = sched_get_priority_max(SCHED_RR);
    if (!sched_setscheduler(0, SCHED_RR, &sched_param)) {
        //printf("Scheduler set to Round Robin with priority %i...\n", sched_param.sched_priority);
        return;
    }
    printf("!!!Scheduler set to Round Robin with priority %i FAILED!!!\n", sched_param.sched_priority);
}

static int handle_fastboot_audiodata(u32 op_address, u32 op_size, u32 duration,char *filename,int mono_flag);
static int audiodata_resample(char *filename,int resample_type);

int main (int argc, char **argv)
{
    u32 op_address = 0x07700000;
    u32 op_size = 0x400000;
    u32 duration_size = 0;
    int   duration = 30;
    int  mono_flag = 0;
    char *filename = NULL;
    int  resample_type = -1;
    int c;

    setscheduler();

    /* check args */
    while (1) {
        c = getopt_s(argc, argv, "r:s:f:d:mt:");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'r':
            if( sscanf(optarg_s, "0x%08x", &op_address) != 1)
                usage(argv[0]);
            break;
        case 's':
            if( sscanf(optarg_s, "0x%08x", &op_size) != 1)
                usage(argv[0]);
            break;
        case 'f':
            filename = optarg_s;
            break;
        case 'd':
            duration = atoi(optarg_s);
            break;
        case 'm':
            mono_flag = 1;
            break;
        case 't':
            resample_type = atoi(optarg_s);
            break;
        default:
            usage(argv[0]);
        }
    }

    if(resample_type != -1){
        if(resample_type != 0 && resample_type != 1){
            usage(argv[0]);
        }
        if(!filename){
            usage(argv[0]);
        }
        audiodata_resample(filename,resample_type);
        return 0;
    }
    if(duration < 0 || duration > 300) {
        usage(argv[0]);
    }
    duration_size = duration * 4 * 48000;//48K,S16_LE,Stereo
    handle_fastboot_audiodata(op_address,op_size,duration_size,filename,mono_flag);
    stop_audio_encode();
    return 0;
}

static int do_handle_data(char *pmem, u32 start, u32 size_limit,u32 address, u32 size, u32 duration_size,unsigned char *pcm_data,int mono_flag);
static int handle_fastboot_audiodata(u32 op_address, u32 op_size, u32 duration_size,char *filename,int mono_flag)
{
    #define PAGE_SHIFT 12

    int fd = -1,fd2 = -1;
    char *debug_mem = NULL;
    u32 map_start = 0;

    int input_data_size;
    u32  pcm_data_size = duration_size >> mono_flag;
    unsigned char *pcm_data = (unsigned char *)malloc(sizeof(unsigned char) * pcm_data_size);
    if(!pcm_data){
        printf("NOT ENOUGH MEMORY TO SAVE INPUT PCM DATA\n");
        goto exit;
    }

    if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
        perror("/dev/ambad");
        goto exit;
    }

    map_start = op_address;
    map_start >>= PAGE_SHIFT;
    map_start <<= PAGE_SHIFT;
    debug_mem = (char *)mmap(NULL, op_address - map_start + op_size,PROT_READ|PROT_WRITE, MAP_SHARED, fd, map_start);
    if ((int)debug_mem < 0) {
        perror("mmap");
        goto exit;
    }
    input_data_size = do_handle_data(debug_mem, map_start, 0xffffffff, op_address, op_size,duration_size,pcm_data,mono_flag);

    if (debug_mem){
        munmap(debug_mem, op_size);
    }
    close(fd);

    if(input_data_size){
        if ((fd2 = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
            perror(filename);
           goto exit;
        }
        if (write(fd2, (void *)pcm_data, input_data_size) < 0) {
            perror("handle_fastboot_audiodata --- write");
            goto exit;
        }
        close(fd2);
    }
    if(pcm_data){
        free(pcm_data);
    }
    return 0;
exit:
    if(pcm_data){
        free(pcm_data);
    }
    if(fd != -1){
        close(fd);
    }
    if(fd2 != -1){
        close(fd2);
    }
    return -1;
}

/* n1: number of samples */
static void mono_left_channel(short *output, short *input, int n1)
{
    short *p, *q;
    int n = n1;

    p = input;
    q = output;
    while (n >= 4) {
        q[0] = p[0];
        q[1] = p[2];
        q[2] = p[4];
        q[3] = p[6];
        q += 4;
        p += 8;
        n -= 4;
    }
    while (n > 0) {
        q[0] = p[0];
        q++;
        p += 2;
        n--;
    }
}

#define  MAX_SAMPLE_NUMBER 1024
#define  CHUNK_SIZE  (MAX_SAMPLE_NUMBER * sizeof(u32))
static unsigned char data_buffer[CHUNK_SIZE];
static unsigned char mono_left[CHUNK_SIZE/2];

static inline int is_sample_valid(u32 *pdata, u32 *pstart, u32 *pend,u32 flag){
    #define NEXT_SAMPLE_PTR(curr) ((curr >= pend) ? pstart : (curr + 1))
    u32 *data = pdata;
    u32 *data1 = NEXT_SAMPLE_PTR(data);
    u32 *data2 = NEXT_SAMPLE_PTR(data1);
    u32 *data3 = NEXT_SAMPLE_PTR(data2);
    if(*data != flag || * data1 != flag || *data2 != flag  || *data3 != flag){
        return 1;
    }
    return 0;
}
static int do_handle_data(char *pmem, u32 start, u32 size_limit,u32 address, u32 size, u32 duration_size,unsigned char *pcm_data_,int mono_flag)
{
    u32 *pstart,*pend,*pdata;
    u32 flag = 0xdeadbeaf;
    u32 record_size = 0;

    u32 *input_data = (u32*)data_buffer;
    int sample_count = 0;
    unsigned char *pcm_data = pcm_data_;

    pstart = (u32 *)pmem;
    pend = (u32 *)(pmem + size - 4);
    pdata = pstart;
    while(record_size < duration_size){
        int count = 0;
        while(!is_sample_valid(pdata,pstart,pend,flag)){
            usleep(1000 * 10);
            ++count;
            if(count > 20){
                printf("ERROR, audio capture stopped?\n");
                goto exit;
            }
            //TODO
            continue;
        }
        input_data[sample_count++] = *pdata;
        *pdata = flag;//set invalid data flag

        if(sample_count >= MAX_SAMPLE_NUMBER){
            size_t len = sample_count * sizeof(u32);
            void *pcm = (void*)input_data;
            if(mono_flag){
                int sample_numbers = sample_count;
                mono_left_channel((short*)mono_left,(short*)input_data,sample_numbers);
                len/=2;
                pcm = (void*)mono_left;
            }
            memcpy(pcm_data,pcm,len);
            pcm_data += len;

            sample_count = 0;
        }
        if(pdata >= pend){
            pdata = pstart;
        }else{
            ++pdata;
        }
        record_size += sizeof(u32);
    }

exit:
    if(sample_count){
        size_t len = sample_count * sizeof(u32);
        void *pcm = (void*)input_data;
        if(mono_flag){
             int sample_numbers = sample_count;
             mono_left_channel((short*)mono_left,(short*)input_data,sample_numbers);
             len/=2;
             pcm = (void*)mono_left;
        }
        memcpy(pcm_data,pcm,len);
        pcm_data += len;

        sample_count = 0;
    }
    return (pcm_data - pcm_data_);
}


#include "speex/speex_resampler.h"
#define MAX_RESAMPLE_INPUT_LEN  3072
static int audiodata_resample(char *filename,int resample_type){
    if(resample_type != 0 && resample_type != 1){
        return -1;
    }

    int fd_in = -1,fd_out = -1;
    char filename_out[1024];
    sprintf(filename_out,"%s_%d",filename,resample_type);

    //create resampler, 48K to 8K
    SpeexResamplerState *resampler = NULL;
    int quality = 3;//good enough and fast
    resampler = speex_resampler_init(1, 48000, 8000, quality,NULL);
    if(!resampler){
        return -1;
    }

    if((fd_in = open(filename, O_RDONLY, 0644)) < 0) {
         perror(filename);
        return -1;
    }
    if((fd_out = open(filename_out, O_CREAT | O_TRUNC | O_WRONLY)) < 0) {
        perror(filename_out);
        return -1;
    }

    unsigned char tmp_buf[MAX_RESAMPLE_INPUT_LEN];
    unsigned char stereo_to_mono_buf[MAX_RESAMPLE_INPUT_LEN/2];
    unsigned char resample_result_buf[MAX_RESAMPLE_INPUT_LEN];

    int bytes_to_write;
    while(1){
        int bytes_read = read(fd_in,tmp_buf,sizeof(tmp_buf));
        if(bytes_read <= 0) goto exit;

        {
            unsigned char *input = tmp_buf;
            int input_len = bytes_read;
            if(resample_type == 1){
                int sample_numbers = input_len/4;
                mono_left_channel((short*)stereo_to_mono_buf,(short*)input,sample_numbers);
                input = stereo_to_mono_buf;
                input_len = bytes_read/2;
           }
           spx_uint32_t in_len = input_len/sizeof(short);//samples
           spx_uint32_t out_len = MAX_RESAMPLE_INPUT_LEN;
           speex_resampler_process_int (resampler,0,(short *)input, &in_len, (short *)resample_result_buf, &out_len);
           bytes_to_write = out_len * sizeof(short);
        }

        int bytes_written = write(fd_out,resample_result_buf,bytes_to_write);
        if(bytes_written != bytes_to_write){
            goto exit;
        }
        if(bytes_read < sizeof(tmp_buf)){
            break;
        }
    }
exit:
    if(fd_in != -1) close(fd_in),fd_in = -1;
    if(fd_out != -1) close(fd_out),fd_out = -1;
    if(resampler){
        speex_resampler_destroy(resampler);
    }
    return 0;
}

