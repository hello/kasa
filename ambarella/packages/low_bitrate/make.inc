##
## ambarella/packages/lbr/make.inc
##
## History:
##    2014/02/17 - [Louis Sun] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_LBR_PACKAGE), y)
include $(call my-dir)/arch_$(AMBARELLA_ARCH)/make.inc
endif