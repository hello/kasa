

#ifndef _DATATX_LIB_H_
#define _DATATX_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum trans_method_s {
	TRANS_METHOD_NONE = 0,
	TRANS_METHOD_NFS,
	TRANS_METHOD_TCP,
	TRANS_METHOD_USB,
	TRANS_METHOD_STDOUT,
	TRANS_METHOD_UNDEFINED,
} trans_method_t;

int amba_transfer_init(int transfer_method);
int amba_transfer_deinit(int transfer_method);
int amba_transfer_open(const char *name, int transfer_method, int port);
int amba_transfer_write(int fd, void *data, int bytes, int transfer_method);
int amba_transfer_close(int fd, int transfer_method);

#ifdef __cplusplus
}
#endif


#endif
