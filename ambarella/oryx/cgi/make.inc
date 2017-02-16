##
## oryx/cgi/make.inc
##
## History:
##    2015/08/27 - [Huaiqing Wang] Created file
##
## Copyright (C) 2012-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_ORYX_CGI), y)
include $(call all-subdir-makefiles)
endif
