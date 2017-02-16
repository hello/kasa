#
# Main Makefile
#
export ATH_TOPDIR=$(shell pwd)/..
export BOARD_TYPE=arm

include scripts/${BOARD_TYPE}/config.${BOARD_TYPE}
include scripts/${BOARD_TYPE}/Makefile.${BOARD_TYPE}

#################################################################################################################################
ifndef KERNELPATH
$(error "Must specify KERNELPATH value")
endif

ifndef KERNELARCH
$(error "Must specify KERNELARCH value")
endif

#################################################################################################################################
export MAKEARCH=make ARCH=$(KERNELARCH) 
#CROSS_COMPILE=$(TOOLPREFIX)

export INSTALL_ROOT?=${ATH_TOPDIR}/rootfs-$(BOARD_TYPE).build
export ROOTFS_TOPDIR=${ATH_TOPDIR}/rootfs
export WLAN_DRIVER_TOPDIR=${ATH_TOPDIR}/drivers
export WPA_SUPPLICANT_TOPDIR=${ATH_TOPDIR}/apps/hostap
export WPA_SUPPLICANT_COMBO_TOPDIR=${ATH_TOPDIR}/apps/athr-hostap-wfd
export IW30_TOPDIR=${ATH_TOPDIR}/apps/iw-3.0
export LIBNL11_TOPDIR=${ATH_TOPDIR}/apps/libnl-1
export WIRELESSTOOLS_TOPDIR=${ATH_TOPDIR}/apps/wireless_tools.29

#################################################################################################################################

# clean: libnl_clean iw_clean wpa_supplicant_clean drivers_clean rootfs_clean wireless_tools_clean
clean: hostapd_clean wpa_supplicant_clean drivers_clean rootfs_clean

#
# rootfs build
#
rootfs_prep:
	@mkdir -p $(INSTALL_ROOT)
	@mkdir -p $(INSTALL_ROOT)/sbin
	@mkdir -p $(INSTALL_ROOT)/usr/sbin
	@mkdir -p $(INSTALL_ROOT)/lib/modules
	@mkdir -p $(INSTALL_ROOT)/etc

rootfs_build:
	@echo Build rootfs
	@cp -Rf $(ROOTFS_TOPDIR)/* $(INSTALL_ROOT)

rootfs_clean:
	@echo Clean rootfs
	@rm -rf $(INSTALL_ROOT)

#
# drivers build
#
drivers: rootfs_prep
	@echo Build drivers
	cd ${WLAN_DRIVER_TOPDIR} && ${MAKEARCH}
	find ${WLAN_DRIVER_TOPDIR} -name '*.ko' -exec cp '{}' $(INSTALL_ROOT)/lib/modules/*/$(KMODDIR) ';'

drivers_firmware: rootfs_prep
	@echo Build drivers firmware
	cd ${WLAN_DRIVER_TOPDIR} && ${MAKEARCH} firmware_install

drivers_firmware_transfer:
	@echo Transfer driver firmwares
	cd ${WLAN_DRIVER_TOPDIR} && ${MAKEARCH} firmware_toarray

drivers_clean:
	@echo Clean drivers
	cd ${WLAN_DRIVER_TOPDIR} && ${MAKEARCH} driver_clean

#
# wpa_supplicant build
#
wpa_supplicant: rootfs_prep wpa_supplicant_clean
	@echo Build wpa_supplicant
	cd $(WPA_SUPPLICANT_TOPDIR)/wpa_supplicant && \
	mkdir -p $(INSTALL_ROOT)/sbin/systemd $(INSTALL_ROOT)/sbin/dbus && \
	${MAKEARCH} CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	${MAKEARCH} DESTDIR=$(INSTALL_ROOT) BINDIR=/sbin LIBDIR=/lib install

wpa_supplicant_clean:
	@echo Clean wpa_supplicant
	cd $(WPA_SUPPLICANT_TOPDIR)/wpa_supplicant && ${MAKEARCH} clean
	
#
# wpa_supplicant_combo build
#
wpa_supplicant_combo: rootfs_prep wpa_supplicant_combo_clean
	@echo Build wpa_supplicant_combo
	cd $(WPA_SUPPLICANT_COMBO_TOPDIR)/wpa_supplicant && \
	mkdir -p $(INSTALL_ROOT)/sbin/systemd $(INSTALL_ROOT)/sbin/dbus && \
	${MAKEARCH} CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	${MAKEARCH} DESTDIR=$(INSTALL_ROOT) BINDIR=/sbin LIBDIR=/lib install

wpa_supplicant_combo_clean:
	@echo Clean wpa_supplicant_combo
	cd $(WPA_SUPPLICANT_COMBO_TOPDIR)/wpa_supplicant && ${MAKEARCH} clean
	
#
# hostapd build
#
hostapd: rootfs_prep hostapd_clean
	@echo Build wpa_supplicant
	cd $(WPA_SUPPLICANT_TOPDIR)/hostapd && \
	${MAKEARCH} CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	cp -f hostapd $(INSTALL_ROOT)/sbin && \
	cp -f hostapd_cli $(INSTALL_ROOT)/sbin

hostapd_clean:
	@echo Clean hostapd
	cd $(WPA_SUPPLICANT_TOPDIR)/hostapd && ${MAKEARCH} clean

#
# iw build
#
iw: rootfs_prep iw_clean
	@echo Build iw-3.0
	cd $(IW30_TOPDIR) && \
	${MAKEARCH} CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld && \
	${MAKEARCH} DESTDIR=$(INSTALL_ROOT) SBINDIR=/sbin install

iw_clean:
	@echo Clean iw-3.0
	cd $(IW30_TOPDIR) && ${MAKEARCH} clean

#
# libnl build
#
libnl_config:
	@echo Config libnl1-1
	cd $(LIBNL11_TOPDIR) && \
	./configure --host=$(TOOLPREFIX) CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld

libnl: libnl_config libnl_clean
	@echo Build libnl1-1
	cd $(LIBNL11_TOPDIR) && \
	${MAKEARCH}

libnl_clean:
	@echo Clean libnl1-1
	cd $(LIBNL11_TOPDIR) && ${MAKEARCH} clean


#
# wireless_tools build (test purpose)
#
wireless_tools_build: wireless_tools_clean
	@echo Build wireless_tools 
	cd ${WIRELESSTOOLS_TOPDIR} && \
	${MAKEARCH} CC=$(TOOLPREFIX)gcc AR=$(TOOLPREFIX)ar LD=$(TOOLPREFIX)ld BUILD_STATIC=y && \
	cp -f iwconfig $(INSTALL_ROOT)/sbin && \
	cp -f iwlist $(INSTALL_ROOT)/sbin

wireless_tools_clean:
	@echo Clean wireless_tools
	cd ${WIRELESSTOOLS_TOPDIR} && ${MAKEARCH} clean
