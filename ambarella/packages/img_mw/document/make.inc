##
## /S2l/packages/img_mw/document/make.inc
##
## History:
##   Apr 22, 2015 - [ypchang] created file
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
