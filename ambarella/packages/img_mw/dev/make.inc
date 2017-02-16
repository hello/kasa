##
## packages/img_mw/dev/make.inc
##
## History:
##    2013/05/13 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2013-2017, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_IMAGE_DEVICE_PROC), y)

include $(call my-dir)/arch_$(AMBARELLA_ARCH)/make.inc

endif
