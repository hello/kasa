##
## prebuild/imgproc/img_data/make.inc
##
## History:
##    2012/11/06 - [Zhaoyang Chen] Created file
##    2012/12/10 - [Jingyang Qiu] modified file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


###
ifeq ($(CONFIG_AMBARELLA_PREBUILD), y)
ifeq ($(CONFIG_AMBARELLA_IMAGE_DATA), y)

include $(call my-dir)/arch_$(AMBARELLA_ARCH)/make.inc

endif
endif

