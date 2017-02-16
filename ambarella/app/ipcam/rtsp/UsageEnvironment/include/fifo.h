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
    u_int8_t    *packet_addr;
	u_int32_t	packet_size;
} packet_info_t;

void create_fifo(u_int32_t buffer_size, u_int32_t max_packet_num);

int write_one_packet(u_int8_t *packet_addr, u_int32_t packet_size);
int read_one_packet(u_int8_t **packet_addr, u_int32_t *packet_len);
inline int free_last_read_packet(void);

extern u_int32_t CheckFIFOLength(void);

extern void FreeFIFO(void);

u_int32_t check_can_write_size(void);
u_int32_t check_can_read_size(void);
u_int32_t check_packet_num(void);

#ifdef __cplusplus
}
#endif 

#endif