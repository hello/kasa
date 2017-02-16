##
## /S2l/packages/img_mw/document/make.inc
##
## History:
##   Apr 22, 2015 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

IMG_MW_DOC_LOCAL := $(LOCAL_PATH)
IMG_MW_DOC_PATH  := $(IMG_MW_DOC_LOCAL)/doxygen
IMG_MW_DOX_OUT   := $(AMB_BOARD_OUT)/packages/img_mw/document/doxygen
IMG_MW_DOX_SRC   := $(wildcard $(IMG_MW_DOC_PATH)/*.doxygen)
IMG_MW_DOX       := $(patsubst $(IMG_MW_DOC_PATH)/%.doxygen, $(IMG_MW_DOX_OUT)/%.dox, $(IMG_MW_DOX_SRC))
IMG_MW_DOP       := $(patsubst $(IMG_MW_DOC_PATH)/%.doxygen, $(IMG_MW_DOX_OUT)/%.dop, $(IMG_MW_DOX_SRC))

$(IMG_MW_DOX_OUT)/%.dox: $(IMG_MW_DOC_PATH)/%.doxygen
	@mkdir -p $(IMG_MW_DOX_OUT)
	@cat $< | sed -e 's,@DOCUMENT@,$(IMG_MW_DOC_LOCAL),g'            \
	              -e 's,@TOPDIR@,$(AMB_TOPDIR),g'                    \
	              -e 's,@IMG_MW@,$(AMB_TOPDIR)/packages/img_mw/mw,g' \
	              -e 's,@ARCH@,$(AMBARELLA_ARCH),g' > $@
	@if [ -e $@ ]; then \
		doxout=$$(cat $@ | grep "OUTPUT_DIRECTORY" | awk -F[=] '{print $$2}' | sed 's/\s*//g'); \
		rm -rf $${doxout}; \
		mkdir -p $${doxout}; \
		doxygen $@ > /dev/null 2>&1 && \
		echo "Generated document of $(patsubst $(IMG_MW_DOX_OUT)/%.dox,%x,$@)" || \
		echo "Failed to generate document of $(patsubst $(IMG_MW_DOX_OUT)/%.dox,%x,$@)"; \
	fi

$(IMG_MW_DOX_OUT)/%.dop: $(IMG_MW_DOC_PATH)/%.doxygen
	@mkdir -p $(IMG_MW_DOX_OUT)
	@mkdir -p $(IMG_MW_DOC_LOCAL)/pdf/$(AMBARELLA_ARCH)
	@cat $< | sed -e 's,@DOCUMENT@,$(IMG_MW_DOC_LOCAL),g'            \
	              -e 's,@TOPDIR@,$(AMB_TOPDIR),g'                    \
	              -e 's,@IMG_MW@,$(AMB_TOPDIR)/packages/img_mw/mw,g' \
	              -e 's,@ARCH@,$(AMBARELLA_ARCH),g'                  \
	              -e 's,\(GENERATE_LATEX\ *=\).*,\1\ YES,g'          \
	              -e 's,\(GENERATE_HTML\ *=\).*,\1\ NO,g' > $@
	@if [ -e $@ ]; then \
		doxout=$$(cat $@ | grep "OUTPUT_DIRECTORY" | awk -F[=] '{print $$2}' | sed 's/\s*//g'); \
		if [ -e $${doxout} ]; then \
			rm -rf $${doxout}/latex; \
		fi; \
		mkdir -p $${doxout}; \
		doxygen $@ > /dev/null 2>&1 && \
		make pdf -C $${doxout}/latex > /dev/null 2>&1 && \
		mv $${doxout}/latex/refman.pdf $(IMG_MW_DOC_LOCAL)/pdf/$(AMBARELLA_ARCH)/$(patsubst $(IMG_MW_DOX_OUT)/%.dop,%,$@).pdf && \
		echo "Generated document of $(patsubst $(IMG_MW_DOX_OUT)/%.dop,%x,$@)" || \
		echo "Failed to generate document of $(patsubst $(IMG_MW_DOX_OUT)/%.dop,%x,$@)"; \
		rm -rf $${doxout}/latex; \
	fi

img_mw_doc_clean:
	@rm -rf $(IMG_MW_DOX)

img_mw_pdf_clean:
	@rm -rf $(IMG_MW_DOP)
	@rm -rf $(IMG_MW_DOC_LOCAL)/pdf/$(AMBARELLA_ARCH)*

img_mw_doc: img_mw_doc_clean $(IMG_MW_DOX)
	@echo "All documents are generated..."

img_mw_pdf: img_mw_pdf_clean $(IMG_MW_DOP)
	@echo "All PDF are generated..."
