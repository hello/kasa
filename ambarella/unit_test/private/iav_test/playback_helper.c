/*
 * playback_helper.c
 *
 * History:
 *  2015/02/10 - [Zhi He] create file
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

#include "codec_parser.h"
#include "playback_helper.h"

int file_reader_init(_t_file_reader *p_reader, FILE *file_fd, unsigned long read_buffer_size, void *prealloc_file_buffer)
{
    if (!p_reader || !file_fd) {
        printf("NULL p_reader(%p) or NULL file_fd(%p)\n", p_reader, file_fd);
        return (-1);
    }

    if (!prealloc_file_buffer) {

        if (!read_buffer_size) {
            read_buffer_size = 16 * 1024 * 1024;
        }

        if (!p_reader->b_alloc_read_buffer) {
            p_reader->p_read_buffer_base = malloc(read_buffer_size);
            if (p_reader->p_read_buffer_base) {
                p_reader->read_buffer_size = read_buffer_size;
                p_reader->p_read_buffer_end = p_reader->p_read_buffer_base + p_reader->read_buffer_size;
                p_reader->b_alloc_read_buffer = 1;
            } else {
                printf("no memory!\n");
                return (-2);
            }
            p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
            p_reader->data_remainning_size_in_buffer = 0;
            printf("[file_reader_init]: alloc memory size %lu, addr %p\n", p_reader->read_buffer_size, p_reader->p_read_buffer_base);
        } else if (p_reader->read_buffer_size < read_buffer_size) {
            if (p_reader->p_read_buffer_base) {
                free(p_reader->p_read_buffer_base);
                p_reader->p_read_buffer_base = NULL;
            }

            p_reader->p_read_buffer_base = malloc(read_buffer_size);
            if (p_reader->p_read_buffer_base) {
                p_reader->read_buffer_size = read_buffer_size;
                p_reader->p_read_buffer_end = p_reader->p_read_buffer_base + p_reader->read_buffer_size;
                p_reader->b_alloc_read_buffer = 1;
            } else {
                printf("no memory!\n");
                return (-2);
            }
            p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
            p_reader->data_remainning_size_in_buffer = 0;
            printf("[file_reader_init]: re-alloc memory size %lu, addr %p\n", p_reader->read_buffer_size, p_reader->p_read_buffer_base);
        } else {
            p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
            p_reader->data_remainning_size_in_buffer = 0;
            printf("[file_reader_init]: re-use read buffer %lu, addr %p, request size %lu\n", p_reader->read_buffer_size, p_reader->p_read_buffer_base, read_buffer_size);
        }
    } else {
        p_reader->b_alloc_read_buffer = 0;
        p_reader->read_buffer_size = read_buffer_size;
        p_reader->p_read_buffer_base = prealloc_file_buffer;
        p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
        p_reader->p_read_buffer_end = p_reader->p_read_buffer_base + p_reader->read_buffer_size;
        p_reader->data_remainning_size_in_buffer = 0;
    }

    p_reader->fd = file_fd;
    p_reader->b_opened_file = 0;

    fseek(p_reader->fd, 0L, SEEK_END);
    p_reader->file_total_size = ftell(p_reader->fd);
    p_reader->file_remainning_size = p_reader->file_total_size;

    fseek(p_reader->fd, 0L, SEEK_SET);

    return 0;
}

void file_reader_deinit(_t_file_reader *p_reader)
{
    if (!p_reader) {
        printf("NULL p_reader(%p)\n", p_reader);
        return;
    }

    if (p_reader->b_alloc_read_buffer) {
        if (p_reader->p_read_buffer_base) {
            free(p_reader->p_read_buffer_base);
            p_reader->b_alloc_read_buffer = 0;
        } else {
            printf("NULL p_reader->p_read_buffer_base\n");
        }
    }

    return;
}

void file_reader_reset(_t_file_reader *p_reader)
{
    p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_base;
    p_reader->data_remainning_size_in_buffer = 0;
    p_reader->file_remainning_size = p_reader->file_total_size;

    fseek(p_reader->fd, 0L, SEEK_SET);
}

int file_reader_read_trunk(_t_file_reader *p_reader)
{
    unsigned long size = 0;

    size = (unsigned long)(p_reader->p_read_buffer_end - p_reader->p_read_buffer_cur_end);

    if (p_reader->file_remainning_size <= size) {
        size = p_reader->file_remainning_size;
        fread(p_reader->p_read_buffer_cur_end, 1, size, p_reader->fd);
        p_reader->file_remainning_size -= size;
        p_reader->data_remainning_size_in_buffer += size;
        p_reader->p_read_buffer_cur_end += size;
        return 1;//file read done
    } else {
        memmove(p_reader->p_read_buffer_base, p_reader->p_read_buffer_cur_start, p_reader->data_remainning_size_in_buffer);
        p_reader->p_read_buffer_cur_start = p_reader->p_read_buffer_base;
        p_reader->p_read_buffer_cur_end = p_reader->p_read_buffer_cur_start + p_reader->data_remainning_size_in_buffer;
        size = p_reader->p_read_buffer_end - p_reader->p_read_buffer_cur_end;

        if (p_reader->file_remainning_size <= size) {
            size = p_reader->file_remainning_size;
            fread(p_reader->p_read_buffer_cur_end, 1, size, p_reader->fd);
            p_reader->file_remainning_size -= size;
            p_reader->data_remainning_size_in_buffer += size;
            p_reader->p_read_buffer_cur_end += size;
            return 1;//file read done
        }

        fread(p_reader->p_read_buffer_cur_end, 1, size, p_reader->fd);
        p_reader->file_remainning_size -= size;
        p_reader->data_remainning_size_in_buffer += size;
        p_reader->p_read_buffer_cur_end += size;
    }

    return 0;
}

static fast_navi_gop *_new_fast_navi_gop()
{
    fast_navi_gop *p_gop = (fast_navi_gop *) malloc(sizeof(fast_navi_gop));
    if (p_gop) {
        memset(p_gop, 0x0, sizeof(fast_navi_gop));
    } else {
        printf("error: malloc fail\n");
    }

    return p_gop;
}

unsigned int get_next_frame(unsigned char *pstart, unsigned char *pend, unsigned char *p_nal_type, unsigned char *p_slice_type, unsigned char *need_more_data)
{
    unsigned char *pcur = pstart;
    unsigned int state = 0;
    unsigned int is_header = 1;

    unsigned char   nal_type;
    unsigned char   first_mb_in_slice = 0;

    *need_more_data = 0;
    *p_slice_type = 0;
    *p_nal_type = 0;

    while (pcur < pend) {
        switch (state) {
            case 0:
                if (*pcur++ == 0x0)
                { state = 1; }
                break;
            case 1://0
                if (*pcur++ == 0x0)
                { state = 2; }
                else
                { state = 0; }
                break;
            case 2://0 0
                if (*pcur == 0x1)
                { state = 3; }
                else if (*pcur != 0x0)
                { state = 0; }
                pcur ++;
                break;
            case 3://0 0 1
                if ((*pcur) == 0x0A) {
                    //eos
                    if (is_header) {
                        printf("eos comes, pcur + 1 - pstart is %d\n", pcur + 1 - pstart);
                        *p_nal_type = 0x0A;
                        return pcur + 1 - pstart;
                    } else {
                        if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
                            printf("before eos in bit-stream, pcur - 4 - pstart is %d\n", pcur - 4 - pstart);
                            return pcur - 4 - pstart;
                        } else {
                            printf("before eos in bit-stream, pcur - 3 - pstart is %d\n", pcur - 3 - pstart);
                            return pcur - 3 - pstart;
                        }
                    }
                } else if ((*pcur)) { //nal uint type
                    nal_type = (*pcur) & 0x1F;
                    //*p_nal_type = nal_type;
                    if (nal_type >= 1 && nal_type <= 5) {
                        if (!is_header) {
                            if (pcur + 16 < pend) {
                                //printf("1 %p: %02x %02x %02x %02x, %02x %02x %02x %02x\n", pcur, *(pcur -3), *(pcur -2), *(pcur -1), *(pcur), *(pcur + 1), *(pcur + 2), *(pcur + 3), *(pcur + 4));
                                get_h264_slice_type_le(pcur + 1, &first_mb_in_slice);

                                if (!first_mb_in_slice) {
                                    if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
                                        return pcur - 4 - pstart;
                                    } else {
                                        return pcur - 3 - pstart;
                                    }
                                } else {
                                    state = 0;
                                }
                            } else {
                                *need_more_data = 1;
                                return 0;
                            }
                        } else {
                            if (pcur + 16 < pend) {
                                //printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", *(pcur - 4), *(pcur - 3), *(pcur - 2), *(pcur - 1), *(pcur), *(pcur + 1), *(pcur + 2), *(pcur + 3));
                                *p_slice_type = get_h264_slice_type_le(pcur + 1, &first_mb_in_slice);
                                *p_nal_type = nal_type;
                                state = 0;
                                is_header = 0;
                            } else {
                                printf("[error]: must not comes here!\n");
                                *need_more_data = 1;
                                return 0;
                            }
                        }
                    } else if (nal_type >= 6 && nal_type <= 9) {
                        if (!is_header) {
                            if ((pcur > (pstart + 4)) && (*(pcur - 4) == 0)) {
                                return pcur - 4 - pstart;
                            } else {
                                return pcur - 3 - pstart;
                            }
                        }
                        state = 0;
                    }/* else if (nal_type == 9) {
                        state = 0;
                    }*/ else {
                        printf("[error]: why comes here? nal_type %d\n", nal_type);
                    }
                } else {
                    state = 1;
                }
                pcur ++;
                break;

            default:
                printf("[error]: must not comes here! state %d\n", state);
                state = 0;
                break;
        }
    }

    *need_more_data = 1;
    return 0;
}

static unsigned int __skip_delimter_size(unsigned char *p)
{
    if ((0 == p[0]) && (0 == p[1]) && (0 == p[2]) && (1 == p[3]) && (0x09 == p[4])) {
        return 6;
    }

    return 0;
}

int build_fast_navi_tree(FILE *file_fd, fast_navi_file *navi_tree)
{
    unsigned char *p_cur_ptr = NULL;
    _file_size_t file_remain_data_size = 0;
    _file_size_t buf_remain_data_size = 0;

    if ((!file_fd) || (!navi_tree)) {
        printf("NULL params file_fd %p, or navi_tree %p\n", file_fd, navi_tree);
        return (-1);
    }

    navi_tree->file_fd = file_fd;
    navi_tree->buf_size = 8 * 1024 * 1024;
    navi_tree->p_buf_base = (unsigned char *) malloc(navi_tree->buf_size);
    if (!navi_tree->p_buf_base) {
        printf("[error]: no memory, request size %ld\n", navi_tree->buf_size);
        return (-2);
    }

    fseek(file_fd, 0L, SEEK_END);
    navi_tree->file_size = ftell(file_fd);
    fseek(file_fd, 0L, SEEK_SET);
    navi_tree->cur_file_offset = 0;

    navi_tree->tot_frame_count = 0;
    navi_tree->tot_gop_count = 0;
    memset(&navi_tree->gop_list_header, 0x0, sizeof(fast_navi_gop));
    navi_tree->gop_list_header.p_next = &navi_tree->gop_list_header;
    navi_tree->gop_list_header.p_pre = &navi_tree->gop_list_header;

    file_remain_data_size = navi_tree->file_size;
    printf("[flow]: begin build navi tree from file, file size %ld, please wait a while ......\n", navi_tree->file_size);

    //read first data chunk
    if (file_remain_data_size > navi_tree->buf_size) {
        fread(navi_tree->p_buf_base, 1, navi_tree->buf_size, file_fd);
        buf_remain_data_size = navi_tree->buf_size;
        file_remain_data_size -= navi_tree->buf_size;
    } else {
        fread(navi_tree->p_buf_base, 1, file_remain_data_size, file_fd);
        buf_remain_data_size = file_remain_data_size;
        file_remain_data_size = 0;
    }

    unsigned int frame_size = 0;
    unsigned char nal_type = 0;
    unsigned char slice_type = 0;
    unsigned char need_more_data = 0;
    unsigned char eos_reached = 0;
    fast_navi_gop *new_gop = NULL;

    //skip begining delimiter for some bitstream
    unsigned int skip_size = __skip_delimter_size(navi_tree->p_buf_base);
    if (skip_size) {
        printf("skip delimiter in beginning!\n");
        buf_remain_data_size -= skip_size;
    }
    p_cur_ptr = navi_tree->p_buf_base + skip_size;

    while (1) {
        frame_size = get_next_frame(p_cur_ptr, p_cur_ptr + buf_remain_data_size, &nal_type, &slice_type, &need_more_data);
        if (!frame_size && need_more_data) {
            if (!file_remain_data_size) {
                printf("[flow]: file end reached, this is last frame\n");
                frame_size = buf_remain_data_size;
                eos_reached = 1;
            } else {
                if (navi_tree->p_buf_base == p_cur_ptr) {
                    printf("[error]: do not find start code in file, data corruption, or not h264 es file\n");
                    return (-7);
                }
                navi_tree->cur_file_offset += navi_tree->buf_size - buf_remain_data_size;
                memmove(navi_tree->p_buf_base, p_cur_ptr, buf_remain_data_size);
                p_cur_ptr = navi_tree->p_buf_base;

                if ((file_remain_data_size + buf_remain_data_size) > navi_tree->buf_size) {
                    fread(navi_tree->p_buf_base + buf_remain_data_size, 1, navi_tree->buf_size - buf_remain_data_size, file_fd);
                    file_remain_data_size -= navi_tree->buf_size - buf_remain_data_size;
                    buf_remain_data_size = navi_tree->buf_size;
                } else {
                    fread(navi_tree->p_buf_base + buf_remain_data_size, 1, file_remain_data_size, file_fd);
                    buf_remain_data_size += file_remain_data_size;
                    file_remain_data_size = 0;
                }
                continue;
            }
        }

        if (AVC_NAL_TYPE_IDR == nal_type) {
            new_gop = _new_fast_navi_gop();
            if (!new_gop) {
                printf("[error]: new gop fail\n");
                return (-3);
            }

            //append to tail
            new_gop->p_next = &navi_tree->gop_list_header;
            new_gop->p_pre = navi_tree->gop_list_header.p_pre;
            navi_tree->gop_list_header.p_pre->p_next = new_gop;
            navi_tree->gop_list_header.p_pre = new_gop;

            //printf("insert gop %p, its next is %p, header %p\n", new_gop, new_gop->p_next, &navi_tree->gop_list_header);

            new_gop->gop_index = navi_tree->tot_gop_count;
            new_gop->gop_offset = navi_tree->cur_file_offset + p_cur_ptr - navi_tree->p_buf_base;
            new_gop->start_frame_index = navi_tree->tot_frame_count;

            new_gop->frames[0].frame_offset = 0;
            new_gop->frames[1].frame_offset = frame_size;
            new_gop->frame_count = 1;

            navi_tree->tot_gop_count ++;
            navi_tree->tot_frame_count ++;
        } else if (AVC_NAL_TYPE_IDR > nal_type) {
            if (!new_gop) {
                printf("[error]: file not start with IDR frame? skip..\n");
                p_cur_ptr += frame_size;
                buf_remain_data_size -= frame_size;
                continue;
            }
            if (new_gop->frame_count > (DMAX_GOP_LENGTH - 2)) {
                printf("[error]: exceed max gop length!\n");
                return (-4);
            }

            new_gop->frames[new_gop->frame_count].frame_offset = navi_tree->cur_file_offset + p_cur_ptr - navi_tree->p_buf_base - new_gop->gop_offset;
            new_gop->frames[new_gop->frame_count + 1].frame_offset = new_gop->frames[new_gop->frame_count].frame_offset + frame_size;
            new_gop->frame_count ++;

            navi_tree->tot_frame_count ++;
        } else {
            printf("[error]: unexpected nal type %d!\n", nal_type);
            return (-5);
        }

        if (eos_reached) {
            break;
        }
        p_cur_ptr += frame_size;
        buf_remain_data_size -= frame_size;
    }

    printf("build navi tree done, total gop number %d, total frame number %d\n", navi_tree->tot_gop_count, navi_tree->tot_frame_count);

    return 0;
}

void delete_fast_navi_tree(fast_navi_file *navi_tree)
{
    if (navi_tree) {
        if (navi_tree->p_buf_base) {
            free(navi_tree->p_buf_base);
            navi_tree->p_buf_base = NULL;
        }

        fast_navi_gop *p_gop = navi_tree->gop_list_header.p_next;
        fast_navi_gop *p_next = NULL;
        while (p_gop != &navi_tree->gop_list_header) {
            p_next = p_gop->p_next;
            free(p_gop);
            p_gop = p_next;
        }
        navi_tree->gop_list_header.p_next = &navi_tree->gop_list_header;
        navi_tree->gop_list_header.p_pre = &navi_tree->gop_list_header;

        navi_tree->tot_frame_count = 0;
        navi_tree->tot_gop_count = 0;
    }

}

