##
## kernel/private/drivers/vin/decoders/adv7441a/make.inc
##
## History:
##    2012/06/04 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_YUV_ADV7441A), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/Kbuild

include $(BUILD_DRIVER)


###
include $(CLEAR_VARS)

.PHONY: adv7441a_edid_bin

adv7441a_edid_bin: PRIVATE_PATH:=$(LOCAL_PATH)
adv7441a_edid_bin: basic_fs
	@mkdir -p $(FIRMWARE_INSTALL_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/EDID.bin $(FIRMWARE_INSTALL_PATH)/
	@echo "Copy $@ Done."

$(call add-target-into-build, adv7441a_edid_bin)

endif

