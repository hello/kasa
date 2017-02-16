#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
#include <time.h>
#include <sys\timeb.h>

#include "athtypes_win.h"


A_INT16 statsPrintf(FILE *pFile, const char * format, ...);
A_UINT16 uiWriteToLog(char *string);
void configPrint(A_BOOL flag);

#ifndef __ATH_DJGPPDOS__
A_INT32 uiPrintf(const char * format, ... );
A_INT32 q_uiPrintf(const char * format, ...);
A_UINT16 uilog(char *filename, A_BOOL append);
void uilogClose(void);
void dk_quiet(A_UINT16 Mode);
#endif

A_UINT16 uiOpenYieldLog(char *filename,	A_BOOL append);
A_UINT16 uiYieldLog(char *string);
void uiCloseYieldLog(void);

void milliSleep (A_UINT32 millitime);
A_UINT32 milliTime ();
char* GetTimeStamp();
double GetTimeStampDouble();


#endif //_UTILS_H_