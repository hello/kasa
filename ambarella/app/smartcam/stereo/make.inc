##
##
## History:
##    2015/07/21 - [Zhenwu Xue] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_SMARTCAM_APP_STEREO), y)
LOCAL_PATH	:= $(call my-dir)
UTILS_PATH	:= $(LOCAL_PATH)/../utils
OPENCV_PATH	:= $(PREBUILD_3RD_PARTY_DIR)/opencv

###
include $(CLEAR_VARS)

LOCAL_TARGET    :=  test_stereo
LOCAL_SRCS      :=  $(LOCAL_PATH)/test_stereo.cpp $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/fb.c
LOCAL_LIBS      :=  libstereo.a
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/packages/smartcam/stereo_lib -I$(OPENCV_PATH)/include -I$(OPENCV_PATH)/include/opencv -I$(UTILS_PATH)
LOCAL_LDFLAGS	:= -L$(OPENCV_PATH)/lib -Wl,-rpath=/lib
LOCAL_LDFLAGS	+= -lpthread -lrt -lm
LOCAL_LDFLAGS	+= -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann
LOCAL_LDFLAGS	+= -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml
LOCAL_LDFLAGS	+= -lopencv_nonfree -lopencv_objdetect -lopencv_photo
LOCAL_LDFLAGS	+= -lopencv_video -lopencv_videostab

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/	
	@chmod +x $(APP_INSTALL_PATH)/$@	
	@cp -dpRf $(PRIVATE_PATH)/map.xml.gz $(APP_INSTALL_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif


