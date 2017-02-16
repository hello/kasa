RTSP_CLIENT_DIR = ../../Windows/Filters/rtspClient
INCLUDES = -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include -I../BasicUsageEnvironment/include
MODULENAME = mediaServer

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
CONSOLE_UI_OPTS =		$(conlflags) $(conlibsdll)
LINK_OPTS_0 =		$(linkdebug)
CONSOLE_LINK_OPTS =	$(LINK_OPTS_0) $(CONSOLE_UI_OPTS)
# change default OUTDIR : XP32_DEBUG/XP32_RETAIL
OUTDIR = $(RTSP_CLIENT_DIR)/$(MODULENAME)
EXE = $(OUTDIR)/live555MediaServer.exe

ALL = $(OUTDIR) $(EXE)
all:	$(ALL)

MEDIA_SERVER_OBJS = $(OUTDIR)/live555MediaServer.$(OBJ) $(OUTDIR)/DynamicRTSPServer.$(OBJ)

live555MediaServer.$(CPP):	DynamicRTSPServer.hh version.hh
DynamicRTSPServer.$(CPP):	DynamicRTSPServer.hh

USAGE_ENVIRONMENT_DIR = $(RTSP_CLIENT_DIR)/UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = $(RTSP_CLIENT_DIR)/BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(LIB_SUFFIX)
LIVEMEDIA_DIR = $(RTSP_CLIENT_DIR)/liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(LIB_SUFFIX)
GROUPSOCK_DIR = $(RTSP_CLIENT_DIR)/groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(LIB_SUFFIX)
LOCAL_LIBS =	$(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) \
		$(BASIC_USAGE_ENVIRONMENT_LIB) $(USAGE_ENVIRONMENT_LIB)
LIBS =			$(LOCAL_LIBS)

.$(CPP){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<
.$(C){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<
	
$(EXE):	$(MEDIA_SERVER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(MEDIA_SERVER_OBJS) $(LOCAL_LIBS)

#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	if exist "$(OUTDIR)" rd /s /q "$(OUTDIR)"

##### Any additional, platform-specific rules come here:
