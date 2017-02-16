/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <wmi.h>
#include <bmi.h>
#include <fmi.h>
#include <htc_api.h>
#include <common_drv.h>

/* Atheros platform include files */
#include "athdrv_rexos.h"
#include "qcom_firmware.h"
#include "fwimg.h"
#include "a_debug.h"

#define MAX_DATA_SETS	10
static struct dset_entry
{
	A_UINT32 id;
	A_UINT32 address;
	A_UINT32 length;
	A_UINT32 version;
} s_dset_index[MAX_DATA_SETS];

/* 1552 is the max size in QCOM's implementation.    */ 
#define MAX_CHUNCK_SIZE 1552

#ifdef ATHR_EMULATION

void *do_fl_initialize(char* file, char* root, unsigned int flags)
{
	char filename[256];
	int fd;

	sprintf(filename, "%s/%s", root, file);

	if ((fd = open(filename, flags, S_IRWXU)) < 0)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
			(DBGFMT "Failed open \"%s\".\n", DBGARG, filename));
		return(FL_NULL);
	}

	return((void *) fd);
}

void do_fl_uninitialize(void *handle)
{
	close((int) handle);
}

int do_fl_seek(void* handle, unsigned int offset, int whence)
{
    if (handle != NULL) {
        if (lseek((int) handle, offset, whence) < 0)
            return -1;
    }
    return 0;
}

int get_img_hdr(void *handle, unsigned int flags, void *header,
	int reset)
{
	a_imghdr_t* h = (a_imghdr_t *) header;
	int fd = (int) handle;

	/* Should we start from the beginning */
	if(reset == TRUE)
	{
		if(lseek(fd, 0, SEEK_SET) != 0)
		{
			DPRINTF(DBG_FIRMWARE|DBG_ERROR,
				(DBGFMT "lseek(%d) failed.\n", DBGARG, fd));
			return(-1);
		}
	}

	do
	{
		if(read(fd, h, sizeof(a_imghdr_t)) != sizeof(a_imghdr_t))
		{
			DPRINTF(DBG_FIRMWARE|DBG_ERROR,
				(DBGFMT "read(%d) failed.\n", DBGARG, fd));
			return(-1);
		}

		h->flags = ntohl(h->flags);
		h->address = ntohl(h->address);
		h->data = ntohl(h->data);
		h->length = ntohl(h->length);

		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Found flags 0x%08x address 0x%08x data %d length %d.\n",
			DBGARG, h->flags, h->address, h->data, h->length));

		if((h->flags & flags) == flags)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "Match 0x%08x & 0x%08x = 0x%08x.\n",
				DBGARG, h->flags, flags, h->flags & flags));
			break;
		}
		else
		{
			if(lseek(fd, h->length, SEEK_CUR) < 0)
			{
				DPRINTF(DBG_FIRMWARE|DBG_ERROR,
					(DBGFMT "lseek(%d) failed.\n", DBGARG, fd));
				return(-1);
			}
		}
	} while((h->flags & FW_FLAG_END) != FW_FLAG_END);

	return(h->flags & FW_FLAG_END ? -1 : 0);
}

int get_fl_data(void *handle, unsigned int flags, char *buff, int len)
{
	int fd = (int) handle;
	
	if(read(fd, buff, len) != len)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
			(DBGFMT "Failed to read %d bytes for \"0x%08x\".\n", DBGARG,
			(int) len, flags));
		return(-1);
	}

	return(0);
}

int set_fl_data(void *handle, unsigned int flags, char *buff, int len)
{
    int fd = (int) handle;
    
    if(write(fd, buff, len) != len)
    {
        DPRINTF(DBG_FIRMWARE|DBG_ERROR,
            (DBGFMT "Failed to write %d bytes for \"0x%08x\".\n", DBGARG,
            (int) len, flags));
        return(-1);
    }

    return(0);
}

A_STATUS get_fl_size(A_CHAR* file, A_CHAR* root, A_UINT32* size)
{
	int fd;
    char filepath[256];
    
    strcpy(filepath, root);
    strcat(filepath, file);
    
	if ((fd = open(filepath, O_RDONLY)) < 0)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
			(DBGFMT "Failed open \"%s\".\n", DBGARG, filepath));
		return A_ERROR;
	}
	
	*size = lseek(fd, 0, SEEK_END);
	
	if (*size < 0)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
		    (DBGFMT "Failed open \"%s\".\n", DBGARG, filepath));
		close(fd);
		return A_ERROR;
	}
	
	close(fd);
	return A_OK;
}
#endif

static int write_bmi_data(AR_SOFTC_T *ar, void *handle, unsigned int flags,
	char *buf, int buf_len, A_UINT32 address, int length)
{
	int len;
	int lr;
	int lw;

	len = (length + 3) & ~0x3;  /* Make sure its a multiple of 4 */

	DPRINTF(DBG_FIRMWARE,
		(DBGFMT "%d (%d) bytes @ 0x%08x.\n", DBGARG, len, length, address));

	while(len > 0)
	{
		lr = min(buf_len, length);
		lw = min(buf_len, len);

#if 0
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "lr %d length %d lw %d len %d @ 0x%08x.\n", DBGARG,
			lr, length, lw, len, address));
#endif

		if(lr > 0 && A_FL_GET_DATA(handle, flags, buf, lr) < 0)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "A_FL_GET_DATA() failed.\n", DBGARG));
			return(-1);
		}
	
		if(BMIWriteMemory(ar->arHifDevice, address, (A_UCHAR*) buf, lw) != A_OK)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "BMIWriteMemory(0x%08x, %d) failed.\n", DBGARG,
				address, lw));
			return(-1);
		}

		address += lw;
		len -= lw;
		length -= lr;
	}

	return(0);
}

static int write_fmi_data(AR_SOFTC_T *ar, void *handle, unsigned int flags,
	char *buf, int buf_len, A_UINT32 address, int length, int erase)
{
	int len;
	int val;
	int lr;
	int lw;
	A_UINT32 start_address = address;

	len = (length + 3) & ~0x3;  /* Make sure its a multiple of 4 */

	DPRINTF(DBG_FIRMWARE,
		(DBGFMT "%d (%d) bytes @ 0x%08x.\n", DBGARG, len, length, address));

	if(erase == TRUE)
	{
		/* Erase */
		if(FMIPartialEraseFlash(ar->arHifDevice, start_address, len) != A_OK)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "FMIPartialEraseFlash(0x%08x) failed.\n", DBGARG,
				FW_FLAG_OS));
			return(-1);
		}
	}

	while(len > 0)
	{
		lr = min(buf_len, length);
		lw = min(buf_len, len);

#if 0
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "lr %d length %d lw %d len %d @ 0x%08x.\n", DBGARG,
			lr, length, lw, len, address));
#endif

		if(lr > 0 && A_FL_GET_DATA(handle, flags, buf, lr) < 0)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "A_FL_GET_DATA() failed.\n", DBGARG));
			return(-1);
		}
	
		/* Write */
		if(FMIWriteFlash(ar->arHifDevice, address, (A_UCHAR*) buf, lw) != A_OK)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "FMIWriteFlash(0x%08x, %d) failed.\n", DBGARG,
				address, lr));
			return(-1);
		}

		address += lw;
		len -= lw;
		length -= lr;
	}

	/* "Flush" */
	if(FMIReadFlash(ar->arHifDevice, start_address,
		(A_UCHAR *) &val, sizeof(val)) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "FMIReadFlash(0x%08x) (\"flush\") failed.\n", DBGARG,
			start_address));
		return(-1);
	}

	return(0);
}

int qcom_fw_upload(AR_SOFTC_T *ar)
{
	a_imghdr_t hdr;
	char *buf;
	int len;
	int val;
	void *handle;
	int use_flashlib = FALSE;
	int dset;
	void *pkt;
	int buf_len;
    A_UINT32 buf_data;

	DPRINTF(DBG_FIRMWARE, (DBGFMT "Enter\n", DBGARG));

	/* Initialize the fw interface */
	handle = A_FL_INITIALIZE(FIRMWARE_IMAGE_NAME, "Images/", A_FL_RDONLY);

	/* Check that we got something */
	if(handle == FL_NULL)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
			(DBGFMT "Unable to retrieve handle.\n", DBGARG));
		return(-1);
	}

	/* Allocate a network buffer of maximum size to hold data */
	if((pkt = A_NETBUF_ALLOC(MAX_CHUNCK_SIZE)) == NULL)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
			(DBGFMT "Unable to retrieve network buffer.\n", DBGARG));
		goto error;
	}

	/* For convenient access .... */
	buf_len = MAX_CHUNCK_SIZE;
	buf = A_NETBUF_DATA(pkt);

	/* $BMILOADER --write --address=$ECOSADDR --file=$ECOSRAM */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_BMI_OS, &hdr, TRUE) == 0)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load OS RAM image.\n", DBGARG));

		if(write_bmi_data(ar, handle, FW_FLAG_BMI_OS, buf, buf_len,
			hdr.address, hdr.length) < 0)
		{
			goto error;
		}
	}

	/* $BMILOADER --write --address=$FLASHADDR --file=$FLASHBIN */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_BMI_FLASHER, &hdr, TRUE) == 0)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load flasher RAM image.\n", DBGARG));

		if(write_bmi_data(ar, handle, FW_FLAG_BMI_FLASHER, buf, buf_len,
			hdr.address, hdr.length) < 0)
		{
			goto error;
		}
	}

	/* $BMILOADER --write --address=$ALT_FLASHPARTADDR --file=$ALT_FLASHPARTBIN */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_BMI_FLASHLIB, &hdr, TRUE) == 0)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load flash library RAM image.\n", DBGARG));

		/* Remember that we use a flash library */
		use_flashlib = TRUE;

		if(write_bmi_data(ar, handle, FW_FLAG_BMI_FLASHLIB, buf, buf_len,
			hdr.address, hdr.length) < 0)
		{
			goto error;
		}
	}

	/* $BMILOADER --set --address=0xac0140c0 --param=0x49 */
	DPRINTF(DBG_FIRMWARE, (DBGFMT "Write 0x09 to 0xac0140c0.\n", DBGARG));
	if(BMIWriteSOCRegister(ar->arHifDevice, 0xac0140c0, 0x49) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "BMIWriteSOCRegister() failed.\n", DBGARG));
		goto error;
	}

    /* $BMILOADER --set --address=0x8000060c --param=0x1 */
    DPRINTF(DBG_FIRMWARE, (DBGFMT "Write 0x1 to 0x8000060c.\n", DBGARG));
    buf_data = 0x1; 
    if(BMIWriteMemory(ar->arHifDevice, 0x8000060c, (A_UCHAR*) &buf_data,
       sizeof(buf_data)) 
       != A_OK)
    {
        DPRINTF(DBG_FIRMWARE,
            (DBGFMT "BMIWriteSOCRegister() failed.\n", DBGARG));
        goto error;
    }
    
    /* $BMILOADER --set --address=0xac004004 --param=0x920000a2 */
    DPRINTF(DBG_FIRMWARE, (DBGFMT "Write 0x1 to 0xac004004.\n", DBGARG));
    if(BMIWriteSOCRegister(ar->arHifDevice, 0xac004004, 0x920000a2) != A_OK)
    {
        DPRINTF(DBG_FIRMWARE,
            (DBGFMT "BMIWriteSOCRegister() failed.\n", DBGARG));
        goto error;
    }
    
	/* $BMILOADER --begin --address=$ECOSADDRU */
	DPRINTF(DBG_FIRMWARE,
		(DBGFMT "Set start address to 0xa0009000.\n", DBGARG));
	if(BMISetAppStart(ar->arHifDevice, 0xa0009000) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "BMISetAppStart() failed.\n", DBGARG));
		goto error;
	}

	DPRINTF(DBG_FIRMWARE, (DBGFMT "BMI Done.\n", DBGARG));
	if(BMIDone(ar->arHifDevice) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "BMIDone() failed.\n", DBGARG));
		goto error;
	}
	
	/***********************************************************************/
	/** Start the actual flashing.                                        **/
	/***********************************************************************/

	DPRINTF(DBG_FIRMWARE, (DBGFMT "Start Flashing.\n", DBGARG));
	FMIInit();

	/* Check if we need to initialize another flash library */
	if(use_flashlib == TRUE)
	{
		DPRINTF(DBG_FIRMWARE, (DBGFMT "Use flash library.\n", DBGARG));

		if(A_FL_GET_IMG_HDR(handle, FW_FLAG_BMI_FLASHLIB, &hdr, TRUE) == 0)
		{
			/* Initialize flash part */
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "Initialize flash part @ 0x%08x\n", DBGARG,
				hdr.address));

			if(FMIPartInitFlash(ar->arHifDevice, hdr.address) != A_OK)
			{
				DPRINTF(DBG_FIRMWARE,
					(DBGFMT "FMIPartInitFlash() failed.\n", DBGARG));
				goto error;
			}
		}
	}

	/* $FLASHLOADER $ALT_FLASHPART --write --address=$OSADDR --file=$OS */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_OS, &hdr, TRUE) == 0)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load OS Flash image.\n", DBGARG));

		if(write_fmi_data(ar, handle, FW_FLAG_OS, buf, buf_len,
			hdr.address, hdr.length, TRUE) < 0)
		{
			goto error;
		}
	}

	/* $FLASHLOADER $ALT_FLASHPART --write --address=$WLANADDR --file=$WLAN */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_APP, &hdr, TRUE) == 0)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load Application Flash image.\n", DBGARG));

		if(write_fmi_data(ar, handle, FW_FLAG_APP, buf, buf_len,
			hdr.address, hdr.length, TRUE) < 0)
		{
			goto error;
		}
	}

	/* Clear the area that will hold the Data sets and the Data set Index */
	/* $FLASHLOADER $ALT_FLASHPART --erase --address=$DSET_INDEX --length=$DSET_INDEX_SZ */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_DSETINDEX, &hdr, TRUE) == 0)
	{
		/* Erase */
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Erase \"DSETINDEX\" to 0x%08x %d bytes.\n", DBGARG,
			hdr.address, hdr.data));

		if(FMIPartialEraseFlash(ar->arHifDevice, hdr.address, hdr.data)
			!= A_OK)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "FMIPartialEraseFlash(0x%08x) failed.\n", DBGARG,
				FW_FLAG_DSETINDEX));
				goto error;
		}
    }
    
	/* Make sure we have a valid DataSet index */
	/* Collect all the data sets - max MAX_DATA_SETS */
	for(dset = 0; dset < MAX_DATA_SETS; dset++)
	{
		/* $FLASHLOADER $ALT_FLASHPART --write --dataset=0 --address=$DSET_INDEX */
		if(A_FL_GET_IMG_HDR(handle, FW_FLAG_DSET, &hdr,
			dset == 0 ? TRUE : FALSE) < 0)
		{
			break;
		}

		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load Data Set %d Flash image.\n", DBGARG, hdr.data));

		if(write_fmi_data(ar, handle, FW_FLAG_DSET, buf, buf_len,
			hdr.address, hdr.length, FALSE) < 0)
		{
			goto error;
		}

		/* Save data set index information */
		s_dset_index[dset].id = hdr.data;
		s_dset_index[dset].address = hdr.address & 0x00ffffff;
		s_dset_index[dset].length = hdr.length;
		s_dset_index[dset].version = 0xffffffff;
	}

    /* $MKDSETIMG --desc=$DSETS --out=$DSETBIN --start=DSETSTART */
    if(A_FL_GET_IMG_HDR(handle, FW_FLAG_DSETFILE, &hdr, TRUE) == 0)
    {
        DPRINTF(DBG_FIRMWARE,
            (DBGFMT "Load Dataset image.\n", DBGARG));

        if(write_fmi_data(ar, handle, FW_FLAG_DSETFILE, buf, buf_len,
            hdr.address, hdr.length, TRUE) < 0)
        {
            goto error;
        }
    }
    
	DPRINTF(DBG_FIRMWARE,
		(DBGFMT "Processed %d data sets.\n", DBGARG, dset));

	/* Update the dataset index */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_DSETINDEX, &hdr, TRUE) == 0)
	{
		len = sizeof(struct dset_entry) * dset;

		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Write 0x%08x to 0x%08x %d bytes.\n", DBGARG,
			FW_FLAG_DSETINDEX, hdr.address, len));

		/* Write */
		if(FMIWriteFlash(ar->arHifDevice, hdr.address, (A_UCHAR *) s_dset_index,
			len) != A_OK)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "FMIWriteFlash(0x%08x) failed.\n", DBGARG,
				FW_FLAG_DSETINDEX));
			goto error;
		}

		/* "Flush" */
		if(FMIReadFlash(ar->arHifDevice, hdr.address,
			(A_UCHAR *) &val, sizeof(val)) != A_OK)
		{
			DPRINTF(DBG_FIRMWARE,
				(DBGFMT "FMIReadFlash(0x%08x) (\"flush\") failed.\n", DBGARG,
				FW_FLAG_DSETINDEX));
			goto error;
		}
	}

	/* We are done now */
	DPRINTF(DBG_FIRMWARE, (DBGFMT "FMI Done.\n", DBGARG));
	if(FMIDone(ar->arHifDevice) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "FMIDone() failed.\n", DBGARG));
		goto error;
	}

	/* Clean up */
	A_FL_UNINITIALIZE(handle);
	A_NETBUF_FREE(pkt);

	/* Give it some time to be safe */
	A_MDELAY(300);

	DPRINTF(DBG_FIRMWARE, (DBGFMT "Exit.\n", DBGARG));

	return(0);

error:

	DPRINTF(DBG_FIRMWARE, (DBGFMT "Error.\n", DBGARG));

	/* Clean up */
	if(handle != FL_NULL)
	{
		A_FL_UNINITIALIZE(handle);
	}

	if(pkt != NULL)
	{
		A_NETBUF_FREE(pkt);
	}

	return(-1);
}

#ifdef CONFIG_HOST_TCMD_SUPPORT
int qcom_test_fw_upload(AR_SOFTC_T *ar)
{
	a_imghdr_t hdr;
	char *buf;
	void *handle, *pkt;
	
	DPRINTF(DBG_FIRMWARE, (DBGFMT "Enter\n", DBGARG));

	/* Initialize the fw interface */
	handle = A_FL_INITIALIZE(TST_FIRMWARE_IMAGE_NAME, "Images/", A_FL_RDONLY);

	/* Check that we got something */
	if(handle == FL_NULL)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
			(DBGFMT "Unable to retrieve handle.\n", DBGARG));
		return(-1);
	}

	/* Allocate a network buffer of maximum size to hold data */
	if((pkt = A_NETBUF_ALLOC(MAX_CHUNCK_SIZE)) == NULL)
	{
		DPRINTF(DBG_FIRMWARE|DBG_ERROR,
				(DBGFMT "Unable to retrieve network buffer.\n", DBGARG));
		goto error;
	}

	/* For convenient access .... */
	buf = A_NETBUF_DATA(pkt);
	
	/* $BMILOADER --write --address=$TCMD_APP_ADDR --file=$TCMD_APP_RAM */
	if(A_FL_GET_IMG_HDR(handle, FW_FLAG_TCMD_APP, &hdr, TRUE) == 0)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Load AR6000 TCMD image.\n", DBGARG));

		if(write_bmi_data(ar, handle, FW_FLAG_TCMD_APP, buf, MAX_CHUNCK_SIZE,
						  hdr.address, hdr.length) < 0)
		{
			goto error;
		}
	}
	
	/* $BMILOADER --begin --address=$TCMD_APP_ADDRU */
	DPRINTF(DBG_FIRMWARE,
			(DBGFMT "Set start address to 0x%x.\n", DBGARG, hdr.address));

	if(BMISetAppStart(ar->arHifDevice, hdr.address) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "BMISetAppStart() failed.\n", DBGARG));
		goto error;
	}

	DPRINTF(DBG_FIRMWARE, (DBGFMT "BMI Done.\n", DBGARG));
	if(BMIDone(ar->arHifDevice) != A_OK)
	{
		DPRINTF(DBG_FIRMWARE,
			(DBGFMT "BMIDone() failed.\n", DBGARG));
		goto error;
	}

	/* Clean up */
	A_FL_UNINITIALIZE(handle);
	A_NETBUF_FREE(pkt);

	/* Give it some time to be safe */
	A_MDELAY(200);
	
	DPRINTF(DBG_FIRMWARE, (DBGFMT "Exit.\n", DBGARG));

	return(0);
	
error:

	DPRINTF(DBG_FIRMWARE, (DBGFMT "Error.\n", DBGARG));

	/* Clean up */
	if(handle != FL_NULL)
	{
		A_FL_UNINITIALIZE(handle);
	}

	if(pkt != NULL)
	{
		A_NETBUF_FREE(pkt);
	}

	return(-1);
}
#endif
