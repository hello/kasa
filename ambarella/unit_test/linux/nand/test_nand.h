/*
 * test_nand.h
 *
 * History:
 *	2008/7/10 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#ifndef   TEST_NAND_H
#define   TEST_NAND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>


typedef struct 
{
         pthread_mutex_t op_mutex;
         char *op_file;
	 char rd_flag;
	 
}Fname;

void *readfile(void *arg);

void *writefile(void *arg);

void *deletefile(void *arg);

void signal_handler(int sig);


#endif /* end   */

