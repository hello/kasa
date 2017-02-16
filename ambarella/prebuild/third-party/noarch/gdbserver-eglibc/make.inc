##
## prebuild/third-party/gdbserver-eglibc/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_GDBSERVER_SUPPORT), y)

.PHONY: prebuild_gdbserver

prebuild_gdbserver:
	@mkdir -p $(FAKEROOT_DIR)/usr/bin
ifeq ($(CONFIG_CPU_ARM1136JS), y)
	@cp -dpRf $(GDBSERVER_DIR_ARMV6)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
ifeq ($(CONFIG_CPU_CORTEXA9), y)
	@cp -dpRf $(GDBSERVER_DIR_ARMV7_SF)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
	@cp -dpRf $(GDBSERVER_DIR_ARMV7)/gdbserver $(FAKEROOT_DIR)/usr/bin
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, prebuild_gdbserver)

endif
