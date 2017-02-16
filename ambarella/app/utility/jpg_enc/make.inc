##
##
LOCAL_PATH	:= $(call my-dir)

ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)
ifeq ($(CONFIG_AMBARELLA_LIBJPEG_TURBO_SUPPORT), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= jpg_enc
LOCAL_SRCS	:= $(LOCAL_PATH)/jpg_enc.c $(LOCAL_PATH)/chrome_convert.S
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer \
		-I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include

LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -ljpeg
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif


