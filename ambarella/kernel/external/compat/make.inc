##
## kernel/private/drivers/atheros/make.inc
##
## History:
##    2012/06/07 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
LOCAL_PATH	:= $(call my-dir)
COMPAT_PATH	:=$(LOCAL_PATH)

ifeq ($(BUILD_COMPAT_WIRELESS), y)

ifeq ($(ATH6KL_SDIO), y)
export CONFIG_ATH6KL_SDIO=m
endif

ifeq ($(ATH6KL_USB), y)
export CONFIG_ATH6KL_USB=m
endif

ifeq ($(BUILD_QCA6234_USE_AMBARELLA_SOFTMAC), y)
export CONFIG_QCA6234_USE_AMBARELLA_SOFTMAC=y
endif

ifeq ($(BUILD_COMPAT_WIRELESS_QCA6234_350374), y)
include $(COMPAT_PATH)/qca6234_350374/make.inc
include $(COMPAT_PATH)/qca6234_350374/external/libtcmd/make.inc
include $(COMPAT_PATH)/qca6234_350374/external/tcmd/make.inc
endif

ifeq ($(BUILD_COMPAT_WIRELESS_QCA6234_350415), y)
include $(COMPAT_PATH)/qca6234_350415/make.inc
include $(COMPAT_PATH)/qca6234_350415/external/libtcmd/make.inc
include $(COMPAT_PATH)/qca6234_350415/external/tcmd/make.inc
endif
endif ## BUILD_COMPAT_WIRELESS
