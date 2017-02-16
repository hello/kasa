##
## packages/bsreader/make.inc
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

ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libbsreader.a

LOCAL_CFLAGS	:=

LOCAL_SRCS	:= $(LOCAL_PATH)/fifo.c
ifeq ($(CONFIG_ARCH_A5S),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_a5s.c
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/build/include
else
ifeq ($(CONFIG_ARCH_S2),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s2.c
endif
ifeq ($(CONFIG_ARCH_S2E),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s2.c
endif
ifeq ($(CONFIG_ARCH_S2L),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_s2l.c
endif
ifeq ($(CONFIG_ARCH_A7L),y)
LOCAL_SRCS	+= $(LOCAL_PATH)/bsreader_a7l.c
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/build/include
endif
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

