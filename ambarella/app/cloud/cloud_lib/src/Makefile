###############################################################################
## $(MODULE_NAME_TAG)/cloud_lib/src/Makefile
##
## History:
##    2013/04/15 - [Zhi He] Create File
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

default: all

include ../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS = protocols user_protocols

cloud_lib_interface_src := $(wildcard *.cpp)
cloud_lib_interface_obj := $(cloud_lib_interface_src:%.cpp=%.o)

cloud_lib_protocols_sacp_src = $(wildcard protocols/sacp/*.cpp)
cloud_lib_protocols_sacp_obj = $(cloud_lib_protocols_sacp_src:protocols/sacp/%.cpp=protocols/sacp/%.o)

cloud_lib_user_protocols_default_src = $(wildcard user_protocols/default/*.cpp)
cloud_lib_user_protocols_default_obj = $(cloud_lib_user_protocols_default_src:user_protocols/default/%.cpp=user_protocols/default/%.o)

MODULE_SRC := 	 $(cloud_lib_interface_src)	\
			 $(cloud_lib_protocols_sacp_src) \
			 $(cloud_lib_user_protocols_default_src)

MODULE_OBJ := 	 $(cloud_lib_interface_obj)	\
			 $(cloud_lib_protocols_sacp_obj) \
			 $(cloud_lib_user_protocols_default_obj)

STATIC_LIB_NAMES = mwcg_cloud_lib

mwcg_cloud_lib_src     := $(MODULE_SRC)
mwcg_cloud_lib_obj     := $(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



