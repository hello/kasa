##
## ambarella/packages/security/lw_media/make.inc
##
## History:
##   2015-09-18  [Zhi He] created file
##
## Copyright (C) 2015-2025, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_LW_MEDIA), y)
include $(call all-subdir-makefiles)
endif
