
EXTRA_CFLAGS += -I$(ATH_SRC_BASE)/include
EXTRA_CFLAGS += -I$(ATH_SRC_BASE)/../include
EXTRA_CFLAGS += -I$(ATH_SRC_BASE)/os/linux/include

EXTRA_CFLAGS += -I$(ATH_SRC_BASE)/hif/cf/linux/include \
                -DEXPORT_SYMTAB -D__KERNEL__ -DLINUX -DDEBUG 

ifneq ($(ATH_OS_SUB_TYPE), linux_2_4)
obj-m := ar6000_cs.o
else
EXTRA_CFLAGS += -DKERNEL_2_4
O_TARGET := ar6000_cs.o
export-objs := ar6000_cs.o

obj-y += $(ATH_SRC_BASE)/hif/cf/linux/src/ar6000_cs.c
obj-m += ar6000_cs.o
include $(ATH_LINUXPATH)/Rules.make
endif
