#
# rules_linux.mk
#
# History:
#    2013/04/16 - [Zhi He] Create file
#
# Copyright (C) 2013, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

######################################################################
#   Module defined variables
######################################################################
# If module has sub directories
SUBDIRS          ?=
# unit test dir
UNIT_TEST_DIR          ?= 
# If module generates shared libraries
SHARED_LIB_NAMES ?=
# If module generates static libraries
STATIC_LIB_NAMES ?=
# If module generates executables
EXECUTABLE_FILES ?=
# Module's source files
MODULE_SRC       ?=
# Module's object files
MODULE_OBJ       ?=
# Module's specific includes
MODULE_INC       ?=
# Module's specific defines
MODULE_DEF       ?=
# Module's specific ld flags
MODULE_LDFLAG    ?=

MKDIR  = mkdir -p
RM     = rm -rf
CP     = cp -a
MV     = mv
LN     = ln

CXXFLAGS = -fno-rtti -fno-exceptions \
				   -fstrict-aliasing -Wstrict-aliasing

CFLAGS   = $(MWCG_CFLAGS) $(MODULE_INC) $(MODULE_DEF)
CPPFLAGS = $(MWCG_CFLAGS) $(MODULE_INC) $(MODULE_DEF) $(CXXFLAGS)
LDFLAGS  = $(MWCG_LDFLAGS) $(MODULE_LDFLAG) 

TMPDIR	:=	$(MWCG_OBJDIR)/$(word 2, $(subst /$(MODULE_NAME_TAG)/, ,$(shell pwd)))

SUBDIRS_ALL   = $(addprefix all_, $(SUBDIRS))
OBJECTS_ALL   = $(addprefix $(TMPDIR)/, $(MODULE_OBJ))
SUBDIRS_CLEAN = $(addprefix clean_, $(SUBDIRS))

UNIT_TEST_ALL   = $(addprefix all_, $(UNIT_TEST_DIR))
UNIT_TEST_CLEAN   = $(addprefix clean_, $(UNIT_TEST_DIR))

.PHONY: $(SUBDIRS_ALL) $(UNIT_TEST_ALL) all
.PHONY: $(SHARED_LIB_NAMES) $(STATIC_LIB_NAMES) $(EXECUTABLE_FILES)
.PHONY: $(SUBDIRS_CLEAN) $(UNIT_TEST_CLEAN) clean
.PHONY: $(addprefix clean_, $(SHARED_LIB_NAMES)) \
				$(addprefix clean_, $(STATIC_LIB_NAMES)) \
				$(addprefix clean_, $(EXECUTABLE_FILES))

######################################################################
#   all
######################################################################
all: tmpdir $(SUBDIRS_ALL) $(UNIT_TEST_ALL) $(OBJECTS_ALL) $(SHARED_LIB_NAMES) \
	   $(STATIC_LIB_NAMES) $(EXECUTABLE_FILES)

tmpdir:
	@$(MKDIR) $(TMPDIR)

ifneq ($(strip $(SUBDIRS_ALL)),)
$(SUBDIRS_ALL):
	@echo "    [Build $(subst all_,,$@)]:"
	@$(MAKE) -C $(subst all_,,$@) default --no-print-directory
endif

ifneq ($(strip $(UNIT_TEST_ALL)),)
$(UNIT_TEST_ALL): $(SUBDIRS_ALL)
	@echo "    [Build $(subst all_,,$@)]:"
	@$(MAKE) -C $(subst all_,,$@) default --no-print-directory
endif

ifneq ($(strip $(SHARED_LIB_NAMES)),)
$(SHARED_LIB_NAMES): \
	$(foreach n, $(addsuffix _obj, $(SHARED_LIB_NAMES)), \
		$(addprefix $(TMPDIR)/, $($n)))
	@$(MKDIR) $(MWCG_INTERNAL_LIBDIR)/
	@$(CXX) -o $(MWCG_INTERNAL_LIBDIR)/lib$@.so.$ -shared \
		-Wl,-soname,lib$@.so $(addprefix $(TMPDIR)/, $($@_obj))
ifneq ($(BUILD_AMBARELLA_MWCG_DEBUG), y)
	@$(STRIP) --strip-unneeded \
		$(MWCG_INTERNAL_LIBDIR)/lib$@.so
endif
endif

ifneq ($(strip $(STATIC_LIB_NAMES)),)
$(STATIC_LIB_NAMES): \
	$(foreach n, $(addsuffix _obj, $(STATIC_LIB_NAMES)), \
		$(addprefix $(TMPDIR)/, $($n)))
	@$(MKDIR) $(MWCG_INTERNAL_LIBDIR)/
	@$(AR) rcus $(MWCG_INTERNAL_LIBDIR)/lib$@.a \
		$(addprefix $(TMPDIR)/, $($@_obj))
ifneq ($(BUILD_AMBARELLA_MWCG_DEBUG), y)
	@$(STRIP) --strip-unneeded \
		$(MWCG_INTERNAL_LIBDIR)/lib$@.a
endif
endif

ifneq ($(strip $(STATIC_TAR_NAMES)),)
$(STATIC_TAR_NAMES): \
	$(foreach n, $(addsuffix _obj, $(STATIC_TAR_NAMES)), \
		$(addprefix $(TMPDIR)/, $($n)))
	@$(MKDIR) $(MWCG_INTERNAL_TARDIR)/
	@$(AR) rcu $(MWCG_INTERNAL_TARDIR)/lib$@.a \
		$(addprefix $(TMPDIR)/, $($@_obj))
endif

ifneq ($(strip $(EXECUTABLE_FILES)),)
$(EXECUTABLE_FILES): \
	$(foreach n, $(addsuffix _obj, $(EXECUTABLE_FILES)), \
		$(addprefix $(TMPDIR)/, $($n)))
	@$(MKDIR) $(MWCG_INTERNAL_BINARY)
	@$(CXX) -o $(MWCG_INTERNAL_BINARY)/$@ \
		$(addprefix $(TMPDIR)/, $($@_obj)) \
		$(LDFLAGS) $($@_ldflag)
ifneq ($(BUILD_AMBARELLA_MWCG_DEBUG), y)
	@$(STRIP) --strip-unneeded $(MWCG_INTERNAL_BINARY)/bin/$@
endif
endif

######################################################################
#   clean
######################################################################
clean: $(SUBDIRS_CLEAN) $(UNIT_TEST_CLEAN) clean_all \
			 $(addprefix clean-, $(SHARED_LIB_NAMES)) \
			 $(addprefix clean-, $(STATIC_LIB_NAMES)) \
			 $(addprefix clean-, $(EXECUTABLE_FILES))

$(SUBDIRS_CLEAN):
	@echo "    [Clean $(subst clean_,,$@)]:"
	@$(MAKE) -C $(subst clean_,,$@) clean --no-print-directory

$(UNIT_TEST_CLEAN):
	@echo "    [Clean $(subst clean_,,$@)]:"
	@$(MAKE) -C $(subst clean_,,$@) clean --no-print-directory

ifneq ($(strip $(SHARED_LIB_NAMES)),)
$(addprefix clean-, $(SHARED_LIB_NAMES)):
	-@$(RM) $(MWCG_INTERNAL_LIBDIR)/lib$(word 2, $(subst -, ,$@)).so*
endif

ifneq ($(strip $(STATIC_LIB_NAMES)),)
$(addprefix clean-, $(STATIC_LIB_NAMES)):
	-@$(RM) $(MWCG_INTERNAL_LIBDIR)/lib$(word 2, $(subst -, ,$@)).a
endif

ifneq ($(strip $(EXECUTABLE_FILES)),)
$(addprefix clean-, $(EXECUTABLE_FILES)):
	-@$(RM) $(MWCG_INTERNAL_BINARY)/$(word 2, $(subst -, ,$@))
endif

clean_all:
	-@$(RM) $(TMPDIR)/*.o *.o $(TMPDIR)/*.d .*.d

######################################################################
#		compile
######################################################################
$(TMPDIR)/%.o: $(shell pwd)/%.cpp
	@echo "      compiling $(shell basename $<)..."
	@$(CXX) $(CPPFLAGS) -c -MMD -o $@ $<

$(TMPDIR)/%.o: $(shell pwd)/%.c
	@echo "      compiling $(shell basename $<)..."
	@$(GCC) $(CFLAGS) -c -MMD -o $@ $<

$(TMPDIR)/%.o: $(shell pwd)/%.S
	@echo "      compiling $(shell basename $<)..."
	@$(GCC) $(CFLAGS) -c -MMD -o $@ $<

-include $(addprefix $(TMPDIR)/, $(MODULE_OBJ:.o=.d))

