##
## /S2l/oryx/stream/playback/make.inc
##
## History:
##   2014-9-10 - [ypchang] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_STREAM_PLAYBACK), y)

PLAYBACK_DIR := $(call my-dir)
LOCAL_PATH   := $(PLAYBACK_DIR)

## Filters and Engine
include $(PLAYBACK_DIR)/filters/demuxer/make.inc
include $(PLAYBACK_DIR)/filters/decoder/make.inc
include $(PLAYBACK_DIR)/filters/player/make.inc
include $(PLAYBACK_DIR)/instance/make.inc
include $(PLAYBACK_DIR)/demuxer/make.inc
include $(PLAYBACK_DIR)/engine/make.inc

endif
