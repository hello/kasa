##
## kernel/private/lib/make.inc
##
## History:
##    2012/06/01 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_PRIVATE_MODULES), y)
.PHONY: prebuild_firmware

prebuild_firmware: LOCAL_PATH:=$(call my-dir)
prebuild_firmware: basic_fs
	@mkdir -p $(FIRMWARE_INSTALL_PATH)/
	@-cp -dpRf $(LOCAL_PATH)/firmware_$(AMBARELLA_ARCH)/*.bin $(FIRMWARE_INSTALL_PATH)/
ifeq ($(CONFIG_DSP_FIRMWARE_ENCODE_ONLY), y)
	@-cp -dpRf $(LOCAL_PATH)/firmware_$(AMBARELLA_ARCH)/encode_only/*.bin $(FIRMWARE_INSTALL_PATH)/
else
	@-cp -dpRf $(LOCAL_PATH)/firmware_$(AMBARELLA_ARCH)/generic/*.bin $(FIRMWARE_INSTALL_PATH)/
endif
	@echo "Copy $@ Done."

$(call add-target-into-build, prebuild_firmware)

###
endif

