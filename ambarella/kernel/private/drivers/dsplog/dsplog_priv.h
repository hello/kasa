#ifndef __DSP_LOG_PRIV_H__
#define  __DSP_LOG_PRIV_H__



typedef struct amba_dsplog_controller_s {
    int reserved;
} amba_dsplog_controller_t;


typedef struct amba_dsplog_context_s {
	void 	* file;
	struct mutex	* mutex;
	amba_dsplog_controller_t * controller;
} amba_dsplog_context_t;


int dsplog_init(void);
int dsplog_deinit(amba_dsplog_controller_t * controller);
int dsplog_start_cap(void);
int dsplog_stop_cap(void);
int dsplog_set_level(int level);
int dsplog_get_level(int * level);
int dsplog_parse(int arg);
int dsplog_read(char  *buffer, size_t size);

void dsplog_lock(void);
void dsplog_unlock(void);

//extern function provided by DSP driver
int dsp_init_logbuf(u8 **print_buffer, u32 * buffer_size);
int dsp_deinit_logbuf(u8 *print_buffer, u32 buffer_size);

#endif // __DSP_LOG_PRIV_H__
