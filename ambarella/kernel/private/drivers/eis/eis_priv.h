/*
 * kernel/private/drivers/eis/arch_s2_ipc/eis_priv.h
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#ifndef __EIS_PRIV_H__
#define __EIS_PRIV_H__

#define	EIS_MAJOR		248
#define	EIS_MINOR		10
//#define EIS_VIN_IRQ		IDSP_VIN_SOFT_IRQ  //use IDSP VIN Soft IRQ instead (vic 2.3)
#define EIS_VIN_IRQ		IDSP_VIN_LAST_PIXEL_IRQ  //use IDSP last pixel instead (vic 2.5)

#define MAX_ENTRY_IN_RING_BUFFER     1024


typedef struct amba_eis_controller_s {
	int vin_irq;
	//ring buffer
	gyro_data_t * gyro_ring_buffer;
	int ring_buffer_read_index;
	int ring_buffer_write_index;
	int ring_buffer_entry_num;
	int ring_buffer_frame_sync_index; //this index is a record of write_index at the time of FRAME START , to sync with frame start.
	struct completion frame_compl;
	int ring_buffer_full;
	spinlock_t data_lock;
	int wait_count;
} amba_eis_controller_t;

typedef struct amba_eis_context_s {
	void * file;
	struct mutex * mutex;
	amba_eis_controller_t *controller;
} amba_eis_context_t;

int amba_eis_start_stat(amba_eis_context_t * context);
int amba_eis_stop_stat(amba_eis_context_t * context);
int amba_eis_get_stat(amba_eis_context_t * context,  amba_eis_stat_t * eis_stat);
int amba_eis_get_info(amba_eis_context_t * context,  amba_eis_info_t * eis_info);

int eis_init(amba_eis_controller_t * context);
void eis_exit(amba_eis_controller_t * context);
#endif
