##
## unit_test/linux/make.inc
##
## History:
##    2012/05/31 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_PUBLIC_LINUX_UNIT_TESTS),y)
include $(call all-subdir-makefiles)
endif

