##
## build_targets.mk
##
## History:
##    2012/06/03 - [Cao Rongrong] Create
##
## Copyright (C) 2015 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
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
