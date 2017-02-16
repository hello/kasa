/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "core.h"
#include "hif-ops.h"
#include "target.h"
#include "debug.h"

int ath6kl_bmi_done(struct ath6kl *ar)
{
	int ret;
	u32 cid = BMI_DONE;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_dbg(ATH6KL_DBG_BMI, "bmi done skipped\n");
		return 0;
	}

	ar->bmi.done_sent = true;
	endianBuf32 = cpu_to_le32(cid);

	ret = ath6kl_hif_bmi_write(ar, (u8 *)&endianBuf32, sizeof(endianBuf32));
	if (ret) {
		ath6kl_err("Unable to send bmi done: %d\n", ret);
		return ret;
	}

	return 0;
}

int ath6kl_bmi_get_target_info(struct ath6kl *ar,
			       struct ath6kl_bmi_target_info *targ_info)
{
	int ret;
	u32 cid = BMI_GET_TARGET_INFO;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}
	endianBuf32 = cpu_to_le32(cid);
	ret = ath6kl_hif_bmi_write(ar, (u8 *)&endianBuf32, sizeof(endianBuf32));
	if (ret) {
		ath6kl_err("Unable to send get target info: %d\n", ret);
		return ret;
	}

	if (ar->hif_type == ATH6KL_HIF_TYPE_USB) {
		ret = ath6kl_hif_bmi_read(ar, (u8 *)targ_info,
					  sizeof(*targ_info));
	} else {
		ret = ath6kl_hif_bmi_read(ar, (u8 *)&targ_info->version,
				sizeof(targ_info->version));
	}

	if (ret) {
		ath6kl_err("Unable to recv target info: %d\n", ret);
		return ret;
	}

	if (le32_to_cpu(targ_info->version) == TARGET_VERSION_SENTINAL) {
		/* Determine how many bytes are in the Target's targ_info */
		ret = ath6kl_hif_bmi_read(ar,
				   (u8 *)&targ_info->byte_count,
				   sizeof(targ_info->byte_count));
		if (ret) {
			ath6kl_err("unable to read target info byte count: %d\n",
				   ret);
			return ret;
		}

		/*
		 * The target's targ_info doesn't match the host's targ_info.
		 * We need to do some backwards compatibility to make this work.
		 */
		if (le32_to_cpu(targ_info->byte_count) != sizeof(*targ_info)) {
			WARN_ON(1);
			return -EINVAL;
		}

		/* Read the remainder of the targ_info */
		ret = ath6kl_hif_bmi_read(ar,
				   ((u8 *)targ_info) +
				   sizeof(targ_info->byte_count),
				   sizeof(*targ_info) -
				   sizeof(targ_info->byte_count));

		if (ret) {
			ath6kl_err("Unable to read target info (%d bytes): %d\n",
				   le32_to_cpu(targ_info->byte_count), ret);
			return ret;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_BMI, "target info (ver: 0x%x type: 0x%x)\n",
		le32_to_cpu(targ_info->version), le32_to_cpu(targ_info->type));

	return 0;
}

int ath6kl_bmi_read(struct ath6kl *ar, u32 addr, u8 *buf, u32 len)
{
	u32 cid = BMI_READ_MEMORY;
	int ret;
	u32 offset;
	u32 len_remain, rx_len;
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = ar->bmi.max_data_size + sizeof(cid) + sizeof(addr) + sizeof(len);
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI,
		   "bmi read memory: device: addr: 0x%x, len: %d\n",
		   addr, len);

	len_remain = len;

	while (len_remain) {
		rx_len = (len_remain < ar->bmi.max_data_size) ?
					len_remain : ar->bmi.max_data_size;
		offset = 0;
		endianBuf32 = cpu_to_le32(cid);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(cid);
		endianBuf32 = cpu_to_le32(addr);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(addr);
		endianBuf32 = cpu_to_le32(rx_len);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(len);

		ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
		if (ret) {
			ath6kl_err("Unable to write to the device: %d\n",
				   ret);
			return ret;
		}
		ret = ath6kl_hif_bmi_read(ar, ar->bmi.cmd_buf, rx_len);
		if (ret) {
			ath6kl_err("Unable to read from the device: %d\n",
				   ret);
			return ret;
		}
		memcpy(&buf[len - len_remain], ar->bmi.cmd_buf, rx_len);
		len_remain -= rx_len; addr += rx_len;
	}

#ifdef __BIG_ENDIAN
	if (len == 4) {
		u32 buf_tmp;

		memcpy(&buf_tmp, buf, sizeof(u32));
		buf_tmp = le32_to_cpu(buf_tmp);
		memcpy(buf, &buf_tmp, sizeof(u32));
	}
#endif

	return 0;
}

int ath6kl_bmi_write(struct ath6kl *ar, u32 addr, u8 *buf, u32 len)
{
	u32 cid = BMI_WRITE_MEMORY;
	int ret;
	u32 offset;
	u32 len_remain, tx_len;
	const u32 header = sizeof(cid) + sizeof(addr) + sizeof(len);
	u8 aligned_buf[400];
	u8 *src;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	if ((ar->bmi.max_data_size + header) > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (WARN_ON(ar->bmi.max_data_size > sizeof(aligned_buf)))
		return -E2BIG;

	memset(ar->bmi.cmd_buf, 0, ar->bmi.max_data_size + header);

	ath6kl_dbg(ATH6KL_DBG_BMI,
		  "bmi write memory: addr: 0x%x, len: %d\n", addr, len);

	len_remain = len;
	while (len_remain) {
		src = &buf[len - len_remain];

		if (len_remain < (ar->bmi.max_data_size - header)) {
			if (len_remain & 3) {
				/* align it with 4 bytes */
				len_remain = len_remain +
					     (4 - (len_remain & 3));
				memcpy(aligned_buf, src, len_remain);
				src = aligned_buf;
			}
			tx_len = len_remain;
		} else {
			tx_len = (ar->bmi.max_data_size - header);
		}

		offset = 0;
		endianBuf32 = cpu_to_le32(cid);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(cid);
		endianBuf32 = cpu_to_le32(addr);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(addr);
		endianBuf32 = cpu_to_le32(tx_len);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(tx_len);
		if (len == 4) {
			endianBuf32 = cpu_to_le32(*((u32 *)src));
			memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
				sizeof(endianBuf32));
		} else {
			memcpy(&(ar->bmi.cmd_buf[offset]), src, tx_len);
		}
		offset += tx_len;

		ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
		if (ret) {
			ath6kl_err("Unable to write to the device: %d\n",
				   ret);
			return ret;
		}
		len_remain -= tx_len; addr += tx_len;
	}

	return 0;
}

int ath6kl_bmi_execute(struct ath6kl *ar, u32 addr, u32 *param)
{
	u32 cid = BMI_EXECUTE;
	int ret;
	u32 offset;
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = sizeof(cid) + sizeof(addr) + sizeof(param);
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI, "bmi execute: addr: 0x%x, param: %d)\n",
		   addr, *param);

	offset = 0;
	endianBuf32 = cpu_to_le32(cid);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(cid);
	endianBuf32 = cpu_to_le32(addr);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(addr);
	endianBuf32 = cpu_to_le32(*param);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(*param);

	ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
	if (ret) {
		ath6kl_err("Unable to write to the device: %d\n", ret);
		return ret;
	}

	ret = ath6kl_hif_bmi_read(ar, ar->bmi.cmd_buf, sizeof(*param));
	if (ret) {
		ath6kl_err("Unable to read from the device: %d\n", ret);
		return ret;
	}

	memcpy(param, ar->bmi.cmd_buf, sizeof(*param));

#ifdef __BIG_ENDIAN
	{
		u32 buf_tmp;

		memcpy(&buf_tmp, param, sizeof(u32));
		buf_tmp = le32_to_cpu(buf_tmp);
		memcpy(param, &buf_tmp, sizeof(u32));
	}
#endif

	return 0;
}

int ath6kl_bmi_set_app_start(struct ath6kl *ar, u32 addr)
{
	u32 cid = BMI_SET_APP_START;
	int ret;
	u32 offset;
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = sizeof(cid) + sizeof(addr);
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI, "bmi set app start: addr: 0x%x\n", addr);

	offset = 0;
	endianBuf32 = cpu_to_le32(cid);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(cid);
	endianBuf32 = cpu_to_le32(addr);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(addr);

	ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
	if (ret) {
		ath6kl_err("Unable to write to the device: %d\n", ret);
		return ret;
	}

	return 0;
}

int ath6kl_bmi_reg_read(struct ath6kl *ar, u32 addr, u32 *param)
{
	u32 cid = BMI_READ_SOC_REGISTER;
	int ret;
	u32 offset;
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = sizeof(cid) + sizeof(addr);
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI, "bmi read SOC reg: addr: 0x%x\n", addr);

	offset = 0;
	endianBuf32 = cpu_to_le32(cid);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(cid);
	endianBuf32 = cpu_to_le32(addr);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(addr);

	ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
	if (ret) {
		ath6kl_err("Unable to write to the device: %d\n", ret);
		return ret;
	}

	ret = ath6kl_hif_bmi_read(ar, ar->bmi.cmd_buf, sizeof(*param));
	if (ret) {
		ath6kl_err("Unable to read from the device: %d\n", ret);
		return ret;
	}
	memcpy(param, ar->bmi.cmd_buf, sizeof(*param));

#ifdef __BIG_ENDIAN
	{
		u32 buf_tmp;

		memcpy(&buf_tmp, param, sizeof(u32));
		buf_tmp = le32_to_cpu(buf_tmp);
		memcpy(param, &buf_tmp, sizeof(u32));
	}
#endif

	return 0;
}

int ath6kl_bmi_reg_write(struct ath6kl *ar, u32 addr, u32 param)
{
	u32 cid = BMI_WRITE_SOC_REGISTER;
	int ret;
	u32 offset;
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = sizeof(cid) + sizeof(addr) + sizeof(param);
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI,
		   "bmi write SOC reg: addr: 0x%x, param: %d\n",
		    addr, param);

	offset = 0;
	endianBuf32 = cpu_to_le32(cid);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(cid);
	endianBuf32 = cpu_to_le32(addr);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(addr);
	endianBuf32 = cpu_to_le32(param);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(param);

	ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
	if (ret) {
		ath6kl_err("Unable to write to the device: %d\n", ret);
		return ret;
	}

	return 0;
}

int ath6kl_bmi_lz_data(struct ath6kl *ar, u8 *buf, u32 len)
{
	u32 cid = BMI_LZ_DATA;
	int ret;
	u32 offset;
	u32 len_remain, tx_len;
	const u32 header = sizeof(cid) + sizeof(len);
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = ar->bmi.max_data_size + header;
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI, "bmi send LZ data: len: %d)\n",
		   len);

	len_remain = len;
	while (len_remain) {
		tx_len = (len_remain < (ar->bmi.max_data_size - header)) ?
			  len_remain : (ar->bmi.max_data_size - header);

		offset = 0;
		endianBuf32 = cpu_to_le32(cid);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(cid);
		endianBuf32 = cpu_to_le32(tx_len);
		memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32,
			sizeof(endianBuf32));
		offset += sizeof(tx_len);
		memcpy(&(ar->bmi.cmd_buf[offset]), &buf[len - len_remain],
			tx_len);
		offset += tx_len;

		ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
		if (ret) {
			ath6kl_err("Unable to write to the device: %d\n",
				   ret);
			return ret;
		}

		len_remain -= tx_len;
	}

	return 0;
}

int ath6kl_bmi_lz_stream_start(struct ath6kl *ar, u32 addr)
{
	u32 cid = BMI_LZ_STREAM_START;
	int ret;
	u32 offset;
	u16 size;
	u32 endianBuf32;

	if (ar->bmi.done_sent) {
		ath6kl_err("bmi done sent already, cmd %d disallowed\n", cid);
		return -EACCES;
	}

	size = sizeof(cid) + sizeof(addr);
	if (size > ar->bmi.max_cmd_size) {
		WARN_ON(1);
		return -EINVAL;
	}
	memset(ar->bmi.cmd_buf, 0, size);

	ath6kl_dbg(ATH6KL_DBG_BMI,
		   "bmi LZ stream start: addr: 0x%x)\n",
		    addr);

	offset = 0;
	endianBuf32 = cpu_to_le32(cid);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(cid);
	endianBuf32 = cpu_to_le32(addr);
	memcpy(&(ar->bmi.cmd_buf[offset]), &endianBuf32, sizeof(endianBuf32));
	offset += sizeof(addr);

	ret = ath6kl_hif_bmi_write(ar, ar->bmi.cmd_buf, offset);
	if (ret) {
		ath6kl_err("Unable to start LZ stream to the device: %d\n",
			   ret);
		return ret;
	}

	return 0;
}

int ath6kl_bmi_fast_download(struct ath6kl *ar, u32 addr, u8 *buf, u32 len)
{
	int ret;
	u32 last_word = 0;
	u32 last_word_offset = len & ~0x3;
	u32 unaligned_bytes = len & 0x3;

	ret = ath6kl_bmi_lz_stream_start(ar, addr);
	if (ret)
		return ret;

	if (unaligned_bytes) {
		/* copy the last word into a zero padded buffer */
		memcpy(&last_word, &buf[last_word_offset], unaligned_bytes);
	}

	ret = ath6kl_bmi_lz_data(ar, buf, last_word_offset);
	if (ret)
		return ret;

	if (unaligned_bytes)
		ret = ath6kl_bmi_lz_data(ar, (u8 *)&last_word, 4);

	if (!ret) {
		/* Close compressed stream and open a new (fake) one.
		 * This serves mainly to flush Target caches. */
		ret = ath6kl_bmi_lz_stream_start(ar, 0x00);
	}
	return ret;
}

void ath6kl_bmi_reset(struct ath6kl *ar)
{
	ar->bmi.done_sent = false;
}

int ath6kl_bmi_init(struct ath6kl *ar)
{
	if (WARN_ON(ar->bmi.max_data_size == 0))
		return -EINVAL;

	/* cmd + addr + len + data_size */
	ar->bmi.max_cmd_size = ar->bmi.max_data_size + (sizeof(u32) * 3);

	ar->bmi.cmd_buf = kzalloc(ar->bmi.max_cmd_size, GFP_ATOMIC);
	if (!ar->bmi.cmd_buf)
		return -ENOMEM;

	return 0;
}

void ath6kl_bmi_cleanup(struct ath6kl *ar)
{
	kfree(ar->bmi.cmd_buf);
	ar->bmi.cmd_buf = NULL;
}
