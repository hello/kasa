###############################################################################
## Makefile
##
## History:
##    2016/04/01 - [Zhi He] Create File
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
##################################################################################

default: all

.PHONY: all
.PHONY: clean

CC := gcc
AR := ar
CFLAGS := -I../include -I../../lw_cryptography/include

arithm.o: arithm.c
data_source.o: data_source.c
data_source_amba_dsp.o: data_source_amba_dsp.c
platform_amba_dsp.o: platform_amba_dsp.c
uniqueid_puf_bpp.o: uniqueid_puf_bpp.c

all: arithm.o data_source.o data_source_amba_dsp.o platform_amba_dsp.o uniqueid_puf_bpp.o
	$(AR) rcus libuniqueidpufbpp.a arithm.o data_source.o data_source_amba_dsp.o platform_amba_dsp.o uniqueid_puf_bpp.o
	-mkdir ../lib/
	-cp libuniqueidpufbpp.a ../lib/
	echo "Build libuniqueidpufbpp.a done."

clean:
	rm *.a -rf
	rm *.o -rf



