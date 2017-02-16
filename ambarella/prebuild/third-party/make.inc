##
## prebuild/third-party/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifneq ($(AMBARELLA_CPU_ARCH),)
include $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/make.inc
include $(AMB_TOPDIR)/prebuild/third-party/noarch/make.inc
endif

