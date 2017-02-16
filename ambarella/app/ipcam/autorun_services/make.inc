##
## app/ipcam/autorun_services/make.inc
##
## History:
##    2014/12/12 - [Alex ZHU] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AUTORUN_SERVICES), y)

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

.PHONY: add_autorun_scripts
.PHONY: del_autorun_services
.PHONY: add_autorun_services

LOCAL_SCPS	:=
LOCAL_SRVS	:=
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AUTORUN_DEMO), y)
LOCAL_SRVS	+=  $(LOCAL_PATH)/autorun_example.service
LOCAL_SCPS  +=  $(LOCAL_PATH)/autorun_example.sh
endif
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_ATR_SHMOO), y)
LOCAL_SRVS	+=  $(LOCAL_PATH)/autorun_shmoo.service
endif
ifeq ($(BUILD_AMBARELLA_APP_CLOUD_AGENT), y)
LOCAL_SRVS	+=  $(LOCAL_PATH)/autorun_ambacloud_agent.service
LOCAL_SCPS  +=  $(LOCAL_PATH)/autorun_ambacloud_agent.sh
endif
LOCAL_SRVS_NODIR	:= $(notdir $(LOCAL_SRVS))
LOCAL_ALLSRVS	:=
LOCAL_ALLSRVS	+= $(wildcard $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/autorun*.service)
LOCAL_ALLSRVS_NODIR	:=	$(notdir $(LOCAL_ALLSRVS))

ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AUTORUN_DEMO), y)
add_autorun_scripts: $(LOCAL_SCPS)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(LOCAL_SCPS) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh
endif

ifeq ($(BUILD_AMBARELLA_APP_CLOUD_AGENT), y)
add_autorun_scripts: $(LOCAL_SCPS)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(LOCAL_SCPS) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh
endif

## delete the autorun service in the folder of multi-user.target.wants when user switch config
del_autorun_services:
	@for n in $(LOCAL_ALLSRVS_NODIR);\
	do \
	rm $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/$$n; \
	done

add_autorun_services: $(LOCAL_SRVS)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/systemd/system/
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/
	@cp -dpRf $(LOCAL_SRVS) $(FAKEROOT_DIR)/usr/lib/systemd/system/
	@ln -s ../$(LOCAL_SRVS_NODIR) $(FAKEROOT_DIR)/usr/lib/systemd/system/multi-user.target.wants/$(LOCAL_SRVS_NODIR)

$(call add-target-into-build, add_autorun_scripts)
$(call add-target-into-build, del_autorun_services)
$(call add-target-into-build, add_autorun_services)

endif
