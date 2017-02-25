##
## prebuild/third-party/armv7-a-hf/connman/make.inc
##
## History:
##    2016/09/06 - [Yupeng Chang] Created file
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

ifeq ($(CONFIG_AMBARELLA_CONNMAN_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_connman
LOCAL_SRCS	:= etc usr var
CONNMAN_ETH0_CONF = $(FAKEROOT_DIR)/var/lib/connman/eth0.config
CONNMAN_ETH1_CONF = $(FAKEROOT_DIR)/var/lib/connman/eth1.config

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/etc/connman
	@mkdir -p $(FAKEROOT_DIR)/var/lib/connman
	@rm -rf $(CONNMAN_ETH0_CONF)
	@rm -rf $(CONNMAN_ETH1_CONF)
	@if [ "$(CONFIG_AMBARELLA_ETH0)" = "y" ]; then   \
		echo "[service_eth0]" >> $(CONNMAN_ETH0_CONF); \
		echo "Type=ethernet"  >> $(CONNMAN_ETH0_CONF); \
		echo "MAC="           >> $(CONNMAN_ETH0_CONF); \
		echo "IPv6=off"       >> $(CONNMAN_ETH0_CONF); \
		if [ "$(CONFIG_AMBARELLA_ETH0_DHCP)" = "y" ]; then \
			echo "IPv4=dhcp"      >> $(CONNMAN_ETH0_CONF);   \
		else \
			echo "IPv4=$(CONFIG_AMBARELLA_ETH0_ADDRESS)/$(CONFIG_AMBARELLA_ETH0_IP_MASK)/$(CONFIG_AMBARELLA_ETH0_GW_ADDRESS)" >> $(CONNMAN_ETH0_CONF); \
		fi \
	fi; \
	if [ "$(CONFIG_AMBARELLA_ETH1)" = "y" ]; then    \
		echo "[service_eth1]" >> $(CONNMAN_ETH1_CONF); \
		echo "Type=ethernet"  >> $(CONNMAN_ETH1_CONF); \
		echo "MAC="           >> $(CONNMAN_ETH1_CONF); \
		echo "IPv6=off"       >> $(CONNMAN_ETH1_CONF); \
		if [ "$(CONFIG_AMBARELLA_ETH1_DHCP)" = "y" ]; then \
			echo "IPv4=dhcp"      >> $(CONNMAN_ETH1_CONF);   \
		else \
			echo "IPv4=$(CONFIG_AMBARELLA_ETH1_ADDRESS)/$(CONFIG_AMBARELLA_ETH1_IP_MASK)/$(CONFIG_AMBARELLA_ETH1_GW_ADDRESS)" >> $(CONNMAN_ETH1_CONF); \
		fi \
	fi
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

