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

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libtcmd.a

LOCAL_SRCS	:= $(LOCAL_PATH)/libtcmd.c  \
		$(LOCAL_PATH)/nl80211.c	\
		$(LOCAL_PATH)/os.c

LOCAL_CFLAGS	:= -I$(LOCAL_PATH) \
		-I$(PREBUILD_3RD_PARTY_DIR)/libnl-1/include \
		-DWLAN_API_NL80211

LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/libnl-1/lib/libnl \
		-lrt -lnl -lnl-1

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))