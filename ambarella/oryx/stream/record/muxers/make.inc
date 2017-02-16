##
## /S2l/oryx/stream/record/muxers/make.inc
##
## History:
##   2014-12-11 - [Zhi He] created file
##
## Copyright (C) 2008-2014, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

LOCAL_PATH := $(call my-dir)

MUXER_DIR := $(call my-dir)

include $(MUXER_DIR)/muxer-ts/make.inc
include $(MUXER_DIR)/muxer-mp4/make.inc
include $(MUXER_DIR)/muxer-rtp/make.inc
include $(MUXER_DIR)/muxer-export/make.inc
