##
## unit_test/private/audio_test/make.inc
##
## History:
##    2015/03/03 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2015-2019, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_AUDIO), y)

include $(call all-subdir-makefiles)

endif

