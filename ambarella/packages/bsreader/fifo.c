/*******************************************************************************
 * fifo.c
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basetypes.h"
#include "fifo.h"

//#define DEBUG

#ifdef DEBUG
#define fifo_printf(fmt...)		printf(fmt)
#else
#define fifo_printf(fmt...)
#endif

#define ENTRY_INVALID	0
#define ENTRY_VALID		1


inline static void wraparound_entry_index_if_necessary(fifo_t *fifo, int *entry_index)
{
	if (*entry_index > fifo->last_entry_index)
		*entry_index = fifo->first_entry_index;
}


/***************************************************************************************
	function:	void fifo_create(u32 buffer_size, u32 max_packet_num)
	input:	buffer_size: the space that will be allocated for the data buffer
			max_packet_nume: maximum number of packets can be buffered
	return value:	None
	remarks:
***************************************************************************************/
fifo_t* fifo_create(u32 buffer_size, u32 header_size, u32 max_packet_num)
{
	fifo_t *new_fifo = (fifo_t *)malloc(sizeof(fifo_t));
	// buffer related initialization
    	new_fifo->buffer_start_addr = (u8 *)malloc(buffer_size);
    	new_fifo->buffer_end_addr = new_fifo->buffer_start_addr + buffer_size - 1;
	new_fifo->addr_for_next_write = new_fifo->buffer_start_addr;
	new_fifo->buffer_total_size = buffer_size;
	new_fifo->buffer_used_size = 0;

	// header buffer related initialization
	new_fifo->header_size = header_size;
	if (header_size > 0)
		new_fifo->header_entries = (u8 *)malloc(max_packet_num * header_size);

	// entry related initialization
	new_fifo->packet_info_entries = (packet_info_t *)malloc(max_packet_num * sizeof(packet_info_t));
	memset(new_fifo->packet_info_entries, 0, max_packet_num * sizeof(packet_info_t));
	new_fifo->max_entry_num = max_packet_num;
	new_fifo->first_entry_index = 0;
	new_fifo->last_entry_index = max_packet_num - 1;
	new_fifo->entry_index_being_used = -1;		// no entry being used
	new_fifo->entry_index_for_next_read = 0;
	new_fifo->entry_index_for_next_write = 0;
	new_fifo->used_entry_num = 0;

	pthread_mutex_init(&new_fifo->mutex, NULL);
	pthread_cond_init(&new_fifo->cond, NULL);

//	printf("buffer_size %d, entry num %d\n", buffer_size, max_packet_num);	//jay
	return new_fifo;
}

void fifo_close(fifo_t *fifo)
{
	free(fifo->buffer_start_addr);
	free(fifo->header_entries);
	free(fifo->packet_info_entries);

	pthread_cond_destroy(&fifo->cond);
	pthread_mutex_destroy(&fifo->mutex);
	free(fifo);
}

static void cancel_read_wait(void *arg)
{
	pthread_mutex_t *mutex = arg;
//	printf("unlock, cancel\n");
	pthread_mutex_unlock(mutex);
}

inline static int free_last_read_packet(fifo_t *fifo)
{
//	pthread_mutex_lock(&fifo->mutex);

	if (fifo->entry_index_being_used != -1) {
		// update related info

		fifo->used_entry_num--;
		fifo->packet_info_entries[fifo->entry_index_being_used].entry_state = ENTRY_INVALID;
//		fifo->buffer_used_size -= fifo->packet_info_entries[fifo->entry_index_being_used].packet_size;
		fifo->entry_index_being_used = -1;
	}
//	pthread_mutex_unlock(&fifo->mutex);

	return 0;
}

/***************************************************************************************
	function:	int fifo_write_one_packet(u8 *packet_addr, u32 packet_size)
	input:	packet_addr: address of the packet to be stored into the fifo
			packet_size: size of packet to be stored into the fifo
	return value:	0: successful, -1: failed
	remarks:	the algorithm gives priority to writers for sharing the free entries
***************************************************************************************/
int fifo_write_one_packet(fifo_t *fifo, u8 *header, u8 *packet_addr, u32 packet_size)
{
	u8 *addr_for_this_write;
	int entry_index_for_this_write;

//	printf("lock, write\n");
	pthread_mutex_lock(&fifo->mutex);
//	printf("locked, write\n");

	addr_for_this_write = fifo->addr_for_next_write;
	entry_index_for_this_write = fifo->entry_index_for_next_write;

	// skip the entry being used for this write
	if (entry_index_for_this_write == fifo->entry_index_being_used) {
		entry_index_for_this_write++;
		wraparound_entry_index_if_necessary(fifo, &entry_index_for_this_write);
	}
	fifo_printf("[%x]write %d, using %d, next read %d, entry %d\n",
		(u32)fifo&0xf, entry_index_for_this_write, fifo->entry_index_being_used,
		fifo->entry_index_for_next_read, fifo->used_entry_num);

	// if necessary, adjust write address to avoid wrapping around data
	if ((addr_for_this_write + packet_size) > (fifo->buffer_end_addr + 1))
		addr_for_this_write = fifo->buffer_start_addr;

	// save the packet
	memcpy(addr_for_this_write, packet_addr, packet_size);

	// save the header
	memcpy(&fifo->header_entries[entry_index_for_this_write*(fifo->header_size)],
		header, fifo->header_size);

	// update related info
	if (fifo->packet_info_entries[entry_index_for_this_write].entry_state == ENTRY_INVALID)
		fifo->used_entry_num++;

	// fill the entry
	fifo->packet_info_entries[entry_index_for_this_write].packet_addr = addr_for_this_write;
	fifo->packet_info_entries[entry_index_for_this_write].packet_size = packet_size;
	fifo->packet_info_entries[entry_index_for_this_write].entry_state = ENTRY_VALID;

	// advance the entry for next read, if it is overwirtten by this write, when fifo is full
	if ((entry_index_for_this_write == fifo->entry_index_for_next_read) &&
		(fifo->used_entry_num > 2)) {	 // being used entry is still ocupying one entry
		// if fifo is nearly full
		fifo->entry_index_for_next_read++;
		wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_read);
		if (fifo->entry_index_for_next_read == fifo->entry_index_being_used) {
			fifo->entry_index_for_next_read++;	// skip the entry being used for next read
			wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_read);
		}
		fifo_printf("fifo full, advance read entry to %d >>>>>>>>>>>>>>"
			">>>>>>>>>>>>>>>>>>\n", fifo->entry_index_for_next_read);
	}

	// update entry index for next write
	fifo->entry_index_for_next_write = entry_index_for_this_write + 1;
	wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_write);

	// update address for next write
	fifo->addr_for_next_write = addr_for_this_write + packet_size;
	if (fifo->addr_for_next_write > fifo->buffer_end_addr)
		fifo->addr_for_next_write -= fifo->buffer_total_size;

	// signal the condition variable if necessary
	if (fifo->used_entry_num <= 1)
		pthread_cond_signal(&fifo->cond);

//	fifo->buffer_used_size += packet_size;

//	printf("unlock, write\n");
	pthread_mutex_unlock(&fifo->mutex);

#if 0
	printf("fifo_write_one_packet\n");
	for (i = 0; i < 4; i++) {
		printf("%p\t", fifo->packet_info_entries[i].packet_addr);
	}
	printf("\nnow %d entry used\n", fifo->used_entry_num);
#endif
	return 0;
}

/***************************************************************************************
	function:	int fifo_write_one_packet(u8 *packet_addr, u32 packet_size)
	input:	packet_addr: address of the packet to be stored into the fifo
			packet_size: size of packet to be stored into the fifo
	return value:	0: successful, -1: failed
	remarks:	the algorithm gives priority to writers for sharing the free entries
***************************************************************************************/
int fifo_write_one_packet_slice(fifo_t *fifo, u8 *header, u8 *packet_addr[],u32 *slice_size, int tile_num)
{
	u8 *addr_for_this_write;
	int entry_index_for_this_write;
	int i;
	u32 offset_size = 0;
	u32 packet_size = 0;

//	printf("lock, write\n");
	pthread_mutex_lock(&fifo->mutex);
//	printf("locked, write\n");
	addr_for_this_write = fifo->addr_for_next_write;
	entry_index_for_this_write = fifo->entry_index_for_next_write;

	for (i = 0; i < tile_num; i++) {
		// save the packet
		packet_size += slice_size[i];
	}

	// skip the entry being used for this write
	if (entry_index_for_this_write == fifo->entry_index_being_used) {
		entry_index_for_this_write++;
		wraparound_entry_index_if_necessary(fifo, &entry_index_for_this_write);
	}
	fifo_printf("[%x]write %d, using %d, next read %d, entry %d\n",
		(u32)fifo&0xf, entry_index_for_this_write, fifo->entry_index_being_used,
		fifo->entry_index_for_next_read, fifo->used_entry_num);

	// if necessary, adjust write address to avoid wrapping around data
	if ((addr_for_this_write + packet_size) > (fifo->buffer_end_addr + 1))
		addr_for_this_write = fifo->buffer_start_addr;
	for (i = 0; i < tile_num; i++) {
		// save the packet
		memcpy(addr_for_this_write + offset_size, packet_addr[i], slice_size[i]);
		offset_size += slice_size[i];
	}
	// save the header
	memcpy(&fifo->header_entries[entry_index_for_this_write*(fifo->header_size)],
		header, fifo->header_size);

	// update related info
	if (fifo->packet_info_entries[entry_index_for_this_write].entry_state == ENTRY_INVALID)
		fifo->used_entry_num++;

	// fill the entry
	fifo->packet_info_entries[entry_index_for_this_write].packet_addr = addr_for_this_write;
	fifo->packet_info_entries[entry_index_for_this_write].packet_size = packet_size;
	fifo->packet_info_entries[entry_index_for_this_write].entry_state = ENTRY_VALID;

	// advance the entry for next read, if it is overwirtten by this write, when fifo is full
	if ((entry_index_for_this_write == fifo->entry_index_for_next_read) &&
		(fifo->used_entry_num > 2)) {	 // being used entry is still ocupying one entry
		// if fifo is nearly full
		fifo->entry_index_for_next_read++;
		wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_read);
		if (fifo->entry_index_for_next_read == fifo->entry_index_being_used) {
			fifo->entry_index_for_next_read++;	// skip the entry being used for next read
			wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_read);
		}
		fifo_printf("fifo full, advance read entry to %d >>>>>>>>>>>>>>"
			">>>>>>>>>>>>>>>>>>\n", fifo->entry_index_for_next_read);
	}

	// update entry index for next write
	fifo->entry_index_for_next_write = entry_index_for_this_write + 1;
	wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_write);

	// update address for next write
	fifo->addr_for_next_write = addr_for_this_write + packet_size;
	if (fifo->addr_for_next_write > fifo->buffer_end_addr)
		fifo->addr_for_next_write -= fifo->buffer_total_size;

	// signal the condition variable if necessary
	if (fifo->used_entry_num <= 1)
		pthread_cond_signal(&fifo->cond);

//	fifo->buffer_used_size += packet_size;

//	printf("unlock, write\n");
	pthread_mutex_unlock(&fifo->mutex);

#if 0
	printf("fifo_write_one_packet\n");
	for (i = 0; i < 4; i++) {
		printf("%p\t", fifo->packet_info_entries[i].packet_addr);
	}
	printf("\nnow %d entry used\n", fifo->used_entry_num);
#endif
	return 0;
}

/***************************************************************************************
	function:	int fifo_read_one_packet(u8 **packet_addr, u32 *packet_size)
	input:	packet_addr: pointer to the address of the packet for read
			packet_size: pointer to the size of the packet for read
	return value:	0: successful, -1: failed
	remarks:	value-result arguments are used
***************************************************************************************/
int fifo_read_one_packet(fifo_t *fifo, u8 *header, u8 **packet_addr, u32 *packet_size)
{
    	int entry_index_for_this_read;

//	printf("lock, read\n");
	pthread_mutex_lock(&fifo->mutex);
//	printf("locked, read\n");

	free_last_read_packet(fifo);

	// wait on the condition variable if fifo is empty
	while (fifo->used_entry_num == 0) {
//		printf("unlock, waiting...\n");	//jay
		pthread_cleanup_push(cancel_read_wait, (void *) &fifo->mutex);
		pthread_cond_wait(&fifo->cond, &fifo->mutex);
//		printf("lock, awake\n");
		pthread_cleanup_pop(0);
	}

	entry_index_for_this_read = fifo->entry_index_for_next_read;

	while (fifo->packet_info_entries[entry_index_for_this_read].entry_state == ENTRY_INVALID) {
		fifo_printf("\t\t\t\tskip entry %d\n", entry_index_for_this_read);
		entry_index_for_this_read++;
		wraparound_entry_index_if_necessary(fifo, &entry_index_for_this_read);
	}

	fifo_printf("\t\t\t\t\t[%x]read %d, entry %d\n", (u32)fifo&0xf, entry_index_for_this_read, fifo->used_entry_num);

	// advertise the packet
	*packet_addr = fifo->packet_info_entries[entry_index_for_this_read].packet_addr;
	*packet_size = fifo->packet_info_entries[entry_index_for_this_read].packet_size;
	memcpy(header, &fifo->header_entries[entry_index_for_this_read*(fifo->header_size)],
		fifo->header_size);

	// update entry index for next read
	fifo->entry_index_for_next_read = entry_index_for_this_read + 1;
	wraparound_entry_index_if_necessary(fifo, &fifo->entry_index_for_next_read);

	// update related info
	fifo->entry_index_being_used = entry_index_for_this_read;

//	printf("unlock, read\n");
	pthread_mutex_unlock(&fifo->mutex);

	return 0;
}

/***************************************************************************************
	function:	int fifo_flush(fifo_t *fifo)
	input:	packet_addr: address of the packet to be stored into the fifo
			packet_size: size of packet to be stored into the fifo
	return value:	0: successful, -1: failed
	remarks:	the algorithm gives priority to writers for sharing the free entries
***************************************************************************************/
int fifo_flush(fifo_t *fifo)
{
	int i;
	pthread_mutex_lock(&fifo->mutex);

	// buffer related initialization
	fifo->addr_for_next_write = fifo->buffer_start_addr;

	// entry related initialization
	fifo->entry_index_being_used = -1;		// no entry being used
	fifo->entry_index_for_next_read = 0;
	fifo->entry_index_for_next_write = 0;
	fifo->used_entry_num = 0;

	for (i = 0; i <= fifo->last_entry_index; i++)
		fifo->packet_info_entries[i].entry_state = ENTRY_INVALID;

	pthread_mutex_unlock(&fifo->mutex);
	return 0;
}


