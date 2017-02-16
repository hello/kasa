CC = gcc
CFLAGS = -Wall -g

NLLIBNAME = libnl-1
WLAN_API = WLAN_API_NL80211
WLAN_API_SRCS = nl80211.c

SRCS = os.c $(WLAN_API_SRCS) libtcmd.c
OBJS = $(SRCS:.c=.o)

LDLIBS += $(shell pkg-config --libs $(NLLIBNAME)) -lrt
CFLAGS += $(shell pkg-config --cflags $(NLLIBNAME)) -D$(WLAN_API)

libtcmd.a: $(SRCS) $(OBJS)
	$(AR) rcs $@ $(OBJS)

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(@:.o=.c) -o $@ $(LDLIBS)

clean:
	rm -f *.a *.o
