RTSP_CLIENT_DIR = ../../Windows/Filters/rtspClient
INCLUDES = -Iinclude -I../UsageEnvironment/include -I../groupsock/include
MODULENAME = BasicUsageEnvironment
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

OBJS = $(OUTDIR)/BasicUsageEnvironment0.$(OBJ) $(OUTDIR)/BasicUsageEnvironment.$(OBJ) \
	$(OUTDIR)/BasicTaskScheduler0.$(OBJ) $(OUTDIR)/BasicTaskScheduler.$(OBJ) \
	$(OUTDIR)/DelayQueue.$(OBJ) $(OUTDIR)/BasicHashTable.$(OBJ)
	
BasicUsageEnvironment0.$(CPP):	include/BasicUsageEnvironment0.hh
include/BasicUsageEnvironment0.hh:	include/BasicUsageEnvironment_version.hh include/DelayQueue.hh
BasicUsageEnvironment.$(CPP):	include/BasicUsageEnvironment.hh
include/BasicUsageEnvironment.hh:	include/BasicUsageEnvironment0.hh
BasicTaskScheduler0.$(CPP):	include/BasicUsageEnvironment0.hh include/HandlerSet.hh
BasicTaskScheduler.$(CPP):	include/BasicUsageEnvironment.hh include/HandlerSet.hh
DelayQueue.$(CPP):		include/DelayQueue.hh
BasicHashTable.$(CPP):		include/BasicHashTable.hh

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
