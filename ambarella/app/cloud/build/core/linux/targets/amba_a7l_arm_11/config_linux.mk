###############################################################################
## $(MODULE_NAME_TAG)/build/core/linux/config_linux.mk
##
## History:
##    2013/03/16 - [Zhi He] Create
##
## Copyright (c) 2016 Ambarella, Inc.
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
##################################################################################

MAKE_PRINT		=	@

##Cross compile
ENABLE_CROSS_COMPILE := y
CROSS_COMPILE_TARGET_ARCH := ARM
CROSS_COMPILE_HOST_ARCH := X86

CROSS_COMPILE_TOOL_CHAIN_PATH := /usr/local/arm-2011.09/bin
CROSS_COMPILE	?=	arm-none-linux-gnueabi-

##CPU arch and option
BUILD_CONFIG_CPU_ARCH_X86 := n
BUILD_CONFIG_CPU_ARCH_X64 := n
BUILD_CONFIG_CPU_OPT_MMX := n
BUILD_CONFIG_CPU_OPT_SSE := n
BUILD_CONFIG_CPU_ARCH_X64 := n
BUILD_CONFIG_CPU_ARCH_ARMV5 := n
BUILD_CONFIG_CPU_ARCH_ARMV6 := y
BUILD_CONFIG_CPU_ARCH_ARMV7 := n
BUILD_CONFIG_CPU_OPT_NEON := n

CONFIG_CPU_CORTEXA9_HF := n
CONFIG_CPU_CORTEXA9 := n
CONFIG_CPU_ARM1136JS := y
CONFIG_CPU_ARM926EJS := n

##OS typs
BUILD_CONGIG_OS_LINUX := y
BUILD_CONGIG_OS_ANDROID := n
BUILD_CONGIG_OS_WINDOWS := n
BUILD_CONGIG_OS_IOS := n

##HW related
BUILD_CONGIG_DSP_AMBA_A5S := n
BUILD_CONGIG_DSP_AMBA_A7L := y
BUILD_CONGIG_DSP_AMBA_I1 := n
BUILD_CONGIG_DSP_AMBA_S2 := n

##SW module related
BUILD_CONFIG_MODULE_FFMPEG := n
BUILD_CONFIG_MODULE_LIBXML2 := n
BUILD_CONFIG_MODULE_ALSA := n
BUILD_CONFIG_MODULE_PULSEAUDIO := n
BUILD_CONFIG_MODULE_LIBAAC := n
BUILD_CONFIG_MODULE_AMBA_DSP := n

BUILDSYSTEM_DIR	?=	$(word 1, $(subst /$(MODULE_NAME_TAG), ,$(PWD)))
MWCG_TOPDIR		=	$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)
MWCG_BINDIR		=	$(BUILDSYSTEM_DIR)/out/xman/fakeroot/usr/local/bin
MWCG_LIBDIR		=	$(BUILDSYSTEM_DIR)/out/xman/fakeroot/usr/local/lib
MWCG_CONFDIR	=	$(BUILDSYSTEM_DIR)/out/xman/fakeroot/etc/mw_cg
MWCG_OBJDIR		=	$(MWCG_TOPDIR)/out/linux/objs
MWCG_INTERNAL_BINARY	=	$(MWCG_TOPDIR)/out/linux/binary
MWCG_INTERNAL_LIBDIR	=	$(MWCG_TOPDIR)/out/linux/lib
MWCG_INTERNAL_TARDIR	=	$(MWCG_TOPDIR)/out/linux/tar
MWCG_INTERNAL_INCDIR	=	$(MWCG_TOPDIR)/out/linux/include

MWCG_CFLAGS		=	-I$(MWCG_TOPDIR)/common/include -I$(MWCG_TOPDIR)/security_utils/include -I$(MWCG_TOPDIR)/cloud_lib/include -I$(MWCG_TOPDIR)/storage_lib/include -I$(MWCG_TOPDIR)/media_mw/include -I$(MWCG_TOPDIR)/im_mw/include -I$(MWCG_TOPDIR)/lightweight_database/include -DNEW_AAC_CODEC_VERSION=1
MWCG_LDFLAGS	=

ifeq ($(CONFIG_LINARO_TOOLCHAIN), y)
ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
MWCG_CFLAGS	+= -mthumb -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=hard -mfpu=neon -Wa,-mimplicit-it=thumb
MWCG_LDFLAGS	+= -mthumb -march=armv7-a -mfloat-abi=hard -mfpu=neon -mimplicit-it=thumb
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
MWCG_CFLAGS	+= -marm -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=softfp -mfpu=neon
MWCG_LDFLAGS	+= -march=armv7-a -mfloat-abi=softfp -mfpu=neon
else ifeq ($(CONFIG_CPU_ARM1136JS), y)
MWCG_CFLAGS	+= -marm -march=armv6k -mtune=arm1136j-s -mlittle-endian -msoft-float -Wa,-mfpu=softvfp
MWCG_LDFLAGS	+= -march=armv6k -mfloat-abi=soft -mfpu=softvfp
else ifeq ($(CONFIG_CPU_ARM926EJS), y)
MWCG_CFLAGS	+= -marm -march=armv5te -mtune=arm9tdmi -msoft-float -Wa,-mfpu=softvfp
MWCG_LDFLAGS	+= -march=armv5te -mfloat-abi=soft -mfpu=softvfp
endif
else
ifeq ($(BUILD_CONFIG_CPU_ARCH_ARMV6), y)
MWCG_CFLAGS += -march=armv6k -mtune=arm1136j-s -msoft-float -mlittle-endian
else
ifeq ($(BUILD_CONFIG_CPU_ARCH_ARMV7), y)
MWCG_CFLAGS += -march=armv7-a -mlittle-endian
ifeq ($(BUILD_CONFIG_CPU_OPT_NEON), y)
MWCG_CFLAGS += -mfloat-abi=softfp -mfpu=neon
else
MWCG_CFLAGS += -msoft-float
endif
else
MWCG_CFLAGS += -march=armv5te -mtune=arm9tdmi
endif
endif
endif

MWCG_CFLAGS += $(call cc-option,-mno-unaligned-access,)

ifeq ($(BUILD_AMBARELLA_MWCG_DEBUG), y)
MWCG_CFLAGS += -g -O2
else
MWCG_CFLAGS += -O3
endif
MWCG_CFLAGS += -Wall -Werror -fPIC -D_REENTRENT

ifeq ($(BUILD_CONGIG_OS_LINUX), y)
MWCG_CFLAGS += -DBUILD_OS_LINUX=1
else
ifeq ($(BUILD_CONGIG_OS_WINDOWS), y)
MWCG_CFLAGS += -DBUILD_OS_WINDOWS=1
else
ifeq ($(BUILD_CONGIG_OS_ANDROID), y)
MWCG_CFLAGS += -DBUILD_OS_ANDROID=1
else
ifeq ($(BUILD_CONGIG_OS_IOS), y)
MWCG_CFLAGS += -DBUILD_OS_IOS=1
endif
endif
endif
endif

ifeq ($(BUILD_CONGIG_DSP_AMBA_I1), y)
MWCG_CFLAGS += -DBUILD_DSP_AMBA_I1=1
endif

ifeq ($(BUILD_CONGIG_DSP_AMBA_S2), y)
MWCG_CFLAGS += -DBUILD_DSP_AMBA_S2=1
endif

ifeq ($(BUILD_CONFIG_MODULE_FFMPEG), y)
MWCG_CFLAGS += -DBUILD_MODULE_FFMPEG=1
endif

ifeq ($(BUILD_CONFIG_MODULE_LIBXML2), y)
MWCG_CFLAGS += -DBUILD_MODULE_LIBXML2=1
endif

ifeq ($(BUILD_CONFIG_MODULE_ALSA), y)
MWCG_CFLAGS += -DBUILD_MODULE_ALSA=1
ifeq ($(BUILD_CONFIG_MODULE_PULSEAUDIO), y)
MWCG_CFLAGS += -DBUILD_MODULE_PULSEAUDIO=1
endif
endif

ifeq ($(BUILD_CONFIG_MODULE_LIBAAC), y)
MWCG_CFLAGS += -DBUILD_MODULE_LIBAAC=1
endif

ifeq ($(BUILD_CONFIG_MODULE_AMBA_DSP), y)
MWCG_CFLAGS += -DBUILD_MODULE_AMBA_DSP=1
endif

ifeq ($(BUILD_WITH_UNDER_DEVELOP_COMPONENT), y)
MWCG_CFLAGS += -DBUILD_WITH_UNDER_DEVELOP_COMPONENT=1
endif

######################################################################
#		tool chain config
######################################################################
ifeq ($(ENABLE_CROSS_COMPILE), y)
ifeq ($(CROSS_COMPILE_TARGET_ARCH), ARM)
CC    = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)gcc
CXX    = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)g++
LD     = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)ld
AS     = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)as
AR     = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)ar
STRIP  = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)strip
RANLIB = $(CROSS_COMPILE_TOOL_CHAIN_PATH)/$(CROSS_COMPILE)ranlib
endif
else
CC    = gcc
CXX    = g++
LD     = ld
AS     = as
AR     = ar
STRIP  = strip
RANLIB = ranlib
endif

export BUILDSYSTEM_DIR
export MWCG_TOPDIR
export MWCG_BINDIR
export MWCG_LIBDIR
export MWCG_CONFDIR
export MWCG_OBJDIR
export MWCG_INTERNAL_BINARY
export MWCG_INTERNAL_LIBDIR
export MWCG_INTERNAL_INCDIR
export MWCG_CFLAGS
export MWCG_LDFLAGS

