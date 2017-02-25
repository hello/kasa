/*#############################################################################
 * Copyright 2014-2015 Sony Corporation.
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Sony Corporation.
 * No part of this file may be copied, modified, sold, and distributed in any
 * form or by any means without prior explicit permission in writing from
 * Sony Corporation.
 * @file sonysd_test.c
 */
/*###########################################################################*/

#include <stdio.h>
#include <string.h>
#include "sonysd.h"

#include "sonysd_local.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


int main(int argc, char** argv){
	int ret = -1;

	if(argc < 2){
		PRINTF("Too few arguments.\n");
		goto Error1;
	}

	if(argc == 3){
		sonysd_set_devpath(argv[2]);
	}

	if(!strcmp(argv[1], "info")){
		sonysd_info info;
		if(SONYSD_SUCCESS != (ret = sonysd_get_info(&info))) {
			if(ret == SONYSD_ERR_NOCARD) {
				PRINTF("[Card Status] Card Not Inserted\n");
			}
			else if(ret == SONYSD_ERR_UNSUPPORT) {
				PRINTF("[Card Status] Lifetime Notification Unsupported\n");
			}
			else {
				PRINTF("[Card Status] Failed to Get Status\n");
			}
			goto Error1;
		}

		if(info.spare_block_rate >= 100) {
			PRINTF("[Card Status] Has Reached its Lifetime\n");
		}
		else if(info.life_information_den / 10 * 9 <= info.life_information_num) {
			PRINTF("[Card Status] Replacement Recommended\n");
		}
		else {
			PRINTF("[Card Status] Normally Functioning\n");
		}
		sonysd_debug_print_info(&info);
	}
	else if(!strcmp(argv[1], "err")){
		sonysd_errlog log;
		if(SONYSD_SUCCESS != (ret = sonysd_get_errlog(&log))){
			PRINTF("sonysd_get_errlog failed (%d).\n", ret);
			goto Error1;
		}
		sonysd_debug_print_errlog(&log);
	}
	else if(!strcmp(argv[1], "on")){
		if(SONYSD_SUCCESS != (ret = sonysd_set_operation_mode(1))){
			PRINTF("sonysd_set_operation_mode failed (%d).\n", ret);
			goto Error1;
		}
	}
	else if(!strcmp(argv[1], "off")){
		if(SONYSD_SUCCESS != (ret = sonysd_set_operation_mode(0))){
			PRINTF("sonysd_set_operation_mode failed (%d).\n", ret);
			goto Error1;
		}
	}
	else{
		PRINTF("Usage: sonysd_test [info | err | on | off] [device]\n");
		goto Error1;
	}

	ret = 0;

Error1:
	return ret;
}






