##
## kernel/private/drivers/vin/make.inc
##
## History:
##    2012/06/04 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_VIN), y)

LOCAL_PATH	:= $(call my-dir)

###

include $(call all-makefiles-under, $(LOCAL_PATH)/*)
include $(call all-makefiles-under, $(LOCAL_PATH)/)

endif

