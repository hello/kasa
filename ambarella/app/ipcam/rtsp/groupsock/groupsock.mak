RTSP_CLIENT_DIR = ../../Windows/Filters/rtspClient
MODULENAME = groupsock
INCLUDES = -Iinclude -I../UsageEnvironment/include -I$(RTSP_CLIENT_DIR)/include -DNO_STRSTREAM

!include    <ntwin32.mak>
COMPILE_OPTS =		$(INCLUDES) $(cdebug) $(cflags) $(cvarsdll) /Gy /GX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\"
C =	c
CPP =			cpp
C_FLAGS =	$(COMPILE_OPTS)
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS)
OBJ =			obj
LINK =			$(link) -out:
LIBRARY_LINK =		lib -out:
LIB_SUFFIX =		lib
# change default OUTDIR : XP32_DEBUG/XP32_RETAIL
OUTDIR = $(RTSP_CLIENT_DIR)/$(MODULENAME) 
LIB = $(OUTDIR)/lib$(MODULENAME).$(LIB_SUFFIX)

ALL = $(OUTDIR) $(LIB)
all:	$(ALL)

OBJS = $(OUTDIR)\GroupsockHelper.$(OBJ) $(OUTDIR)\GroupEId.$(OBJ) \
	$(OUTDIR)\inet.$(OBJ) $(OUTDIR)\Groupsock.$(OBJ) \
	$(OUTDIR)\NetInterface.$(OBJ) $(OUTDIR)\NetAddress.$(OBJ) $(OUTDIR)\IOHandlers.$(OBJ)
	

GroupsockHelper.$(CPP):	include/GroupsockHelper.hh
include/GroupsockHelper.hh:	include/NetAddress.hh
include/NetAddress.hh:	include/NetCommon.h
GroupEId.$(CPP):	include/GroupEId.hh
include/GroupEId.hh:	include/NetAddress.hh
inet.$(C):		include/NetCommon.h
Groupsock.$(CPP):	include/Groupsock.hh include/GroupsockHelper.hh include/TunnelEncaps.hh
include/Groupsock.hh:	include/groupsock_version.hh include/NetInterface.hh include/GroupEId.hh
include/NetInterface.hh:	include/NetAddress.hh
include/TunnelEncaps.hh:	include/NetAddress.hh
NetInterface.$(CPP):	include/NetInterface.hh include/GroupsockHelper.hh
NetAddress.$(CPP):	include/NetAddress.hh include/GroupsockHelper.hh
IOHandlers.$(CPP):	include/IOHandlers.hh include/TunnelEncaps.hh

.$(CPP){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<
.$(C){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<

$(LIB): $(OBJS)
	$(LIBRARY_LINK)$@ $(OBJS)

#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	if exist "$(OUTDIR)" rd /s /q "$(OUTDIR)"

##### Any additional, platform-specific rules come here:
