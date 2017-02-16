RTSP_CLIENT_DIR = ../../Windows/Filters/rtspClient
INCLUDES = -Iinclude -I../groupsock/include
MODULENAME = UsageEnvironment
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

OBJS = $(OUTDIR)/UsageEnvironment.$(OBJ) $(OUTDIR)/HashTable.$(OBJ) $(OUTDIR)/strDup.$(OBJ) 

UsageEnvironment.$(CPP):	include/UsageEnvironment.hh
include/UsageEnvironment.hh:	include/UsageEnvironment_version.hh include/strDup.hh
HashTable.$(CPP):		include/HashTable.hh
include/HashTable.hh:		include/Boolean.hh
strDup.$(CPP):			include/strDup.hh

.$(CPP){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<
.$(C){$(OUTDIR)}.$(OBJ):
	$(cc) -c $(CPLUSPLUS_FLAGS) $<

$(LIB) : $(OBJS)
	$(LIBRARY_LINK)$@ $(OBJS)

#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	if exist "$(OUTDIR)" rd /s /q "$(OUTDIR)"

