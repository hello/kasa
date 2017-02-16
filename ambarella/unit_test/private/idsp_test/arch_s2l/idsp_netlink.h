/********************************************************************
 * idsp_netlink.h
 *
 * History:
 *	2014/09/25 - [Jing-yang Qiu] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ********************************************************************/


int init_netlink(void);
void * netlink_loop(void * data);

int do_prepare_aaa(void);
int do_start_aaa(void);
int do_stop_aaa(void);
int send_image_msg_exit(void);
int check_iav_work(void);


