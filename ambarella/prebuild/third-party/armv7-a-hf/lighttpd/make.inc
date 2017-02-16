##
## prebuild/third-party/armv7-a/lighttpd/make.inc
##
## History:
##    2012/11/12 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_WEB_SERVICE_SUPPORT), y)

###
LOCAL_PATH	:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_lighttpd
LOCAL_SRCS	:= etc usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET):$(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/media
	@mkdir -p $(FAKEROOT_DIR)/webSvr/logs
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_WEB_CGI_C), y)
	@sed -i -e "s/\(\"\.cgi\"\ =>\ \)\"\/usr\/bin\/python\"/\1\"\"/g" $(FAKEROOT_DIR)/etc/lighttpd/lighttpd.conf
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif
