#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2010 Atheros Corporation.  All rights reserved.
# 
# The software source and binaries included in this development package are
# licensed, not sold. You, or your company, received the package under one
# or more license agreements. The rights granted to you are specifically
# listed in these license agreement(s). All other rights remain with Atheros
# Communications, Inc., its subsidiaries, or the respective owner including
# those listed on the included copyright notices.  Distribution of any
# portion of this package must be in strict compliance with the license
# agreement(s) terms.
# </copyright>
# 
# <summary>
# 	Wifi driver for AR6002
# </summary>
#
#------------------------------------------------------------------------------
#==============================================================================
# Author(s): ="Atheros"
#==============================================================================

#
# MII HIF makefile for Linux
#  

EXTRA_CFLAGS += -DHIF_MESSAGE_BASED
ATH_HIF_MESSAGE_BASED =yes

ifneq ($(ATH_OS_SUB_TYPE),linux_2_4)
ar6000-objs := ../../hif/mii/linux/hif_mii.o

endif

all:

clean:
