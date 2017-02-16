##
## /S2l/oryx/stream/record/make.inc
##
## History:
##   2014-12-8 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_RECORD), y)

RECORD_DIR := $(call my-dir)
LOCAL_PATH := $(RECORD_DIR)

## Filter and Engine
include $(RECORD_DIR)/filters/avqueue/make.inc
include $(RECORD_DIR)/filters/event/make.inc
include $(RECORD_DIR)/filters/audio/make.inc
include $(RECORD_DIR)/filters/video/make.inc
include $(RECORD_DIR)/filters/muxer/make.inc
include $(RECORD_DIR)/instance/make.inc
include $(RECORD_DIR)/muxers/make.inc
include $(RECORD_DIR)/engine/make.inc

endif
