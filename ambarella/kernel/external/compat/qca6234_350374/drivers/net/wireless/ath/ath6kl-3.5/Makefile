#------------------------------------------------------------------------------
# Copyright (c) 2004-2010 Atheros Communications Inc.
# All rights reserved.
#
#
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
#
#
# Author(s): ="Atheros"
#------------------------------------------------------------------------------

obj-$(CONFIG_ATH6KL_SDIO) := ath6kl_sdio.o
ath6kl_sdio-y += debug.o
ath6kl_sdio-y += hif.o
ath6kl_sdio-y += htc.o
ath6kl_sdio-y += bmi.o
ath6kl_sdio-y += cfg80211.o
ath6kl_sdio-y += cfg80211_btcoex.o
ath6kl_sdio-y += init.o
ath6kl_sdio-y += main.o
ath6kl_sdio-y += txrx.o
ath6kl_sdio-y += wmi.o
ath6kl_sdio-y += wmi_btcoex.o
ath6kl_sdio-y += sdio.o
ath6kl_sdio-$(CONFIG_NL80211_TESTMODE) += testmode.o
ath6kl_sdio-y += rttm.o
ath6kl_sdio-y += diag.o
ath6kl_sdio-y += htcoex.o
ath6kl_sdio-y += ath_netlink.o
ath6kl_sdio-$(CONFIG_QC_INTERNAL) += intra_reg.o
ath6kl_sdio-y += pm.o
ath6kl_sdio-y += p2p.o
ath6kl_sdio-y += ap.o
ath6kl_sdio-y += reg.o
ath6kl_sdio-y += regdb.o

obj-$(CONFIG_ATH6KL_USB) += ath6kl_usb.o
ath6kl_usb-y += debug.o
ath6kl_usb-y += htc_pipe.o
ath6kl_usb-y += bmi.o
ath6kl_usb-y += cfg80211.o
ath6kl_usb-y += cfg80211_btcoex.o
ath6kl_usb-y += init.o
ath6kl_usb-y += main.o
ath6kl_usb-y += txrx.o
ath6kl_usb-y += wmi.o
ath6kl_usb-y += wmi_btcoex.o
ath6kl_usb-y += usb.o
ath6kl_usb-$(CONFIG_NL80211_TESTMODE) += testmode.o
ath6kl_usb-y += rttm.o
ath6kl_usb-y += diag.o
ath6kl_usb-y += htcoex.o
ath6kl_usb-y += ath_netlink.o
ath6kl_usb-$(CONFIG_QC_INTERNAL) += intra_reg.o
ath6kl_usb-y += pm.o
ath6kl_usb-y += p2p.o
ath6kl_usb-y += ap.o
ath6kl_usb-y += reg.o
ath6kl_usb-y += regdb.o
ath6kl_usb-$(CONFIG_ATHTST_SUPPORT) += ce_athtst.o
ath6kl_usb-$(CONFIG_ACS_SUPPORT) += acs.o
ath6kl_usb-$(CONFIG_ACL_SUPPORT) += acl_ioctl.o
ath6kl_usb-$(CONFIG_TX99_SUPPORT) += tx99_ioctl.o

ccflags-$(CONFIG_QC_INTERNAL) += -DCONFIG_QC_INTERNAL
ccflags-$(CONFIG_FPGA) += -DCONFIG_FPGA
ccflags-y += -D__CHECK_ENDIAN__
ccflags-y += -DCONFIG_BTCOEX_OLCA_3_5
ccflags-$(CONFIG_ATH6KL_UB134) += -DCONFIG_ATH6KL_UDP_TPUT_WAR

EXTRA_CFLAGS += -I${KBUILDPATH}/drivers/net/wireless/ath/ath6kl
