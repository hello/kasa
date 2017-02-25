###############################################################################
## $(MODULE_NAME_TAG)/licence/Makefile
##
## History:
##    2014/03/10 - [Zhi He] Create File
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

include ../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

SUBDIRS = encryption misc client
MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/licence/include 

encryption_src = $(wildcard encryption/*.cpp)
encryption_obj = $(encryption_src:encryption/%.cpp=encryption/%.o) 

encryption_customized_v1_src = $(wildcard encryption/customized_v1/*.cpp)
encryption_customized_v1_obj = $(encryption_customized_v1_src:encryption/customized_v1/%.cpp=encryption/customized_v1/%.o) 

client_src = $(wildcard client/*.cpp)
client_obj = $(client_src:client/%.cpp=client/%.o) 

client_standalone_src := $(wildcard client/standalone/*.cpp)
client_standalone_obj := $(client_standalone_src:client/standalone/%.cpp=client/standalone/%.o) 

misc_src = $(wildcard misc/*.cpp)
misc_obj = $(misc_src:misc/%.cpp=misc/%.o) 

MODULE_SRC = $(encryption_src)   \
             $(encryption_customized_v1_src)     \
             $(client_src) \
			 $(client_standalone_src) \
			 $(misc_src)

MODULE_OBJ = $(encryption_obj)   \
             $(encryption_customized_v1_obj)     \
			 $(client_obj) \
             $(client_standalone_obj) \
			 $(misc_obj)

STATIC_LIB_NAMES = mwcg_licence
#SHARED_LIB_NAMES = mwcg_licence

mwcg_licence_src     = $(MODULE_SRC)
mwcg_licence_obj     = $(MODULE_OBJ)

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk



