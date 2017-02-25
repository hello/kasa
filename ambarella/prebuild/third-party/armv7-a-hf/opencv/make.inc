##
## prebuild/third-party/armv7-a-hf/opencv/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (c) 2015 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##

ifeq ($(CONFIG_AMBARELLA_OPENCV_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_opencv
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_OPENCV_CORE_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_calib3d.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_core.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_features2d.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_flann.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_highgui.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_imgcodecs.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_imgproc.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_ml.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_objdetect.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_photo.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_shape.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_stitching.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_superres.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_video.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_videoio.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_videostab.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_IMGPROC_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_imgproc.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_IMGCODECS_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_imgcodecs.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_VIDEOIO_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_videoio.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_VIDEO_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_video.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_VIDEOSTAB_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_videostab.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_COREMISC_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_calib3d.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_features2d.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_flann.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_highgui.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_ml.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_objdetect.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_photo.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_shape.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_stitching.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_superres.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_CONTRIB_LIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_aruco.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_bgsegm.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_bioinspired.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_ccalib.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_datasets.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_dnn.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_dpm.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_face.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_fuzzy.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_line_descriptor.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_optflow.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_plot.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_reg.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_rgbd.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_saliency.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_stereo.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_structured_light.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_surface_matching.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_text.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_tracking.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_ximgproc.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_xobjdetect.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libopencv_xphoto.so*
endif
ifneq ($(CONFIG_AMBARELLA_OPENCV_UTILS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/opencv_*
	@rm -rf $(FAKEROOT_DIR)/usr/share/OpenCV
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

