##
## kernel/external/marvell/make.inc
##
## History:
##    2012/12/24 - [Tao Wu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

MRVL_DRV_PATH := $(call my-dir)

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8688), y)
ifeq ($(BUILD_MARVELL_WIFI_BT_SD8688_BT), y)
include $(MRVL_DRV_PATH)/sd8688/bt/make.inc
endif

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8688_WLAN), y)
include $(MRVL_DRV_PATH)/sd8688/wlan/make.inc
endif
endif ##BUILD_MARVELL_WIFI_BT_SD8688

#############################

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8787), y)
ifeq ($(BUILD_MARVELL_WIFI_BT_SD8787_WLAN), y)
include $(MRVL_DRV_PATH)/sd8787/wlan/make.inc
endif

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8787_BT), y)
include $(MRVL_DRV_PATH)/sd8787/bt/make.inc
endif
endif ##BUILD_MARVELL_WIFI_BT_SD8787

#############################

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8977), y)
ifeq ($(BUILD_MARVELL_WIFI_BT_SD8977_WLAN), y)
include $(MRVL_DRV_PATH)/sd8977/wlan_src/make.inc
endif

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8977_BT), y)
include $(MRVL_DRV_PATH)/sd8977/mbt_src/make.inc
endif

ifeq ($(BUILD_MARVELL_WIFI_BT_SD8977_BTC), y)
include $(MRVL_DRV_PATH)/sd8977/mbtc_src/make.inc
endif
endif ##BUILD_MARVELL_WIFI_BT_SD8977