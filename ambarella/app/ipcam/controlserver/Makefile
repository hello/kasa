#
# Makefile
#
# History:
#    2011/03/13 - [Yupeng Chang] created file
#
# Copyright (C) 2008, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

default: all

AMBABUILD_TOPDIR = $(word 1, $(subst /app/ipcam, ,$(shell pwd)))
DAEMON_PATH = $(AMBABUILD_TOPDIR)/rootfs/fakeroot/sbin
SCRIPT_PATH = $(AMBABUILD_TOPDIR)/rootfs/fakeroot/etc/init.d
CONFIG_PATH = $(AMBABUILD_TOPDIR)/rootfs/fakeroot/etc/ambaipcam/controlserver
MODULE_CONFIG = module.conf
INIT_SCRIPT = S99IPCamControlServer
MODULE_NAME	= IPCamControlServer
MODULE_SRCS = $(wildcard *.cpp)
MODULE_HDRS = $(wildcard *.h)
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

export AMBABUILD_TOPDIR

-include $(AMBABUILD_TOPDIR)/build/app/common.mk

LINK_FLAGS = -L$(AMBABUILD_TOPDIR)/prebuild/third-party/libdaemon/lib -ldaemon \
						 -lpthread
MODULE_INC = -I$(AMBABUILD_TOPDIR) -I$(AMBABUILD_TOPDIR)/build/include \
						 -I$(AMBABUILD_TOPDIR)/prebuild/third-party/libdaemon/include
CFLAGS = -march=armv6 -mtune=arm1136j-s $(MODULE_INC) $(APP_FLAG) -Wall -O3

%.o:	%.cpp
	@echo "      compiling $<..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ $(CFLAGS) -c $< -o $@

all: $(MODULE_OBJS)
	@echo "    compile $(MODULE_NAME)..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)g++ -o $(MODULE_NAME) $(MODULE_OBJS) \
																						$(LINK_FLAGS)
	@$(AMBA_MAKEFILE_V)mkdir -p $(DAEMON_PATH)
	@$(AMBA_MAKEFILE_V)cp $(MODULE_NAME) $(DAEMON_PATH)
	@$(AMBA_MAKEFILE_V)cp $(INIT_SCRIPT) $(SCRIPT_PATH)
	@$(AMBA_MAKEFILE_V)chmod 755 $(DAEMON_PATH)/$(MODULE_NAME)
	@$(AMBA_MAKEFILE_V)chmod 755 $(SCRIPT_PATH)/$(INIT_SCRIPT)
	@echo "AutoLoad:True" > $(MODULE_CONFIG)
	@echo "iav"     >> $(MODULE_CONFIG)
	@echo "ambtve"  >> $(MODULE_CONFIG)
	@echo "ambdbus" >> $(MODULE_CONFIG)
	@echo "ambhdmi" >> $(MODULE_CONFIG)
	@echo "ambad"   >> $(MODULE_CONFIG)
	@echo "pwm_bl"  >> $(MODULE_CONFIG)
	@$(AMBA_MAKEFILE_V)mkdir -p $(CONFIG_PATH)
	@$(AMBA_MAKEFILE_V)mv $(MODULE_CONFIG) $(CONFIG_PATH)
	@$(AMBA_MAKEFILE_V)chmod 644 $(CONFIG_PATH)/$(MODULE_CONFIG)

clean:
	$(AMBA_MAKEFILE_V)rm -f $(MODULE_NAME) $(MODULE_OBJS)

.PHONY: all clean

