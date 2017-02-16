CC=$(TOOLPREFIX)gcc

ATHTST_BINARY = athtst

SRCS=$(shell ls *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
PWD=$(shell pwd)

#CFLAGS += -I$(PWD)
#CFLAGS += -I$(PWD)/../../../include
#CFLAGS += -I$(PWD) -I$(PWD)/.. -Wall -Werror
CFLAGS += -Wall -Werror
CFLAGS += -D_GNU_SOURCE
CFLAGS += ${ATH_LOCAL_CFLAGS}
#CFLAGS += -L $(PWD)/.. 

all:
	@echo -e "\nBuilding ATHTST command..."
	$(CC) $(SRCS) $(CFLAGS) -o $(ATHTST_BINARY)

clean:
	@echo -e "\nCleaning ATHTST command..."
	rm -f $(ATHTST_BINARY)
	rm -f $(OBJS)
