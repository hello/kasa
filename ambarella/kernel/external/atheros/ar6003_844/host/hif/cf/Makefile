EXTRA_CFLAGS += -I$(ATH_SRC_BASE)/hif/cf/linux/include
EXTRA_CFLAGS += -DCF -DHTC_SYNC

ifdef ATH_CROSS_COMPILE_TYPE
_CROSS_COMPILE_LINE := ARCH=$(ATH_ARCH_CPU_TYPE) CROSS_COMPILE=$(ATH_CROSS_COMPILE_TYPE)
endif

# Select kernel module extension (2.6 versus 2.4)
ifneq ($(ATH_OS_SUB_TYPE),linux_2_4)
KMOD_EXTENSION :=ko
endif
            
ifeq ($(ATH_OS_SUB_TYPE),linux_2_4)
KMOD_EXTENSION :=o
endif

ar6000-objs := ../../hif/cf/linux/src/hif.o

all:
	$(MAKE) $(_CROSS_COMPILE_LINE) -C  $(ATH_LINUXPATH) SUBDIRS=$(ATH_SRC_BASE)/hif/cf/linux/src modules
	$(MAKE) -C $(ATH_SRC_BASE)/apps/wpa_supplicant
	cp -f $(ATH_SRC_BASE)/hif/cf/linux/src/ar6000_cs.$(KMOD_EXTENSION) $(COMPILED_IMAGE_OBJECTS_PATH)

clean:
	rm -f linux/src/ar6000_cs.o linux/src/ar6000_cs.ko
