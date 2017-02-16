##
## kernel/external/realtek/rtl8192cu/make.inc
##
## History:
##    2012/09/02 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

ifeq ($(BUILD_REALTEK_WIFI_8188E_USB), y)
$(shell cp -dpf $(LOCAL_PATH)/Kbuild.usb $(LOCAL_PATH)/Kbuild )
$(shell cp -dpf $(LOCAL_PATH)/autoconf_rtl8188e_usb_linux.h $(LOCAL_PATH)/include/autoconf.h )
else
$(shell cp -dpf $(LOCAL_PATH)/Kbuild.sdio $(LOCAL_PATH)/Kbuild )
$(shell cp -dpf $(LOCAL_PATH)/autoconf_rtl8189e_sdio_linux.h $(LOCAL_PATH)/include/autoconf.h )
endif

LOCAL_SRCS	:= $(shell find $(LOCAL_PATH) -name "*.[ch]")
LOCAL_SRCS	+= $(LOCAL_PATH)/Kbuild


include $(BUILD_EXT_DRIVER)

