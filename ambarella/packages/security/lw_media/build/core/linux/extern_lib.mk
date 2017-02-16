##
## $(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk
##
## History:
##    2013/03/16 - [Zhi He] Create
##
## Copyright (C) 2014 - 2024, the Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of the Ambarella, Inc.
##

THIRD_PARTY_DIR := $(DEVICE_DIR)/prebuild/third-party/armv7-a-hf
EXTERN_LIB_LIBAAC_DIR = $(THIRD_PARTY_DIR)/aac
EXTERN_LIB_ALSA_DIR = $(THIRD_PARTY_DIR)/alsa-lib
EXTERN_LIB_FFMPEG_DIR = $(THIRD_PARTY_DIR)/ffmpeg-2.7.2

EXTERN_LIB_LIBAAC_INC = -I$(EXTERN_LIB_LIBAAC_DIR)/include/
EXTERN_LIB_ALSA_INC = -I$(EXTERN_LIB_ALSA_DIR)/include/
EXTERN_LIB_FFMPEG_INC = -I$(EXTERN_LIB_FFMPEG_DIR)/include/

EXTERN_LIB_LIBAAC_LIB = -L$(EXTERN_LIB_LIBAAC_DIR)/lib/
EXTERN_LIB_ALSA_LIB = -L$(EXTERN_LIB_ALSA_DIR)/usr/lib/
EXTERN_LIB_FFMPEG_LIB = -L$(EXTERN_LIB_FFMPEG_DIR)/lib/

