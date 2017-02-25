Here is the instructions for compile projects under directory "ambarella\app\ipcam\Windows\":

1. Develop platform: Microsoft Visual Studio 2005, Microsoft Windows SDK.
   winsdk_web.exe is provided here for install Microsoft Windows SDK, which can be get from www.microsoft.com for free.
   Please ensure the belows options are seletcted:
	|- Windows Native Code Development
	| |-Samples
          |- Windows Headers and Libraries
	    |-Windows Headers
	    |-x86 Libraries
	    |-x64 Libraries
	    |-Itanium Libraries

2. Add Environment Variable:
	$MSSDK = the directory of Windows SDK (E.g. C:\Program Files\Microsoft SDKs\Windows\v7.1)

3. Extract rtsp_windows.7z and use the extracted folder to replace the "ambarella\app\ipcam\rtsp".
   This is source code of live555 libs for rtsp client. The base version is '2010.02.10'.
   The libs are provided on "ambarella\app\ipcam\Windows\Filters\rtspClient\: lib*.lib"
   If you want to compile these libs by yourselves,follow the steps:
   a) Open Microsft Visual Studio 2005->Visual Studio tools -> Visual Studio 2005 command line
   b) Run combat.bat for compile debug version, run combat_release.bat for release version.

4. Use Microsoft Visual Studio 2005 to open and build the projects.
