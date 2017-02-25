#----- Include the PSDK's WIN32.MAK to pick up defines-------------------
!include <win32.mak>

LINK =			$(link) -out:
COMPILE_OPTS =	$(cdebug) $(cflags) /Gy /GX 
CPP =			cpp
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS)

all:	setup.exe

.cpp.obj:
	$(cc) -c $(CPLUSPLUS_FLAGS) $<

setup.exe:	setup.obj
	$(LINK)$@ setup.obj


clean:
	del /s /q setup.obj setup.exe
