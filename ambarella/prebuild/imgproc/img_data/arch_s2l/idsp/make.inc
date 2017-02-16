##
## prebuild/imgproc/img_data/arch_s2l/idsp/make.inc
##
## History:
##    2012/11/06 - [Zhaoyang Chen] Created file
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

.PHONY: prebuild_idsp_bin

prebuild_idsp_bin: PRIVATE_PATH:=$(LOCAL_PATH)
prebuild_idsp_bin:
	@mkdir -p $(IMAGE_DATA_DIR)/
	@cp -dpRf $(PRIVATE_PATH)/*.bin $(IMAGE_DATA_DIR)/
	@mkdir -p $(IMAGE_DATA_DIR)/sensors/
	@mv -f $(IMAGE_DATA_DIR)/*_3D*.bin $(IMAGE_DATA_DIR)/sensors/
$(call add-target-into-build, prebuild_idsp_bin)

