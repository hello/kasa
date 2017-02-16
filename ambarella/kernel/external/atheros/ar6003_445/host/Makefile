#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2005-2010 Atheros Corporation.  All rights reserved.
# 
# 
# The software source and binaries included in this development package are
# licensed, not sold. You, or your company, received the package under one
# or more license agreements. The rights granted to you are specifically
# listed in these license agreement(s). All other rights remain with Atheros
# Communications, Inc., its subsidiaries, or the respective owner including
# those listed on the included copyright notices.  Distribution of any
# portion of this package must be in strict compliance with the license
# agreement(s) terms.
# </copyright>
# 
# <summary>
# 	Wifi driver for AR6002
# </summary>
#
#------------------------------------------------------------------------------
#==============================================================================
# Author(s): ="Atheros"
#==============================================================================

# AR6K Host driver makefile
# 
# Minimal build invocation:
#   
#     make (all | clean | clobber)  
#
# Extended Make invocation:
#
#     make ATH_BUILD_TYPE=<build type> ATH_SDIO_STACK_BASE=<sdio stack install path> (all | clean | clobber)
#
# Notes:
#     1. This makefile must be invoked from the host/ directory
#     2. The <build type> must match an entry in localmake.linux.inc.
#     3. The localmake.linux.inc can be overridden using an include file outside the build tree.
#        This file (name and path) can be set via the ATH_MAKE_INCLUDE_OVERRIDE variable.
#        ** If ATH_MAKE_INCLUDE_OVERRIDE is used, you can define all build variables in that file 
#        instead of using command line arguments **.  This feature is provided for developers
#        that may want to customize the build using a single include file.
#
#        For example :
#             
#            " make ATH_MAKE_INCLUDE_OVERRIDE=$HOME/mymake.inc "
#        
#             could be used, as long as "mymake.inc" defines all the required variables (see below)
#
#  Required Variables:  
#      
#      ATH_OS_SUB_TYPE - on linux, this must be "linux_2_4" for 2.4 kernels or left blank for 2.6 kernels.
#      ATH_LINUXPATH - linux kernel source path
#      ATH_CROSS_COMPILE_TYPE - optional cross compiler path , leave blank for local gcc compilation
#      ATH_ARCH_CPU_TYPE  - CPU architecture type, leave blank for local gcc compilation
#      ATH_SDIO_STACK_BASE - SDIO Stack installation path to compile the SDIO HIF layer against an externally supplied
#                            SDIO stack source.
#
#  Override variables:
#
#    ATH_MAKE_INCLUDE_OVERRIDE - full path to include file which overrides the default (localmake.linux.inc)
#                                this file can contain other overrides specific to a developers
#                                workspace environment.
#    ATH_BUILD_OUTPUT_OVERRIDE - output path override for compiled executable and
#                                database image
#

# Include local variables
ifdef ATH_MAKE_INCLUDE_OVERRIDE
_LOCALMAKE_INCLUDE = $(ATH_MAKE_INCLUDE_OVERRIDE)
else
_LOCALMAKE_INCLUDE = localmake.linux.inc
endif

-include $(_LOCALMAKE_INCLUDE)

export  ATH_SRC_BASE
export  ATH_BUILD_TYPE
export  ATH_OS_SUB_TYPE
export  ATH_LINUXPATH
export  ATH_CROSS_COMPILE_TYPE
export  ATH_ARCH_CPU_TYPE
export  ATH_SDIO_STACK_BASE
export  ATH_BUS_TYPE
export  ATH_BUS_SUBTYPE
export  ATH_HC_DRIVERS
export  ATH_HTC_RAW_INT_ENV
export  ATH_ANDROID_ENV
export  ATH_SOFTMAC_FILE_USED
export  ATH_CFG80211_ENV
export  ATH_DEBUG_DRIVER
export  ATH_SRC_BASE
export  ATH_BUILD_P2P

ATH_SRC_BASE :=$(shell pwd)
MAKE :=make
CC :=$(ATH_CROSS_COMPILE_TYPE)gcc
LD :=$(ATH_CROSS_COMPILE_TYPE)ld
STRIP :=$(ATH_CROSS_COMPILE_TYPE)strip
ATH_HIF_TYPE :=`echo $(ATH_BUS_TYPE) | tr [:upper:] [:lower:]`

export  STRIP

#export compiler variables for 3rd party applications (like the WPA supplicant)
export  CC
export  LD
export  STRIP

# Set cross compile type (if any)
ifdef ATH_CROSS_COMPILE_TYPE
_CROSS_COMPILE_LINE := ARCH=$(ATH_ARCH_CPU_TYPE) CROSS_COMPILE=$(ATH_CROSS_COMPILE_TYPE)
endif

# Figure out module extension
ifneq ($(ATH_OS_SUB_TYPE),linux_2_4)
KMOD_EXTENSION :=ko
endif
ifeq ($(ATH_OS_SUB_TYPE),linux_2_4)
KMOD_EXTENSION :=o
endif

# Set up object output areas
ifdef ATH_BUILD_OUTPUT_OVERRIDE
_COMPILED_OBJECTS_PATH :=$(ATH_BUILD_OUTPUT_OVERRIDE)
COMPILED_BIN_OBJECTS_PATH :=$(ATH_BUILD_OUTPUT_OVERRIDE)
COMPILED_IMAGE_OBJECTS_PATH :=$(ATH_BUILD_OUTPUT_OVERRIDE)
_MAKE_IMAGE_OUTPUT_DIR :=
_MAKE_BIN_OUTPUT_DIR :=
_CLEAN_IMAGE_OUTPUT_DIR :=
_CLEAN_BIN_OUTPUT_DIR :=
else
_COMPILED_OBJECTS_PATH := $(ATH_SRC_BASE)/.output/$(ATH_BUILD_TYPE)-$(ATH_BUS_TYPE)
COMPILED_BIN_OBJECTS_PATH := $(ATH_SRC_BASE)/.output/bin
COMPILED_IMAGE_OBJECTS_PATH :=$(_COMPILED_OBJECTS_PATH)/$(COMPILED_OBJECTS_PREFIX)image
_MAKE_IMAGE_OUTPUT_DIR := mkdir --parents $(COMPILED_IMAGE_OBJECTS_PATH)
_MAKE_BIN_OUTPUT_DIR := mkdir --parents $(COMPILED_BIN_OBJECTS_PATH)
_CLEAN_IMAGE_OUTPUT_DIR := rm -R -f $(COMPILED_IMAGE_OBJECTS_PATH)
_CLEAN_BIN_OUTPUT_DIR := rm -R -f $(COMPILED_BIN_OBJECTS_PATH)
endif

# Export the required variables to other Makefiles in the system
export COMPILED_IMAGE_OBJECTS_PATH
export COMPILED_BIN_OBJECTS_PATH

# Environment variable for cfg80211 support
ATH_CFG80211_ENV ?=no
ATH_BUILD_PAL_APP ?= yes

# control variables for conditionally compiling some tools
ATH_BUILD_SYSTEM_TOOLS ?=no
ATH_BUILD_BTFILTER ?=no
ATH_BUILD_3RDPARTY ?=yes
ATH_BUILD_P2P ?=yes

export ATH_BUILD_PAL_APP

#######################
# packaging for ART 
ARTROOTDIR := $(WORKAREA)/host/tools/systemtools
ARTSCRIPTDIR := $(ARTROOTDIR)/tools/scripts
# create needed directories for packaging
PKG_ROOT_DIR := $(WORKAREA)/host/tools/systemtools
PKG_PARENT_DIR := $(COMPILED_IMAGE_OBJECTS_PATH)/art_rel
PKG_DIR := $(PKG_PARENT_DIR)/art/bin
#######################

all: modules tools tests lib 3rdparty install

modules:
ifndef ATH_BUILD_TYPE 
	@echo "Please edit the $(_LOCALMAKE_INCLUDE) file"
	exit 1
endif
	$(_MAKE_IMAGE_OUTPUT_DIR)
	$(_MAKE_BIN_OUTPUT_DIR)
	$(MAKE) -C hif/$(ATH_HIF_TYPE)
	-rm -f $(ATH_SRC_BASE)/os/linux/Module.symvers
	-cp -f $(COMPILED_IMAGE_OBJECTS_PATH)/Module.symvers $(ATH_SRC_BASE)/os/linux
	$(MAKE)  $(_CROSS_COMPILE_LINE) -C $(ATH_LINUXPATH) ATH_HIF_TYPE=$(ATH_HIF_TYPE) SUBDIRS=$(ATH_SRC_BASE)/os/linux modules

tools: FORCE
	$(MAKE) -C tools/athprof/
	$(MAKE) -C tools/bmiloader/
	$(MAKE) -C tools/mkdsetimg/
	$(MAKE) -C tools/eeprom/
	if [ -e tools/forcereset/ ]; \
	then \
		$(MAKE) -C tools/forcereset/; \
	fi
	$(MAKE) -C tools/pal_host_intf/
	$(MAKE) -C tools/palEventRecv/
	$(MAKE) -C tools/regDbGen/
	if [ -e tools/ini2dset/ ]; \
	then \
		$(MAKE) -C tools/ini2dset/; \
	fi
	$(MAKE) -C tools/tcmd/
	$(MAKE) -C tools/systemtools/tools/eepromUtil/ -f makefile.Linux all
	$(MAKE) -C tools/wmiconfig/
	$(MAKE) -C tools/createimage/
	$(MAKE) -C tools/checkTargetStatus/
	$(MAKE) -C tools/recEvent/
	$(MAKE) -C tools/rompatcher/
	$(MAKE) -C tools/bdiff/
	if [ -e tools/licensing/ ]; \
	then \
		$(MAKE) -C tools/licensing/; \
	fi	
ifeq ($(ATH_BUILD_SYSTEM_TOOLS),yes) 		
	if [ -e tools/systemtools/ ]; \
	then \
		if [ -e tools/systemtools/tools/createini_ar6003/ ]; \
                then \
				$(MAKE) -C tools/systemtools/ -f makefile.linux all; \
                else \
				$(MAKE) -C tools/systemtools/ -f makefile.linux.customer all; \
		fi \
	fi
endif
	$(MAKE) -C tools/dbgParser/
ifeq ($(ATH_BUILD_BTFILTER),yes) 	
	$(MAKE) -C btfilter/ -f Makefile
	$(MAKE) -C tools/athbtfilter/bluez/
endif
	$(MAKE) -C tools/drvdebugctrl

lib: FORCE
	$(MAKE) -C lib/wac/
#	$(MAKE) -C tools/athbtfilter/bluez/

tests: FORCE
	$(MAKE) -C tests/floodtest/
	$(MAKE) -C tests/mboxping/
#	$(MAKE) -C tests/dsetpatch/

3rdparty: FORCE
ifeq ($(ATH_BUILD_3RDPARTY),yes)
	$(MAKE) -C 3rdparty/supplicant/opensrc_0_6_10/wpa_supplicant/
	$(MAKE) -C 3rdparty/supplicant/opensrc_0_8_x/wpa_supplicant/
	$(MAKE) -C 3rdparty/hostapd/hostapd-0.6.8/hostapd
	$(MAKE) -C 3rdparty/hostapd/hostapd-0.8.x/hostapd
#	if [ -e ../target/ram/security ]; \
#	then	\
#		$(MAKE) -C 3rdparty/supplicant/sec_lib/;	\
#		$(MAKE) -C tests/sec-test/;	\
#	fi
endif
ifeq ($(ATH_BUILD_P2P),yes) 	
	$(MAKE) -C 3rdparty/p2p/wpa_supplicant/
endif
install: FORCE
	cp -f $(ATH_SRC_BASE)/os/linux/ar6000.$(KMOD_EXTENSION) $(COMPILED_IMAGE_OBJECTS_PATH)
	if test -e $(ATH_SRC_BASE)/os/linux/bt_hci_sdio.$(KMOD_EXTENSION); \
	then	\
		cp -f $(ATH_SRC_BASE)/os/linux/bt_hci_sdio.$(KMOD_EXTENSION) $(COMPILED_IMAGE_OBJECTS_PATH);	\
	fi
	cp -f tools/athprof/athprof $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/bmiloader/bmiloader $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/mkdsetimg/mkdsetimg $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/eeprom/eeprom $(COMPILED_IMAGE_OBJECTS_PATH)
	if [ -e tools/forcereset/ ]; \
	then \
		cp -f tools/forcereset/forcereset $(COMPILED_IMAGE_OBJECTS_PATH); \
	fi
	cp -f tools/wmiconfig/wmiconfig $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/createimage/createimage $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/checkTargetStatus/checkTargetStatus $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/tcmd/athtestcmd $(COMPILED_IMAGE_OBJECTS_PATH)
#	cp -f tools/systemtools/tools/eepromUtil $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/recEvent/recEvent $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/pal_host_intf/pal_app $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/palEventRecv/palLogEvents $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/pal_host_intf/libpal_intf.a $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/recEvent/restore.sh $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/rompatcher/rompatcher $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/rompatcher/fwpatch $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/bdiff/bdiff $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tests/floodtest/floodtest $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tests/mboxping/mboxping $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f tools/regDbGen/regulatory*.bin $(COMPILED_BIN_OBJECTS_PATH)
#	cp -f tests/dsetpatch/regulatory*.diff $(COMPILED_BIN_OBJECTS_PATH)
	if [ -e tools/ini2dset/ ]; \
	then \
	    cp -rf tools/ini2dset/ini_ar600* $(COMPILED_BIN_OBJECTS_PATH); \
	fi
ifeq ($(ATH_BUILD_SYSTEM_TOOLS),yes) 	
#	if [ -e tools/systemtools/ ];   \
#	then \
#		if [ -e tools/systemtools/tools/createini ]; \
#		then \
#			if [ -e tools/systemtools/tools/createini/Linux ]; \
#			then \
#			cp -f tools/systemtools/art/Linux/art.out $(COMPILED_IMAGE_OBJECTS_PATH); \
#			cp -f tools/systemtools/devlib/Linux/libdevlib.so $(COMPILED_IMAGE_OBJECTS_PATH); \
#			cp -f tools/systemtools/mdk/Linux/mdk.out $(COMPILED_IMAGE_OBJECTS_PATH); \
#			cp -f tools/systemtools/mdk/Linux/dk.pm $(COMPILED_IMAGE_OBJECTS_PATH); \
#			fi \
#		else \
#			if [ -e tools/systemtools/art/Linux_cust ]; \
#			then \
#			cp -f tools/systemtools/art/Linux_cust/art.out $(COMPILED_IMAGE_OBJECTS_PATH); \
#			cp -f tools/systemtools/devlib/Linux_cust/libdevlib.so $(COMPILED_IMAGE_OBJECTS_PATH); \
#			fi \
#		fi \
#	fi	
	#add for ART packaging 

	mkdir -p $(PKG_PARENT_DIR)
	mkdir -p $(PKG_PARENT_DIR)/art
	mkdir -p $(PKG_PARENT_DIR)/art/bin
	mkdir -p $(PKG_PARENT_DIR)/config

	#end for ART packaging 

	if [ -e tools/systemtools/ ];   \
	then \
		if [ -e tools/systemtools/tools/createini_ar6003 ]; \
		then \
			if [ -e tools/systemtools/tools/createini_ar6003/Linux ]; \
			then \
				cp -f tools/systemtools/art_ar6003/Linux/art.out $(COMPILED_IMAGE_OBJECTS_PATH); \
				cp -f tools/systemtools/testcmd/Linux/tc.out $(COMPILED_IMAGE_OBJECTS_PATH); \
				cp -f tools/systemtools/devlib_ar6003/Linux/libdevlib.so $(COMPILED_IMAGE_OBJECTS_PATH); \
				cp -f $(ARTROOTDIR)/art_ar6003/Linux/art.out $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/devlib_ar6003/Linux/libdevlib.so $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/devlib_ar6003/addnl_inst/libnrp.so $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/devlib_ar6003/addnl_inst/librsnrpz.so $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/testcmd/Linux/tc.out $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/bringup/ar5k/test/art/*.* $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/art_ar6003/*.bin $(PKG_DIR); \
				rm -f $(PKG_DIR)/artsetup.txt; \
				mv $(PKG_DIR)/artsetup_ar6003.txt $(PKG_DIR)/artsetup.txt; \
				rm -f $(PKG_DIR)/calsetup.txt; \
				mv $(PKG_DIR)/calsetup_ar6003.txt $(PKG_DIR)/calsetup.txt; \
				cp -f $(ARTROOTDIR)/bringup/ar5k/test/art/calTrgtPwr_ar6* $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/bringup/ar5k/test/art/calTrgtPwr_ar500* $(PKG_DIR); \
				cp -f $(ARTSCRIPTDIR)/loadTgt.local.sh $(PKG_DIR)/loadTgt.sh; \
				cp -f $(ARTSCRIPTDIR)/unloadTgt.local.sh $(PKG_DIR)/unloadTgt.sh; \
				cp -f $(ARTSCRIPTDIR)/mercury_env_bash $(PKG_DIR); \
				cp -f $(ARTSCRIPTDIR)/mercury_env_csh $(PKG_DIR); \
				cp -f $(ARTSCRIPTDIR)/runART.local.sh $(PKG_DIR)/runART.sh; \
				cp -f $(ARTROOTDIR)/doc/ARTSupplementaryNotes.txt $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/art_ar6003/macid.txt $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/art_ar6003/btid.txt $(PKG_DIR); \
				cp -f $(ARTROOTDIR)/lib/libgpibenet.so $(PKG_DIR); \
				cp -fp $(ARTROOTDIR)/bringup/ar5k/config/*.eep $(PKG_PARENT_DIR)/config; \
			fi \
		else \
			if [ -e tools/systemtools/art_ar6003/Linux_cust ]; \
			then \
				cp -f tools/systemtools/art_ar6003/Linux_cust/art.out $(COMPILED_IMAGE_OBJECTS_PATH); \
				cp -f tools/systemtools/devlib_ar6003/Linux_cust/libdevlib.so $(COMPILED_IMAGE_OBJECTS_PATH); \
			fi \
		fi \
	fi	
endif		
	cp -f tools/dbgParser/dbgParser $(COMPILED_IMAGE_OBJECTS_PATH)

ifeq ($(ATH_BUILD_3RDPARTY),yes)
	cp -f 3rdparty/supplicant/opensrc_0_8_x/wpa_supplicant/wpa_supplicant  $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/supplicant/opensrc_0_8_x/wpa_supplicant/wpa_cli  $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/supplicant/opensrc_0_8_x/wpa_supplicant/wps.conf  $(COMPILED_IMAGE_OBJECTS_PATH)/wps_sta.conf		
	cp -f 3rdparty/supplicant/opensrc_0_8_x/wpa_supplicant/p2p.conf  $(COMPILED_IMAGE_OBJECTS_PATH)/p2p.conf
	$(STRIP) 3rdparty/hostapd/hostapd-0.6.8/hostapd/hostapd
	$(STRIP) 3rdparty/hostapd/hostapd-0.6.8/hostapd/hostapd_cli
	$(STRIP) 3rdparty/hostapd/hostapd-0.6.8/hostapd/hostapd_wps
	$(STRIP) 3rdparty/hostapd/hostapd-0.8.x/hostapd/hostapd
	$(STRIP) 3rdparty/hostapd/hostapd-0.8.x/hostapd/hostapd_cli
	$(STRIP) 3rdparty/hostapd/hostapd-0.8.x/hostapd/hostapd_wps
	cp -f 3rdparty/hostapd/hostapd-0.6.8/hostapd/hostapd $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/hostapd/hostapd-0.6.8/hostapd/hostapd_cli $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/hostapd/hostapd-0.6.8/hostapd/hostapd_wps $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/hostapd/hostapd-0.6.8/hostapd/config/ar6k* $(COMPILED_IMAGE_OBJECTS_PATH)
	if [ -e 3rdparty/wapid/ ]; \
	then 	\
		cp -f 3rdparty/wapid/bin/wapid $(COMPILED_IMAGE_OBJECTS_PATH);	\
		cp -f 3rdparty/wapid/wapi.conf $(COMPILED_IMAGE_OBJECTS_PATH);	\
	fi
endif
ifeq ($(ATH_BUILD_P2P),yes) 	
	cp -f  3rdparty/p2p/wpa_supplicant/p2p $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/p2p/wpa_supplicant/p2p_cli $(COMPILED_IMAGE_OBJECTS_PATH)
	cp -f 3rdparty/p2p/wpa_supplicant/p2p.conf $(COMPILED_IMAGE_OBJECTS_PATH)
endif
	if [ -e btfilter/libbtfilt.a ]; \
		then \
		cp -f btfilter/libbtfilt.a $(COMPILED_IMAGE_OBJECTS_PATH); \
	fi
	if [ -e tools/athbtfilter/bluez/abtfilt ]; \
		then \
		cp -f tools/athbtfilter/bluez/abtfilt $(COMPILED_IMAGE_OBJECTS_PATH); \
	fi
	cp -f tools/drvdebugctrl/drvdebugctrl $(COMPILED_IMAGE_OBJECTS_PATH); \

clean:
	rm -f os/linux/ar6000.mod.c os/linux/*.o os/linux/*.ko wmi/*.o htc/AR6000/src/*.o htc/*.o regtable/*.o\
	bmi/src/*.o wlan/src/*.o driver/* apps/wpa_supplicant/*.o tools/athprof/athprof \
	tools/bmiloader/bmiloader tools/dsetsvr/dsetsvr \
	tools/forcereset/forcereset tools/regDbGen/regDomainGenDb tools/regDbGen/*.bin \
	tools/rompatcher/rompatcher tools/rompatcher/fwpatch tests/floodtest/floodtest \
	tests/mboxping/mboxping tools/wmiconfig/wmiconfig tools/mkdsetimg/mkdsetimg \
	tools/eeprom/eeprom tools/eeprom/eeprom \
	btfilter/*.o btfilter/*.a 3rdparty/hostapd/hostapd-0.5.10/*.d \
	tools/athbtfilter/bluez/abtfilt \
	tools/bdiff/bdiff \
	tools/drvdebugctrl/drvdebugctrl \
	tools/palEventRecv/palLogEvents \
	tools/tcmd/athtestcmd \
	tools/licensing/*.[od]

	rm -f $(ATH_SRC_BASE)/os/linux/Module.symvers
	rm -f $(ATH_SRC_BASE)/os/linux/Module.markers
	rm -f $(ATH_SRC_BASE)/os/linux/modules.order
	rm -f $(ATH_SRC_BASE)/sdiostack/src/Module.markers
	rm -f $(ATH_SRC_BASE)/sdiostack/src/lib/modules.order
	rm -f $(ATH_SRC_BASE)/sdiostack/src/modules.order
	rm -f $(ATH_SRC_BASE)/sdiostack/src/busdriver/modules.order
	rm -f $(ATH_SRC_BASE)/sdiostack/src/hcd/pci_std/modules.order
	rm -f $(ATH_SRC_BASE)/sdiostack/src/hcd/modules.order
	rm -f $(ATH_SRC_BASE)/sdiostack/src/hcd/pci_ellen/modules.order
	rm -f $(COMPILED_IMAGE_OBJECTS_PATH)/*.ko

# The kernel module build process leaves some intermediate files, this will clean up all these files
	find $(ATH_SRC_BASE) \( -not -path "*.output*" -a -name '*.[oas]' -o -name core -o -name '.*.flags' -o -name '.ko' -o -name '.*.cmd' \) -type f -print \
		| grep -v lxdialog/ | xargs rm -f
	$(MAKE) -C hif/$(ATH_HIF_TYPE) clean
	if [ -e tools/ini2dset/ ]; \
	then \
		$(MAKE) -C tools/ini2dset/ clean; \
	fi
	$(MAKE) -C tests/dsetpatch/ clean
ifeq ($(ATH_BUILD_3RDPARTY),yes)
	$(MAKE) -C 3rdparty/hostapd/hostapd-0.6.8/hostapd/ clean
	$(MAKE) -C 3rdparty/hostapd/hostapd-0.8.x/hostapd/ clean
	$(MAKE) -C 3rdparty/supplicant/opensrc_0_6_10/wpa_supplicant/ clean
	$(MAKE) -C 3rdparty/supplicant/opensrc_0_8_x/wpa_supplicant/ clean
endif
ifeq ($(ATH_BUILD_P2P),yes) 	
	$(MAKE) -C 3rdparty/p2p/wpa_supplicant/ clean
endif
ifeq ($(ATH_BUILD_SYSTEM_TOOLS),yes) 	

	if [ -e tools/systemtools/ ];   \
	then \
		if [ -e tools/systemtools/tools/createini_ar6003 ]; \
		then \
				$(MAKE) -C tools/systemtools/ -f makefile.linux clean; \
		else \
				$(MAKE) -C tools/systemtools/ -f makefile.linux.customer clean; \
		fi \
	fi	
endif		
clobber:clean
	rm -rf .output

FORCE:
