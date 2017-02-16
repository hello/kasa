##
## Common build system definitions.  Mostly standard
## commands for building various types of targets, which
## are used by others to construct the final targets.
##
## History:
##    2011/12/09 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ALL_TARGETS :=

###########################################################
## Retrieve the directory of the current makefile
###########################################################

# Figure out where we are.
define my-dir
$(strip \
	$(eval md_file_ := $$(lastword $$(MAKEFILE_LIST))) \
	$(patsubst %/,%,$(dir $(md_file_))) \
	$(eval MAKEFILE_LIST := $$(lastword $$(MAKEFILE_LIST))) \
 )
endef


###########################################################
## Retrieve a list of all makefiles immediately below some directory
###########################################################

define all-makefiles-under
$(wildcard $(1)/*/make.inc)
endef


###########################################################
## Retrieve a list of all makefiles immediately below your directory
###########################################################

define all-subdir-makefiles
$(call all-makefiles-under,$(call my-dir))
endef


###########################################################
## Add target into ALL_$(CLASS)
###########################################################

define add-target-into-build
$(eval ALL_TARGETS += $(strip $(1)))
endef

#define add-target-into-build
#	$(eval $(1) += $(strip $(2))) \
#	$(eval ALL_TARGETS += $(strip $(1)).$(strip $(2)).$(strip $(call my-dir)))
#endef



###########################################################
## Commands for post-processing the dependency files
###########################################################
define transform-d-to-p
@cp $(@:%.o=%.d) $(@:%.o=%.P); \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(@:%.o=%.d) >> $(@:%.o=%.P); \
	rm -f $(@:%.o=%.d)
endef


###########################################################
## Build static library, if there are libraries in prerequisites, then merge them
###########################################################

define build-static-library
	$(if $(wildcard $@),@rm -rf $@)
	$(if $(wildcard $(dir $@)/ar.mac),@rm -rf $(dir $@)/ar.mac)
	$(if $(filter %.a, $^),
		@echo CREATE $@ > $(dir $@)/ar.mac
		@echo SAVE >> $(dir $@)/ar.mac
		@echo END >> $(dir $@)/ar.mac
		@ar -M < $(dir $@)/ar.mac
	)

	$(if $(filter %.o,$^),@ar -c -q $@ $(filter %.o, $^))
	$(if $(filter %.a, $^),
		@echo OPEN $@ > $(dir $@)/ar.mac
		$(foreach LIB, $(filter %.a, $^),
			@echo ADDLIB $(LIB) >> $(dir $@)/ar.mac
		)
		@echo SAVE >> $(dir $@)/ar.mac
		@echo END >> $(dir $@)/ar.mac
		@ar -M < $(dir $@)/ar.mac
		@rm -rf $(dir $@)/ar.mac
	)
endef

define strip-binary
	$(if $(findstring y, $(BUILD_AMBARELLA_APP_DEBUG)),echo -n, \
	$(CROSS_COMPILE)strip -s $@)
endef

define strip-library
	$(if $(findstring y, $(BUILD_AMBARELLA_APP_DEBUG)),echo -n, \
	$(CROSS_COMPILE)strip --strip-unneeded $@)
endef

###############################
# Usefull functions ported from Kbuild.include
try-run = $(shell set -e; ($(1)) >/dev/null 2>&1 && echo "$(2)" || echo "$(3)")
cc-option = $(call try-run, $(TOOLCHAIN_PATH)/$(CROSS_COMPILE)gcc $(1) -c -xc /dev/null -o /dev/null,$(1),$(2))
cc-version = $(shell $(AMB_TOPDIR)/build/bin/gcc-version.sh $(TOOLCHAIN_PATH)/$(CROSS_COMPILE)gcc)
cc-ifversion = $(shell [ $(call cc-version) $(1) $(2) ] && echo $(3))

