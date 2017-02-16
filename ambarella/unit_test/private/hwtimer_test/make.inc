##
## unit_test/private/hwtimer_test/make.inc
##
## History:
##    2014/07/10 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_HW_TIMER), y)
LOCAL_UT_ARCH_DIR=$(call my-dir)/arch_$(AMBARELLA_ARCH)/make.inc
ifeq ($(wildcard $(LOCAL_UT_ARCH_DIR)), $(LOCAL_UT_ARCH_DIR))
include $(LOCAL_UT_ARCH_DIR)
endif
endif

