##
## rootfs/make.inc
##
## History:
##    2012/05/29 - [Cao Rongrong] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##

ifeq ($(CONFIG_AMBARELLA_MKFS), y)

ROOTFS_DIR		:= $(call my-dir)
PRE_BUSYBOX_DIR		:= $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/busybox
EXT_BUSYBOX_DIR		:= $(AMB_TOPDIR)/../external/busybox

ifeq ($(CONFIG_CPU_CORTEXA53), y)
ARM_ARCH		:= arm64
else
ARM_ARCH		:= arm
endif

####### Basic Filesystem #######

include $(ROOTFS_DIR)/ext_bb/make.inc

.PHONY: basic_fs

ifeq ($(CONFIG_AMBARELLA_BUSYBOX_BUILD), y)
basic_fs: build_extbb
else
basic_fs: prebuild_bb
endif

basic_fs: $(DOT_CONFIG)
ifeq ($(CONFIG_CPU_ARM1136JS), y)
	@cp -dpRf $(shell find $(SYS_LIB_DIR_ARMV6)/ -name "*.so" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(shell find $(SYS_LIB_DIR_ARMV6)/ -name "*.so.*" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_ARMV6)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_ARMV6)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
	@cp -dpRf $(shell find $(SYS_LIB_DIR_ARMV7_SF)/ -name "*.so" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(shell find $(SYS_LIB_DIR_ARMV7_SF)/ -name "*.so.*" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_ARMV7_SF)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_ARMV7_SF)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
else ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
	@cp -dpRf $(shell find $(SYS_LIB_DIR_ARMV7)/ -name "*.so" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(shell find $(SYS_LIB_DIR_ARMV7)/ -name "*.so.*" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_ARMV7)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_ARMV7)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
else ifeq ($(CONFIG_CPU_CORTEXA53), y)
	@cp -dpRf $(shell find $(SYS_LIB_DIR_AARCH64)/ -name "*.so" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(shell find $(SYS_LIB_DIR_AARCH64)/ -name "*.so.*" | grep -v stdc++) $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_AARCH64)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_AARCH64)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
endif
	@find $(FAKEROOT_DIR) -name "libgcc_s.so" | xargs rm -f
	@find $(FAKEROOT_DIR) -name "libstdc++.so.*.py" | xargs rm -f
	@rm -rf $(FAKEROOT_DIR)/lib/*.*a*
	@echo "export AMBARELLA_ARCH=$(AMBARELLA_ARCH)" > $(FAKEROOT_DIR)/etc/ambarella.conf
	@echo "export SYS_BOARD_BSP=$(AMB_BOARD)" >> $(FAKEROOT_DIR)/etc/ambarella.conf
	@echo "export SYS_BOARD_VERSION=$(BOARD_VERSION_STR)" >> $(FAKEROOT_DIR)/etc/ambarella.conf
	@chmod 644 $(FAKEROOT_DIR)/etc/ambarella.conf
ifneq ($(BUIDL_AMBARELLA_APP_DEBUG_SANITIZE_ADDRESS), y)
	@rm -rf $(FAKEROOT_DIR)/lib/libubsan.*
	@rm -rf $(FAKEROOT_DIR)/lib/libasan.*
endif
	@echo "Prepare BasicFS Done"

$(call add-target-into-build, basic_fs)


####### Prepare to add root password  #######
ifneq ($(CONFIG_AMBARELLA_ROOT_PASSWORD),"")
.PHONY: add_root_password
add_root_password: basic_fs
	@python $(ROOTFS_DIR)/bin/create_rootpass.py $(CONFIG_AMBARELLA_ROOT_PASSWORD) $(FAKEROOT_DIR);

$(call add-target-into-build, add_root_password)
endif

####### Prepare to add default user with password  #######
ifeq ($(CONFIG_AMBARELLA_NORMAL_USER), y)
.PHONY: add_normal_user_password
add_normal_user_password: basic_fs
	@python $(ROOTFS_DIR)/bin/add_normal_user.py $(CONFIG_AMBARELLA_NORMAL_USER_NAME) \
	$(CONFIG_AMBARELLA_NORMAL_USER_PASSWORD) $(FAKEROOT_DIR);
$(call add-target-into-build, add_normal_user_password)
endif

####### Update /etc/inittab Configuration #######

.PHONY: configure_inittab

configure_inittab: basic_fs add_board_rootfs
	@if [ -e $(BOARD_ROOTFS_DIR)/etc/inittab ]; then \
		cp $(BOARD_ROOTFS_DIR)/etc/inittab $(FAKEROOT_DIR)/etc/inittab; \
	else \
		cp -dpRf $(PRE_BUSYBOX_DIR)/etc/inittab $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_AUTO_TELNETD)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Start telnetd" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/sbin/telnetd" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_TFTPD_SUPPORT)" = "y" ]; then \
		echo "21 stream tcp nowait root ftpd ftpd -w /" > $(FAKEROOT_DIR)/etc/inetd.conf; \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Start inetd" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/sbin/inetd" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_SERIAL_PORT0_GETTY)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Put a getty on the serial port 0" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_SERIAL_PORT1_GETTY)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Put a getty on the serial port 1" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS1::respawn:/sbin/getty -L ttyS1 115200 vt100" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_ASKFIRST_CONSOLE)" = "y" ]; then \
		echo "::askfirst:-/bin/sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_SERIAL_PORT_USB0)" = "y" ]; then \
		echo "export SYS_USB_G_TYPE=serial" >> $(FAKEROOT_DIR)/etc/ambarella.conf; \
		echo "export SYS_USB_G_PARAMETER=use_acm=1" >> $(FAKEROOT_DIR)/etc/ambarella.conf; \
		echo "ttyGS0::respawn:/sbin/getty -L ttyGS0 115200 vt100" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_START_WATCHDOG)" = "y" ] && [ "$(BUILD_AMBARELLA_ORYX_MW)" != "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Start watchdog" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/sbin/watchdog -t 5 /dev/watchdog" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_AUTO_DEMO)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run auto demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/local/bin/demo.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_AUTO_CLOUD_DEMO)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run ambarella cloud agent" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/bin/apps_launcher &" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/local/bin/amba_cloud_agent --disableaudio --enableencryption --stream 0" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_SHMOO_TEST)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run shmoo test" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/shmoo.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_MEDIASERVER)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run mediaserver demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/mediaserver.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_DEWARP)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run fisheye dewarp demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/fisheye_dewarp.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_IVAAPP)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run ivaapp" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/ivaapp.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(BUILD_AMBARELLA_APP_IPCAM_WEB_CGI_PARTNER_Y)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run FastCGI" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/bin/spawn-fcgi -f /webSvr/web/cgi-bin/v1/api_router.cgi -s /tmp/fcgi.socket" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@echo "Configure inittab Done"

$(call add-target-into-build, configure_inittab)


####### Update /etc/fstab Configuration #######

.PHONY: configure_fstab

configure_fstab: basic_fs
	@cp -dpRf $(PRE_BUSYBOX_DIR)/etc/fstab $(FAKEROOT_DIR)/etc/fstab
	@if [ -n "$(CONFIG_AMBARELLA_MOUNT_MNT)" ]; then \
		echo "# NFS configuration for ttyS0" >> $(FAKEROOT_DIR)/etc/fstab; \
		echo "$(CONFIG_AMBARELLA_MOUNT_MNT)" >> $(FAKEROOT_DIR)/etc/fstab; \
	fi
	@echo "Configure fstab Done"

$(call add-target-into-build, configure_fstab)


####### Update /etc/profile Configuration #######

.PHONY: configure_profile

configure_profile: basic_fs
	@cp -dpRf $(PRE_BUSYBOX_DIR)/etc/profile $(FAKEROOT_DIR)/etc/profile
	@if [ "$(CONFIG_AMBARELLA_DBUS_SUPPORT)" = "y" ]; then \
		echo "if [ -f /tmp/dbus/dbus-session-bus-address ]; then . /tmp/dbus/dbus-session-bus-address; fi" \
			>> $(FAKEROOT_DIR)/etc/profile; \
	fi
	@if [ "$(CONFIG_AMBARELLA_QT_SUPPORT)" = "y" ]; then \
		echo "export QWS_MOUSE_PROTO=Tslib:/dev/input/event1" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export QWS_KEYBOARD=linuxinput:/dev/input/event0:keymap=/usr/share/qt4/keymap/A5s.qmap" \
			>> $(FAKEROOT_DIR)/etc/profile; \
		echo "export QWS_DISPLAY=ambafb:csc:/dev/fb0" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_TSDEVICE=/dev/input/event1" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_CONFFILE=/etc/ts.conf" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_PLUGINDIR=/usr/lib/ts" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_CALIBFILE=/etc/pointercal" >> $(FAKEROOT_DIR)/etc/profile; \
	fi
	@echo "if [ -f /etc/TZ ]; then export TZ=\`cat /etc/TZ\`; fi" >> $(FAKEROOT_DIR)/etc/profile
	@if [ "$(CONFIG_AMBARELLA_INIT_SYSTEMD)" = "y" ]; then \
		echo "if [ -f /etc/ambarella.conf ]; then . /etc/ambarella.conf; fi" >> $(FAKEROOT_DIR)/etc/profile; \
	fi
	@echo "Configure profile Done"

$(call add-target-into-build, configure_profile)


####### Configure Network #######

.PHONY: configure_network

configure_network: add_board_rootfs
	@NETCONF=$(FAKEROOT_DIR)/etc/network/interfaces; \
	cp -dpRf $(PRE_BUSYBOX_DIR)/etc/network/interfaces $(FAKEROOT_DIR)/etc/network/interfaces; \
	if [ "$(CONFIG_AMBARELLA_ETH0)" = "y" ]; then \
		echo "" >> $$NETCONF; \
		echo "auto eth0" >> $$NETCONF; \
		if [ "$(CONFIG_AMBARELLA_ETH0_DHCP)" = "y" ]; then \
			echo "iface eth0 inet dhcp" >> $$NETCONF; \
		else \
			if [ -n "$(CONFIG_AMBARELLA_ETH0_ADDRESS)" -a -n "$(CONFIG_AMBARELLA_ETH0_IP_MASK)" ]; then \
				echo "iface eth0 inet static" >> $$NETCONF; \
				echo "address $(CONFIG_AMBARELLA_ETH0_ADDRESS)" >> $$NETCONF; \
				echo "netmask $(CONFIG_AMBARELLA_ETH0_IP_MASK)" >> $$NETCONF; \
				if [ -n $(CONFIG_AMBARELLA_ETH0_GW_ADDRESS) ]; then \
					echo "gateway $(CONFIG_AMBARELLA_ETH0_GW_ADDRESS)" >> $$NETCONF; \
				fi \
			fi \
		fi \
	fi; \
	if [ "$(CONFIG_AMBARELLA_ETH1)" = "y" ]; then \
		echo "" >> $$NETCONF; \
		echo "auto eth1" >> $$NETCONF; \
		if [ "$(CONFIG_AMBARELLA_ETH1_DHCP)" = "y" ]; then \
			echo "iface eth1 inet dhcp" >> $$NETCONF; \
		else \
			if [ -n "$(CONFIG_AMBARELLA_ETH1_ADDRESS)" -a -n "$(CONFIG_AMBARELLA_ETH1_IP_MASK)" ]; then \
				echo "iface eth1 inet static" >> $$NETCONF; \
				echo "address $(CONFIG_AMBARELLA_ETH1_ADDRESS)" >> $$NETCONF; \
				echo "netmask $(CONFIG_AMBARELLA_ETH1_IP_MASK)" >> $$NETCONF; \
				if [ -n $(CONFIG_AMBARELLA_ETH1_GW_ADDRESS) ]; then \
					echo "gateway $(CONFIG_AMBARELLA_ETH1_GW_ADDRESS)" >> $$NETCONF; \
				fi \
			fi \
		fi \
	fi; \
	if [ "$(CONFIG_AMBARELLA_ETH0)" != "y" -a "$(CONFIG_AMBARELLA_ETH1)" != "y" -o "$(CONFIG_AMBARELLA_NETWORKMANAGER_SUPPORT)" = "y" -o "$(CONFIG_AMBARELLA_CONNMAN_SUPPORT)" = "y" ]; then \
		rm -rf $(FAKEROOT_DIR)/etc/init.d/S40network; \
		if [ "$(CONFIG_AMBARELLA_CONNMAN_SUPPORT)" = "y" ]; then \
			rm -rf $(FAKEROOT_DIR)/etc/network; \
		fi \
	fi
	@echo "Configure network Done"

$(call add-target-into-build, configure_network)

####### board default rootfs #######

.PHONY: add_board_rootfs

ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
add_board_rootfs: prebuild_udev
endif
ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
add_board_rootfs: basic_fs prebuild_systemd
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC_FASTBOOT), y)
ifeq ($(BUILD_AMBARELLA_PRIVATE_MODULES), y)
add_board_rootfs: prebuild_firmware
endif
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC_SMALLBOOT), y)
add_board_rootfs: basic_fs
endif
ifeq ($(CONFIG_AMBARELLA_ALSA_SUPPORT), y)
add_board_rootfs: prebuild_alsa
endif
ifeq ($(CONFIG_AMBARELLA_NETWORKMANAGER_SUPPORT), y)
add_board_rootfs: prebuild_networkmanager
endif
ifeq ($(CONFIG_AMBARELLA_PULSEAUDIO_SUPPORT), y)
add_board_rootfs: prebuild_pulseaudio
endif

ifeq ($(BUILD_AMBARELLA_ORYX_MW), y)
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_AUDIO), y)
add_board_rootfs: audio_svc
endif
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_VIDEO), y)
add_board_rootfs: video_svc
endif
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_MEDIA), y)
add_board_rootfs: media_svc
endif
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_EVENT), y)
add_board_rootfs: event_svc
endif
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_IMAGE), y)
add_board_rootfs: img_svc
endif
ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_SYSTEM), y)
add_board_rootfs: sys_svc
endif
endif

add_board_rootfs:
	@cp -dpRf $(BOARD_ROOTFS_DIR)/* $(FAKEROOT_DIR)/ && chmod +x $(FAKEROOT_DIR)/etc/init.d/S*
ifeq ($(CONFIG_AMBARELLA_PULSEAUDIO_SUPPORT), y)
	@[ -e $(FAKEROOT_DIR)/etc/pulse ] && chmod 644 $(FAKEROOT_DIR)/etc/pulse/*
	@[ -e $(FAKEROOT_DIR)/usr/share/pulseaudio ] && find $(FAKEROOT_DIR)/usr/share/pulseaudio -name "*.conf" | xargs chmod 644
	@echo "tmpfs              /var/lib/pulse  tmpfs      defaults                0      0" >> $(FAKEROOT_DIR)/etc/fstab
endif
ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
	@rm -rf $(FAKEROOT_DIR)/etc/init.d/S*
	@chmod 644 $(FAKEROOT_DIR)/etc/systemd/*.conf
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd
	@rm -rf $(FAKEROOT_DIR)/usr/lib/tmpfiles.d
	@rm -rf $(FAKEROOT_DIR)/etc/systemd
	@rm -rf $(FAKEROOT_DIR)/etc/tmpfiles.d
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC_FASTBOOT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd
	@rm -rf $(FAKEROOT_DIR)/etc/systemd
	@rm -f $(FAKEROOT_DIR)/etc/modprobe.d/ambarella.conf
	@rm -f $(FAKEROOT_DIR)/lib/libstdc++.*
#	@rm -f $(FAKEROOT_DIR)/usr/lib/libstdc++.*
	@cp -rf $(BOARD_ROOTFS_FASTBOOT_DIR)/* $(FAKEROOT_DIR)/ && chmod +x $(FAKEROOT_DIR)/etc/init.d/S*
	@cp -rf $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/speex/usr/lib/libspeexdsp.* $(FAKEROOT_DIR)/usr/lib/
	@cp -rf $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/libg7xx/usr/lib/libg7xx.* $(FAKEROOT_DIR)/usr/lib/
ifeq ($(BUILD_AMBARELLA_PRIVATE_MODULES), y)
ifeq ($(AMBOOT_BOOT_DSP), y)
ifneq ($(AMBOOT_DSP_LOG_CAPTURE), y)
	@rm -f $(FAKEROOT_DIR)/lib/firmware/default_binary.bin
	@rm -f $(FAKEROOT_DIR)/lib/firmware/orccode.bin
	@rm -f $(FAKEROOT_DIR)/lib/firmware/orcme.bin
endif
endif
endif
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IAV), y)
	@rm -f $(FAKEROOT_DIR)/data/font_index.bin
	@rm -f $(FAKEROOT_DIR)/data/font_map.bin
	@rm -f $(FAKEROOT_DIR)/data/clut.bin
endif
	@echo "export IDSP_RAM_START=$(IDSP_RAM_START)" >> $(FAKEROOT_DIR)/etc/ambarella.conf
	@echo "export DSP_BSB_SIZE=$(DSP_BSB_SIZE)" >> $(FAKEROOT_DIR)/etc/ambarella.conf

ifeq ($(CONFIG_S2LMKIWI_SENSOR_OV9710), y)
	@echo "export SYS_BOARD_SENSOR=ov9710" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_S2LMKIWI_SENSOR_AR0230), y)
	@echo "export SYS_BOARD_SENSOR=ar0230" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_S2LMKIWI_SENSOR_AR0230_WDR_IN), y)
	@echo "export SYS_BOARD_SENSOR=ar0230_wdr_in" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_S2LMKIWI_SENSOR_AR0230_HDR_2X), y)
	@echo "export SYS_BOARD_SENSOR=ar0230_hdr_2x" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_S2LMKIWI_SENSOR_OV4689), y)
	@echo "export SYS_BOARD_SENSOR=ov4689" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_MANGO_SENSOR_OV4689), y)
	@echo "export SYS_BOARD_SENSOR=ov4689" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_MANGO_SENSOR_IMX322), y)
	@echo "export SYS_BOARD_SENSOR=imx322" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_OLIVE_SENSOR_OV4689), y)
	@echo "export SYS_BOARD_SENSOR=ov4689" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_OLIVE_SENSOR_IMX322), y)
	@echo "export SYS_BOARD_SENSOR=imx322" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_ELEKTRAS_SENSOR_IMX322), y)
	@echo "export SYS_BOARD_SENSOR=imx322" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_BOARD_VERSION_S2LMELEKTRA_OV4689_S2L22M), y)
	@echo "export SYS_BOARD_SENSOR=ov4689" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_BOARD_VERSION_S2LMELEKTRA_OV4689_S2L55M), y)
	@echo "export SYS_BOARD_SENSOR=ov4689" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L22M), y)
	@echo "export SYS_BOARD_SENSOR=imx322" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else ifeq ($(CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L55M), y)
	@echo "export SYS_BOARD_SENSOR=imx322" >> $(FAKEROOT_DIR)/etc/ambarella.conf
endif
ifeq ($(CONFIG_S2LMIRONMAN_FASTBOOT_CES_DEMO), y)
	@echo "export FASTBOOT_CES_DEMO=y" >> $(FAKEROOT_DIR)/etc/ambarella.conf
	@mkdir -p $(FAKEROOT_DIR)/tmp/demo
	@echo "ap_ssid=$(CONFIG_S2LMIRONMAN_AP_SSID)" > $(FAKEROOT_DIR)/tmp/demo/ap.conf
	@echo "ip_addr=$(CONFIG_S2LMIRONMAN_WLAN0_ADDRESS)" >> $(FAKEROOT_DIR)/tmp/demo/ap.conf
	@echo "http_url=$(CONFIG_S2LMIRONMAN_HTTP_URL)" >> $(FAKEROOT_DIR)/tmp/demo/ap.conf
endif
ifeq ($(CONFIG_S2LMELEKTRA_ENABLE_UART_COMMUNICATION_MCU), y)
	@echo "export S2LMELEKTRA_ENABLE_UART_COMMUNICATION_MCU=1" >> $(FAKEROOT_DIR)/etc/ambarella.conf
endif
ifeq ($(CONFIG_S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE), y)
	@echo "export S2LMELEKTRA_ENABLE_ADVANCED_ISO_MODE=1" >> $(FAKEROOT_DIR)/etc/ambarella.conf
endif

ifeq ($(CONFIG_BSP_BOARD_S2LM_ELEKTRA), y)
ifeq ($(CONFIG_S2LMELEKTRA_VCA_IN_LINUX), y)
	@echo "export VCA_MODE=enable" >> $(FAKEROOT_DIR)/etc/ambarella.conf
endif
ifeq ($(CONFIG_S2LMELEKTRA_BUILD_THAW_HIBERNATION), y)
	@echo "export ELEKTRA_FASTBOOT_MODE=fastboot_hibernation" >> $(FAKEROOT_DIR)/etc/ambarella.conf
else
	@echo "export ELEKTRA_FASTBOOT_MODE=fastboot_normal" >> $(FAKEROOT_DIR)/etc/ambarella.conf
endif
endif

endif
ifneq ($(BUILD_AMBARELLA_ORYX_MW), y)
	@rm -rf $(FAKEROOT_DIR)/etc/oryx
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC_SMALLBOOT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd
	@rm -rf $(FAKEROOT_DIR)/etc/systemd
	@rm -f $(FAKEROOT_DIR)/lib/libasan.so.1.0.0
	@rm -f $(FAKEROOT_DIR)/lib/libgomp.so.1.0.0
	@rm -f $(FAKEROOT_DIR)/lib/libitm.so.1.0.0
	@rm -f $(FAKEROOT_DIR)/lib/libssp.so.0.0.0
	@rm -f $(FAKEROOT_DIR)/lib/libthread_db-1.0.so
	@rm -f $(FAKEROOT_DIR)/lib/libubsan.so.0.0.0
	@rm -f $(FAKEROOT_DIR)/lib/libnss_db-2.19-2014.06.so
	@rm -f $(FAKEROOT_DIR)/lib/libmemusage.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/libstdc++.*
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_evhost.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_simple_vhost.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_userdir.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_access.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_cml.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_expire.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_magnet.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_rrdtool.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_ssi.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_usertrack.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_alias.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_compress.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_extforward.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_mysql_vhost.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_scgi.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_webdav.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_proxy.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_secdownload.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_status.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_evasive.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_flv_streaming.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_redirect.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_setenv.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/lighttpd/mod_trigger_b4_dl.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/libpcrecpp.so*
	@rm -f $(FAKEROOT_DIR)/usr/lib/libpcreposix.so*
	@rm -f $(FAKEROOT_DIR)/usr/lib/libpanel.so*
	@rm -f $(FAKEROOT_DIR)/usr/lib/libncurses.so*
	@rm -f $(FAKEROOT_DIR)/usr/lib/libmenu.so*
	@rm -f $(FAKEROOT_DIR)/usr/lib/libform.so*
	@rm -rf $(FAKEROOT_DIR)/usr/lib/engines
	@rm -f $(FAKEROOT_DIR)/usr/bin/openssl
	@rm -f $(FAKEROOT_DIR)/usr/bin/c_rehash
	@rm -f $(FAKEROOT_DIR)/usr/bin/c_rehash
	@rm -rf $(FAKEROOT_DIR)/etc/pki
	@rm -rf $(FAKEROOT_DIR)/etc/idsp/sensors/imx*.bin
	@rm -rf $(FAKEROOT_DIR)/etc/idsp/sensors/mn*.bin
	@rm -rf $(FAKEROOT_DIR)/etc/idsp/sensors/ov5658*.bin
	@rm -rf $(FAKEROOT_DIR)/etc/idsp/sensors/ov9718*.bin
	@find $(FAKEROOT_DIR) -name ".config" | xargs rm -f
endif
	@echo "Copy board's rootfs Done"

$(call add-target-into-build, add_board_rootfs)

####### Remove init.sh when board type is xman cloudcam peard #######

.PHONY: remove_config

remove_config: add_board_rootfs
	@if [ "$(CONFIG_BSP_BOARD_CLOUDCAM)" = "y" ]; then \
		rm -f $(FAKEROOT_DIR)/usr/local/bin/init.sh; \
	fi
	@if [ "$(CONFIG_BSP_BOARD_XMAN)" = "y" ]; then \
		rm -f $(FAKEROOT_DIR)/usr/local/bin/init.sh; \
	fi
	@if [ "$(CONFIG_BSP_BOARD_PEAR)" = "y" ]; then \
		rm -f $(FAKEROOT_DIR)/usr/local/bin/init.sh; \
	fi

$(call add-target-into-build, remove_initsh)

####### Rules to Build Rootfs Image  #######

# Nand Page/Block Size
MTD_PAGESIZE		:= 0x0200
MTD_PEBSIZE		:= 0x4000
UBI_LEBSIZE		:= 0x3C00
UBI_SUBPAGESIZE	:= 0x0200
UBIFSOUTFLAG		:= -F -o

ifeq ($(CONFIG_NAND_P512_E16K), y)
MTD_PAGESIZE		:= 0x0200
MTD_PEBSIZE		:= 0x4000
UBI_LEBSIZE		:= 0x3C00
endif
ifeq ($(CONFIG_NAND_P2K_E64K), y)
MTD_PAGESIZE		:= 0x00800
MTD_PEBSIZE		:= 0x10000
UBI_LEBSIZE		:= 0x0F000
UBI_SUBPAGESIZE	:= 0x0800
endif
ifeq ($(CONFIG_NAND_P2K_E128K), y)
MTD_PAGESIZE		:= 0x00800
MTD_PEBSIZE		:= 0x20000
UBI_LEBSIZE		:= 0x1F000
UBI_SUBPAGESIZE	:= 0x0800
endif
ifeq ($(CONFIG_NAND_P4K_E64K), y)
MTD_PAGESIZE		:= 0x01000
MTD_PEBSIZE		:= 0x10000
UBI_LEBSIZE		:= 0x0E000
UBI_SUBPAGESIZE	:= 0x01000
endif
ifeq ($(CONFIG_NAND_P4K_E128K), y)
MTD_PAGESIZE		:= 0x01000
MTD_PEBSIZE		:= 0x20000
UBI_LEBSIZE		:= 0x1E000
UBI_SUBPAGESIZE	:= 0x01000
endif
ifeq ($(CONFIG_SPINOR_P512_E256K), y)
MTD_PAGESIZE		:= 0x0200
MTD_PEBSIZE		:= 0x0100
UBI_LEBSIZE		:= 0x3ff80
UBI_SUBPAGESIZE		:= 0x0001
endif
ifeq ($(CONFIG_SPINOR_P256_E64K), y)
MTD_PAGESIZE		:= 0x0100
MTD_PEBSIZE		:= 0x0040
UBI_LEBSIZE		:= 0x3ff80
UBI_SUBPAGESIZE		:= 0x0001
endif
ifeq ($(CONFIG_SPINAND_P2K_E128K), y)
MTD_PAGESIZE		:= 0x00800
MTD_PEBSIZE		:= 0x20000
UBI_LEBSIZE		:= 0x1F000
UBI_SUBPAGESIZE		:= 0x0800
endif

# Build Image

.PHONY: build_fsimage_common build_fsimage

build_fsimage:

build_fsimage_common: $(DOT_CONFIG)
	@echo "  Build $(PRIVATE_FS_TYPE) ..."
	@mkdir -p $(ROOTFS_OUT_DIR)
	@if [ -n "$(CONFIG_AMBARELLA_EXT_CMD_FILE)" ]; then \
		if [ -n "" ]; then \
			cp -dpRf $(CONFIG_AMBARELLA_EXT_CMD_FILE) $(ROOTFS_OUT_DIR)/.tmp_ext_cmd; \
			sed 's/$$(AMBABUILD_TOPDIR)/$(AMBABUILD_STR)/g' $(ROOTFS_OUT_DIR)/.tmp_ext_cmd \
					> $(ROOTFS_OUT_DIR)/.ext_cmd; \
			sed 's/$$(FAKEROOT_DIR)/$(FAKEROOT_STR)/g' $(ROOTFS_OUT_DIR)/.ext_cmd \
					> $(ROOTFS_OUT_DIR)/.tmp_ext_cmd; \
			sed 's/$$(ROOTFS_DIR)/$(ROOTFS_STR)/g' $(ROOTFS_OUT_DIR)/.tmp_ext_cmd \
					> $(ROOTFS_OUT_DIR)/.ext_cmd; \
			sed 's/$$(BSP_DIR)/$(AMB_BOARD)/g' $(ROOTFS_OUT_DIR)/.ext_cmd \
					> $(ROOTFS_OUT_DIR)/.tmp_ext_cmd; \
			sed 's/$$(BOARD_VERSION)/$(BOARD_VERSION_STR)/g' $(ROOTFS_OUT_DIR)/.tmp_ext_cmd \
					> $(ROOTFS_OUT_DIR)/.ext_cmd; \
			chmod +x $(ROOTFS_OUT_DIR)/.ext_cmd; \
			sh $(ROOTFS_OUT_DIR)/.ext_cmd; \
		fi; \
	fi
	@find $(FAKEROOT_DIR) -type d | xargs chmod 755
	@find $(FAKEROOT_DIR) -type f -name *.so | xargs chmod 755
	@find $(FAKEROOT_DIR) -type f -name *.so.* | xargs chmod 755
	@for l in `find $(FAKEROOT_DIR)/usr/lib -type l -name "lib*.so*" | sort`; do \
		if readlink -e $${l} > /dev/null; then \
			lname=`basename $${l}`; \
			lsoname=`$(CROSS_COMPILE)objdump -p $${l} | grep SONAME | awk '{print $$2}'`; \
			if [ x"$${lsoname}" != x -a "$${lname}" != "$${lsoname}" ]; then \
				rm -rf $${l}; \
			fi; \
		else \
			rm -rf $${l}; \
		fi; \
	done
	@for l in `find $(FAKEROOT_DIR)/lib -type l -name "lib*.so*" | sort`; do \
		if readlink -e $${l} > /dev/null; then \
			lname=`basename $${l}`; \
			lsoname=`$(CROSS_COMPILE)objdump -p $${l} | grep SONAME | awk '{print $$2}'`; \
			if [ x"$${lsoname}" != x -a "$${lname}" != "$${lsoname}" ]; then \
				rm -rf $${l}; \
			fi; \
		else \
			rm -rf $${l}; \
		fi; \
	done
ifneq ($(BUILD_AMBARELLA_APP_DEBUG), y)
	@find $(FAKEROOT_DIR) -type f -executable -exec sh -c "file -i '{}' | grep -q 'x-sharedlib; charset=binary'" \; -print | xargs $(CROSS_COMPILE)strip -s
	@find $(FAKEROOT_DIR) -type f -executable -exec sh -c "file -i '{}' | grep -q 'x-executable; charset=binary'" \; -print | grep -v gdb | xargs $(CROSS_COMPILE)strip -s
endif
	@rm -rf $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@touch $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "chown -R 0:0 $(FAKEROOT_DIR)" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh


ifeq ($(CONFIG_AMBARELLA_ROOTFS_UBIFS), y)
.PHONY: build_ubifs
build_ubifs: PRIVATE_FS_TYPE:=UBIFS
build_ubifs: build_fsimage_common
	@echo "cp -dpRf $(ROOTFS_DIR)/ubicfg.ini $(ROOTFS_OUT_DIR)/ubicfg.ini " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(MKUBIFS) -r $(FAKEROOT_DIR) \
		-m $(MTD_PAGESIZE) -e $(UBI_LEBSIZE) -c 2047 $(UBIFSOUTFLAG) $(ROOTFS_OUT_DIR)/ubifs.img" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(UBINIZE) -o $(ROOTFS_OUT_DIR)/ubifs \
		-p $(MTD_PEBSIZE) -m $(MTD_PAGESIZE) -s $(UBI_SUBPAGESIZE) $(ROOTFS_OUT_DIR)/ubicfg.ini" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ubifs
endif

ifeq ($(CONFIG_AMBARELLA_ROOTFS_SQUASHFS), y)
.PHONY: build_squashfs

build_squashfs: PRIVATE_FS_TYPE:=SQUASHFS
build_squashfs: build_fsimage_common
# Build ubifs for ADD partition on fastboot mode
ifeq ($(CONFIG_AMBARELLA_ADD_PARTITION), y)
	@echo "cp -dpRf $(ROOTFS_DIR)/ubicfg_fastboot.ini $(ROOTFS_OUT_DIR)/ubicfg_fastboot.ini " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@mkdir -p $(FAKEROOT_DIR)/tmp/config
	@touch $(FAKEROOT_DIR)/tmp/config/resolv.conf
	@rm -fr $(FAKEROOT_DIR)/etc/resolv.conf
	@ln -s /tmp/config/resolv.conf $(FAKEROOT_DIR)/etc/resolv.conf
	@echo "CST-8" > $(FAKEROOT_DIR)/tmp/config/TZ
	@rm -fr $(FAKEROOT_DIR)/etc/TZ
	@ln -s /tmp/config/TZ $(FAKEROOT_DIR)/etc/TZ

	@mkdir -p $(FAKEROOT_DIR)/tmp/config/calibration
	@mkdir -p $(FAKEROOT_DIR)/ambarella
	@rm -rf $(FAKEROOT_DIR)/ambarella/calibration
	@ln -s /tmp/config/calibration $(FAKEROOT_DIR)/ambarella/calibration

	@echo "$(MKUBIFS) -r $(FAKEROOT_DIR)/tmp/config \
		-m $(MTD_PAGESIZE) -e $(UBI_LEBSIZE) -c 2047 $(UBIFSOUTFLAG) $(ROOTFS_OUT_DIR)/ubifs_fastboot.img" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(UBINIZE) -o $(ROOTFS_OUT_DIR)/ubifs \
		-p $(MTD_PEBSIZE) -m $(MTD_PAGESIZE) -s $(UBI_SUBPAGESIZE) $(ROOTFS_OUT_DIR)/ubicfg_fastboot.ini" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
endif
	@cp -rf $(ROOTFS_DIR)/squashfs_table.txt $(FAKEROOT_DIR)/etc
	@echo "udev /dev devtmpfs rw,mode=0755 0 0" >> $(FAKEROOT_DIR)/etc/fstab
	@echo "if [ -e $(ROOTFS_OUT_DIR)/squashfs ];then rm -f $(ROOTFS_OUT_DIR)/squashfs; fi" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(MKSQUASHFS)  $(FAKEROOT_DIR) $(ROOTFS_OUT_DIR)/squashfs -no-xattrs -root-owned -noappend -comp lzo" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@rm -f $(FAKEROOT_DIR)/etc/squashfs_table.txt
	@sed -i "/devtmpfs/d" $(FAKEROOT_DIR)/etc/fstab

build_fsimage: build_squashfs
endif

ifeq ($(CONFIG_AMBARELLA_ROOTFS_JFFS2), y)

.PHONY: build_jffs2

build_jffs2: PRIVATE_FS_TYPE:=JFFS2
build_jffs2: build_fsimage_common
ifeq ($(CONFIG_BOOT_MEDIA_NAND), y)
	@echo "$(MKJFFS2) -p --root=$(FAKEROOT_DIR) \
		-e $(MTD_PEBSIZE) -s $(MTD_PAGESIZE) -o $(ROOTFS_OUT_DIR)/jffs2 -l -n -m priority" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
endif
ifeq ($(CONFIG_BOOT_MEDIA_SPINOR), y)
	@echo "$(MKJFFS2) -p --root=$(FAKEROOT_DIR) \
		-e $(MTD_PEBSIZE) -s $(MTD_PAGESIZE) -o $(ROOTFS_OUT_DIR)/jffs2" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
endif
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_jffs2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_YAFFS), y)

.PHONY: build_yaffs2

build_yaffs2: PRIVATE_FS_TYPE:=YAFFS2
build_yaffs2: build_fsimage_common
	echo "shfos"
	@echo "$(MKYAFFS2) $(FAKEROOT_DIR) $(ROOTFS_OUT_DIR)/yaffs2" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_yaffs2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_CPIO), y)

.PHONY: build_cpio

build_cpio: PRIVATE_FS_TYPE:=CPIO
build_cpio: build_fsimage_common
	@echo "if [ -e $(ROOTFS_OUT_DIR)/subjffs2dir ];then rm -rf $(ROOTFS_OUT_DIR)/subjffs2dir; fi" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "if [ -e $(ROOTFS_OUT_DIR)/rootfs.cpio.lzma ];then rm -f $(ROOTFS_OUT_DIR)/rootfs.cpio.lzma; fi" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
ifeq ($(CONFIG_AMBARELLA_ROOTFS_SUBJFFS2), y)
	@echo "echo '/dev/mtdblock4	   /mnt   	       jffs2      defaults		          0	     0' >> $(FAKEROOT_DIR)/etc/fstab" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
endif
	@echo "$(MAKEDEVS) -d $(ROOTFS_DIR)/device_table.txt $(FAKEROOT_DIR)" \
			>> $(ROOTFS_OUT_DIR)/_fakeroot.sh;
	@echo "cd $(FAKEROOT_DIR) && find . | cpio --quiet -o -H newc \
		> $(ROOTFS_OUT_DIR)/rootfs.cpio" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@ln -s -f sbin/init $(FAKEROOT_DIR)/init
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh
ifeq ($(CONFIG_BOOT_MEDIA_SPINOR), y)
	@if [ "$(CONFIG_AMBARELLA_COMPRESS_RAMDISK)" = "y" ]; then \
		lzma $(ROOTFS_OUT_DIR)/rootfs.cpio; \
	else \
		mv $(ROOTFS_OUT_DIR)/rootfs.cpio $(ROOTFS_OUT_DIR)/rootfs.cpio.lzma; \
	fi
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		CONFIG_INITRAMFS_SOURCE="$(ROOTFS_OUT_DIR)/rootfs.cpio.lzma" \
		CONFIG_INITRAMFS_ROOT_UID=0 CONFIG_INITRAMFS_ROOT_GID=0 all
endif
ifeq ($(CONFIG_BOOT_MEDIA_NAND), y)
	@if [ "$(CONFIG_AMBARELLA_COMPRESS_RAMDISK)" = "y" ]; then \
		gzip -f -9 $(ROOTFS_OUT_DIR)/rootfs.cpio; \
	else \
		mv $(ROOTFS_OUT_DIR)/rootfs.cpio $(ROOTFS_OUT_DIR)/rootfs.cpio.gz; \
	fi
	@$(MAKE) $(AMBA_MAKE_PARA) ARCH=$(CPU_ARCH) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		CONFIG_INITRAMFS_SOURCE="$(ROOTFS_OUT_DIR)/rootfs.cpio.gz" \
		CONFIG_INITRAMFS_ROOT_UID=0 CONFIG_INITRAMFS_ROOT_GID=0 all
endif
ifeq ($(CONFIG_CPU_CORTEXA53), y)
	@cp -dpRf $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image.gz $(KERNEL_OUT_DIR)/zImage
else
	@cp -dpRf $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage $(KERNEL_OUT_DIR)/zImage
endif
	@cp -dpRf $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image $(KERNEL_OUT_DIR)/Image

build_fsimage: build_cpio
endif

ifeq ($(CONFIG_AMBARELLA_ROOTFS_SUBJFFS2), y)

.PHONY: build_subjffs2

build_subjffs2: PRIVATE_FS_TYPE:=JFFS2
build_subjffs2: build_fsimage_common
ifeq ($(CONFIG_BOOT_MEDIA_NAND), y)
	@echo "$(MKJFFS2) -p --root=$(ROOTFS_OUT_DIR)/subjffs2dir \
		-e $(MTD_PEBSIZE) -s $(MTD_PAGESIZE) -o $(ROOTFS_OUT_DIR)/subjffs2 -l -n -m priority" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
endif
ifeq ($(CONFIG_BOOT_MEDIA_SPINOR), y)
	@echo "mkdir -p subjffs2dir/customer; $(MKJFFS2) -p --root=$(ROOTFS_OUT_DIR)/subjffs2dir \
		-e $(MTD_PEBSIZE) -s $(MTD_PAGESIZE) -o $(ROOTFS_OUT_DIR)/subjffs2" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
endif
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_subjffs2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_RAM), y)

.PHONY: build_ramdisk

build_ramdisk: PRIVATE_FS_TYPE:= RAM DISK
build_ramdisk: build_fsimage_common
	@echo "$(MAKEDEVS) -d $(ROOTFS_DIR)/device_table.txt $(FAKEROOT_DIR)" \
			>> $(ROOTFS_OUT_DIR)/_fakeroot.sh;
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) -d $(FAKEROOT_DIR) \
		-U -m 0 $(ROOTFS_OUT_DIR)/ramdisk" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@if [ "$(CONFIG_AMBARELLA_COMPRESS_RAMDISK)" = "y" ]; then \
		gzip -f -9 $(ROOTFS_OUT_DIR)/ramdisk; \
	else \
		mv $(ROOTFS_OUT_DIR)/ramdisk $(ROOTFS_OUT_DIR)/ramdisk.gz; \
	fi

build_fsimage: build_ramdisk
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_EXT2), y)

.PHONY: build_ext2

build_ext2: PRIVATE_FS_TYPE:=EXT2
build_ext2: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) -d $(FAKEROOT_DIR) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) $(ROOTFS_OUT_DIR)/ext2 " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "fsck -t ext2 -y $(ROOTFS_OUT_DIR)/ext2" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ext2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_EXT3), y)

.PHONY: build_ext3

build_ext3: PRIVATE_FS_TYPE:=EXT3
build_ext3: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) -d $(FAKEROOT_DIR) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) $(ROOTFS_OUT_DIR)/ext3 " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "tune2fs -j $(ROOTFS_OUT_DIR)/ext3" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "fsck -t ext3 -y $(ROOTFS_OUT_DIR)/ext3" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ext3
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_EXT4), y)

.PHONY: build_ext4

build_ext4: PRIVATE_FS_TYPE:=EXT4
build_ext4: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) -d $(FAKEROOT_DIR) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) $(ROOTFS_OUT_DIR)/ext4 " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "tune2fs -j -O extents,dir_index $(ROOTFS_OUT_DIR)/ext4" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "fsck -t ext4 -y $(ROOTFS_OUT_DIR)/ext4" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ext4
endif


ifeq ($(CONFIG_BACKUP_CPIO),y)
.PHONY: build_backup_cpio

build_backup_cpio:
	@echo "Build backup cpio"
	@mkdir -p $(ROOTFS_OUT_DIR)
	@rm -f $(ROOTFS_OUT_DIR)/backup_cpio.sh $(ROOTFS_OUT_DIR)/backup_cpio.gz && touch $(ROOTFS_OUT_DIR)/backup_cpio.sh
ifeq ($(CONFIG_CPU_CORTEXA53), y)
	@echo "tar Jxf $(ROOTFS_DIR)/mini_fake_s5.tar.xz -C $(ROOTFS_OUT_DIR)" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
else
	@echo "tar Jxf $(ROOTFS_DIR)/mini_fake.tar.xz -C $(ROOTFS_OUT_DIR)" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
endif
	@if [ -e $(PRE_BUSYBOX_DIR)/usr/share/udhcpc/default.script ];then \
		echo "cp -dp $(PRE_BUSYBOX_DIR)/usr/share/udhcpc/default.script $(ROOTFS_OUT_DIR)/mini_fake/usr/share/udhcpc/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
	fi
	@if [ -e $(ORYX_BIN_DIR)/pba_upgrade ];then \
		echo "cp -dp $(ORYX_BIN_DIR)/pba_upgrade $(ROOTFS_OUT_DIR)/mini_fake/usr/bin/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libstdc++.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libamconfig.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libamosal.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libamutil.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libamdec7z.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/lib7zDec.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libarchive.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libamarchive.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/liblua* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/liblzma.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libz.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
		echo "cp -dpRf $(FAKEROOT_DIR)/usr/lib/libbz2.so* $(ROOTFS_OUT_DIR)/mini_fake/lib/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
	fi
	@echo "find $(ROOTFS_OUT_DIR)/mini_fake/lib/ -type f -executable -exec sh -c \"file -i '{}' | grep -q 'x-sharedlib; charset=binary'\" \; -print | xargs $(CROSS_COMPILE)strip -s">> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@if [ -e $(FAKEROOT_DIR)/usr/local/bin/upgrade_partition ];then \
		echo "cp -dp $(FAKEROOT_DIR)/usr/local/bin/upgrade_partition $(ROOTFS_OUT_DIR)/mini_fake/usr/sbin/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh; \
	fi
	@echo "chown -R 0:0 $(ROOTFS_OUT_DIR)/mini_fake" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "$(MAKEDEVS) -d $(ROOTFS_DIR)/initramfs_table.txt $(ROOTFS_OUT_DIR)/mini_fake" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "cd $(ROOTFS_OUT_DIR)/mini_fake && find . | cpio --quiet -o -H newc > $(ROOTFS_OUT_DIR)/backup.cpio" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "gzip -f -9 $(ROOTFS_OUT_DIR)/backup.cpio" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@chmod u+x $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@if [ -e $(KERNEL_OUT_DIR)/pba_zImage ];then rm -rf $(KERNEL_OUT_DIR)/pba_zImage; fi
	@mv $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image_ori
ifeq ($(CONFIG_CPU_CORTEXA53), y)
	@mv $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image.gz $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage_ori
else
	@mv $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage_ori
endif
	@echo $(LINUX_SRC_DIR)
	@$(MAKE) $(AMBA_MAKE_PARA) ARCH=$(CPU_ARCH) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
			CONFIG_INITRAMFS_SOURCE="$(ROOTFS_OUT_DIR)/backup.cpio.gz" \
			CONFIG_INITRAMFS_ROOT_UID=0 CONFIG_INITRAMFS_ROOT_GID=0 \
			CONFIG_MMC=y CONFIG_MMC_AMBARELLA=y CONFIG_MMC_BLOCK=y \
			CONFIG_FUSE_FS=y CONFIG_FAT_FS=y CONFIG_VFAT_FS=y \
			CONFIG_FAT_DEFAULT_CODEPAGE=437 \
			CONFIG_FAT_DEFAULT_IOCHARSET="iso8859-1" all
ifeq ($(CONFIG_CPU_CORTEXA53), y)
	@cp -dpRf $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image $(KERNEL_OUT_DIR)/pba_zImage
	@mv $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage_ori $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image.gz
else
	@cp -dpRf $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage $(KERNEL_OUT_DIR)/pba_zImage
	@mv $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage_ori $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/zImage
endif
	@mv $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image_ori $(LINUX_OUT_DIR)/arch/$(ARM_ARCH)/boot/Image


build_fsimage: build_backup_cpio
endif


endif

