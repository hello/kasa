/*
 * iav_enc_api.h
 *
 * History:
 *	2013/11/11 - [Jian Tang] created file
 *
 * Copyright (C) 2013-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#ifndef __IAV_ENC_API_H__
#define __IAV_ENC_API_H__

/* iav_enc_api.c */
int iav_encode_init(struct ambarella_iav *iav);
void iav_encode_exit(struct ambarella_iav *iav);
int iav_encode_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long args);
int iav_restore_dsp_cmd(struct ambarella_iav *iav);
int iav_boot_dsp_action(struct ambarella_iav *iav);
int iav_enable_preview(struct ambarella_iav *iav, u32 is_resume);

/* iav_enc_buf.c */
int check_main_buffer_input(u32 is_pre_main, struct iav_rect * input, struct iav_rect * vin);
int iav_cfg_vproc_dptz(struct ambarella_iav *iav, struct iav_digital_zoom *dptz);
int iav_ioc_s_srcbuf_setup(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_srcbuf_setup(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_s_srcbuf_format(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_srcbuf_format(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_s_dptz(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_dptz(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_srcbuf_info(struct ambarella_iav * iav, void __user * arg);
void iav_init_source_buffer(struct ambarella_iav *iav);

/* iav_enc_bufcap.c */
void irq_update_bufcap(struct ambarella_iav *iav, encode_msg_t *msg);
void iav_init_bufcap(struct ambarella_iav *iav);
int iav_query_rawdesc(struct ambarella_iav *iav, struct iav_rawbufdesc *rawdesc);
int iav_query_yuvdesc(struct ambarella_iav *iav, struct iav_yuvbufdesc *yuvdesc);
int iav_query_me1desc(struct ambarella_iav *iav, struct iav_mebufdesc *me1desc);
int iav_query_me0desc(struct ambarella_iav *iav, struct iav_mebufdesc *me0desc);
int iav_query_bufcapdesc(struct ambarella_iav *iav, struct iav_bufcapdesc *bufdesc);

/* iav_enc_perf.c */
int iav_check_sys_mem_usage(struct ambarella_iav * iav, int iav_state, u32 stream_map);
int iav_check_sys_clock(struct ambarella_iav * iav);
int iav_check_warp_idsp_perf(struct ambarella_iav *iav, struct iav_warp_main* warp_areas);

/* iav_enc_mem.c */
int iav_create_mmap_table(struct ambarella_iav *iav);
int iav_mmap(struct file *filp, struct vm_area_struct *vma);
void handle_mem_msg(struct ambarella_iav *iav, DSP_MSG *msg);
int iav_map_dsp_partitions(struct ambarella_iav *iav);
int iav_unmap_dsp_partitions(struct ambarella_iav *iav);

/* iav_enc_pm.c */
void iav_init_pm(struct ambarella_iav *iav);
void iav_reset_pm(struct ambarella_iav *iav);
int iav_set_pm_bpc(struct ambarella_iav *iav, struct iav_privacy_mask *priv_mask);
int iav_set_pm_mctf(struct ambarella_iav *iav,
	struct iav_privacy_mask * priv_mask, u32 cmd_delay);
int iav_cfg_vproc_pm(struct ambarella_iav *iav, struct iav_privacy_mask *pm);
int iav_ioc_g_pm_info(struct ambarella_iav *iav, void __user * arg);
int iav_ioc_s_pm(struct ambarella_iav *iav, void __user * arg);
int iav_ioc_g_pm(struct ambarella_iav *iav, void __user * arg);
int iav_ioc_s_bpc_setup(struct ambarella_iav *iav, void __user * arg);
int iav_ioc_s_static_bpc(struct ambarella_iav *iav, void __user * arg);

/* iav_enc_pts.c */
int hw_pts_init(struct ambarella_iav *iav);
int hw_pts_deinit(void);
u64 get_hw_pts(struct ambarella_iav *iav, u32 audio_pts);

/* iav_enc_stream.c */
int iav_ioc_start_encode(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_stop_encode(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_g_query_desc(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_s_stream_format(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_stream_format(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_s_stream_cfg(struct ambarella_iav * iav, void __user * args);
int iav_ioc_g_stream_cfg(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_stream_info(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_s_stream_fps_sync(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_s_h264_cfg(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_h264_cfg(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_s_mjpeg_cfg(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_g_mjpeg_cfg(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_cfg_frame_sync(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_get_frame_sync(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_apply_frame_sync(struct ambarella_iav * iav, void __user * arg);
void iav_init_streams(struct ambarella_iav *iav);
void irq_iav_queue_frame(void *data, DSP_MSG *msg);
void irq_update_frame_count(struct ambarella_iav * iav, encode_msg_t * enc_msg,
	u32 force_update_flag);
int irq_update_stopping_stream_state(struct iav_stream *stream);
int is_stream_in_starting(struct iav_stream *stream);
int is_stream_in_stopping(struct iav_stream *stream);
int is_stream_in_encoding(struct iav_stream * stream);
int is_stream_in_idle(struct iav_stream * stream);
int check_buffer_config_limit(struct ambarella_iav *iav, u32 stream_id);
int check_stream_config_limit(struct ambarella_iav *iav, u32 stream_id);
void iav_sync_bsh_queue(void *data);
u32 iav_get_stream_map(struct ambarella_iav *iav);
void iav_clear_stream_state(struct ambarella_iav *iav);

/* iav_enc_test.c */
void iav_init_debug(struct ambarella_iav *iav);
int iav_ioc_s_debug_cfg(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_g_debug_cfg(struct ambarella_iav *iav, void __user *arg);

/* iav_enc_warp.c */
void iav_init_warp(struct ambarella_iav *iav);
int iav_ioc_c_warp_ctrl(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_g_warp_ctrl(struct ambarella_iav *iav, void __user *arg);
int iav_ioc_a_warp_ctrl(struct ambarella_iav *iav, void __user *arg);

/* iav_enc_efm.c */
void handle_efm_msg(struct ambarella_iav *iav, DSP_MSG *msg);
int iav_create_efm_pool(struct ambarella_iav *iav);
int iav_ioc_efm_get_pool_info(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_efm_request_frame(struct ambarella_iav * iav, void __user * arg);
int iav_ioc_efm_handshake_frame(struct ambarella_iav * iav, void __user * arg);

/* iav_netlink.c */
int init_netlink(struct ambarella_iav *iav);
int nl_send_request(struct iav_nl_obj *nl_obj, int cmd);

/* hw_timer.c */
int get_hwtimer_output_ticks(u64 *out_tick);
int get_hwtimer_output_freq(u32 *out_tick);

#endif	// __IAV_ENC_API_H__

