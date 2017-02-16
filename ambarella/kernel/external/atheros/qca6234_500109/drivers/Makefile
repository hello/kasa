#
export ATH_DRIVER_TOPDIR=${ATH_TOPDIR}/drivers

#################################################################################################################################
# Compat related flags
export
COMPAT_LATEST_VERSION = 1
KERNEL_VERSION := $(shell make -C $(KERNELPATH) kernelversion | sed -n 's/^\([0-9]\)\..*/\1/p')
ifneq ($(KERNEL_VERSION),2)
KERNEL_SUBLEVEL := $(shell make -C $(KERNELPATH) kernelversion | sed -n 's/^3\.\([0-9]\+\).*/\1/p')
else
COMPAT_26LATEST_VERSION = 39
KERNEL_26SUBLEVEL := $(shell make -C $(KERNELPATH) kernelversion | sed -n 's/^2\.6\.\([0-9]\+\).*/\1/p')
COMPAT_26VERSIONS := $(shell I=$(COMPAT_26LATEST_VERSION); while [ "$$I" -gt $(KERNEL_26SUBLEVEL) ]; do echo $$I; I=$$(($$I - 1)); done)
$(foreach ver,$(COMPAT_26VERSIONS),$(eval CONFIG_COMPAT_KERNEL_2_6_$(ver)=y))
KERNEL_SUBLEVEL := -1
endif
COMPAT_VERSIONS := $(shell I=$(COMPAT_LATEST_VERSION); while [ "$$I" -gt $(KERNEL_SUBLEVEL) ]; do echo $$I; I=$$(($$I - 1)); done)
$(foreach ver,$(COMPAT_VERSIONS),$(eval CONFIG_COMPAT_KERNEL_3_$(ver)=y))
#
# Need to build compat_firmware_class.ko ?
#
include $(KERNELPATH)/.config
ifdef CONFIG_COMPAT_KERNEL_2_6_33
ifdef CONFIG_FW_LOADER
 CONFIG_COMPAT_FIRMWARE_CLASS=m
endif
endif

CONFIG_ATH6KL_SDIO=m

#################################################################################################################################
# CFG80211 related flags

# Default always ON even kernel config don't support it.
ifndef CONFIG_CFG80211
CONFIG_CFG80211=m
DEF_CFG80211=y
endif

ifndef CONFIG_CFG80211_DEFAULT_PS
CONFIG_CFG80211_DEFAULT_PS=y
DEF_CFG80211_DEFAULT_PS=y
endif

# We need CONFIG_WIRELESS_EXT for CONFIG_CFG80211_WEXT for every kernel 
# version. The new way CONFIG_CFG80211_WEXT is called from the kernel 
# does not work with compat-wireless because it calls some callback 
# function on struct wiphy. This struct is shipped with compat-wireless 
# and changes from kernel version to version. We are using the 
# wireless_handlers attribute which will be activated by 
ifdef CONFIG_WIRELESS_EXT
ifndef CONFIG_CFG80211_WEXT
CONFIG_CFG80211_WEXT=y
DEF_CFG80211_WEXT=y
endif
endif

CONFIG_CFG80211_INTERNAL_REGDB=y

ifdef CONFIG_CFG80211_INTERNAL_REGDB
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211_INTERNAL_REGDB=y
ATH_LOCAL_CFLAGS += -DCONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB
endif

ifdef CONFIG_ACS_SUPPORT
ATH_LOCAL_CFLAGS += -DACS_SUPPORT=y
endif

ifdef CONFIG_HARDWARE_PBC
ATH_LOCAL_CFLAGS += -DCONFIG_HARDWARE_PBC=y
endif

#################################################################################################################################
# ATH6KL related flags
ATH_LOCAL_CFLAGS += -DCONFIG_ATH6KL_DEBUG=y

ifdef DEF_CONFIG_NL80211_TESTMODE
ATH_LOCAL_CFLAGS += -DCONFIG_NL80211_TESTMODE=y
endif

#################################################################################################################################
# Add flags to compiler
ifdef DEF_CFG80211
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211=y
endif

ifdef DEF_CFG80211_DEFAULT_PS
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211_DEFAULT_PS=y
endif

ifdef DEF_CFG80211_WEXT
ATH_LOCAL_CFLAGS += -DCONFIG_CFG80211_WEXT=y
endif



#################################################################################################################################
all: driver_clean driver_build

clean: driver_clean

driver_build: firmware_toarray
	@echo Build drivers
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_DRIVER_TOPDIR)/ath6kl KBUILD_EXTRA_SYMBOLS=$(KERNELPATH)/Module.symvers modules

driver_clean:
	@echo Clean drivers
#	@if [ "$(kernel)" != "3.8.8" ]; then \
#	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_DRIVER_TOPDIR)/compat clean; \
#	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_DRIVER_TOPDIR)/cfg80211 clean; \
#	fi
	${MAKEARCH} -C $(KERNELPATH) SUBDIRS=$(ATH_DRIVER_TOPDIR)/ath6kl clean

# Move to BOARD-specific Makefile
#driver_patch:
#	@echo Patch drivers
#	@patch -p0 < patches/00-remore_ns-type.patch
#	@patch -p0 < patches/02-remore_netlink-seq.patch
#	@patch -p0 < patches/03-change_IFF_BRIDGE_PORT.patch

# This is ONLY for USB solution.
firmware_install:
	@echo ============================================================================
	@echo install firmware 
	@echo ============================================================================
	@echo install firmware to ${INSTALL_ROOT}/lib/firmware/ath6k
	@rm -rf ${INSTALL_ROOT}/lib/firmware/ath6k
	@mkdir -p ${INSTALL_ROOT}/lib/firmware/ath6k
	cp -rf $(ATH_DRIVER_TOPDIR)/fw/firmware  ${INSTALL_ROOT}/lib/firmware/ath6k
	@if [ "$(BOARD_TYPE)" = "x86" ]; then \
		if [ "$(BUILD_DRIVERFW_TOLOCAL)" = "y" ]; then \
			echo install AR6006 v1.1 firmware to local x86:/lib/firmware/ath6k/AR6006/hw1.1/; \
			rm -rf /lib/firmware/ath6k/AR6006/hw1.1/; \
			mkdir -p /lib/firmware/ath6k/AR6006/hw1.1/; \
			cp $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6006/hw1.1/* /lib/firmware/ath6k/AR6006/hw1.1/; \
			echo install AR6004 REV5 firmware to local x86:/lib/firmware/ath6k/AR6004/hw1.3/; \
			rm -rf /lib/firmware/ath6k/AR6004/hw1.3/; \
			mkdir -p /lib/firmware/ath6k/AR6004/hw1.3/; \
			cp $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw1.3/fw.ram.bin /lib/firmware/ath6k/AR6004/hw1.3/fw.ram.bin; \
			cp $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw1.3/boardData_2_1_UB134_051.bin  /lib/firmware/ath6k/AR6004/hw1.3/bdata.bin; \
			echo install AR6004 REV8 firmware to local x86:/lib/firmware/ath6k/AR6004/hw3.0/; \
			rm -rf /lib/firmware/ath6k/AR6004/hw3.0/; \
			mkdir -p /lib/firmware/ath6k/AR6004/hw3.0/; \
			cp $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw3.0/fw.ram.bin /lib/firmware/ath6k/AR6004/hw3.0/fw.ram.bin; \
			cp $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw3.0/boardData_2_1_UB134_060.bin  /lib/firmware/ath6k/AR6004/hw3.0/bdata.bin; \
			if [ "$(BOARD_DATA)" = "QCA6234-USB" ]; then \
				cp -rf $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw3.0/boardData_2_1_QCA6234_050.bin  /lib/firmware/ath6k/AR6004/hw3.0/bdata.bin; \
			elif [ "$(BOARD_DATA)" = "UB134" ]; then \
				cp -rf $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw3.0/boardData_2_1_UB134_060.bin  /lib/firmware/ath6k/AR6004/hw3.0/bdata.bin; \
			fi \
		else \
			echo You must be root to install firmware to local...; \
		fi \
	fi
	
firmware_toarray:
	@echo Transfer defaulte bdata to C array
	@${ATH_TOPDIR}/build/util/ath6kl_bin2hex.sh $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw1.3/bdata.bin  ${ATH_DRIVER_TOPDIR}/ath6kl/fw_bdata.h       fw_bdata;
	@echo Transfer ${ATH_DRIVER_TOPDIR}/ath6kl/fw_bdata.h DONE...
	@${ATH_TOPDIR}/build/util/ath6kl_bin2hex.sh $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw1.3/fw.ram.bin ${ATH_DRIVER_TOPDIR}/ath6kl/fw_ram.h         fw_ram; 
	@echo Transfer $${ATH_DRIVER_TOPDIR}/ath6kl/fw_ram.h DONE...
	@${ATH_TOPDIR}/build/util/ath6kl_bin2hex.sh $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw3.0/bdata.bin  ${ATH_DRIVER_TOPDIR}/ath6kl/fw_bdata_3_0.h       fw_bdata_3_0;
	@echo Transfer ${ATH_DRIVER_TOPDIR}/ath6kl/fw_bdata_3_0.h DONE...
	@${ATH_TOPDIR}/build/util/ath6kl_bin2hex.sh $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6004/hw3.0/fw.ram.bin ${ATH_DRIVER_TOPDIR}/ath6kl/fw_ram_3_0.h         fw_ram_3_0
	@echo Transfer ${ATH_DRIVER_TOPDIR}/ath6kl/fw_ram_3_0.h  DONE...
	@${ATH_TOPDIR}/build/util/ath6kl_bin2hex.sh $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6006/hw1.1/bdata.bin  ${ATH_DRIVER_TOPDIR}/ath6kl/fw_bdata_6006_1_1.h       fw_bdata_6006_1_1;
	@echo Transfer ${ATH_DRIVER_TOPDIR}/ath6kl/fw_bdata_6006_1_1.h DONE...
	@${ATH_TOPDIR}/build/util/ath6kl_bin2hex.sh $(ATH_DRIVER_TOPDIR)/fw/firmware/AR6006/hw1.1/fw.ram.bin ${ATH_DRIVER_TOPDIR}/ath6kl/fw_ram_6006_1_1.h         fw_ram_6006_1_1
	@echo Transfer ${ATH_DRIVER_TOPDIR}/ath6kl/fw_ram_6006_1_1.h DONE...
