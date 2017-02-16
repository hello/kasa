##
## ambarella/oryx/analytics/make.inc
##
## History:
##    2014/12/17 - [Long Li] Created file
##
## Copyright (C) 2008-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_ANALYTICS), y)
include $(call all-subdir-makefiles)
endif
