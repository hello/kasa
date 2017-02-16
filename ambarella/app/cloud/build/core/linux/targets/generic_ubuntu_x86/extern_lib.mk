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

EXTERN_LIB_FFMPEG_DIR = $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/extern_lib/ffmpeg_v0.7
EXTERN_LIB_LIBXML2_DIR = $(THIRD_PARTY_DIR)/libxml2/
EXTERN_LIB_LIBAAC_DIR = $(THIRD_PARTY_DIR)/misc/
EXTERN_LIB_ALSA_DIR = $(THIRD_PARTY_DIR)/alsa-lib/
EXTERN_BUILD_DIR = $(BUILDSYSTEM_DIR)/build/

EXTERN_LIB_FFMPEG_INC = -I$(EXTERN_LIB_FFMPEG_DIR)/libavutil -I$(EXTERN_LIB_FFMPEG_DIR) \
	-I$(EXTERN_LIB_FFMPEG_DIR)/libavcodec -I$(EXTERN_LIB_FFMPEG_DIR)/libavformat 
EXTERN_LIB_LIBXML2_INC = -I$(EXTERN_LIB_LIBXML2_DIR)/include/
EXTERN_LIB_LIBAAC_INC = -I$(EXTERN_LIB_LIBAAC_DIR)/include/
EXTERN_LIB_ALSA_INC = -I$(EXTERN_LIB_ALSA_DIR)/include/
EXTERN_BUILD_INC = -I$(EXTERN_BUILD_DIR)/include/
	
EXTERN_LIB_FFMPEG_LIB = -L$(EXTERN_LIB_FFMPEG_DIR)/$(MWCG_FFMPEG_LIBDIR)/
EXTERN_LIB_LIBXML2_LIB = -L$(EXTERN_LIB_LIBXML2_DIR)/lib/
EXTERN_LIB_LIBAAC_LIB = -L$(EXTERN_LIB_LIBAAC_DIR)/lib/
EXTERN_LIB_ALSA_LIB = -L$(EXTERN_LIB_ALSA_DIR)/usr/lib/

