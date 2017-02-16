##
## packages/img_mw/mw/arch_s2l/src/make.inc
##
## History:
##    2014/05/05 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
MW_PATH	:= $(LOCAL_PATH)
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params
LENS_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/lens_params

PREBUILD_3D_PATH := $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/idsp
BOARD_ADJ_PARAMS_PATH := $(AMB_TOPDIR)/boards/$(AMB_BOARD)/img_data/adj_params
BOARD_PREBUILD_3D_PATH := $(AMB_TOPDIR)/boards/$(AMB_BOARD)/img_data/idsp

###
include $(CLEAR_VARS)

LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/../include
#LOCAL_CFLAGS	+= -I$(ADJ_PARAMS_PATH)

AMP_MODULE = libamp.so
AMP_VER_PREFIX = AMP
AMP_VERSION_FILE = $(MW_PATH)/mw_version.c
AMP_SO_VER_MAJOR  := $(shell awk '/define\ $(AMP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMP_VERSION_FILE))
AMP_SO_VER_MINOR  := $(shell awk '/define\ $(AMP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMP_VERSION_FILE))
AMP_SO_VER_PATCH  := $(shell awk '/define\ $(AMP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMP_VERSION_FILE))
AMP_SO_VER_STRING := $(AMP_SO_VER_MAJOR).$(AMP_SO_VER_MINOR).$(AMP_SO_VER_PATCH)
AMP_SO_NAME   = $(patsubst %.so,%.so.$(AMP_SO_VER_MAJOR), $(AMP_MODULE))
AMP_SO_TARGET = $(patsubst %.so,%.so.$(AMP_SO_VER_STRING), $(AMP_MODULE))
LOCAL_SO_NAME := $(AMP_SO_NAME)

LOCAL_TARGET	:= $(AMP_MODULE)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_SRCS): prebuild_idsp_bin create_aaa_binary board_create_aaa_binary

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(AMP_SO_TARGET)
	@ln -sf $(AMP_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(AMP_SO_NAME)
	@ln -sf $(AMP_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(AMP_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

########build image data in ambarella/prebuild/imgproc/img_data/#########
include $(CLEAR_VARS)

MW_HEADER_PATH:=$(AMB_TOPDIR)/packages/img_mw/mw/arch_$(AMBARELLA_ARCH)/include
ADJ_PARAMS_CFLAGS:=	-I $(MW_HEADER_PATH) -I $(AMBARELLA_CFLAGS)

PREBUILD_PARAM_FOLDER:=$(AMB_BOARD_OUT)/prebuild/img_data
PREBUILD_LENS_PARAM_FOLDER:=$(AMB_BOARD_OUT)/prebuild/img_data/lens_params

#PARSE_AAA_PARAM	:= $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.c, $(wildcard ${ADJ_PARAMS_PATH}/*.c))
#S_AAA_PARAM	:= $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.s, $(wildcard ${ADJ_PARAMS_PATH}/*.c))
#OBJS_AAA_PARAM = $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.o, $(wildcard ${ADJ_PARAMS_PATH}/*.c))

BIN_AAA_3D	:=
BIN_AAA_SRC	:=

### Aptina Sensors
ifeq ($(CONFIG_SENSOR_AR0130), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ar0130*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ar0130*.c)
endif
ifeq ($(CONFIG_SENSOR_AR0141), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ar0141*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ar0141*.c)
endif
ifeq ($(CONFIG_SENSOR_AR0230), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ar0230*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ar0230*.c)
endif
ifeq ($(CONFIG_SENSOR_MT9T002), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/mt9t002*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/mt9t002*.c)
else
ifeq ($(CONFIG_SENSOR_MT9T002_MIPI), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/mt9t002*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/mt9t002*.c)
endif
endif

### Onsemi Sensors
ifeq ($(CONFIG_SENSOR_AR0237), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ar0237*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ar0237*.c)
endif

### OV Sensors
ifeq ($(CONFIG_SENSOR_OV9710), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ov9715*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ov9715*.c)
endif
ifeq ($(CONFIG_SENSOR_OV9718_MIPI), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ov9718*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ov9718*.c)
endif
ifeq ($(CONFIG_SENSOR_OV9750_MIPI), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ov9750*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ov9750*.c)
endif
ifeq ($(CONFIG_SENSOR_OV2710P), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ov2710*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ov2710*.c)
endif
ifeq ($(CONFIG_SENSOR_OV4689_MIPI), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ov4689*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ov4689*.c)
endif
ifeq ($(CONFIG_SENSOR_OV5658_MIPI), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/ov5658*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/ov5658*.c)
endif

### Sony Sensors
ifeq ($(CONFIG_SENSOR_IMX104), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx104*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx104*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX224), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx224*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx224*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX122), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx122*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx122*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX322), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx322*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx322*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX136P), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx136*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx136*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX290), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx290*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx290*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX291), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx291*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx291*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX123), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx123*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx123*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX124), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx124*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx124*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX178), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx178*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx178*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX172), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx172*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx172*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX174), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/imx174*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/imx174*.c)
endif

### Panasonic Sensors
ifeq ($(CONFIG_SENSOR_MN34041PL), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/mn34041pl*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/mn34041pl*.c)
endif
ifeq ($(CONFIG_SENSOR_MN34220PL), y)
BIN_AAA_3D	+= $(wildcard ${PREBUILD_3D_PATH}/mn34220pl*_3D.bin)
BIN_AAA_SRC	+= $(wildcard ${ADJ_PARAMS_PATH}/mn34220pl*.c)
endif

BIN_AAA_PARAM = $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.bin, ${BIN_AAA_SRC})

ifeq ($(CONFIG_TAMRON_M13VP288IR), y)
BIN_LENS_SRC	+= $(wildcard ${LENS_PARAMS_PATH}/m13vp288ir*.c)
endif
ifeq ($(CONFIG_EVETAR_MZ128BP2810ICR), y)
BIN_LENS_SRC	+= $(wildcard ${LENS_PARAMS_PATH}/mz128bp2810icr*.c)
endif

BIN_LENS_PARAM := $(patsubst ${LENS_PARAMS_PATH}/%.c, ${PREBUILD_LENS_PARAM_FOLDER}/%.bin, ${BIN_LENS_SRC})

.PHONY: create_aaa_binary

.PHONY: $(PREBUILD_PARAM_FOLDER)/%.o
.PHONY: $(PREBUILD_PARAM_FOLDER)/%.bin
.PHONY: $(PREBUILD_PARAM_FOLDER)/%.c
.PHONY: $(PREBUILD_PARAM_FOLDER)/%.s

LOCAL_GCC:=$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc
LOCAL_OBJCOPY:=$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)objcopy

${PREBUILD_PARAM_FOLDER}/%.s: ${ADJ_PARAMS_PATH}/%.c
	@mkdir -p ${PREBUILD_PARAM_FOLDER}
	@${LOCAL_GCC} ${ADJ_PARAMS_CFLAGS} -S $^ -o $@

${PREBUILD_PARAM_FOLDER}/%.c : ${PREBUILD_PARAM_FOLDER}/%.s ${ADJ_PARAMS_PATH}/%.c
	@sh ${MW_PATH}/build_aaa_binary.sh $@ $^

$(PREBUILD_PARAM_FOLDER)/%.o : ${PREBUILD_PARAM_FOLDER}/%.c
	@${LOCAL_GCC} ${ADJ_PARAMS_CFLAGS} -o $@ -c $^
	@rm $^

$(PREBUILD_PARAM_FOLDER)/%.bin : $(PREBUILD_PARAM_FOLDER)/%.o
	@mkdir -p ${PREBUILD_PARAM_FOLDER}
	@$(LOCAL_OBJCOPY) -O binary $^ $@
	@rm $^

.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.o
.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.bin
.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.c
.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.s

${PREBUILD_LENS_PARAM_FOLDER}/%.s: ${LENS_PARAMS_PATH}/%.c
	@mkdir -p ${PREBUILD_LENS_PARAM_FOLDER}
	@gcc ${ADJ_PARAMS_CFLAGS} -S $^ -o $@

${PREBUILD_LENS_PARAM_FOLDER}/%.c : ${PREBUILD_LENS_PARAM_FOLDER}/%.s ${LENS_PARAMS_PATH}/%.c
	@sh ${MW_PATH}/build_aaa_binary.sh $@ $^

$(PREBUILD_LENS_PARAM_FOLDER)/%.o : ${PREBUILD_LENS_PARAM_FOLDER}/%.c
	@gcc ${ADJ_PARAMS_CFLAGS} -o $@ -c $^
	@rm $^

$(PREBUILD_LENS_PARAM_FOLDER)/%.bin : $(PREBUILD_LENS_PARAM_FOLDER)/%.o
	@mkdir -p ${PREBUILD_LENS_PARAM_FOLDER}
	@objcopy -O binary $^ $@
	@rm $^

ifeq ($(BIN_AAA_PARAM),)
	@echo "placeholder for non-existing 3A image to avoid compile error"
else
ifeq ($(BIN_LENS_PARAM),)
create_aaa_binary: $(BIN_AAA_PARAM)
	@mkdir -p $(IMAGE_DATA_DIR)/adj_params/
	@cp -f $(BIN_AAA_PARAM) $(IMAGE_DATA_DIR)/adj_params/
	@mkdir -p $(IMAGE_DATA_DIR)/sensors/
	@rm  -f $(IMAGE_DATA_DIR)/sensors/*
	@cp -f ${BIN_AAA_3D} $(IMAGE_DATA_DIR)/sensors/
	@echo "Build $@ Done."
else
create_aaa_binary: $(BIN_AAA_PARAM) $(BIN_LENS_PARAM)
	@mkdir -p $(IMAGE_DATA_DIR)/adj_params/
	@cp -f $(BIN_AAA_PARAM) $(IMAGE_DATA_DIR)/adj_params/
	@mkdir -p $(IMAGE_DATA_DIR)/sensors/
	@rm  -f $(IMAGE_DATA_DIR)/sensors/*
	@cp -f ${BIN_AAA_3D} $(IMAGE_DATA_DIR)/sensors/
	@mkdir -p $(IMAGE_DATA_DIR)/lens_params/
	@rm  -f $(IMAGE_DATA_DIR)/lens_params/*
	@cp -f ${BIN_LENS_PARAM} $(IMAGE_DATA_DIR)/lens_params/
	@echo "Build $@ Done."
endif
endif
$(call add-target-into-build, create_aaa_binary)

########build image data in ambarella/boards/xxxboard/img_data#########
########if ambarella/boards/xxx_board/img_data exists,        #########
########it will replace ambarella/prebuild/imgproc/img_data   #########
BOARD_PARAM_FOLDER:=$(AMB_BOARD_OUT)/board_img_data
BOARD_BIN_AAA_3D	:=
BOARD_BIN_AAA_SRC	:=

### Aptina Sensors
ifeq ($(CONFIG_SENSOR_AR0130), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ar0130*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ar0130*.c)
endif
ifeq ($(CONFIG_SENSOR_AR0141), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ar0141*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ar0141*.c)
endif
ifeq ($(CONFIG_SENSOR_AR0230), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ar0230*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ar0230*.c)
endif
ifeq ($(CONFIG_SENSOR_MT9T002), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/mt9t002*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/mt9t002*.c)
else
ifeq ($(CONFIG_SENSOR_MT9T002_MIPI), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/mt9t002*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/mt9t002*.c)
endif
endif

### Onsemi Sensors
ifeq ($(CONFIG_SENSOR_AR0237), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ar0237*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ar0237*.c)
endif

### OV Sensors
ifeq ($(CONFIG_SENSOR_OV9718_MIPI), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ov9718*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ov9718*.c)
endif
ifeq ($(CONFIG_SENSOR_OV9750_MIPI), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ov9750*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ov9750*.c)
endif
ifeq ($(CONFIG_SENSOR_OV2710P), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ov2710*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ov2710*.c)
endif
ifeq ($(CONFIG_SENSOR_OV4689_MIPI), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ov4689*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ov4689*.c)
endif
ifeq ($(CONFIG_SENSOR_OV5658_MIPI), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/ov5658*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/ov5658*.c)
endif

### Sony Sensors
ifeq ($(CONFIG_SENSOR_IMX104), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx104*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx104*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX224), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx224*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx224*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX122), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx122*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx122*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX322), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx322*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx322*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX136P), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx136*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx136*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX290), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx290*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx290*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX291), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx291*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx291*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX123), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx123*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx123*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX124), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx124*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx124*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX178), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx178*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx178*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX172), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx172*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx172*.c)
endif
ifeq ($(CONFIG_SENSOR_IMX174), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/imx174*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/imx174*.c)
endif

### Panasonic Sensors
ifeq ($(CONFIG_SENSOR_MN34041PL), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/mn34041pl*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/mn34041pl*.c)
endif
ifeq ($(CONFIG_SENSOR_MN34220PL), y)
BOARD_BIN_AAA_3D	+= $(wildcard ${BOARD_PREBUILD_3D_PATH}/mn34220pl*_3D.bin)
BOARD_BIN_AAA_SRC	+= $(wildcard ${BOARD_ADJ_PARAMS_PATH}/mn34220pl*.c)
endif

BOARD_BIN_AAA_PARAM = $(patsubst ${BOARD_ADJ_PARAMS_PATH}/%.c, ${BOARD_PARAM_FOLDER}/%.bin, ${BOARD_BIN_AAA_SRC})

.PHONY: board_create_aaa_binary

.PHONY: $(BOARD_PARAM_FOLDER)/%.o
.PHONY: $(BOARD_PARAM_FOLDER)/%.bin
.PHONY: $(BOARD_PARAM_FOLDER)/%.c
.PHONY: $(BOARD_PARAM_FOLDER)/%.s

LOCAL_GCC:=$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc
LOCAL_OBJCOPY:=$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)objcopy

${BOARD_PARAM_FOLDER}/%.s: ${BOARD_ADJ_PARAMS_PATH}/%.c
	@mkdir -p ${BOARD_PARAM_FOLDER}
	@${LOCAL_GCC} ${ADJ_PARAMS_CFLAGS} -S $^ -o $@

${BOARD_PARAM_FOLDER}/%.c : ${BOARD_PARAM_FOLDER}/%.s ${BOARD_ADJ_PARAMS_PATH}/%.c
	@sh ${MW_PATH}/build_aaa_binary.sh $@ $^

$(BOARD_PARAM_FOLDER)/%.o : ${BOARD_PARAM_FOLDER}/%.c
	@${LOCAL_GCC} ${ADJ_PARAMS_CFLAGS} -o $@ -c $^
	@rm $^

$(BOARD_PARAM_FOLDER)/%.bin : $(BOARD_PARAM_FOLDER)/%.o
	@mkdir -p ${BOARD_PARAM_FOLDER}
	@$(LOCAL_OBJCOPY) -O binary $^ $@
	@rm $^

ifeq ($(BOARD_BIN_AAA_PARAM),)

else
board_create_aaa_binary: $(BOARD_BIN_AAA_PARAM) create_aaa_binary
	@mkdir -p $(IMAGE_DATA_DIR)/adj_params/
	@cp -f $(BOARD_BIN_AAA_PARAM) $(IMAGE_DATA_DIR)/adj_params/
	@mkdir -p $(IMAGE_DATA_DIR)/sensors/
	@rm  -f $(IMAGE_DATA_DIR)/sensors/*
	@cp -f ${BOARD_BIN_AAA_3D} $(IMAGE_DATA_DIR)/sensors/
	@echo "Build $@ Done."
endif

$(call add-target-into-build, board_create_aaa_binary)
