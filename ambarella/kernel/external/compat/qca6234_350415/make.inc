##
## kernel/external/compat/qca6234_350374/make.inc
##
## History:
##    2013/08/21 - [Tao Wu] Created file
##
## Copyright (C) 2004-2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
include $(CLEAR_VARS)

LOCAL_TARGET := compat_wireless_qca6234

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): build_private_drivers
	@echo "Build $@ Drivers:"
	@$(ECHO) "  $(patsubst %/,%\n ,$(EXT_DRIVER_MODULES))"
	@mkdir -p $(OUT_EXTERNAL_DRIVER_DIR)/compat
	@cp -dpRf $(EXTERNAL_DRIVER_DIR)/compat/qca6234_350415 $(OUT_EXTERNAL_DRIVER_DIR)/compat
	$(AMBA_MAKEFILE_V)$(MAKE) -s -C $(OUT_EXTERNAL_DRIVER_DIR)/compat/qca6234_350415 \
		ARCH=arm KMODDIR=extra KLIB_BUILD=$(LINUX_OUT_DIR) \
		KLIB=$(FAKEROOT_DIR) DESTDIR=$(FAKEROOT_DIR) install-modules;
	@echo "Build $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

