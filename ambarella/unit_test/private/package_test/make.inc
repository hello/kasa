##
## unit_test/private/pacakge_test/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2015 Ambarella, Inc.
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

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PACKAGE), y)

LOCAL_PATH	:= $(call my-dir)
DEWARP_LIB_PATH	:= $(AMB_TOPDIR)/prebuild/ambarella/library/dewarp

###
include $(CLEAR_VARS)
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

LOCAL_TARGET	:= test_ldc
LOCAL_SRCS	:= $(LOCAL_PATH)/test_ldc.c
LOCAL_CFLAGS	:= -I$(DEWARP_LIB_PATH)/include/arch_$(AMBARELLA_ARCH) \
				-I$(AMB_TOPDIR)/packages/utils
LOCAL_LDFLAGS	:= -lm -lrt -lpthread
LOCAL_LIBS	:= libutils.so
LOCAL_LDFLAGS	+= -L$(DEWARP_LIB_PATH)/lib/arch_$(AMBARELLA_ARCH) -ldewarp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_MIDDLEWARE_PROC), y)

LOCAL_TARGET    := test_image
LOCAL_SRCS      := $(LOCAL_PATH)/test_image.c
LOCAL_LDFLAGS   := -lpthread -lm
LOCAL_LIBS      := libamp.so libamdev.so
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LIBS	+= libimg_algo_s2l.a
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@mkdir -p $(FAKEROOT_DIR)/etc/idsp/cfg
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
include $(CLEAR_VARS)
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

LOCAL_TARGET	:= test_dewarp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_dewarp.c
LOCAL_CFLAGS	:= -I$(DEWARP_LIB_PATH)/include/arch_$(AMBARELLA_ARCH) \
				-I$(AMB_TOPDIR)/packages/utils
LOCAL_LDFLAGS	:= -lm -lrt -lpthread
LOCAL_LIBS	:= libutils.so
LOCAL_LDFLAGS	+= -L$(DEWARP_LIB_PATH)/lib/arch_$(AMBARELLA_ARCH) -ldewarp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###

ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_textinsert
LOCAL_SRCS	:= $(LOCAL_PATH)/test_textinsert.c
LOCAL_LIBS	:= libtextinsert.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/textinsert/
LOCAL_LDFLAGS	:= -L $(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
					-L $(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2 \
					-L $(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###

ifeq ($(BUILD_AMBARELLA_LBR_PACKAGE), y)

include $(CLEAR_VARS)
UTILS_PATH	:= $(AMB_TOPDIR)/app/smartcam/utils
MDET_PATH	:= $(AMB_TOPDIR)/prebuild/ambarella/library/smartcam/mdet

LOCAL_TARGET	:= test_lbr
LOCAL_SRCS	:= $(LOCAL_PATH)/test_lbr.c $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/iav.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/low_bitrate/include -I$(UTILS_PATH) -I$(MDET_PATH)/include
LOCAL_LDFLAGS	:= -lpthread \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/opencv/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib
LOCAL_LIBS	:= liblbr.so
LOCAL_LDFLAGS	+= -L$(MDET_PATH)/lib/arch_$(AMBARELLA_ARCH) -lmdet
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
include $(CLEAR_VARS)
SMARTRC_PATH	:= $(AMB_TOPDIR)/prebuild/ambarella/library/smartrc/lib

LOCAL_TARGET	:= test_smartrc
LOCAL_SRCS	:= $(LOCAL_PATH)/test_smartrc.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/prebuild/ambarella/library/smartrc/include \
					-I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:=
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)
LOCAL_LIBS += libdatatx.a
endif
LOCAL_LDFLAGS	+= -L$(SMARTRC_PATH)/arch_$(AMBARELLA_ARCH) -lsmartrc

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_md_motbuf
LOCAL_SRCS	:= $(LOCAL_PATH)/test_md_motbuf.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/md_motbuf/arch_$(AMBARELLA_ARCH)
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= libmd_motbuf.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

ifeq ($(BUILD_AMBARELLA_SECURITY_PACKAGE), y)

ifeq ($(BUILD_OPENSSL_WRAPPER_LIBRARY), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_cypher
LOCAL_SRCS	:= $(LOCAL_PATH)/test_cypher.cpp
LOCAL_LIBS	:= libopensslwrapper.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/security/openssl_wrapper/
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lopensslwrapper -lssl -lcrypto -ldl
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

ifeq ($(BUILD_AMBARELLA_LW_CRYPTOGRAPHY), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_sha256
LOCAL_SRCS	:= $(LOCAL_PATH)/test_sha256.cpp
LOCAL_LIBS	:= liblwcryptography.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/security/lw_cryptography/include/
LOCAL_LDFLAGS	:= -llwcryptography
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_md5
LOCAL_SRCS	:= $(LOCAL_PATH)/test_md5.cpp
LOCAL_LIBS	:= liblwcryptography.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/security/lw_cryptography/include/
LOCAL_LDFLAGS	:= -llwcryptography
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

ifeq ($(BUILD_AMBARELLA_DIGITAL_SIGNATURE), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_digital_signature
LOCAL_SRCS	:= $(LOCAL_PATH)/test_digital_signature.cpp
LOCAL_LIBS	:= libdigisign.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/security/digital_signature/
LOCAL_LDFLAGS	:=
ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_NATIVE), y)
LOCAL_LIBS	+= liblwcryptography.a
LOCAL_LDFLAGS	+= -llwcryptography
endif
ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_OPENSSL), y)
LOCAL_LDFLAGS	+= -lcrypto -lssl
endif
LOCAL_LDFLAGS	+= -ldigisign
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

ifeq ($(BUILD_AMBARELLA_CRYPTOCHIP_LIBRARY), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_cryptochip
LOCAL_SRCS	:= $(LOCAL_PATH)/test_cryptochip.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/security/cryptochip_library/
LOCAL_LDFLAGS	:= -lcryptochip
ifeq ($(BUILD_AMBARELLA_CRYPTOCHIP_LIBRARY_FROM_SOURCE), y)
LOCAL_LIBS	:= libcryptochip.a
else
LOCAL_LDFLAGS += -L$(AMB_TOPDIR)/packages/security/cryptochip_library/arch_$(AMBARELLA_ARCH)
endif
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_convert_openssl_key
LOCAL_SRCS	:= $(LOCAL_PATH)/test_convert_openssl_key.cpp
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include/
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto -ldl
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

ifeq ($(BUILD_AMBARELLA_LW_MEDIA), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_media
LOCAL_SRCS	:= $(LOCAL_PATH)/test_media.cpp
LOCAL_LIBS	:= liblwmedia.so
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/security/lw_media/include/
LOCAL_LDFLAGS	:=
ifeq ($(BUILD_AMBARELLA_LW_MEDIA_ALSA_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_FFMPEG_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/ffmpeg/usr/lib -lavformat -lavcodec -lavutil
endif

ifeq ($(BUILD_AMBARELLA_LW_MEDIA_AMBA_AAC_LIBRARY), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacdec -laacenc
endif

LOCAL_LIBS	:= liblwmedia.so
LOCAL_LDFLAGS += -lpthread -ldl -lrt -llwmedia
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

endif

###
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_eis_warp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_eis_warp.c
LOCAL_LIBS	:=
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/prebuild/ambarella/library/eis/include/arch_$(AMBARELLA_ARCH) -I$(AMB_TOPDIR)/packages/utils/ -g
LOCAL_LDFLAGS	:= -lm -lrt -lpthread -L$(AMB_TOPDIR)/prebuild/ambarella/library/eis/lib/arch_$(AMBARELLA_ARCH) -leis
LOCAL_LIBS	+= libutils.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(AMB_TOPDIR)/prebuild/ambarella/library/eis/lib/arch_$(AMBARELLA_ARCH)/* $(FAKEROOT_DIR)/usr/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
endif
