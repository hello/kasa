/*
 * amba_dsp.cpp
 *
 * History:
 *    2015/07/31 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifdef BUILD_DSP_AMBA_S2L

#include "basetypes.h"
#include "iav_ioctl.h"

static TInt __s2l_enter_decode_mode(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config)
{
    struct iav_decode_mode_config decode_mode;
    TInt i = 0;

    memset(&decode_mode, 0x0, sizeof(decode_mode));

    if (mode_config->num_decoder > DAMBADSP_MAX_DECODER_NUMBER) {
        LOG_FATAL("BAD num_decoder %d\n", mode_config->num_decoder);
        return (-100);
    }
    decode_mode.num_decoder = mode_config->num_decoder;

    for (i = 0; i < decode_mode.num_decoder; i ++) {
        decode_mode.decoder_configs[i].max_frm_num = mode_config->decoder_configs[i].max_frm_num;
        decode_mode.decoder_configs[i].max_frm_width = mode_config->decoder_configs[i].max_frm_width;
        decode_mode.decoder_configs[i].max_frm_height = mode_config->decoder_configs[i].max_frm_height;
    }

    i = ioctl(iav_fd, IAV_IOC_ENTER_DECODE_MODE, &decode_mode);
    if (0 > i) {
        perror("IAV_IOC_ENTER_UDEC_MODE");
        LOG_FATAL("enter decode mode fail\n");
        return i;
    }

    return 0;
}

static TInt __s2l_leave_decode_mode(TInt iav_fd)
{
    TInt ret = ioctl(iav_fd, IAV_IOC_LEAVE_DECODE_MODE);

    if (0 > ret) {
        perror("IAV_IOC_LEAVE_DECODE_MODE");
        LOG_FATAL("leave decode mode fail\n");
    }

    return ret;
}

static TInt __s2l_create_decoder(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info)
{
    TInt i = 0;
    struct iav_decoder_info decoder_info;

    memset(&decoder_info, 0x0, sizeof(decoder_info));

    decoder_info.decoder_id = p_decoder_info->decoder_id;

    if (EAMDSP_VIDEO_CODEC_TYPE_H264 == p_decoder_info->decoder_type) {
        decoder_info.decoder_type = IAV_DECODER_TYPE_H264;
    } else if (EAMDSP_VIDEO_CODEC_TYPE_H265 == p_decoder_info->decoder_type) {
        decoder_info.decoder_type = IAV_DECODER_TYPE_H265;
    } else {
        LOG_FATAL("bad video codec type %d\n", p_decoder_info->decoder_type);
        return (-101);
    }
    decoder_info.num_vout = p_decoder_info->num_vout;
    decoder_info.setup_done = p_decoder_info->setup_done;
    decoder_info.width = p_decoder_info->width;
    decoder_info.height = p_decoder_info->height;

    if (decoder_info.num_vout > DAMBADSP_MAX_DECODE_VOUT_NUMBER) {
        LOG_FATAL("BAD num_vout %d\n", p_decoder_info->num_vout);
        return (-100);
    }

    for (i = 0; i < decoder_info.num_vout; i ++) {
        decoder_info.vout_configs[i].vout_id = p_decoder_info->vout_configs[i].vout_id;
        decoder_info.vout_configs[i].enable = p_decoder_info->vout_configs[i].enable;
        decoder_info.vout_configs[i].flip = p_decoder_info->vout_configs[i].flip;
        decoder_info.vout_configs[i].rotate = p_decoder_info->vout_configs[i].rotate;

        decoder_info.vout_configs[i].target_win_offset_x = p_decoder_info->vout_configs[i].target_win_offset_x;
        decoder_info.vout_configs[i].target_win_offset_y = p_decoder_info->vout_configs[i].target_win_offset_y;

        decoder_info.vout_configs[i].target_win_width = p_decoder_info->vout_configs[i].target_win_width;
        decoder_info.vout_configs[i].target_win_height = p_decoder_info->vout_configs[i].target_win_height;

        decoder_info.vout_configs[i].zoom_factor_x = p_decoder_info->vout_configs[i].zoom_factor_x;
        decoder_info.vout_configs[i].zoom_factor_y = p_decoder_info->vout_configs[i].zoom_factor_y;

        LOG_PRINTF("vout(%d), w %d, h %d, zoom x %08x, y %08x\n", i, decoder_info.vout_configs[i].target_win_width, decoder_info.vout_configs[i].target_win_height, decoder_info.vout_configs[i].zoom_factor_x, decoder_info.vout_configs[i].zoom_factor_y);
    }

    decoder_info.bsb_start_offset = p_decoder_info->bsb_start_offset;
    decoder_info.bsb_size = p_decoder_info->bsb_size;

    i = ioctl(iav_fd, IAV_IOC_CREATE_DECODER, &decoder_info);
    if (0 > i) {
        perror("IAV_IOC_CREATE_DECODER");
        LOG_FATAL("create decoder fail, ret %d\n", i);
        return i;
    }

    p_decoder_info->bsb_start_offset = decoder_info.bsb_start_offset;
    p_decoder_info->bsb_size = decoder_info.bsb_size;

    return 0;
}

static TInt __s2l_destroy_decoder(TInt iav_fd, TU8 decoder_id)
{
    TInt ret = ioctl(iav_fd, IAV_IOC_DESTROY_DECODER, decoder_id);

    if (0 > ret) {
        perror("IAV_IOC_DESTROY_DECODER");
        LOG_FATAL("destroy decoder fail, ret %d\n", ret);
    }

    return ret;
}

static int __s2l_decode_trick_play(int iav_fd, TU8 decoder_id, TU8 trick_play)
{
    int ret;
    struct iav_decode_trick_play trickplay;

    trickplay.decoder_id = decoder_id;
    trickplay.trick_play = trick_play;
    ret = ioctl(iav_fd, IAV_IOC_DECODE_TRICK_PLAY, &trickplay);
    if (0 > ret) {
        perror("IAV_IOC_DECODE_TRICK_PLAY");
        LOG_ERROR("trickplay error, ret %d\n", ret);
        return ret;
    }

    return 0;
}

static int __s2l_decode_start(int iav_fd, TU8 decoder_id)
{
    int ret = ioctl(iav_fd, IAV_IOC_DECODE_START, decoder_id);

    if (ret < 0) {
        perror("IAV_IOC_DECODE_START");
        LOG_ERROR("decode start error, ret %d\n", ret);
        return ret;
    }

    return 0;
}

static int __s2l_decode_stop(int iav_fd, TU8 decoder_id, TU8 stop_flag)
{
    int ret;
    struct iav_decode_stop stop;

    stop.decoder_id = decoder_id;
    stop.stop_flag = stop_flag;

    ret = ioctl(iav_fd, IAV_IOC_DECODE_STOP, &stop);
    if (0 > ret) {
        perror("IAV_IOC_UDEC_STOP");
        LOG_ERROR("decode stop error, ret %d\n", ret);
        return ret;
    }

    return 0;
}

static int __s2l_decode_speed(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction)
{
    int ret;
    struct iav_decode_speed spd;

    spd.decoder_id = decoder_id;
    spd.direction = direction;
    spd.speed = speed;
    spd.scan_mode = scan_mode;

    LOG_PRINTF("speed, direction %d, speed %x, scanmode %d\n", spd.direction, spd.speed, spd.scan_mode);
    ret = ioctl(iav_fd, IAV_IOC_DECODE_SPEED, &spd);
    if (0 > ret) {
        perror("IAV_IOC_DECODE_SPEED");
        LOG_ERROR("decode speed error, ret %d\n", ret);
        return ret;
    }

    return 0;
}

static int __s2l_decode_request_bits_fifo(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset)
{
    struct iav_decode_bsb wait;
    int ret;

    wait.decoder_id = decoder_id;
    wait.room = size;
    wait.start_offset = (TU32) cur_pos_offset;

    ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODE_BSB, &wait);
    if (0 > ret) {
        LOG_ERROR("IAV_IOC_WAIT_DECODE_BSB fail, ret %d.\n", ret);
        perror("IAV_IOC_WAIT_DECODE_BSB");
        return ret;
    }

    return 0;
}

static int __s2l_decode(int iav_fd, SAmbaDSPDecode *dec)
{
    TInt ret = 0;
    struct iav_decode_video decode_video;

    memset(&decode_video, 0, sizeof(decode_video));
    decode_video.decoder_id = dec->decoder_id;
    decode_video.num_frames = dec->num_frames;

    decode_video.start_ptr_offset = dec->start_ptr_offset;
    decode_video.end_ptr_offset = dec->end_ptr_offset;
    decode_video.first_frame_display = dec->first_frame_display;

    ret = ioctl(iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video);
    if (0 > ret) {
        perror("IAV_IOC_DECODE_VIDEO");
        LOG_ERROR("IAV_IOC_DECODE_VIDEO fail, ret %d.\n", ret);
        return ret;
    }

    return 0;
}

static int __s2l_decode_query_bsb_status_and_print(int iav_fd, TU8 decoder_id)
{
    int ret;
    struct iav_decode_bsb bsb;

    memset(&bsb, 0x0, sizeof(bsb));
    bsb.decoder_id = decoder_id;

    ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
    if (0 > ret) {
        LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, ret %d.\n", ret);
        perror("IAV_IOC_QUERY_DECODE_BSB");
        return ret;
    }

    LOG_PRINTF("[bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", bsb.start_offset, bsb.dsp_read_offset, bsb.room, bsb.free_room);

    return 0;
}

static int __s2l_decode_query_status_and_print(int iav_fd, TU8 decoder_id)
{
    int ret;
    struct iav_decode_status status;

    memset(&status, 0x0, sizeof(status));
    status.decoder_id = decoder_id;

    ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
    if (0 > ret) {
        perror("IAV_IOC_QUERY_DECODE_STATUS");
        LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, ret %d.\n", ret);
        return ret;
    }

    LOG_PRINTF("[decode status]: decode_state %d, decoded_pic_number %d, error_status %d, total_error_count %d, irq_count %d\n", status.decode_state, status.decoded_pic_number, status.error_status, status.total_error_count, status.irq_count);
    LOG_PRINTF("[decode status, bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", status.write_offset, status.dsp_read_offset, status.room, status.free_room);
    LOG_PRINTF("[decode status, last pts]: %d, is_started %d, is_send_stop_cmd %d\n", status.last_pts, status.is_started, status.is_send_stop_cmd);
    LOG_PRINTF("[decode status, yuv addr]: yuv422_y 0x%08x, yuv422_uv 0x%08x\n", status.yuv422_y_addr, status.yuv422_uv_addr);

    return 0;
}

static int __s2l_decode_query_bsb_status(int iav_fd, SAmbaDSPBSBStatus *status)
{
    int ret;
    struct iav_decode_bsb bsb;
    memset(&bsb, 0x0, sizeof(bsb));
    bsb.decoder_id = status->decoder_id;
    ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
    if (0 > ret) {
        LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, ret %d.\n", ret);
        perror("IAV_IOC_QUERY_DECODE_BSB");
        return ret;
    }

    status->start_offset = bsb.start_offset;
    status->room = bsb.room;
    status->dsp_read_offset = bsb.dsp_read_offset;
    status->free_room = bsb.free_room;

    return 0;
}

static int __s2l_decode_query_status(int iav_fd, SAmbaDSPDecodeStatus *status)
{
    int ret;
    struct iav_decode_status dec_status;
    memset(&dec_status, 0x0, sizeof(dec_status));
    dec_status.decoder_id = status->decoder_id;
    ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &dec_status);
    if (0 > ret) {
        perror("IAV_IOC_QUERY_DECODE_STATUS");
        LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, ret %d.\n", ret);
        return ret;
    }

    status->is_started = dec_status.is_started;
    status->is_send_stop_cmd = dec_status.is_send_stop_cmd;
    status->last_pts = dec_status.last_pts;
    status->decode_state = dec_status.decode_state;
    status->error_status = dec_status.error_status;
    status->total_error_count = dec_status.total_error_count;
    status->decoded_pic_number = dec_status.decoded_pic_number;

    status->write_offset = dec_status.write_offset;
    status->room = dec_status.room;
    status->dsp_read_offset = dec_status.dsp_read_offset;
    status->free_room = dec_status.free_room;
    status->irq_count = dec_status.irq_count;
    status->yuv422_y_addr = dec_status.yuv422_y_addr;
    status->yuv422_uv_addr = dec_status.yuv422_uv_addr;
    return 0;
}

static int __s2l_get_single_vout_info(int iav_fd, int chan, int type, SAmbaDSPVoutInfo *voutinfo)
{
    int num;
    int sink_type = 0;
    int i;
    struct amba_vout_sink_info  sink_info;
    if (EAMDSP_VOUT_TYPE_DIGITAL == type) {
        sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
    } else if (EAMDSP_VOUT_TYPE_HDMI == type) {
        sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
    } else if (EAMDSP_VOUT_TYPE_CVBS == type) {
        sink_type = AMBA_VOUT_SINK_TYPE_CVBS;
    } else {
        LOG_ERROR("not valid type %d\n", type);
        return (-1);
    }
    num = 0;
    if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
        perror("IAV_IOC_VOUT_GET_SINK_NUM");
        LOG_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail\n");
        return (-2);
    }
    if (num < 1) {
        LOG_PRINTF("Please load vout driver!\n");
        return (-3);
    }
    for (i = 0; i < num; i++) {
        sink_info.id = i;
        if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
            perror("IAV_IOC_VOUT_GET_SINK_INFO");
            LOG_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail\n");
            return (-4);
        }
        if ((sink_info.sink_type == sink_type) && (sink_info.source_id == chan)) {
            if (voutinfo) {
                voutinfo->sink_id = sink_info.id;
                voutinfo->source_id = sink_info.source_id;
                voutinfo->sink_type = sink_info.sink_type;
                voutinfo->width = sink_info.sink_mode.video_size.vout_width;
                voutinfo->height = sink_info.sink_mode.video_size.vout_height;
                voutinfo->offset_x = sink_info.sink_mode.video_offset.offset_x;
                voutinfo->offset_y = sink_info.sink_mode.video_offset.offset_y;
                voutinfo->rotate = sink_info.sink_mode.video_rotate;
                voutinfo->flip = sink_info.sink_mode.video_flip;
                voutinfo->mode =  sink_info.sink_mode.mode;
                return 0;
            }
        }
    }
    LOG_NOTICE("no vout with type %d, id %d\n", type, chan);
    return (-5);
}

static int __s2l_map_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
    int ret = 0;
    unsigned int map_size = 0;
    struct iav_querybuf querybuf;

    querybuf.buf = IAV_BUFFER_BSB;
    ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
    if (0 > ret) {
        perror("IAV_IOC_QUERY_BUF");
        LOG_ERROR("IAV_IOC_QUERY_BUF fail\n");
        return ret;
    }

    map_bsb->size = querybuf.length;
    if (map_bsb->b_two_times) {
        map_size = querybuf.length * 2;
    } else {
        map_size = querybuf.length;
    }

    if (map_bsb->b_enable_read && map_bsb->b_enable_write) {
        map_bsb->base = mmap(NULL, map_size, PROT_WRITE | PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
    } else if (map_bsb->b_enable_read && !map_bsb->b_enable_write) {
        map_bsb->base = mmap(NULL, map_size, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
    } else if (!map_bsb->b_enable_read && map_bsb->b_enable_write) {
        map_bsb->base = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
    } else {
        LOG_ERROR("not read or write\n");
        return (-1);
    }

    if (map_bsb->base == MAP_FAILED) {
        perror("mmap");
        LOG_ERROR("mmap fail\n");
        return -1;
    }

    LOG_PRINTF("bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    return 0;
}

static int __s2l_read_bitstream(int iav_fd, SAmbaDSPReadBitstream *bitstream)
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

static void __setup_s2l_al_context(SFAmbaDSPDecAL *al)
{
    al->f_enter_mode = __s2l_enter_decode_mode;
    al->f_leave_mode = __s2l_leave_decode_mode;
    al->f_create_decoder = __s2l_create_decoder;
    al->f_destroy_decoder = __s2l_destroy_decoder;

    al->f_trickplay = __s2l_decode_trick_play;
    al->f_start = __s2l_decode_start;
    al->f_stop = __s2l_decode_stop;
    al->f_speed = __s2l_decode_speed;
    al->f_request_bsb = __s2l_decode_request_bits_fifo;

    al->f_decode = __s2l_decode;

    al->f_query_print_decode_bsb_status = __s2l_decode_query_bsb_status_and_print;
    al->f_query_print_decode_status = __s2l_decode_query_status_and_print;
    al->f_query_decode_bsb_status = __s2l_decode_query_bsb_status;
    al->f_query_decode_status = __s2l_decode_query_status;

    al->f_get_vout_info = __s2l_get_single_vout_info;
    al->f_map_bsb = __s2l_map_bsb;
    al->f_read_bitstream = __s2l_read_bitstream;
}

#elif defined (BUILD_DSP_AMBA_S2) || defined (BUILD_DSP_AMBA_S2E)
#include "basetypes.h"
#include "ambas_common.h"
#include "iav_drv.h"

static void __setup_s2_s2e_al_context(SFAmbaDSPDecAL *al)
{
    al->f_enter_mode = NULL;
    al->f_leave_mode = NULL;
    al->f_create_decoder = NULL;
    al->f_destroy_decoder = NULL;
    al->f_trickplay = NULL;
    al->f_start = NULL;
    al->f_stop = NULL;
    al->f_speed = NULL;
    al->f_request_bsb = NULL;
    al->f_decode = NULL;
    al->f_query_print_decode_bsb_status = NULL;
    al->f_query_print_decode_status = NULL;
    al->f_query_decode_bsb_status = NULL;
    al->f_query_decode_status = NULL;
    al->f_get_vout_info = NULL;
    al->f_map_bsb = NULL;
    al->f_read_bitstream = NULL;

    LOG_FATAL("add support here(s2, s2e)\n");
}

#endif

#endif

int gfOpenIAVHandle()
{
#ifdef BUILD_MODULE_AMBA_DSP
    int fd = open("/dev/iav", O_RDWR, 0);
    if (0 > fd) {
        LOG_FATAL("open iav fail, %d.\n", fd);
    }
    return fd;
#endif

    LOG_FATAL("dsp related is not compiled\n");
    return (-4);
}

void gfCloseIAVHandle(int fd)
{
#ifdef BUILD_MODULE_AMBA_DSP
    if (0 > fd) {
        LOG_FATAL("bad fd %d\n", fd);
        return;
    }
    close(fd);
    return;
#endif

    LOG_FATAL("dsp related is not compiled\n");
    return;
}

void gfSetupDSPAlContext(SFAmbaDSPDecAL *al)
{
#ifdef BUILD_MODULE_AMBA_DSP
#ifdef BUILD_DSP_AMBA_S2L
    __setup_s2l_al_context(al);
#elif defined (BUILD_DSP_AMBA_S2) || defined (BUILD_DSP_AMBA_S2E)
    __setup_s2_s2e_al_context(al);
#else
    LOG_FATAL("add support here\n");
#endif
    return;
#endif

    LOG_FATAL("dsp related is not compiled\n");
    return;
}

void gfFillAmbaGopHeader(TU8 *p_gop_header, TU32 frame_tick, TU32 time_scale, TU32 pts, TU8 gopsize, TU8 m)
{
    TU32 tick_high = frame_tick;
    TU32 tick_low = tick_high & 0x0000ffff;
    TU32 scale_high = time_scale;
    TU32 scale_low = scale_high & 0x0000ffff;
    TU32 pts_high = 0;
    TU32 pts_low = 0;

    tick_high >>= 16;
    scale_high >>= 16;

    p_gop_header[0] = 0; // start code prefix
    p_gop_header[1] = 0;
    p_gop_header[2] = 0;
    p_gop_header[3] = 1;

    p_gop_header[4] = 0x7a; // nal type
    p_gop_header[5] = 0x01; // version main
    p_gop_header[6] = 0x01; // version sub

    p_gop_header[7] = tick_high >> 10;
    p_gop_header[8] = tick_high >> 2;
    p_gop_header[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
    p_gop_header[10] = tick_low >> 3;

    p_gop_header[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
    p_gop_header[12] = scale_high >> 4;
    p_gop_header[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
    p_gop_header[14] = scale_low >> 5;

    p_gop_header[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
    p_gop_header[16] = pts_high >> 6;

    p_gop_header[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
    p_gop_header[18] = pts_low >> 7;
    p_gop_header[19] = (pts_low << 1) | 1;

    p_gop_header[20] = gopsize;
    p_gop_header[21] = (m & 0xf) << 4;
}

void gfUpdateAmbaGopHeader(TU8 *p_gop_header, TU32 pts)
{
    TU32 pts_high = (pts >> 16) & 0x0000ffff;
    TU32 pts_low = pts & 0x0000ffff;

    p_gop_header[15] = (p_gop_header[15]  & 0xFC) | (pts_high >> 14);
    p_gop_header[16] = pts_high >> 6;

    p_gop_header[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
    p_gop_header[18] = pts_low >> 7;
    p_gop_header[19] = (pts_low << 1) | 1;
}


