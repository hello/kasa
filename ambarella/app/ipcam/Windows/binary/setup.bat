@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{3BCDAA6A-7306-42FF-B8CF-BE5D3534C1E4}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
)
@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{3A17A4E2-82B1-4E24-AD33-C27F199872F6}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
)
@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{75437AFA-6A06-4237-9FEB-7746A8624F0A}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
) 
if not exist "%ProgramFiles%\Ambarella" (mkdir "%ProgramFiles%\Ambarella")
set dir=%~dp0%

mshta vbscript:msgbox("Amba plugins are installed or updated to '%ProgramFiles%\Ambarella\'! ",64,"Notice")(window.close)

copy "%dir%AmbaIPCmrWebPlugIn.ocx" "%ProgramFiles%\Ambarella\" /y
if ERRORLEVEL 1 goto error1

regsvr32 /s "%ProgramFiles%\Ambarella\AmbaIPCmrWebPlugIn.ocx"
if ERRORLEVEL 1 goto end

copy  "%dir%AmbaTCPClient.ax" "%ProgramFiles%\Ambarella\" /y
if ERRORLEVEL 1 goto error2

regsvr32 /s "%ProgramFiles%\Ambarella\AmbaTCPClient.ax"
if ERRORLEVEL 1 goto error_end

copy  "%dir%AmbaRTSPClient.ax" "%ProgramFiles%\Ambarella\" /y
if ERRORLEVEL 1 goto error3

regsvr32 /s "%ProgramFiles%\Ambarella\AmbaRTSPClient.ax"
if ERRORLEVEL 1 goto error_end
goto end:

@echo off


:error1
	mshta vbscript:msgbox("Copying AmbaIPCmrWebPlugIn.ocx failed! Please check '%ProgramFiles%\Ambarella\setup.log' and try again",64,"Warning")(window.close)
	goto end
:error2
	mshta vbscript:msgbox("Copying AmbaTCPClient.ax failed! Please check '%ProgramFiles%\Ambarella\setup.log' and try again",64,"Warning")(window.close)
	goto error_end
:error3
	mshta vbscript:msgbox("Copying AmbaRTSPClient.ax failed! Please check '%ProgramFiles%\Ambarella\setup.log' and try again",64,"Warning")(window.close)
	goto error_end

:error_end
	@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{3BCDAA6A-7306-42FF-B8CF-BE5D3534C1E4}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
)
:end