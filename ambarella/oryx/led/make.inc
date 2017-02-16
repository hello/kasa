##
## /S2L/oryx/led/make.inc
##
## History:
##   2015-2-6 - [longli] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_LED), y)
include $(call all-subdir-makefiles)
endif