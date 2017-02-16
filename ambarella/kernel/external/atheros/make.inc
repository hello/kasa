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
ATHEROS_PATH	:=$(LOCAL_PATH)

#############################

ifeq ($(BUILD_ATHEROS_WIFI_AR6002), y)
include $(ATHEROS_PATH)/ar6002/host/make.inc
endif

#############################

ifeq ($(BUILD_ATHEROS_WIFI_AR6003), y)

ifeq ($(BUILD_ATHEROS_WIFI_AR6003_V796), y)
include $(ATHEROS_PATH)/ar6003_796/host/make.inc
endif

ifeq ($(BUILD_ATHEROS_WIFI_AR6003_V844), y)
include $(ATHEROS_PATH)/ar6003_844/host/make.inc
endif

ifeq ($(BUILD_ATHEROS_WIFI_BT_AR6233_V445), y)
include $(ATHEROS_PATH)/ar6003_445/host/make.inc
endif

endif ## BUILD_ATHEROS_WIFI_AR6003

#############################

ifeq ($(BUILD_ATHEROS_WIFI_AR9374), y)

ifeq ($(BUILD_ATHEROS_WIFI_AR9374_350213), y)
include $(ATHEROS_PATH)/ar9374_350213/make.inc
endif

ifeq ($(BUILD_ATHEROS_WIFI_AR9374_35028), y)
include $(ATHEROS_PATH)/ar9374_35028/make.inc
endif

endif ## BUILD_ATHEROS_WIFI_AR9374

#############################

ifeq ($(BUILD_ATHEROS_WIFI_AR9375), y)

ifeq ($(BUILD_ATHEROS_WIFI_AR9375_500110), y)
include $(ATHEROS_PATH)/ar9375_500110/make.inc
endif

ifeq ($(BUILD_ATHEROS_WIFI_AR9375_500109), y)
include $(ATHEROS_PATH)/qca6234_500109/make.inc
endif

endif ## BUILD_ATHEROS_WIFI_AR9375

#############################

ifeq ($(BUILD_ATHEROS_WIFI_AR1021), y)

ifeq ($(BUILD_ATHEROS_WIFI_AR1021_50080), y)
include $(ATHEROS_PATH)/ar1021_50080/make.inc
endif

endif ## BUILD_ATHEROS_WIFI_AR1021
#############################