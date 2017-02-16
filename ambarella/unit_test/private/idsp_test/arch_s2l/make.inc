##
## unit_test/private/idsp_test/arch_s2l/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params
LENS_PARAMS_PATH := $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/lens_params

#####
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_ituner
LOCAL_SRCS	:= $(LOCAL_PATH)/test_ituner.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_HdlrManager.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_TextHdlr.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaDSP_ImgCalibItunerImpl.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_Parser.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_Rule.c

LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(AMB_TOPDIR)/packages/img_dsp/arch_s2l/ -I$(LENS_PARAMS_PATH)

ifeq ($(BUILD_AMBARELLA_IMGPROC), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_dsp_s2l_hf
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib  -limg_dsp_s2l_hf
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

#####
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_tuning
LOCAL_SRCS	:= $(LOCAL_PATH)/test_tuning.c $(LOCAL_PATH)/idsp_netlink.c  $(LOCAL_PATH)/ae_timing.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_HdlrManager.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_TextHdlr.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaDSP_ImgCalibItunerImpl.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_Parser.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_Rule.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(LENS_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

#####
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_af
LOCAL_SRCS	:= $(LOCAL_PATH)/test_af.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= customer_loop
LOCAL_SRCS	:= $(LOCAL_PATH)/customer_loop.c $(LOCAL_PATH)/idsp_netlink.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(LENS_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_raw_seq
LOCAL_SRCS	:= $(LOCAL_PATH)/test_raw_seq.c
LOCAL_CFLAGS	:=
LOCAL_LIBS	:=

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif


#####
include $(CLEAR_VARS)

LOCAL_TARGET	:= cfg_srv
LOCAL_SRCS	:= $(LOCAL_PATH)/cfg_srv.c $(LOCAL_PATH)/cfg_parser.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
#ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
#ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
#LOCAL_LIBS	+= libimg_algo_s2l.so
#else
#LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
#endif
#else
#LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
#endif
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/unit_test/private/vout_test/lcd

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

#####
include $(CLEAR_VARS)

LOCAL_TARGET	:= upgrade_adj_bin
LOCAL_SRCS	:= $(LOCAL_PATH)/upgrade_adj_bin.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###