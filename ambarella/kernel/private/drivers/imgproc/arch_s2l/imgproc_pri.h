#ifndef __IMGPROC_PRI_H__
#define __IMGPROC_PRI_H__

#if 1
#define img_printk(S...)		printk(KERN_DEBUG "img: "S)
#define img_dsp(cmd, arg)	\
	printk(KERN_DEBUG "%s: "#arg" = %d\n", __func__, (u32)cmd.arg)

#define img_dsp_hex(cmd, arg)	\
	printk(KERN_DEBUG "%s: "#arg" = 0x%x\n", __func__, (u32)cmd.arg)
#else
#define img_printk(S...)
#define img_dsp(cmd, arg)
#define img_dsp_hex(cmd, arg)
#endif

#ifdef LOG_DSP_CMD
#define DBG_CMD		printk
#else
#define DBG_CMD(...)
#endif


#endif

