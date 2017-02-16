##
## scripts/kconfig/Makefile
##
## History:
##    2012/06/21 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

HOSTCC		:= gcc

.PHONY: all dtc
all: dtc

include Makefile.dtc

SHARED_DEPS	:= \
	dtc.h srcpos.h version_gen.h util.h \
	libfdt/libfdt.h libfdt/libfdt_internal.h \
	libfdt/fdt.h libfdt/libfdt_env.h

dtc-lexer.lex.c: dtc-lexer.lex.c_shipped dtc-lexer.l dtc-parser.tab.h
	@cp $< $@

dtc-parser.tab.c: dtc-parser.tab.c_shipped dtc-parser.y
	@cp $< $@

dtc-parser.tab.h: dtc-parser.tab.h_shipped
	@cp $< $@

$(DTC_OBJS):%.o : %.c $(SHARED_DEPS)
	@$(HOSTCC) -I. -I./libfdt -c $< -o $@

dtc: $(DTC_OBJS)
	@$(HOSTCC) $^ -o $@

.PHONY: clean
clean:
	@rm -rf *.o dtc dtc-lexer.lex.c dtc-parser.tab.c dtc-parser.tab.h

