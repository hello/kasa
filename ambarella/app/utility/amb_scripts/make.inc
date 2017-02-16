##
## app/utility/amb_scripts/make.inc
##
## History:
##    2014/06/04 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS), y)

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

.PHONY: add_amb_scripts

LOCAL_SRCS	:=

ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_ALL), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/*.sh)
else

ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_INITIAV), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/init.sh)
endif
ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_SHMOO), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/shmoo.sh)
endif
ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_AOSS), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/aoss.sh)
endif

endif
PRIVATE_SRCS	:= $(LOCAL_SRCS)
add_amb_scripts: $(LOCAL_SRCS)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(PRIVATE_SRCS) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh

$(call add-target-into-build, add_amb_scripts)

endif

