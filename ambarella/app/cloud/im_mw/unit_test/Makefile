###############################################################################
## $(MODULE_NAME_TAG)/im_mw/unit_test/Makefile
##
## History:
##    2014/06/19 - [Zhi He] Create File
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
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

.PHONY: all clean

MODULE_INC = -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/common/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/cloud_lib/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/lightweight_database/include -I$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/im_mw/include
MODULE_LIB = -lrt -lpthread -L$(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/lib -lmwcg_im_mw -lmwcg_lwdb_lib -lmwcg_cloud_lib -lmwcg_security_lib
ifeq ($(BUILD_CONFIG_WITH_LICENCE), y)
MODULE_LIB += -lmwcg_licence
endif
MODULE_LIB += -lmwcg_common
MODULE_LIB += -lpthread -ldl -lrt
IM_TEST_SRC = im_test.cpp
IM_TEST = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/imtest

IM_CTEST_SRC = im_client_test.cpp
IM_CTEST = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/imctest

##IM_CLIENT_TEST_SRC = im_client_test2.cpp
##IM_CLIENT_TEST = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/imclienttest

##ACCOUNT_TEST_SRC = accounttest.cpp
##ACCOUNT_TEST = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin/accounttest

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
	$(MAKE_PRINT)echo "build imtest:"
	$(MAKE_PRINT)mkdir -p $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/out/linux/bin
	$(MAKE_PRINT)$(CXX) -o $(IM_TEST) $(MODULE_INC) -g -O0 $(IM_TEST_SRC) $(MODULE_LIB)
	
	$(MAKE_PRINT)echo "build imctest:"
	$(MAKE_PRINT)$(CXX) -o $(IM_CTEST) $(MODULE_INC) -g -O0 $(IM_CTEST_SRC) $(MODULE_LIB)

##	$(MAKE_PRINT)echo "build imclienttest:"
##	$(MAKE_PRINT)$(CXX) -o $(IM_CLIENT_TEST) $(MODULE_INC) -g -O0 $(IM_CLIENT_TEST_SRC) $(MODULE_LIB)

##	$(MAKE_PRINT)echo "build accounttest:"
##	$(MAKE_PRINT)$(CXX) -o $(ACCOUNT_TEST) $(MODULE_INC) -g -O0 $(ACCOUNT_TEST_SRC) $(MODULE_LIB)

	

clean:
	$(MAKE_PRINT)echo "clean imtest:"
	$(MAKE_PRINT)$(RM) $(IM_TEST)

	$(MAKE_PRINT)echo "clean imctest:"
	$(MAKE_PRINT)$(RM) $(IM_CTEST)

##	$(MAKE_PRINT)echo "clean imclienttest:"
##	$(MAKE_PRINT)$(RM) $(IM_CLIENT_TEST)

##	$(MAKE_PRINT)echo "clean accounttest:"
##	$(MAKE_PRINT)$(RM) $(ACCOUNT_TEST)




