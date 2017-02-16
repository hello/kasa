## History:
##    2012/06/09 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

$(ALL_TARGETS): $(DOT_CONFIG)

# Add clean target
.PHONY: clean_fakeroot clean_ut clean_linux clean_private_drivers
.PHONY: clean_dtc clean_amboot

clean_fakeroot:
	@rm -rf $(FAKEROOT_DIR)
	@rm -rf $(ROOTFS_OUT_DIR)

clean_ut:
	@rm -rf $(AMB_BOARD_OUT)/unit_test

clean_app:
	@rm -rf $(AMB_BOARD_OUT)/app

clean_mw:
	@rm -rf $(AMB_BOARD_OUT)/mw

clean_linux:
	@rm -rf $(LINUX_OUT_DIR)

clean_private_drivers:
	@rm -rf $(AMB_BOARD_OUT)/kernel/private

clean_external_drivers:
	@rm -rf $(AMB_BOARD_OUT)/kernel/external

clean_dtc:
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(AMB_TOPDIR)/build/dtc clean

clean_amboot: clean_dtc
	@rm -rf $(AMB_BOARD_OUT)/amboot

