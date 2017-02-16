##
## system/src/flashdb/slcnand/make.inc
##
## History:
##    2007/10/03 - [Charles Chiou] created file
##
## Copyright (C) 2004-2007, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

NANDDB_SRCS = \
	$(TOP)/src/flashdb/slcnand/db.c \
	$(TOP)/src/flashdb/slcnand/hy27us08121m.c \
	$(TOP)/src/flashdb/slcnand/hy27us08121a.c \
	$(TOP)/src/flashdb/slcnand/hy27us08281a.c \
	$(TOP)/src/flashdb/slcnand/hy27us08561a.c \
	$(TOP)/src/flashdb/slcnand/hy27us08561m.c \
	$(TOP)/src/flashdb/slcnand/hy27ua081g1m.c \
	$(TOP)/src/flashdb/slcnand/hy27uf081g2a.c \
	$(TOP)/src/flashdb/slcnand/hy27uf082g2b.c \
	$(TOP)/src/flashdb/slcnand/hy27uf082g2b.c \
	$(TOP)/src/flashdb/slcnand/hy27uf084g2b.c \
	$(TOP)/src/flashdb/slcnand/hy27uf084g2m.c \
	$(TOP)/src/flashdb/slcnand/hy27u1g8f2b.c \
	$(TOP)/src/flashdb/slcnand/h27u518s2c.c \
	$(TOP)/src/flashdb/slcnand/h27u2g8f2d.c \
	$(TOP)/src/flashdb/slcnand/k9f1208.c \
	$(TOP)/src/flashdb/slcnand/k9f1208x0c.c \
	$(TOP)/src/flashdb/slcnand/k9f1g08.c \
	$(TOP)/src/flashdb/slcnand/k9f1g08u0b.c \
	$(TOP)/src/flashdb/slcnand/k9f1g08u0e.c \
	$(TOP)/src/flashdb/slcnand/k9f4g08u0e.c \
	$(TOP)/src/flashdb/slcnand/k9f2808.c \
	$(TOP)/src/flashdb/slcnand/k9f2g08.c \
	$(TOP)/src/flashdb/slcnand/k9f2g08u0c.c \
	$(TOP)/src/flashdb/slcnand/k9f5608.c \
	$(TOP)/src/flashdb/slcnand/k9k4g08.c \
	$(TOP)/src/flashdb/slcnand/k9k8g08.c \
	$(TOP)/src/flashdb/slcnand/k9nbg08.c \
	$(TOP)/src/flashdb/slcnand/k9w8g08.c \
	$(TOP)/src/flashdb/slcnand/k9wag08.c \
	$(TOP)/src/flashdb/slcnand/st01gw3a.c \
	$(TOP)/src/flashdb/slcnand/st01gw3b.c \
	$(TOP)/src/flashdb/slcnand/st02gw3b.c \
	$(TOP)/src/flashdb/slcnand/st128w3a.c \
	$(TOP)/src/flashdb/slcnand/st256w3a.c \
	$(TOP)/src/flashdb/slcnand/st512w3a.c \
	$(TOP)/src/flashdb/slcnand/tc58bvg0s3h.c \
	$(TOP)/src/flashdb/slcnand/tc58bvg1s3h.c \
	$(TOP)/src/flashdb/slcnand/tc58dvm72a.c \
	$(TOP)/src/flashdb/slcnand/tc58dvm82a.c \
	$(TOP)/src/flashdb/slcnand/tc58dvm92a.c \
	$(TOP)/src/flashdb/slcnand/tc58nvg0s3c.c \
	$(TOP)/src/flashdb/slcnand/tc58nvg0s3e.c \
	$(TOP)/src/flashdb/slcnand/tc58nvg0s3h.c \
	$(TOP)/src/flashdb/slcnand/tc58nvg1s3h.c \
	$(TOP)/src/flashdb/slcnand/tc58nvg1s3e.c \
	$(TOP)/src/flashdb/slcnand/tc58nvg2s3e.c \
	$(TOP)/src/flashdb/slcnand/tc58nvm9s3c.c \
	$(TOP)/src/flashdb/slcnand/mt29f2g08aac.c \
	$(TOP)/src/flashdb/slcnand/mt29f1g08abaea.c \
	$(TOP)/src/flashdb/slcnand/mt29f2g08aba.c \
	$(TOP)/src/flashdb/slcnand/mt29f8g08daa.c \
	$(TOP)/src/flashdb/slcnand/mt29f2g08abafa.c \
	$(TOP)/src/flashdb/slcnand/ct48248ns486g1.c \
	$(TOP)/src/flashdb/slcnand/asu1ga30ht.c \
	$(TOP)/src/flashdb/slcnand/numonyx02gw3b2d.c \
	$(TOP)/src/flashdb/slcnand/k9f4g08u0a.c \
	$(TOP)/src/flashdb/slcnand/f59l1g81a.c \
	$(TOP)/src/flashdb/slcnand/f59l1g81la.c \
	$(TOP)/src/flashdb/slcnand/f59l1g81ma.c \
	$(TOP)/src/flashdb/slcnand/f59l2g81a.c \
	$(TOP)/src/flashdb/slcnand/f59l4g81a.c \
	$(TOP)/src/flashdb/slcnand/s34ml01g1.c \
	$(TOP)/src/flashdb/slcnand/s34ml02g1.c \
	$(TOP)/src/flashdb/slcnand/s34ml04g1.c \
	$(TOP)/src/flashdb/slcnand/s34ml01g2.c \
	$(TOP)/src/flashdb/slcnand/s34ml02g2.c \
	$(TOP)/src/flashdb/slcnand/s34ml04g2.c \
	$(TOP)/src/flashdb/slcnand/mx30lf1g08aa.c \
	$(TOP)/src/flashdb/slcnand/mx30lf1ge8ab.c \
	$(TOP)/src/flashdb/slcnand/mx30lf2ge8ab.c \
	$(TOP)/src/flashdb/slcnand/w29n01gvscaa.c \
	$(TOP)/src/flashdb/slcnand/w29n02gvsiaa.c

NANDDB_OBJS = \
	db.o \
	hy27us08121m.o \
	hy27us08121a.o \
	hy27us08281a.o \
	hy27us08561a.o \
	hy27us08561m.o \
	hy27ua081g1m.o \
	hy27uf082g2a.o \
	hy27uf082g2b.o \
	hy27uf081g2a.o \
	hy27uf084g2b.o \
	hy27uf084g2m.o \
	hy27u1g8f2b.o \
	h27u518s2c.o \
	h27u2g8f2d.o \
	k9f1208.o \
	k9f1208x0c.o \
	k9f1g08.o \
	k9f1g08u0b.o \
	k9f1g08u0e.o \
	k9f4g08u0e.o \
	k9f2808.o \
	k9f2g08.o \
	k9f2g08u0c.o \
	k9f5608.o \
	k9k4g08.o \
	k9k8g08.o \
	k9nbg08.o \
	k9w8g08.o \
	k9wag08.o \
	st01gw3a.o \
	st01gw3b.o \
	st02gw3b.o \
	st128w3a.o \
	st256w3a.o \
	st512w3a.o \
	tc58bvg0s3h.o \
	tc58bvg1s3h.o \
	tc58dvm72a.o \
	tc58dvm82a.o \
	tc58dvm92a.o \
	tc58nvg0s3c.o \
	tc58nvg0s3e.o \
	tc58nvg0s3h.o \
	tc58nvg1s3h.o \
	tc58nvg1s3e.o \
	tc58nvg2s3e.o \
	tc58nvm9s3c.o \
	mt29f2g08aac.o \
	mt29f1g08abaea.o \
	mt29f2g08aba.o \
	mt29f8g08daa.o \
	mt29f2g08abafa.o \
	ct48248ns486g1.o \
	asu1ga30ht.o \
	numonyx02gw3b2d.o \
	k9f4g08u0a.o \
	f59l1g81a.o \
	f59l1g81la.o \
	f59l1g81ma.o \
	f59l2g81a.o \
	f59l4g81a.o \
	s34ml01g1.o \
	s34ml02g1.o \
	s34ml04g1.o \
	s34ml01g2.o \
	s34ml02g2.o \
	s34ml04g2.o \
	mx30lf1g08aa.o \
	mx30lf1ge8ab.o \
	mx30lf2ge8ab.o \
	w29n01gvscaa.o \
	w29n02gvsiaa.o
