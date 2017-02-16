CFLAGS	=	-g -DATHR_EMULATION -DLINUX_EMULATION $(CHDRS) -DREXOS -std=c99 -Wall
CC		=	gcc

HOST_DIR = ../../../..

ATHR_DIR  = ./
ATHR_HDRS = ../../include/athr
ATHR_SRC  = main.c debug.c qcom_stubs.c sdio.c
ATHR_SRCS = $(ATHR_SRC:%=$(ATHR_DIR)/%)

BMI_DIR   = $(HOST_DIR)/bmi
BMI_HDRS  = $(BMI_DIR)/include
BMI_SRC   = bmi.c
BMI_SRCS  = $(BMI_SRC:%=$(BMI_DIR)/src/%)
BMI_OBJS  = $(BMI_SRCS:%.c=%.o)

CMN_DIR   = $(HOST_DIR)/miscdrv
CMN_HDRS  = $(HOST_DIR)/miscdrv
CMN_SRC   = common_drv.c credit_dist.c
CMN_SRCS  = $(CMN_SRC:%=$(CMN_DIR)/%)
CMN_OBJS  = $(CMN_SRCS:%.c=%.o)

HIF_DIR   = $(HOST_DIR)/hif/sdio/rexos
HIF_HDRS  = $(HOST_DIR)/hif/sdio/rexos/include
HIF_SRC   = hif.c
HIF_SRCS  = $(HIF_SRC:%=$(HIF_DIR)/src/%)
HIF_OBJS  = $(HIF_SRCS:%.c=%.o)

HTC_DIR   = $(HOST_DIR)/htc2
HTC_HDRS  = $(HTC_DIR)
HTC_SRC   = htc_recv.c htc_send.c htc_services.c htc.c AR6000/ar6k_events.c AR6000/ar6k.c
HTC_SRCS  = $(HTC_SRC:%=$(HTC_DIR)/%)
HTC_OBJS  = $(HTC_SRCS:%.c=%.o)

QCOM_CMN_DIR  = ../common
QCOM_CMN_HDRS = ../../include/common
QCOM_CMN_SRC  = config.c eeprom.c fmi.c qcom_buf.c qcom_drv.c qcom_firmware.c \
                qcom_htc.c qcom_osapi.c qcom_wmi.c wlan_oem_ath.c
                
QCOM_CMN_SRCS = $(QCOM_CMN_SRC:%=$(QCOM_CMN_DIR)/%)

WLAN_DIR  = $(HOST_DIR)/wlan
WLAN_HDRS = $(HOST_DIR)/wlan/include
WLAN_SRC  = wlan_node.c wlan_recv_beacon.c wlan_utils.c
WLAN_SRCS = $(WLAN_SRC:%=$(WLAN_DIR)/src/%)
WLAN_OBJS  = $(WLAN_SRCS:%.c=%.o)

WMI_DIR   = $(HOST_DIR)/wmi
WMI_HDRS  = $(WMI_DIR)
WMI_SRC   = wmi.c
WMI_SRCS  = $(WMI_SRC:%=$(WMI_DIR)/%)
WMI_OBJS  = $(WMI_SRCS:%.c=%.o)

HOST_DRV_HDRS = $(HOST_DIR)/include
QCOM_HDRS = ../../include/qcom
REXOS_DRV_HDRS = $(HOST_DIR)/include/rexos
TGT_HDRS = $(HOST_DIR)/../include

CSRC = $(ATHR_SRCS) $(BMI_SRCS) $(CMN_SRCS) $(HIF_SRCS) $(HTC_SRCS) $(QCOM_CMN_SRCS) \
       $(WLAN_SRCS) $(WMI_SRCS)

CHDRS = -I$(TGT_HDRS) -I$(ATHR_HDRS) -I$(BMI_HDRS) -I$(HIF_HDRS) -I$(HTC_HDRS) \
        -I$(QCOM_CMN_HDRS) -I$(WLAN_HDRS) -I$(WMI_HDRS) -I$(HOST_DRV_HDRS) \
        -I$(REXOS_DRV_HDRS) -I$(QCOM_HDRS) -I$(CMN_HDRS)

OBJS = $(patsubst %.c,%.o, $(CSRC))
OUTPUT_DIR = .output
OBJS_NODIR = $(patsubst %.o,$(OUTPUT_DIR)/%.o, $(notdir $(OBJS)))
TARGET	= $(OUTPUT_DIR)/qcom

all: output_dir $(TARGET)

clean:
	-rm -rf $(OUTPUT_DIR) $(BMI_OBJS) $(CMN_OBJS) $(HIF_OBJS) $(HTC_OBJS) $(WLAN_OBJS) $(WMI_OBJS)
	
$(TARGET):	output_dir $(OBJS)
	$(CC) -o $@ $(OBJS_NODIR) -lpthread
	-cp $(HOST_DIR)/../target/.output/AR6002/hw2.0/bin/athwlan.bin.z77 Images/athwlan2_0.bin.z77
	-cp $(HOST_DIR)/../target/.output/AR6002/hw2.0/bin/data.patch.hw2_0.bin Images
	-cp $(HOST_DIR)/../target/.output/AR6002/hw2.0/bin/athtcmd_ram.bin Images/athtcmd2_0.bin
	-chmod u+w Images/*bin*
	
%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $(OUTPUT_DIR)/$(notdir $@)

output_dir:
	-mkdir -p $(OUTPUT_DIR)
	-mkdir -p Images
