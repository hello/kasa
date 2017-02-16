/*
 * codec_parser.h
 *
 * History:
 *	2015/02/10 - [Zhi He] create file
 *
 * Copyright (C) 2015 -2020, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

unsigned char get_h264_slice_type_le(unsigned char* pdata, unsigned char* first_mb_in_slice);
int get_h264_width_height_from_sps(unsigned char* p_data, unsigned int data_size, unsigned int *width, unsigned int * height);

