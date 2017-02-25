## History:
##    2011/12/09 - [Cao Rongrong] Create
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

AMB_BOARD_DIR		:= $(shell pwd)
DOT_CONFIG		:= $(AMB_BOARD_DIR)/.config
export DOT_CONFIG

ifeq ($(wildcard $(DOT_CONFIG)),$(DOT_CONFIG))
include $(DOT_CONFIG)
endif

-include $(AMB_BOARD_DIR)/.config.cmd

AMBA_MAKEFILE_V		:= @
AMBA_MAKE_PARA		:= -s
ALL_TOPDIR		:= $(shell cd $(AMB_TOPDIR)/.. && pwd)
AMB_TOPDIR		:= $(shell cd $(AMB_TOPDIR) && pwd)
export AMB_TOPDIR

######################## TOOLCHAIN #####################################

ifndef ARM_ELF_TOOLCHAIN_DIR
$(error Can not find arm-elf toolchain, please source ENV, eg. CodeSourcery.env)
endif
ifndef ARM_LINUX_TOOLCHAIN_DIR
$(error Can not find arm-linux toolchain, please source ENV, eg. CodeSourcery.env)
endif
ifndef TOOLCHAIN_PATH
$(error Can not find arm-linux path, please source ENV, eg. CodeSourcery.env)
endif

export PATH:=$(TOOLCHAIN_PATH):$(PATH)


########################## BUILD COMMAND ###############################

CLEAR_VARS		:= $(AMB_TOPDIR)/build/core/clear_vars.mk
BUILD_APP		:= $(AMB_TOPDIR)/build/core/build_app.mk
BUILD_DRIVER		:= $(AMB_TOPDIR)/build/core/build_driver.mk
BUILD_EXT_DRIVER	:= $(AMB_TOPDIR)/build/core/build_ext_driver.mk
BUILD_PREBUILD		:= $(AMB_TOPDIR)/build/core/build_prebuild.mk


########################## BUILD IMAGE TOOLS ###########################

CPU_BIT_WIDTH		:= $(shell uname -m)

ifeq ($(CPU_BIT_WIDTH), x86_64)
MAKEDEVS		:= $(AMB_TOPDIR)/rootfs/bin/makedevs-64
MKUBIFS			:= $(AMB_TOPDIR)/rootfs/bin/mkfs.ubifs-64
UBINIZE			:= $(AMB_TOPDIR)/rootfs/bin/ubinize-64
MKSQUASHFS		:= $(AMB_TOPDIR)/rootfs/bin/mksquashfs-64
else
MAKEDEVS		:= $(AMB_TOPDIR)/rootfs/bin/makedevs-32
MKUBIFS			:= $(AMB_TOPDIR)/rootfs/bin/mkfs.ubifs-32
UBINIZE			:= $(AMB_TOPDIR)/rootfs/bin/ubinize-32
MKSQUASHFS		:= $(AMB_TOPDIR)/rootfs/bin/mksquashfs
endif
MKYAFFS2		:= $(AMB_TOPDIR)/rootfs/bin/mkfs.yaffs2
MKJFFS2 		:= $(AMB_TOPDIR)/rootfs/bin/mkfs.jffs2



########################## BOARD #####################################

ifeq ($(DOT_CONFIG), $(wildcard $(DOT_CONFIG)))
AMBARELLA_ARCH_HI	:= $(shell grep ^CONFIG_ARCH $(DOT_CONFIG) | \
				sed -e s/^CONFIG_ARCH_// | \
				sed -e s/=y//)
$(if $(filter $(AMBARELLA_ARCH_HI), $(notdir $(ALL_TOPDIR))), \
		$(error Don't use $(AMBARELLA_ARCH_HI) as project directory))

AMBARELLA_ARCH		:= $(shell echo $(AMBARELLA_ARCH_HI) | tr [:upper:] [:lower:])
ifeq ($(AMBARELLA_ARCH), s2e)
AMBARELLA_ARCH		:= s2
endif
export AMBARELLA_ARCH

AMB_BOARD		:= $(shell grep ^CONFIG_BSP_BOARD $(DOT_CONFIG) | \
				sed -e s/^CONFIG_BSP_BOARD_// | \
				sed -e s/=y// | \
				tr [:upper:] [:lower:])

BOARD_VERSION_STR	:= $(shell grep ^CONFIG_BOARD_VERSION $(DOT_CONFIG) | \
				sed -e s/^CONFIG_BOARD_VERSION_// | \
				sed -e s/=y//)
BOARD_INITD_DIR		:= $(AMB_TOPDIR)/boards/$(AMB_BOARD)/init.d
BOARD_ROOTFS_DIR	:= $(AMB_TOPDIR)/boards/$(AMB_BOARD)/rootfs/default
BOARD_ROOTFS_FASTBOOT_DIR	:= $(AMB_TOPDIR)/boards/$(AMB_BOARD)/rootfs/fastboot

else
AMBARELLA_ARCH		:= unknown
AMB_BOARD		:= unknown

endif


########################## OUT DIRECTORY ################################

AMB_BOARD_OUT		:= $(AMB_TOPDIR)/out/$(AMB_BOARD)
# export for building images
export AMB_BOARD_OUT

KERNEL_OUT_DIR		:= $(AMB_BOARD_OUT)/kernel
ROOTFS_OUT_DIR		:= $(AMB_BOARD_OUT)/rootfs
FAKEROOT_DIR		:= $(AMB_BOARD_OUT)/fakeroot
IMAGES_OUT_DIR		:= $(AMB_BOARD_OUT)/images


########################## GLOBAL FLAGS ################################

AMBARELLA_CFLAGS	:= -I$(AMB_BOARD_DIR) -I$(AMB_TOPDIR)/include \
                     -I$(AMB_TOPDIR)/include/arch_$(AMBARELLA_ARCH) \
                     -Wformat -Werror=format-security
AMBARELLA_AFLAGS	:=
AMBARELLA_LDFLAGS	:=
AMBARELLA_CPU_ARCH	:=

ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
AMBARELLA_CPU_ARCH	:= armv7-a-hf
CPU_ARCH		:= arm
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
AMBARELLA_CPU_ARCH	:= armv7-a
CPU_ARCH		:= arm
else ifeq ($(CONFIG_CPU_ARM1136JS), y)
AMBARELLA_CPU_ARCH	:= armv6k
CPU_ARCH		:= arm
else ifeq ($(CONFIG_CPU_ARM926EJS), y)
AMBARELLA_CPU_ARCH	:= armv5te
CPU_ARCH		:= arm
endif

########################## APP FLAGS ###################################

PREBUILD_3RD_PARTY_DIR	:= $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)
PREBUILD_3RD_PARTY_NOARCH := $(AMB_TOPDIR)/prebuild/third-party/noarch
UNIT_TEST_PATH		:= $(shell echo $(CONFIG_UNIT_TEST_INSTALL_DIR))
UNIT_TEST_IMGPROC_PATH	:= $(shell echo $(CONFIG_UNIT_TEST_IMGPROC_PARAM_DIR))
IMGPROC_INSTALL_PATH	:= $(shell echo $(CONFIG_IMGPROC_INSTALL_DIR))
APP_INSTALL_PATH	:= $(shell echo $(CONFIG_APP_INSTALL_DIR))
APP_IPCAM_CONFIG_PATH	:= $(shell echo $(CONFIG_APP_IPCAM_CONFIG_DIR))

###################### CAMERA MIDDLE-WARE FLAGS ########################

CAMERA_DIR         := $(AMB_TOPDIR)/camera
CAMERA_BIN_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_BIN_DIR))
CAMERA_LIB_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_LIB_DIR))
CAMERA_CONF_DIR    := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_CONF_DIR))
CAMERA_DEMUXER_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_DEMUXER_DIR))
CAMERA_EVENT_PLUGIN_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_EVENT_PLUGIN_DIR))

######################## ORYX MIDDLE-WARE FLAGS ########################

ORYX_DIR         := $(AMB_TOPDIR)/oryx
ORYX_BIN_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_BIN_DIR))
ORYX_LIB_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_LIB_DIR))
ORYX_CONF_DIR    := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_CONF_DIR))
ORYX_FILTER_DIR  := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_FILTER_DIR))
ORYX_CODEC_DIR   := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_CODEC_DIR))
ORYX_MUXER_DIR   := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_MUXER_DIR))
ORYX_DEMUXER_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_DEMUXER_DIR))
ORYX_EVENT_PLUGIN_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_EVENT_PLUGIN_DIR))
ORYX_VIDEO_PLUGIN_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_ORYX_VIDEO_PLUGIN_DIR))

###################### VENDOR FLAGS ########################

VENDORS_BIN_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_VENDORS_BIN_DIR))
VENDORS_LIB_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_VENDORS_LIB_DIR))

#
###################### VENDOR FLAGS ########################
IMAGE_DATA_DIR		:= $(FAKEROOT_DIR)/etc/idsp

#
AMBARELLA_APP_AFLAGS	:= $(AMBARELLA_AFLAGS)

#
AMBARELLA_APP_LDFLAGS	:= $(AMBARELLA_LDFLAGS)

#
ifeq ($(BUILD_AMBARELLA_APP_DEBUG), y)
AMBARELLA_APP_CFLAGS	:= $(AMBARELLA_CFLAGS) -g -O0 -Wall -fPIC -D_REENTRENT -D_GNU_SOURCE
ifeq ($(BUIDL_AMBARELLA_APP_DEBUG_SANITIZE_ADDRESS), y)
AMBARELLA_APP_CFLAGS	+= $(call cc-option,-fsanitize=address,)
AMBARELLA_APP_CFLAGS	+= $(call cc-option,-fno-omit-frame-pointer,)
AMBARELLA_APP_LDFLAGS	+= $(call cc-option,-fsanitize=address,)
endif
else
AMBARELLA_APP_CFLAGS	:= $(AMBARELLA_CFLAGS) -O3 -Wall -fPIC -D_REENTRENT -D_GNU_SOURCE
endif

AMBARELLA_APP_CFLAGS	+= $(call cc-option,-mno-unaligned-access,)
AMBARELLA_APP_CFLAGS	+= $(call cc-option,-fdiagnostics-color=auto,)

#Disable lto for all gcc 4.x, no prove to be good.
#Impact 3A & algo, let's test in gcc 5.x.
#AMBARELLA_APP_CFLAGS	+= $(call cc-ifversion, -ge, 0500, -flto,)
#AMBARELLA_APP_LDFLAGS	+= $(call cc-ifversion, -ge, 0500, -flto,)

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_STATIC), y)
AMBARELLA_APP_LDFLAGS	+= -static
endif

ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
AMBARELLA_APP_CFLAGS	+= -mthumb -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=hard -mfpu=neon -Wa,-mimplicit-it=thumb
AMBARELLA_APP_AFLAGS	+= -mthumb -march=armv7-a -mfloat-abi=hard -mfpu=neon -mimplicit-it=thumb
AMBARELLA_APP_LDFLAGS	+= -mthumb -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=hard -mfpu=neon -Wa,-mimplicit-it=thumb
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
AMBARELLA_APP_CFLAGS	+= -marm -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=softfp -mfpu=neon
AMBARELLA_APP_AFLAGS	+= -march=armv7-a -mfloat-abi=softfp -mfpu=neon
AMBARELLA_APP_LDFLAGS	+= -marm -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=softfp -mfpu=neon
else ifeq ($(CONFIG_CPU_ARM1136JS), y)
AMBARELLA_APP_CFLAGS	+= -marm -march=armv6k -mtune=arm1136j-s -mlittle-endian -msoft-float -Wa,-mfpu=softvfp
AMBARELLA_APP_AFLAGS	+= -march=armv6k -mfloat-abi=soft -mfpu=softvfp
AMBARELLA_APP_LDFLAGS	+= -marm -march=armv6k -mtune=arm1136j-s -mlittle-endian -msoft-float -Wa,-mfpu=softvfp
else ifeq ($(CONFIG_CPU_ARM926EJS), y)
AMBARELLA_APP_CFLAGS	+= -marm -march=armv5te -mtune=arm9tdmi -msoft-float -Wa,-mfpu=softvfp
AMBARELLA_APP_AFLAGS	+= -march=armv5te -mfloat-abi=soft -mfpu=softvfp
AMBARELLA_APP_LDFLAGS	+= -marm -march=armv5te -mtune=arm9tdmi -msoft-float -Wa,-mfpu=softvfp
endif

########################## DRIVER FLAGS ###################################

KERNEL_DEFCONFIG	:= $(shell echo $(CONFIG_KERNEL_DEFCONFIG_STRING))
KERNEL_INSTALL_PATH	:= $(shell echo $(CONFIG_KERNEL_MODULES_INSTALL_DIR))
FIRMWARE_INSTALL_PATH	:= $(KERNEL_INSTALL_PATH)/lib/firmware
LINUX_INSTALL_FLAG	:= INSTALL_MOD_PATH=$(KERNEL_INSTALL_PATH)

LINUX_VERSION		:= $(shell echo $(CONFIG_LINUX_KERNEL_VERSION))
LINUX_SRC_DIR		:= $(AMB_TOPDIR)/kernel/linux-$(LINUX_VERSION)
ifneq ($(wildcard $(LINUX_SRC_DIR)),$(LINUX_SRC_DIR))
LINUX_SRC_DIR		:= $(AMB_TOPDIR)/kernel/linux
endif
LINUX_OUT_DIR		:= $(KERNEL_OUT_DIR)/linux-$(LINUX_VERSION)_$(strip \
				$(shell echo $(KERNEL_DEFCONFIG) | \
				sed -e s/ambarella_// -e s/_defconfig// -e s/$(AMB_BOARD)_// -e s/_kernel_config//))

AMB_DRIVERS_BIN_DIR	:= $(AMB_TOPDIR)/prebuild/ambarella/drivers
AMB_DRIVERS_SRCS_DIR	:= $(AMB_TOPDIR)/kernel/private/drivers
AMB_DRIVERS_OUT_DIR	:= $(KERNEL_OUT_DIR)/private/drivers
EXT_DRIVERS_SRCS_DIR	:= $(AMB_TOPDIR)/kernel/external
EXT_DRIVERS_OUT_DIR	:= $(KERNEL_OUT_DIR)/external

AMBARELLA_DRV_CFLAGS	:= $(AMBARELLA_CFLAGS) \
				-I$(AMB_TOPDIR)/kernel/private/include \
				-I$(AMB_TOPDIR)/kernel/private/include/arch_$(AMBARELLA_ARCH) \
				-I$(AMB_TOPDIR)/kernel/private/lib/firmware_$(AMBARELLA_ARCH)

AMBARELLA_DRV_AFLAGS		:= $(AMBARELLA_AFLAGS)
AMBARELLA_DRV_LDFLAGS		:= $(AMBARELLA_LDFLAGS)

### export for private driver building
export AMBARELLA_DRV_CFLAGS
export AMBARELLA_DRV_AFLAGS
export AMBARELLA_DRV_LDFLAGS

