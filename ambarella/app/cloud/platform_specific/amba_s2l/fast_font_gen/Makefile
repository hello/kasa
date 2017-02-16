
default: all

include ../../../build/core/linux/pre_requirement.mk

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/config_linux.mk
include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/extern_lib.mk

.PHONY: all clean

MODULE_TARGET = font_gen

MODULE_INC = -I/cygdrive/x/zhe/s2l/ambarella/prebuild/third-party/armv7-a-hf/freetype/include -I/cygdrive/x/zhe/s2l/ambarella/prebuild/third-party/armv7-a-hf/freetype/include/freetype2
MODULE_LIB = -lfreetype -L/cygdrive/x/zhe/s2l/ambarella/prebuild/third-party/armv7-a-hf/freetype/usr/lib/
MODULE_LIB += -L/cygdrive/x/zhe/s2l/ambarella/prebuild/third-party/armv7-a-hf/bzip2/usr/lib -lbz2
MODULE_LIB += -L/cygdrive/x/zhe/s2l/ambarella/prebuild/third-party/armv7-a-hf/zlib/usr/lib -lz
MODULE_SRC = fast_font_gen_v2.c

MODULE_TARGET_1 = font_load
MODULE_SRC_1 = fast_font_load_v2.c

include $(BUILDSYSTEM_DIR)/$(MODULE_NAME_TAG)/build/core/linux/rules_linux.mk

all:
	$(MAKE_PRINT)echo "font_gen:"
	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET) $(MODULE_INC) -g -O0 $(MODULE_SRC) $(MODULE_LIB)
	$(MAKE_PRINT)echo "font_load:"
	$(MAKE_PRINT)$(CXX) -o $(MODULE_TARGET_1) -g -O0 $(MODULE_SRC_1)

clean:







