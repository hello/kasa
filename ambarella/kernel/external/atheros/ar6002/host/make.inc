##
## kernel/external/atheros/ar6000/host/make.inc
##
## History:
##    2012/06/07 - [Cao Rongrong] Created file
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

LOCAL_SRCS	:= $(shell find $(LOCAL_PATH) -name "*.[ch]")
LOCAL_SRCS	+= $(LOCAL_PATH)/Kbuild

include $(BUILD_EXT_DRIVER)

