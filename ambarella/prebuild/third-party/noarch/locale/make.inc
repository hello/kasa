##
## /S2l/prebuild/third-party/noarch/locale/make.inc
##
## History:
##   2015年8月24日 - [ypchang] created file
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

ifeq ($(CONFIG_AMBARELLA_LOCALE_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_locale

.PHONY: $(LOCAL_TARGET)
LOCALES :=

ifeq ($(CONFIG_AMBARELLA_LOCALE_ZH_CN_SUPPORT), y)
LOCALES += zh_CN
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_ZH_TW_SUPPORT), y)
LOCALES += zh_TW
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_ZH_HK_SUPPORT), y)
LOCALES += zh_HK
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_KO_KR_SUPPORT), y)
LOCALES += ko_KR
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_JA_JP_SUPPORT), y)
LOCALES += ja_JP
endif

include $(BUILD_PREBUILD)

ifeq ($(CONFIG_CPU_ARM1136JS), y)
gconv_lib: GLIBC_GCONV_PATH:=$(GLIBC_GCONV_PATH_ARMV6)
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
gconv_lib: GLIBC_GCONV_PATH:=$(GLIBC_GCONV_PATH_ARMV7_SF)
else ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
gconv_lib: GLIBC_GCONV_PATH:=$(GLIBC_GCONV_PATH_ARMV7)
endif
gconv_lib:
	@echo "Copying gconv library..."
	@if [ -n "$(GLIBC_GCONV_PATH)" ]; then \
		mkdir -p $(FAKEROOT_DIR)/usr/lib/gconv; \
		cp -dpRf $(GLIBC_GCONV_PATH)/gconv-modules $(FAKEROOT_DIR)/usr/lib/gconv/; \
		cp -dpRf $(GLIBC_GCONV_PATH)/UNICODE.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		cp -dpRf $(GLIBC_GCONV_PATH)/UTF-16.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		cp -dpRf $(GLIBC_GCONV_PATH)/UTF-32.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		cp -dpRf $(GLIBC_GCONV_PATH)/UTF-7.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		if [ "$(CONFIG_AMBARELLA_LOCALE_ZH_CN_SUPPORT)" = "y" ]; then \
			cp -dpRf $(GLIBC_GCONV_PATH)/GB18030.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
			cp -dpRf $(GLIBC_GCONV_PATH)/GBK.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
			cp -dpRf $(GLIBC_GCONV_PATH)/GBGBK.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
			cp -dpRf $(GLIBC_GCONV_PATH)/EUC-CN.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		fi; \
		if [ "$(CONFIG_AMBARELLA_LOCALE_ZH_TW_SUPPORT)" = "y" ]; then \
			cp -dpRf $(GLIBC_GCONV_PATH)/BIG5.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
			cp -dpRf $(GLIBC_GCONV_PATH)/EUC-TW.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		fi; \
		if [ "$(CONFIG_AMBARELLA_LOCALE_ZH_HK_SUPPORT)" = "y" ]; then \
			cp -dpRf $(GLIBC_GCONV_PATH)/BIG5HKSCS.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		fi; \
		if [ "$(CONFIG_AMBARELLA_LOCALE_KO_KR_SUPPORT)" = "y" ]; then \
			cp -dpRf $(GLIBC_GCONV_PATH)/EUC-KR.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		fi; \
		if [ "$(CONFIG_AMBARELLA_LOCALE_JA_JP_SUPPORT)" = "y" ]; then \
			cp -dpRf $(GLIBC_GCONV_PATH)/EUC-JP-MS.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
			cp -dpRf $(GLIBC_GCONV_PATH)/EUC-JP.so $(FAKEROOT_DIR)/usr/lib/gconv/; \
		fi; \
	fi

configure_system_lang: configure_profile
ifeq ($(CONFIG_AMBARELLA_SYSTEM_LANGUAGE_ZH_CN), y)
	@echo "export LANG=zh_CN.UTF-8" >> $(FAKEROOT_DIR)/etc/profile
endif
ifeq ($(CONFIG_AMBARELLA_SYSTEM_LANGUAGE_ZH_TW), y)
	@echo "export LANG=zh_TW.UTF-8" >> $(FAKEROOT_DIR)/etc/profile
endif
ifeq ($(CONFIG_AMBARELLA_SYSTEM_LANGUAGE_ZH_HK), y)
	@echo "export LANG=zh_HK.UTF-8" >> $(FAKEROOT_DIR)/etc/profile
endif
ifeq ($(CONFIG_AMBARELLA_SYSTEM_LANGUAGE_KO_KR), y)
	@echo "export LANG=ko_KR.UTF-8" >> $(FAKEROOT_DIR)/etc/profile
endif
ifeq ($(CONFIG_AMBARELLA_SYSTEM_LANGUAGE_JA_JP), y)
	@echo "export LANG=ja_JP.UTF-8" >> $(FAKEROOT_DIR)/etc/profile
endif

$(LOCAL_TARGET): gconv_lib
$(LOCAL_TARGET): configure_system_lang
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/locale
	@for locale in $(LOCALES); do \
		if [ -n "$(GLIBC_LOCALE_PATH)" ]; then \
			echo "Copying $${locale}.utf8..."; \
			cp -dpRf $(GLIBC_LOCALE_PATH)/$${locale}.utf8 $(FAKEROOT_DIR)/usr/lib/locale; \
		else \
			echo "Generating $${locale}.utf8..."; \
			localedef --prefix=$(FAKEROOT_DIR) --no-archive -c --little-endian -i $${locale} -f UTF-8 $${locale}.utf8; \
		fi \
	done;
	@echo "Generating locales Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif
