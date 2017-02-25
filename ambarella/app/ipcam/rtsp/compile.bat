cd liveMedia
nmake /B -f liveMedia.mak clean
nmake /B -f liveMedia.mak
cd ../groupsock
nmake /B -f groupsock.mak clean
nmake /B -f groupsock.mak
cd ../UsageEnvironment
nmake /B -f UsageEnvironment.mak clean
nmake /B -f UsageEnvironment.mak
cd ../BasicUsageEnvironment
nmake /B -f BasicUsageEnvironment.mak clean
nmake /B -f BasicUsageEnvironment.mak
cd ../testProgs
nmake /B -f testProgs.mak clean
nmake /B -f testProgs.mak
cd ../mediaServer
nmake /B -f testProgs.mak clean
nmake /B -f mediaServer.mak
cd ..