#----- Include the PSDK's WIN32.MAK to pick up defines-------------------
!include <win32.mak>

!ifdef NODEBUG
STRMBASE = strmbase.lib
MSVCRT = msvcrt.lib
ATLS = atls.lib
!else
STRMBASE = strmbasd.lib
MSVCRT = msvcrtd.lib
ATLS = atlsd.lib
cflags=$(cflags) /DDEBUG
!endif

!if "$(CPU)"=="AMD64"
ATLS = 
!endif

STRMBASE="$(MSSDK)\samples\multimedia\directshow\baseclasses\$(OUTDIR)\$(STRMBASE)"

all:	$(OUTDIR) $(OUTDIR)\AmbaA5SDemo.ax 

$(OUTDIR)\AmbaA5SDemo.ax:	$(OUTDIR)\fambademo.obj $(OUTDIR)\iso_muxer.obj $(OUTDIR)\demo_res.res 
	$(link) $(ldebug) /OUT:$(OUTDIR)\AmbaA5SDemo.ax $(dlllflags) /DLL /DEF:.\ambaa5sdemo.def /IMPLIB:"$(OUTDIR)\ambaa5sdemo.lib" /MACHINE:$(CPU) $(OUTDIR)\fambademo.obj $(OUTDIR)\iso_muxer.obj $(OUTDIR)\demo_res.res $(STRMBASE) quartz.lib vfw32.lib winmm.lib $(baselibs) version.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib $(MSVCRT) $(ATLS) /NODEFAULTLIB

$(OUTDIR)\iso_muxer.obj:	iso_muxer.cpp   
	$(cc) $(cflags) $(cdebug) $(cvarsdll) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /DINC_OLE2 /D_MT /D_DLL /Dtry=__try /Dexcept=__except /Dleave=__leave /Dfinally=__finally /I"$(MSSDK)\samples\multimedia\directshow\baseclasses"  iso_muxer.cpp 

$(OUTDIR)\fambademo.obj:	fambademo.cpp
	$(cc) $(cflags) $(cdebug) /Gz $(cvarsmt) /Gy /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /DINC_OLE2 /DSTRICT /D_MT /D_DLL /Dtry=__try /Dexcept=__except /Dleave=__leave /Dfinally=__finally /I"$(MSSDK)\\Samples\\Multimedia\\DirectShow\\BaseClasses" /I"$(DXSDK_DIR)\Include" fambademo.cpp

$(OUTDIR)\demo_res.res:	
	 $(rc) $(rcflags) $(rcvars) /I"$(MSSDK)\\Samples\\Multimedia\\DirectShow\\BaseClasses" /Fo$(OUTDIR)\demo_res.res demo_res.rc


#----- If OUTDIR does not exist, then create directory
$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

clean:
        $(CLEANUP)
