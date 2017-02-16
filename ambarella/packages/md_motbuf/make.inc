##
## packages/md_motbuf/make.inc
##
## History:
##    2013/02/22 - [Qiu jingyang] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)

include $(call my-dir)/arch_$(AMBARELLA_ARCH)/make.inc

endif

