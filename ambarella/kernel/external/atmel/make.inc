##
## kernel/external/atmel/make.inc
##
## History:
##    2015/06/02 - [Zhi He] Created file
##
## Copyright (C) 2015-2025, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
LOCAL_PATH	:= $(call my-dir)
ATMEL_PATH	:= $(LOCAL_PATH)

#############################

ifeq ($(BUILD_ATMEL_ATSHA204_GPL_DRIVER), y)
include $(ATMEL_PATH)/atsha204/make.inc
endif

#############################
