##
## prebuild/third-party/python/make.inc
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

ifeq ($(CONFIG_AMBARELLA_PYTHON_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_python
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/python2.7/site-packages
ifneq ($(CONFIG_AMBARELLA_PYTHON_FUTURE_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/__future__.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_BASE64_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/base64.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_BASEHTTPSERVER_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/BaseHTTPServer.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_BISECT_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/bisect.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_COLLECTIONS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/collections.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_DISTUTILS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/distutils
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_DIS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/dis.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_DUMMY_THREAD_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/dummy_thread.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_DUMMY_THREADING_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/dummy_threading.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_FUNCTOOLS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/functools.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_HASHLIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/hashlib.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_HEAPQ_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/heapq.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_HTTPLIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/httplib.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_INSPECT_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/inspect.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_KEYWORD_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/keyword.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_MIMETOOLS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/mimetools.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_OPCODE_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/opcode.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_RANDOM_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/random.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_RFC822_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/rfc822.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_SCHED_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/sched.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_SIMPLEXMLRPCSERVER_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/SimpleXMLRPCServer.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_SOCKET_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/socket.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_SOCKETSERVER_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/SocketServer.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_STRINGIO_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/StringIO.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_TELNETLIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/telnetlib.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_TEMPFILE_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/tempfile.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_THREADING_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/threading.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_TOKENIZE_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/tokenize.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_TOKEN_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/token.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_URLPARSE_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/urlparse.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_XMLLIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/xmllib.py
endif
ifneq ($(CONFIG_AMBARELLA_PYTHON_XMLRPCLIB_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/python2.7/xmlrpclib.py
endif
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

