##
## /S2l/oryx/stream/common/make.inc
##
## History:
##   2014-11-3 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM), y)

COMMON_PATH := $(call my-dir)
LOCAL_PATH  := $(COMMON_PATH)

include $(COMMON_PATH)/iid/make.inc
include $(COMMON_PATH)/codecs/make.inc
include $(COMMON_PATH)/io/make.inc
include $(COMMON_PATH)/mp4_processer/make.inc

endif
