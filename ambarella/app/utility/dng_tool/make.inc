##
## app/utility/dng_tool/make.inc
##
## History:
##    2013/4/25 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBAREALLA_APP_UTILITY_DNG_TOOL), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

UTILITY_BIN_DIR	= $(FAKEROOT_DIR)/usr/bin

LOCAL_TARGET	:=	raw2dng
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c)


MODULE_INCLUDES	:=	-I$(AMB_TOPDIR)	\
					-I$(AMB_TOPDIR)/build/include \
					-I$(AMB_TOPDIR)/build/include/arch \
					
LIBS_INCLUDES	:=
LOCAL_LDFLAGS	:=



LOCAL_LIBS		:=  $(LIBS_INCLUDES)
LOCAL_CFLAGS	:=	$(MODULE_INCLUDES) -g -O3 -Wall


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UTILITY_BIN_DIR)/
	@cp -dpRf $< $(UTILITY_BIN_DIR)/
	@echo "Build $@ Done."
$(call add-target-into-build, $(LOCAL_TARGET))

endif