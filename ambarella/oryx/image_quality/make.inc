##
## ambarella/oryx/image_quality/make.inc
##
## History:
##    2014/12/31 - [Bin Wang] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_IMAGE_QUALITY), y)
include $(call all-subdir-makefiles)
endif
