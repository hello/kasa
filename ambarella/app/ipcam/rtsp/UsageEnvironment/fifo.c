#include "../groupsock/include/NetCommon.h"
#include <pthread.h>
#include <stdio.h>
#include "fifo.h"

typedef struct _FIFO
{
	// buffer related
	u_int8_t    		*buffer_start_addr;
	u_int8_t    		*buffer_end_addr;
	u_int32_t			buffer_total_size;
	// entry related
	packet_info_t		*packet_info_entries;
	u_int32_t			max_entry_num;
	int				first_entry_index;
	int				last_entry_index;
} FIFO;

struct FIFO_CORE_TAG {
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	// buffer related
	u_int8_t    		*addr_for_next_write;
	u_int32_t			buffer_used_size;
	// entry related
	int				entry_index_for_next_read;
	int				entry_index_for_next_write;
	int				entry_index_being_used;
	u_int32_t			used_entry_num;
} fifo_core = {
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_COND_INITIALIZER,
};

FIFO myFIFO;

inline static void wraparound_entry_index_if_necessary(int *entry_index)
{
	if (*entry_index > myFIFO.last_entry_index)
		*entry_index = myFIFO.first_entry_index;
}


/***************************************************************************************
	function:	void create_fifo(u_int32_t buffer_size, u_int32_t max_packet_num)
	input:	buffer_size: the space that will be allocated for the data buffer
			max_packet_nume: maximum number of packets can be buffered
	return value:	None
	remarks:
***************************************************************************************/
void create_fifo(u_int32_t buffer_size, u_int32_t max_packet_num)
{
	// buffer related initialization
    	myFIFO.buffer_start_addr = (u_int8_t *)malloc(buffer_size);
    	myFIFO.buffer_end_addr = myFIFO.buffer_start_addr + buffer_size - 1;
	fifo_core.addr_for_next_write = myFIFO.buffer_start_addr;
	myFIFO.buffer_total_size = buffer_size;
	fifo_core.buffer_used_size = 0;

	// entry related initialization
	myFIFO.packet_info_entries = (packet_info_t *)malloc(max_packet_num * sizeof(packet_info_t));
	myFIFO.max_entry_num = max_packet_num;
	myFIFO.first_entry_index = 0;
	myFIFO.last_entry_index = max_packet_num - 1;
	fifo_core.entry_index_being_used = -1;		// no entry being used
	fifo_core.entry_index_for_next_read = 0;
	fifo_core.entry_index_for_next_write = 0;
	fifo_core.used_entry_num = 0;
}


/***************************************************************************************
	function:	int write_one_packet(u_int8_t *packet_addr, u_int32_t packet_size)
	input:	packet_addr: address of the packet to be stored into the fifo
			packet_size: size of packet to be stored into the fifo
	return value:	0: successful, -1: failed
	remarks:	the algorithm gives priority to writers for sharing the free entries
***************************************************************************************/
int write_one_packet(u_int8_t *packet_addr, u_int32_t packet_size)
{
	u_int8_t *addr_for_this_write;
	int entry_index_for_this_write;
//	int i;

	pthread_mutex_lock(&fifo_core.mutex);
	addr_for_this_write = fifo_core.addr_for_next_write;
	entry_index_for_this_write = fifo_core.entry_index_for_next_write;


	// skip the entry being used for this write
	if (entry_index_for_this_write == fifo_core.entry_index_being_used) {
		entry_index_for_this_write++;
		wraparound_entry_index_if_necessary(&entry_index_for_this_write);
	}

	// if necessary, adjust write address to avoid wrapping around data
	if ((addr_for_this_write + packet_size) > (myFIFO.buffer_end_addr + 1))
		addr_for_this_write = myFIFO.buffer_start_addr;

	// save the packet
	memcpy(addr_for_this_write, packet_addr, packet_size);

	// fill the entry
	myFIFO.packet_info_entries[entry_index_for_this_write].packet_addr = addr_for_this_write;
	myFIFO.packet_info_entries[entry_index_for_this_write].packet_size = packet_size;

	// advance the entry for next read, if it is overwirtten by this write, when fifo is full
	if (entry_index_for_this_write == fifo_core.entry_index_for_next_read &&
		fifo_core.used_entry_num == myFIFO.max_entry_num) {	// if fifo is full
		fifo_core.entry_index_for_next_read++;
		wraparound_entry_index_if_necessary(&fifo_core.entry_index_for_next_read);
		printf("fifo full, dropping frame...\n");
	}

	// update entry index for next write
	fifo_core.entry_index_for_next_write++;
	wraparound_entry_index_if_necessary(&fifo_core.entry_index_for_next_write);

	if (fifo_core.entry_index_for_next_write == fifo_core.entry_index_being_used) {
		fifo_core.entry_index_for_next_write++;	// skip entry being used by last read
		wraparound_entry_index_if_necessary(&fifo_core.entry_index_for_next_write);
	}

	// update address for next write
	fifo_core.addr_for_next_write = addr_for_this_write + packet_size;
	if (fifo_core.addr_for_next_write > myFIFO.buffer_end_addr)
		fifo_core.addr_for_next_write -= myFIFO.buffer_total_size;

	// signal the condition variable
	if (fifo_core.used_entry_num == 0)
		pthread_cond_signal(&fifo_core.cond);

	// update related info
	fifo_core.used_entry_num++;
	fifo_core.buffer_used_size += packet_size;

	pthread_mutex_unlock(&fifo_core.mutex);

#if 0
	printf("write_one_packet\n");
	for (i = 0; i < 4; i++) {
		printf("%p\t", myFIFO.packet_info_entries[i].packet_addr);
	}
	printf("\nnow %d entry used\n", fifo_core.used_entry_num);
#endif
	return 0;
}


/***************************************************************************************
	function:	int read_one_packet(u_int8_t **packet_addr, u_int32_t *packet_size)
	input:	packet_addr: pointer to the address of the packet for read
			packet_size: pointer to the size of the packet for read
	return value:	0: successful, -1: failed
	remarks:	value-result arguments are used
***************************************************************************************/
int read_one_packet(u_int8_t **packet_addr, u_int32_t *packet_size)
{
    	u_int32_t entry_index_for_this_read;

	free_last_read_packet();

	pthread_mutex_lock(&fifo_core.mutex);

	// wait on the condition variable if fifo is empty
	while (fifo_core.used_entry_num == 0) {
//		printf("fifo empty, waiting...\n");	//jay
		pthread_cond_wait(&fifo_core.cond, &fifo_core.mutex);
	}

    	entry_index_for_this_read = fifo_core.entry_index_for_next_read;

	// advertise the packet
	*packet_addr = myFIFO.packet_info_entries[entry_index_for_this_read].packet_addr;
	*packet_size = myFIFO.packet_info_entries[entry_index_for_this_read].packet_size;

	// update entry index for next read
	fifo_core.entry_index_for_next_read++;
	wraparound_entry_index_if_necessary(&fifo_core.entry_index_for_next_read);

	// update related info
	fifo_core.entry_index_being_used = entry_index_for_this_read;

	pthread_mutex_unlock(&fifo_core.mutex);

	return 0;
}

inline int free_last_read_packet(void)
{
	pthread_mutex_lock(&fifo_core.mutex);

	if (fifo_core.entry_index_being_used != -1) {
		// update related info
		fifo_core.entry_index_being_used = -1;
		fifo_core.used_entry_num--;
		fifo_core.buffer_used_size -= myFIFO.packet_info_entries[fifo_core.entry_index_being_used].packet_size;
	}
	pthread_mutex_unlock(&fifo_core.mutex);

	return 0;
}

u_int32_t CheckFIFOLength(void)
{
    return myFIFO.buffer_total_size;
}

void FreeFIFO(void)
{
    free(myFIFO.buffer_start_addr);
}

u_int32_t check_can_write_size(void)
{
    return (myFIFO.buffer_total_size - fifo_core.buffer_used_size);
}

u_int32_t check_can_read_size(void)
{
    return fifo_core.buffer_used_size;
}

u_int32_t check_packet_num(void)
{
    return fifo_core.used_entry_num;
}


