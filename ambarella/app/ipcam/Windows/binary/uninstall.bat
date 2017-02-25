@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{3BCDAA6A-7306-42FF-B8CF-BE5D3534C1E4}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
)
@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{3A17A4E2-82B1-4E24-AD33-C27F199872F6}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
)
@for /f "skip=2 tokens=2*" %%a in ('@reg query hkcr\CLSID\{75437AFA-6A06-4237-9FEB-7746A8624F0A}\InprocServer32 /ve ') do (
regsvr32 /u /s "%%b"
) 