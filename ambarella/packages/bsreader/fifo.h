/*******************************************************************************
 * fifo.h
 *
 * History:
 *	2010/05/13 - [Louis Sun] created file
 *	2013/03/04 - [Jian Tang] modified file
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
#ifndef _FIFO_H
#define _FIFO_H

#ifdef __cplusplus
extern "C" {
#endif

#define OneCircle     1
#define ZeroCircle    0
#define NOMORENUM     0
#define WriteSuccess  1
#define ReadSuccess   1

#define FIFO_IS_FULL  0
#define FIFO_IS_EMPTY 0

typedef struct packet_info_s
{
	u8    *packet_addr;
	u32	packet_size;
	u32	entry_state;
} packet_info_t;

typedef struct fifo_s
{
	// buffer related
	u8    			*buffer_start_addr;
	u8    			*buffer_end_addr;
	u32				buffer_total_size;
	// entry related
	packet_info_t		*packet_info_entries;
	u32				max_entry_num;
	int				first_entry_index;
	int				last_entry_index;
	// header related
	u8				*header_entries;
	u32				header_size;
	// fifo core
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	// buffer related
	u8    			*addr_for_next_write;
	u32				buffer_used_size;
	// entry related
	int				entry_index_for_next_read;
	int				entry_index_for_next_write;
	int				entry_index_being_used;
	u32				used_entry_num;
	// others
	u32				non_block_flag;
} fifo_t;

fifo_t* fifo_create(u32 buffer_size, u32 header_size, u32 max_packet_num);
void fifo_close(fifo_t *fifo);

int fifo_write_one_packet(fifo_t *fifo, u8 *header, u8 *packet_addr, u32 packet_size);
int fifo_read_one_packet(fifo_t *fifo, u8 *header, u8 **packet_addr, u32 *packet_size);
int fifo_write_one_packet_slice(fifo_t *fifo, u8 *header, u8 *packet_addr[],u32 *slice_size, int tile_num);
int fifo_flush(fifo_t *fifo);

#ifdef __cplusplus
}
#endif

#endif
