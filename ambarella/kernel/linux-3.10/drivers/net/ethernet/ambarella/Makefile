#
# Makefile for the Ambarella network device drivers
#

obj-$(CONFIG_NET_VENDOR_AMBARELLA)		+= ambarella_eth.o
ifeq ($(CONFIG_PLAT_AMBARELLA_S3),y)
ambarella_eth-y					:= ambarella_eth_enhanced.o
else
ambarella_eth-y					:= ambarella_eth_normal.o
endif

