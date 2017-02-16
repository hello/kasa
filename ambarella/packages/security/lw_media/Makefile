##
## $(MODULE_NAME_TAG)/Makefile
##
## History:
##    2015/07/28 - [Zhi He] Create File
##
## Copyright (C) 2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

include build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk

.PHONY: all media_library media_ut
.PHONY: clean clean_media_library clean_media_ut cleandir

media_library:
	$(MAKE_PRINT)echo "Build lw_media.a..."
	$(MAKE_PRINT)$(MAKE) -C source default --no-print-directory

media_ut: media_library
	$(MAKE_PRINT)echo "Build lw_media_ut..."
	$(MAKE_PRINT)$(MAKE) -C unit_test default --no-print-directory

all: media_library media_ut


clean_media_library:
	$(MAKE_PRINT)$(MAKE) -C source clean --no-print-directory

clean_media_ut:
	$(MAKE_PRINT)$(MAKE) -C unit_test clean --no-print-directory

clean: clean_media_library clean_media_ut cleandir

cleandir:
	-@rm -rf out


