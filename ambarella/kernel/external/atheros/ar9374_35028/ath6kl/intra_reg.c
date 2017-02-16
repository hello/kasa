#include <linux/errno.h>
#include "core.h"
#include "debug.h"

#define AR6003_BOARD_DATA_ADDR		0x00540654
#define AR6003_BOARD_DATA_INIT_ADDR	0x00540658
#define AR6003_BOARD_DATA_OFFSET	4
#define AR6003_RD_OFFSET		12

#define AR6004_BOARD_DATA_ADDR		0x00400854
#define AR6004_BOARD_DATA_INIT_ADDR	0x00400858
#define AR6004_BOARD_DATA_OFFSET	4
#ifdef CONFIG_FPGA
#define AR6004_RD_OFFSET		12
#else
#define AR6004_RD_OFFSET		20
#endif

#define AR6006_BOARD_DATA_ADDR		0x00428854
#define AR6006_BOARD_DATA_INIT_ADDR	0x00428858
#define AR6006_BOARD_DATA_OFFSET	4
#define AR6006_RD_OFFSET		    20

extern unsigned short reg_domain;

int ath6kl_set_rd(struct ath6kl *ar) {
	u8 buf[32];
	u16 o_sum, o_ver, o_rd, o_rd_next;
	u32 n_rd, n_sum;
	u32 bd_addr = 0;
	int ret;
	u32 rd_offset;

	switch (ar->target_type) {
	case TARGET_TYPE_AR6003:
		rd_offset = AR6003_RD_OFFSET;
		ret = ath6kl_bmi_read(ar, AR6003_BOARD_DATA_ADDR, (u8 *)&bd_addr, 4);
		break;
	case TARGET_TYPE_AR6004:
		rd_offset = AR6004_RD_OFFSET;	
		ret = ath6kl_bmi_read(ar, AR6004_BOARD_DATA_ADDR, (u8 *)&bd_addr, 4);
		break;
	case TARGET_TYPE_AR6006:
		rd_offset = AR6006_RD_OFFSET;	
		ret = ath6kl_bmi_read(ar, AR6006_BOARD_DATA_ADDR, (u8 *)&bd_addr, 4);
		break;
	default:
		return -EINVAL;
	}

	if (ret)
		return ret;

	memset(buf, 0, sizeof(buf));
	ret = ath6kl_bmi_read(ar, bd_addr, buf, sizeof(buf));
	if (ret)
		return ret;
	switch (ar->target_type) {
	case TARGET_TYPE_AR6003:
		memcpy((u8 *)&o_sum, buf + AR6003_BOARD_DATA_OFFSET, 2);
		memcpy((u8 *)&o_ver, buf + AR6003_BOARD_DATA_OFFSET + 2, 2);
		memcpy((u8 *)&o_rd, buf + AR6003_RD_OFFSET, 2);
		memcpy((u8 *)&o_rd_next, buf + AR6003_RD_OFFSET + 2, 2);
		break;
	case TARGET_TYPE_AR6004:
		memcpy((u8 *)&o_sum, buf + AR6004_BOARD_DATA_OFFSET, 2);
		memcpy((u8 *)&o_ver, buf + AR6004_BOARD_DATA_OFFSET + 2, 2);
		memcpy((u8 *)&o_rd, buf + rd_offset, 2);
		memcpy((u8 *)&o_rd_next, buf + rd_offset + 2, 2);
		break;
	case TARGET_TYPE_AR6006:
		memcpy((u8 *)&o_sum, buf + AR6006_BOARD_DATA_OFFSET, 2);
		memcpy((u8 *)&o_ver, buf + AR6006_BOARD_DATA_OFFSET + 2, 2);
		memcpy((u8 *)&o_rd, buf + rd_offset, 2);
		memcpy((u8 *)&o_rd_next, buf + rd_offset + 2, 2);
		break;
	default:
		return -EINVAL;
	}
	n_rd = (o_rd_next << 16) + reg_domain;
	ret = ath6kl_bmi_write(ar, bd_addr + rd_offset, (u8 *)&n_rd, 4);		
	if (ret)
		return ret;

	n_sum = (o_ver << 16) + (o_sum ^ o_rd ^ reg_domain);
	switch (ar->target_type) {
	case TARGET_TYPE_AR6003:
		ret = ath6kl_bmi_write(ar, bd_addr + AR6003_BOARD_DATA_OFFSET,
			       (u8 *)&n_sum, 4);
		break;
	case TARGET_TYPE_AR6004:
		ret = ath6kl_bmi_write(ar, bd_addr + AR6004_BOARD_DATA_OFFSET,
			       (u8 *)&n_sum, 4);
		break;
	case TARGET_TYPE_AR6006:
		ret = ath6kl_bmi_write(ar, bd_addr + AR6006_BOARD_DATA_OFFSET,
			       (u8 *)&n_sum, 4);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
