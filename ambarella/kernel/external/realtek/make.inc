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
REALTEK_PATH	:= $(LOCAL_PATH)

ifeq ($(BUILD_REALTEK_WIFI_8723DS), y)
ifeq ($(BUILD_REALTEK_WIFI_8723DS_515_19247), y)
include $(REALTEK_PATH)/rtl8723ds_5112_18132/make.inc
endif
endif

ifeq ($(BUILD_REALTEK_WIFI_8812AU), y)
ifeq ($(BUILD_REALTEK_WIFI_8812AU_515_19247), y)
include $(REALTEK_PATH)/rtl8812au_515_19247/make.inc
endif
ifeq ($(BUILD_REALTEK_WIFI_8812AU_4320_16317p16683), y)
include $(REALTEK_PATH)/rtl8812au_4320_16317p16683/make.inc
endif
ifeq ($(BUILD_REALTEK_WIFI_8812AU_438_12175), y)
include $(REALTEK_PATH)/rtl8812au_438_12175/make.inc
endif
endif

ifeq ($(BUILD_REALTEK_WIFI_8821AU), y)
ifeq ($(BUILD_REALTEK_WIFI_8821AU_430_10674), y)
include $(REALTEK_PATH)/rtl8821au_430_10674/make.inc
endif
endif

ifeq ($(BUILD_REALTEK_WIFI_8192EU), y)
ifeq ($(BUILD_REALTEK_WIFI_8192EU_431_10675), y)
include $(REALTEK_PATH)/rtl8192eu_431_10675/make.inc
endif
endif

ifeq ($(BUILD_REALTEK_WIFI_8192CU), y)
ifeq ($(BUILD_REALTEK_WIFI_8192CU_402_9000), y)
include $(REALTEK_PATH)/rtl8192cu_402_9000/make.inc
endif
endif

ifeq ($(BUILD_REALTEK_WIFI_8188E), y)
ifeq ($(BUILD_REALTEK_WIFI_8188E_417_9024), y)
include $(REALTEK_PATH)/rtl8188e/make.inc
endif

ifeq ($(BUILD_REALTEK_WIFI_8188E_430_10600), y)
include $(REALTEK_PATH)/rtl8188eus_430_10600/make.inc
endif

ifeq ($(BUILD_REALTEK_WIFI_8188E_434_11294), y)
include $(REALTEK_PATH)/rtl8189es_434_11294/make.inc
endif

ifeq ($(BUILD_REALTEK_WIFI_8188E_4308_13968), y)
include $(REALTEK_PATH)/rtl8188eus_4308_13968/make.inc
endif
endif
