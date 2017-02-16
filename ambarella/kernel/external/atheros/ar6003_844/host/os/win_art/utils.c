
#include "utils.h"

static FILE     *logFile;  /* file handle of logfile */    
static FILE     *yieldLogFile;  /* file handle of yieldLogfile */    
static A_BOOL   logging;   /* set to 1 if a log file open */
static A_BOOL   enablePrint = 1;
static A_BOOL   yieldLogging = 0;   /* set to 1 if a log file open */
static A_BOOL   yieldEnablePrint = 1;
static A_UINT16 quietMode; /* set to 1 for quiet mode, 0 for not */
static A_UINT16 yieldQuietMode; /* set to 1 for quiet mode, 0 for not */
static char TimeStamp[100];

#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* uiPrintf - print to the perl console
*
* This routine is the equivalent of printf.  It is used such that logging
* capabilities can be added.
*
* RETURNS: same as printf.  Number of characters printed
*/
A_INT32 uiPrintf
    (
    const char * format,
    ...
    )
{
    va_list argList;
    A_INT32     retval = 0;
    A_CHAR    buffer[1024];

    /*if have logging turned on then can also write to a file if needed */

    /* get the arguement list */
    va_start(argList, format);

    /* using vprintf to perform the printing it is the same is printf, only
     * it takes a va_list or arguments
     */
    retval = vprintf(format, argList);
    fflush(stdout);

    if (logging) {
        vsprintf(buffer, format, argList);
        fputs(buffer, logFile);
		fflush(logFile);
    }

    va_end(argList);    /* cleanup arg list */

    return(retval);
}

A_INT32 q_uiPrintf
    (
    const char * format,
    ...
    )
{
    va_list argList;
    A_INT32    retval = 0;
    A_CHAR    buffer[256];
    if ( !quietMode ) {
        va_start(argList, format);

        retval = vprintf(format, argList);
        fflush(stdout);

        if ( logging ) {
            vsprintf(buffer, format, argList);
            fputs(buffer, logFile);
        }

        va_end(argList);    // clean up arg list
    }

    return(retval);
}
#endif

/**************************************************************************
* statsPrintf - print to the perl console and to stats file
*
* This routine is the equivalent of printf.  In addition it will write
* to the file in stats format (ie with , delimits).  If the pointer to 
* the file is null, just print to console
*
* RETURNS: same as printf.  Number of characters printed
*/
A_INT16 statsPrintf
    (
    FILE *pFile,
	const char * format,
    ...
    )
{
    va_list argList;
    int     retval = 0;
    char    buffer[256];

    /*if have logging turned on then can also write to a file if needed */

    /* get the arguement list */
    va_start(argList, format);

    /* using vprintf to perform the printing it is the same is printf, only
     * it takes a va_list or arguments
     */
    retval = vprintf(format, argList);
    fflush(stdout);

    if (logging) {
        vsprintf(buffer, format, argList);
        fputs(buffer, logFile);
    }

    if (pFile)
	{
        vsprintf(buffer, format, argList);
        fputs(buffer, pFile);	
		fputc(',', pFile);
	    fflush(pFile);
	}

	va_end(argList);    /* cleanup arg list */

    return((A_INT16)retval);
}


#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* uilog - turn on logging
*
* A user interface command which turns on logging to a fill, all of the
* information printed on screen
*
* RETURNS: 1 if file opened, 0 if not
*/
A_UINT16 uilog
    (
    char *filename,        /* name of file to log to */
	A_BOOL append
    )
{
    /* open file for writing */
    if (append) {
		logFile = fopen(filename, "a+");
	}
	else {
		logFile = fopen(filename, "w");
	}
    if (logFile == NULL) {
        uiPrintf("Unable to open file %s\n", filename);
        return(0);
    }

    /* set flag to say logging enabled */
    logging = 1;

    return(1);
}
#endif
/**************************************************************************
* uiWriteToLog - write a string to the log file
*
* A user interface command which writes a string to the log file
*
* RETURNS: 1 if sucessful, 0 if not
*/
A_UINT16 uiWriteToLog
(
	char *string
)
{
	if(logFile == NULL) {
		uiPrintf("Error, logfile not valid, unable to write to file\n");
		return 0;
	}

	/* write string to file */
	fprintf(logFile, string);
	return 1;
}

void configPrint
(
     A_BOOL flag
)
{
    enablePrint = flag;
}


#ifndef __ATH_DJGPPDOS__
/**************************************************************************
* uilogClose - close the logging file
*
* A user interface command which closes an already open log file
*
* RETURNS: 1 if file opened, 0 if not
*/
void uilogClose(void)
{
    if ( logging ) {
        fclose(logFile);
        logging = 0;
    }

    return;
}

/**************************************************************************
* quiet - set quiet mode on or off
*
* A user interface command which turns quiet mode on or off
* 0 for off, 1 for on
* RETURNS: N/A
*/
void dk_quiet(A_UINT16 Mode)
{
    quietMode = Mode;
    return;
}
#endif

/**************************************************************************
* uiOpenYieldLog - open the yield log file.
*
* A user interface command which turns on logging to the yield log file
*
* RETURNS: 1 if file opened, 0 if not
*/
A_UINT16 uiOpenYieldLog
(
    char *filename,        /* name of file to log to */
	A_BOOL append
)
{
    /* open file for writing */
    if (append) {
		yieldLogFile = fopen(filename, "a+");
	}
	else {
		yieldLogFile = fopen(filename, "w");
	}
    if (yieldLogFile == NULL) {
        uiPrintf("Unable to open yield log file %s\n", filename);
        return(0);
    } else {
		uiPrintf("Opened file %s for yieldLog\n", filename);
	}

    /* set flag to say yield logging enabled */
    yieldLogging = 1;

    return(1);
}

/**************************************************************************
* uiYieldLog - write a string to the yield log file
*
* A user interface command which writes a string to the log file
*
* RETURNS: 1 if sucessful, 0 if not
*/
A_UINT16 uiYieldLog
(
	char *string
)
{
	if (yieldLogging > 0) {
		if(yieldLogFile == NULL) {
			uiPrintf("Error, yield logfile not valid, unable to write to file\n");
			return 0;
		}

	  /* write string to file */
	  fprintf(yieldLogFile, string);

	  fflush(yieldLogFile);
	}
	return 1;
}

/**************************************************************************
* uiCloseYieldLog - close the yield logging file
*
* A user interface command which closes an already open log file
*
* RETURNS: void
*/
void uiCloseYieldLog(void)
{
    if ( yieldLogging) {
        if (yieldLogFile != NULL) 
			fclose(yieldLogFile);
        yieldLogging = 0;
    }

    return;
}

/**************************************************************************
* milliSleep - sleep for the specified number of milliseconds
*
* This routine calls a OS specific routine for sleeping
* 
* RETURNS: N/A
*/

void milliSleep
	(
	A_UINT32 millitime
	)
{
	Sleep(millitime); //sleep is wince is in milliseconds
}

A_UINT32 milliTime
(
 	void
)
{
	struct timeval tv;
	A_UINT32 millisec;

	//if (gettimeofday(&tv,NULL) < 0) {
	if(1){
			millisec = 0;
	} else {
		millisec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}

	return millisec;
}

char* GetTimeStamp()
{
  time_t CurrentTime;
  struct _timeb tstruct;
  char Buffer1[80], Buffer2[10];

  CurrentTime = time(NULL);
  _ftime( &tstruct );
  strftime(Buffer1, sizeof(Buffer1), "%m/%d/%y %H:%M:%S.",
           localtime(&CurrentTime));
  sprintf(Buffer2, "%03u", tstruct.millitm);

  TimeStamp[0] = '[';
  strcpy(&TimeStamp[1], Buffer1);
  strcpy(&TimeStamp[1 + strlen(Buffer1)], Buffer2);
  TimeStamp[1 + strlen(Buffer1) + strlen(Buffer2)] = ']';

  return TimeStamp;
}

//----------------------------------------------------------------------------

// Returns a double representing time in seconds (with millisecond precision) since Jan 1, 1970.
double GetTimeStampDouble()
{
  double TimeStampFull;
  struct _timeb TimeStruct;

  //Get current time
  _ftime( &TimeStruct );

  //Make a precise time stamp by combining seconds and milliseconds
  TimeStampFull = (double)TimeStruct.time;
  TimeStampFull += ((double) TimeStruct.millitm ) / 1000;

  return TimeStampFull;
}

