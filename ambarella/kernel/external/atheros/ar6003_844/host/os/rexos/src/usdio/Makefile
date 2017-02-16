obj-m:= usdio.o
EXTRA_CFLAGS +=	-I$(WORKAREA)/host/sdiostack/src/include \
                -I$(WORKAREA)/host/os/rexos/include/athr \
                -DLINUX -DDEBUG -DSDIO

usdio-objs := usdio_drv.o

# This sets up the kbuild and invokes make with the proper settings.

include $(WORKAREA)/host/localmake.linux.inc

PWD :=  $(shell pwd)
MAKE := make
OUTPUT_DIR = .output

all: output_dir modules rearrange

output_dir:
	-mkdir -p $(OUTPUT_DIR)
	
modules:
	$(MAKE) -C $(ATH_LINUXPATH) SUBDIRS=$(PWD) SDIO_SRC=$(PWD) modules

rearrange:
	mv -f usdio.ko $(OUTPUT_DIR)
	rm -rf *.o usdio.mod.c .tmp_versions .usdio*
	cp -f $(WORKAREA)/host/.output/LOCAL_i686-SDIO/image/sdio_pcistd_hcd.ko $(OUTPUT_DIR)
	cp -f $(WORKAREA)/host/.output/LOCAL_i686-SDIO/image/sdio_busdriver.ko $(OUTPUT_DIR)
	cp -f $(WORKAREA)/host/.output/LOCAL_i686-SDIO/image/sdio_lib.ko $(OUTPUT_DIR)
    
clean:
	rm -rf *.o $(OUTPUT_DIR) usdio.mod.c .tmp_versions .usdio*

