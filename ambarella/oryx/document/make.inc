##
## /S2l/oryx/document/make.inc
##
## History:
##   2015-4-7 - [ypchang] created file
##
## Copyright (c) 2015 Ambarella, Inc.
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

DOC_LOCAL := $(LOCAL_PATH)
DOC_PATH  := $(DOC_LOCAL)/doxygen
DOX_OUT   := $(AMB_BOARD_OUT)/oryx/document/doxygen
DOX_SRC   := $(wildcard $(DOC_PATH)/*.doxygen)
DOX       := $(patsubst $(DOC_PATH)/%.doxygen, $(DOX_OUT)/%.dox, $(DOX_SRC))
DOP       := $(patsubst $(DOC_PATH)/%.doxygen, $(DOX_OUT)/%.dop, $(DOX_SRC))

$(DOX_OUT)/%.dox: $(DOC_PATH)/%.doxygen
	@mkdir -p $(DOX_OUT)
	@cat $< | sed -e 's,@DOCUMENT@,$(DOC_LOCAL),g' -e 's,@ORYX@,$(ORYX_DIR),g' > $@
	@if [ -e $@ ]; then \
		doxout=$$(cat $@ | grep "OUTPUT_DIRECTORY" | awk -F[=] '{print $$2}' | sed 's/\s*//g'); \
		rm -rf $${doxout}; \
		mkdir -p $${doxout}; \
		doxygen $@ > /dev/null 2>&1 && \
		echo -e "[\033[01;32mSucceeded\033[00m] Generating document of $(patsubst $(DOX_OUT)/%.dox,%,$@)!" || \
		echo -e "[\033[01;31m   Failed\033[00m] Generating document of $(patsubst $(DOX_OUT)/%.dox,%,$@)!"; \
	fi

$(DOX_OUT)/%.dop: $(DOC_PATH)/%.doxygen
	@mkdir -p $(DOX_OUT)
	@mkdir -p $(DOC_LOCAL)/pdf
	@cat $< | sed -e 's,@DOCUMENT@,$(DOC_LOCAL),g'             \
	              -e 's,@ORYX@,$(ORYX_DIR),g'                  \
	              -e 's,\(GENERATE_LATEX\ *=\).*,\1\ YES,g'    \
	              -e 's,\(GENERATE_HTML\ *=\).*,\1\ NO,g' > $@
	@if [ -e $@ ]; then \
		doxout=$$(cat $@ | grep "OUTPUT_DIRECTORY" | awk -F[=] '{print $$2}' | sed 's/\s*//g'); \
		if [ -e $${doxout} ]; then \
			rm -rf $${doxout}/latex; \
		fi; \
		mkdir -p $${doxout}; \
		doxygen $@ > /dev/null 2>&1; \
		make pdf -C $${doxout}/latex > /dev/null 2>&1 && \
		mv $${doxout}/latex/refman.pdf $(DOC_LOCAL)/pdf/$(patsubst $(DOX_OUT)/%.dop,%,$@).pdf && \
		echo -e "[\033[01;32mSucceeded\033[00m] Generating PDF document of $(patsubst $(DOX_OUT)/%.dop,%,$@)!" || \
		echo -e "[\033[01;31m   Failed\033[00m] Generating PDF document of $(patsubst $(DOX_OUT)/%.dop,%,$@)!"; \
		rm -rf $${doxout}/latex; \
	fi

## Define document generating sequence
## Documents that referenced by others must be generated first
AIR_DOC_DEPS := $(DOX_OUT)/ipc.dox
AIR_DOC_DEPS += $(DOX_OUT)/audio.dox
AIR_DOC_DEPS += $(DOX_OUT)/video.dox
AIR_DOC_DEPS += $(DOX_OUT)/event.dox
AIR_DOC_DEPS += $(DOX_OUT)/image.dox
AIR_DOC_DEPS += $(DOX_OUT)/record.dox
AIR_DOC_DEPS += $(DOX_OUT)/playback.dox
AIR_DOC_DEPS += $(DOX_OUT)/sip.dox
AIR_DOC_DEPS += $(DOX_OUT)/system.dox

$(DOX_OUT)/configure.dox:  $(DOX_OUT)/utility.dox
$(DOX_OUT)/ipc.dox:        $(DOX_OUT)/utility.dox
$(DOX_OUT)/amf.dox:        $(DOX_OUT)/configure.dox
$(DOX_OUT)/audio.dox:      $(DOX_OUT)/configure.dox
$(DOX_OUT)/event.dox:      $(DOX_OUT)/configure.dox
$(DOX_OUT)/image.dox:      $(DOX_OUT)/configure.dox
$(DOX_OUT)/video.dox:      $(DOX_OUT)/configure.dox
$(DOX_OUT)/sip.dox:        $(DOX_OUT)/configure.dox
$(DOX_OUT)/system.dox:     $(DOX_OUT)/configure.dox
$(DOX_OUT)/record.dox:     $(DOX_OUT)/amf.dox $(DOX_OUT)/audio.dox $(DOX_OUT)/video.dox
$(DOX_OUT)/playback.dox:   $(DOX_OUT)/amf.dox $(DOX_OUT)/audio.dox
$(DOX_OUT)/air.dox:        $(AIR_DOC_DEPS)
$(DOX_OUT)/services.dox:   $(DOX_OUT)/air.dox

AIR_PDF_DEPS := $(DOX_OUT)/ipc.dop
AIR_PDF_DEPS += $(DOX_OUT)/audio.dop
AIR_PDF_DEPS += $(DOX_OUT)/video.dop
AIR_PDF_DEPS += $(DOX_OUT)/event.dop
AIR_PDF_DEPS += $(DOX_OUT)/image.dop
AIR_PDF_DEPS += $(DOX_OUT)/record.dop
AIR_PDF_DEPS += $(DOX_OUT)/playback.dop
AIR_PDF_DEPS += $(DOX_OUT)/sip.dop
AIR_PDF_DEPS += $(DOX_OUT)/system.dop

$(DOX_OUT)/configure.dop:  $(DOX_OUT)/utility.dop
$(DOX_OUT)/ipc.dop:        $(DOX_OUT)/utility.dop
$(DOX_OUT)/amf.dop:        $(DOX_OUT)/configure.dop
$(DOX_OUT)/audio.dop:      $(DOX_OUT)/configure.dop
$(DOX_OUT)/event.dop:      $(DOX_OUT)/configure.dop
$(DOX_OUT)/image.dop:      $(DOX_OUT)/configure.dop
$(DOX_OUT)/video.dop:      $(DOX_OUT)/configure.dop
$(DOX_OUT)/sip.dop:        $(DOX_OUT)/configure.dop
$(DOX_OUT)/system.dop:     $(DOX_OUT)/configure.dop
$(DOX_OUT)/record.dop:     $(DOX_OUT)/amf.dop $(DOX_OUT)/audio.dop $(DOX_OUT)/video.dop
$(DOX_OUT)/playback.dop:   $(DOX_OUT)/amf.dop $(DOX_OUT)/audio.dop
$(DOX_OUT)/air.dop:        $(AIR_PDF_DEPS)
$(DOX_OUT)/services.dop:   $(DOX_OUT)/air.dop

oryx_doc_clean:
	@rm -rf $(DOX)

oryx_pdf_clean:
	@rm -rf $(DOP)
	@rm -rf $(DOC_LOCAL)/pdf/*

oryx_doc: oryx_doc_clean $(DOX)
	@echo "All documents are generated..."

oryx_pdf: oryx_pdf_clean $(DOP)
	@echo "All PDF are generated..."
