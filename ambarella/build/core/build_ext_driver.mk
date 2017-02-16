## History:
##    2012/09/02 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

$(if $(filter %Kbuild, $(LOCAL_SRCS)),,$(error $(LOCAL_PATH): No Kbuild))

KBUILD_DIR	:= $(shell cd $(dir $(filter %Kbuild, $(LOCAL_SRCS))) && pwd)
LOCAL_TARGET	:= $(KBUILD_DIR:$(EXT_DRIVERS_SRCS_DIR)/%=%)

LOCAL_OBJS	:= $(patsubst $(EXT_DRIVERS_SRCS_DIR)/%, $(EXT_DRIVERS_OUT_DIR)/%,$(LOCAL_SRCS))

$(EXT_DRIVERS_OUT_DIR)/%: $(EXT_DRIVERS_SRCS_DIR)/%
	@mkdir -p $(dir $@)
	@cp -dpRf $< $@

prepare_external_drivers: $(LOCAL_OBJS)

EXT_DRIVER_MODULES += $(LOCAL_TARGET)/

