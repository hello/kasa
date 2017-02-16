##
## build_targets.mk
##
## History:
##    2012/06/03 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

$(if $(LOCAL_TARGET),,$(error $(LOCAL_PATH): LOCAL_TARGET is not defined))

$(LOCAL_TARGET).PATH := $(LOCAL_PATH)

# Note: for prebuild target, $(LOCAL_SRCS) are just directories rather than files

__FILES__ := $(foreach d, $(LOCAL_SRCS), $(shell find $(LOCAL_PATH)/$(d) -type f))

LOCAL_MODULE := $(patsubst $(LOCAL_PATH)/%,$(FAKEROOT_DIR)/%,$(__FILES__))

# copy files
$(FAKEROOT_DIR)/%: $(LOCAL_PATH)/%
	$(if $(wildcard $@),@rm -rf $@)
	@mkdir -p $(dir $@)
	$(if $(findstring .gitignore,$<),,@cp -dpRf $< $@)

# links can't be taken as targets, so we have to create links manually.
__LINKS__ := $(foreach d, $(LOCAL_SRCS), $(shell find $(LOCAL_PATH)/$(d) -type l))
# conver to destination links
__LINKS__ := $(patsubst $(LOCAL_PATH)/%, $(FAKEROOT_DIR)/%,$(__LINKS__))
# if the destination links existed, filter-out it to avoid create again
__LINKS__ := $(filter-out $(wildcard $(__LINKS__)), $(__LINKS__))

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): PRIVATE_LINKS:=$(__LINKS__)
$(LOCAL_TARGET): PRIVATE_FILES:=$(__FILES__)

define overwrite-bb
$(foreach f,$(PRIVATE_FILES),
	$(if $(strip $(f)),
		$(if $(findstring .gitignore,$(f)),,
			@mkdir -p $(dir $(patsubst $(PRIVATE_PATH)/%,$(FAKEROOT_DIR)/%,$(f)));
			@rm -rf $(patsubst $(PRIVATE_PATH)/%,$(FAKEROOT_DIR)/%,$(f));
			@cp -dpRf $(f) $(patsubst $(PRIVATE_PATH)/%,$(FAKEROOT_DIR)/%,$(f));
		)
	)
)
endef

define prebuild-links
$(foreach l,$(PRIVATE_LINKS),
	$(if $(strip $(l)),
		@mkdir -p $(dir $(l));
		@cp -dpRf $(subst $(FAKEROOT_DIR)/,$(PRIVATE_PATH)/,$(l)) $(l);
	)
)
endef
