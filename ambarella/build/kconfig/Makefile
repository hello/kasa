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

.PHONY: all ncurses gconf
all: ncurses conf mconf

HOSTCFLAGS := -DLOCALE

ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
	HOSTNCURSES += -I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>"
else
ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
	HOSTNCURSES += -I/usr/include/ncurses -DCURSES_LOC="<ncurses/curses.h>"
else
ifeq (/usr/local/include/ncurses/ncurses.h, $(wildcard /usr/local/include/ncurses/ncurses.h))
	HOSTNCURSES += -I/usr/local/include/ncurses -DCURSES_LOC="<ncurses.h>"
else
ifeq (/usr/local/include/ncurses/curses.h, $(wildcard /usr/local/include/ncurses/curses.h))
	HOSTNCURSES += -I/usr/local/include/ncurses -DCURSES_LOC="<ncurses/curses.h>"
else
ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
	HOSTNCURSES += -DCURSES_LOC="<ncurses.h>"
else
	HOSTNCURSES += -DCURSES_LOC="<curses.h>"
endif
endif
endif
endif
endif

HOSTGLIBS	:= `pkg-config --libs gtk+-2.0 gmodule-2.0 libglade-2.0`
HOSTGFLAGS	:= `pkg-config --cflags gtk+-2.0 gmodule-2.0 libglade-2.0` -DLKC_DIRECT_LINK

SHARED_DEPS	:= lkc.h lkc_proto.h expr.h

CONF_SRCS	:= conf.c
CONF_OBJS	:= $(patsubst %.c,%.o, $(CONF_SRCS))

GCONF_SRC	:= gconf.c
GCONF_OBJS	:= $(patsubst %.c,%.o, $(GCONF_SRC))

MCONF_SRCS	:= lxdialog/checklist.c lxdialog/util.c lxdialog/inputbox.c
MCONF_SRCS	+= lxdialog/textbox.c lxdialog/yesno.c lxdialog/menubox.c
MCONF_SRCS	+= mconf.c
MCONF_OBJS	:= $(patsubst %.c,%.o, $(MCONF_SRCS))

lex.zconf.o: lex.zconf.c $(SHARED_DEPS)
	@$(HOSTCC) $(HOSTCFLAGS) -c $< -o $@

lex.zconf.c: lex.zconf.c_shipped
	@cp $< $@

zconf.tab.c: zconf.tab.c_shipped
	@cp $< $@

zconf.hash.c: zconf.hash.c_shipped
	@cp $< $@

zconf.tab.o: zconf.tab.c zconf.hash.c lex.zconf.c \
		confdata.c expr.c symbol.c menu.c $(SHARED_DEPS)
	@$(HOSTCC) $(HOSTCFLAGS) -I. -c $< -o $@

$(CONF_OBJS):%.o : %.c $(SHARED_DEPS)
	@$(HOSTCC) $(HOSTCFLAGS) -I. -c $< -o $@

conf: $(CONF_OBJS) zconf.tab.o
	@$(HOSTCC) $^ -o $@

$(GCONF_OBJS): %.o: %.c $(SHARED_DEPS) images.c
	@$(HOSTCC) $(HOSTGFLAGS) -I. -c $< -o $@

gconf: $(GCONF_OBJS) zconf.tab.o
	@$(HOSTCC) $^ -o $@ $(HOSTGLIBS)

$(MCONF_OBJS):%.o : %.c $(SHARED_DEPS)
	@$(HOSTCC) $(HOSTCFLAGS) $(HOSTNCURSES) -I. -c $< -o $@

mconf: $(MCONF_OBJS) zconf.tab.o
	@$(HOSTCC) $^ -o $@ -lncurses

ncurses:
	@echo "main() {}" > lxtemp.c
	@if $(HOSTCC) lxtemp.c $(LIBS) ; then \
		rm -f lxtemp.c a.out a.exe; \
	else \
		rm -f lxtemp.c; \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must have Ncurses installed in order" ;\
		echo ">> to use 'make menuconfig'" ;\
		echo ;\
		exit 1 ;\
	fi


.PHONY: clean
clean:
	@rm -rf *.o lxdialog/*.o conf mconf gconf zconf.tab.c zconf.hash.c lex.zconf.c

