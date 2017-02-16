/*******************************************************************************
 * version.h
 *
 * History:
 *   2015-09-16 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_INCLUDE_COMMON_MP4_PROCESSER_VERSION_H_
#define ORYX_STREAM_INCLUDE_COMMON_MP4_PROCESSER_VERSION_H_

#define MP4_PROCESSER_LIB_MAJOR 1
#define MP4_PROCESSER_LIB_MINOR 0
#define MP4_PROCESSER_LIB_PATCH 0
#define MP4_PROCESSER_LIB_VERSION ((MP4_PROCESSER_LIB_MAJOR << 16) | \
                               (MP4_PROCESSER_LIB_MINOR << 8)  | \
                               MP4_PROCESSER_LIB_PATCH)
#define RECORD_VERSION_STR "1.0.0"

#endif /* ORYX_STREAM_INCLUDE_COMMON_MP4_PROCESSER_VERSION_H_ */
