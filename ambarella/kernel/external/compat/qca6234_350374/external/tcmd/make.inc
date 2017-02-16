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

WIFI_TOOLS 	= $(FAKEROOT_DIR)/usr/local/bin
LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= athtestcmd

LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c) 

LIBTCMD_DIR := 	$(LOCAL_PATH)/../libtcmd

LOCAL_CFLAGS	:= -I$(LOCAL_PATH)	\
		-I$(PREBUILD_3RD_PARTY_DIR)/libnl-1/include \
		-I$(LIBTCMD_DIR)	 -DWLAN_API_NL80211

LOCAL_LDFLAGS	:= -L$(LIBTCMD_DIR)\
		-L$(PREBUILD_3RD_PARTY_DIR)/libnl-1/lib/libnl \
		-lrt -ltcmd -lnl -lm -lpthread

LOCAL_LIBS	:=libtcmd.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WIFI_TOOLS)/
	@cp -dpRf $< $(WIFI_TOOLS)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))