# bcmdhd
# 1. WL_IFACE_COMB_NUM_CHANNELS must be added if Android version is 4.4 with Kernel version 3.0~3.4,
#    otherwise please remove it.
DHDCFLAGS = -Wall -Wstrict-prototypes -Dlinux -DBCMDRIVER                \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE            \
	-DDHD_DEBUG -DSHOW_EVENTS -DBCMDBG                                    \
	-DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT                                \
	-DKEEP_ALIVE -DPKT_FILTER_SUPPORT                                     \
	-DEMBEDDED_PLATFORM -DPNO_SUPPORT                                     \
	-DSET_RANDOM_MAC_SOFTAP -DOEM_ANDROID                                 \
	-DESCAN_RESULT_PATCH -DSUPPORT_PM2_ONLY                               \
	-DTOE -DBDC -DDHD_BCMEVENTS -DPROP_TXSTATUS                           \
	-DWL_SUPPORT_BACKPORTED_KPATCHES                                      \
	-Idrivers/net/wireless/bcmdhd -Idrivers/net/wireless/bcmdhd/include

DHDCFLAGS += -DUSBOS_TX_THREAD -DBCMDHDUSB -DBCMDBUS -DBCMTRXV2          \
	-DDUAL_STA -DUSBOS_THREAD

DHDOFILES = bcmutils.o bcmwifi_channels.o                                \
	dhd_linux.o dhd_linux_sched.o dhd_pno.o                               \
	dhd_common.o dhd_ip.o dhd_custom_gpio.o                               \
	bcmevent.o linux_osl.o wldev_common.o wl_android.o                    \
	dhd_cdc.o dhd_wlfc.o                                                  \
	dbus.o dbus_usb.o dbus_usb_linux.o                                    \
	dhd_config.o

obj-$(CONFIG_BCMDHD) += bcmdhd.o
bcmdhd-objs += $(DHDOFILES)

DHDCFLAGS += -DDHD_OF_SUPPORT -DCUSTOMER_HW2

ifeq ($(CONFIG_DHD_USE_STATIC_BUF),y)
DHDCFLAGS += -DSTATIC_WL_PRIV_STRUCT
endif

ifneq ($(CONFIG_WIRELESS_EXT),)
bcmdhd-objs += wl_iw.o
DHDCFLAGS += -DSOFTAP -DWL_WIRELESS_EXT -DUSE_IW
endif
ifneq ($(CONFIG_CFG80211),)
bcmdhd-objs += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o dhd_cfg80211.o
DHDCFLAGS += -DWL_CFG80211 -DWLP2P -DWL_CFG80211_STA_EVENT -DWL_ENABLE_P2P_IF
DHDCFLAGS += -DWL_CFG80211_GON_COLLISION
DHDCFLAGS += -DCUSTOM_ROAM_TRIGGER_SETTING=-65
DHDCFLAGS += -DCUSTOM_ROAM_DELTA_SETTING=15
DHDCFLAGS += -DCUSTOM_KEEP_ALIVE_SETTING=28000
DHDCFLAGS += -DCUSTOM_PNO_EVENT_LOCK_xTIME=7
DHDCFLAGS += -DWL_SUPPORT_AUTO_CHANNEL
endif
ifneq ($(CONFIG_DHD_USE_SCHED_SCAN),)
DHDCFLAGS += -DWL_SCHED_SCAN
endif
EXTRA_CFLAGS = $(DHDCFLAGS)
ifeq ($(CONFIG_BCMDHD),m)
DHDCFLAGS += -DMULTIPLE_SUPPLICANT
EXTRA_LDFLAGS += --strip-debug
endif
