/*
 * Copyright 2010 Atheros Communications, Inc., All Rights Reserved.
*/

// MyMemory.c

#include <time.h>

#include "MyMalloc.h"

#define MYMALLOC_DEBUG          0x1234
#define DEBUG_MEMORY_LOG_FILE   "DebugMemoryLeak.txt"

MY_MALLOC MyMallocStr;

static void PrintDebugMemory();

MYMALLOC_API int MyMallocStart(unsigned int DebugLeak, unsigned long NumMalloc, char *fileName)
{
    MyMallocStr.debugLeak = (DebugLeak == 1) ? MYMALLOC_DEBUG : 0;
    MyMallocStr.allocCount = 0;
    MyMallocStr.freeCount = 0;
    MyMallocStr.freeWithoutAllocCount = 0;
    MyMallocStr.freeZeroCount = 0;
    MyMallocStr.allocExcessCount = 0;
    MyMallocStr.freeExcessCount = 0;
    MyMallocStr.numAlloc = NumMalloc;
    MyMallocStr.nextIndex = 0;
    MyMallocStr.DebugMemory = NULL;
    if(fileName != NULL)
    {
        strcpy(MyMallocStr.fileName, fileName);
    }
    else
    {
        strcpy(MyMallocStr.fileName, DEBUG_MEMORY_LOG_FILE);
    }
    if (MyMallocStr.debugLeak == MYMALLOC_DEBUG)
    {
        MyMallocStr.DebugMemory = (DEBUG_MEMORY *)malloc(sizeof(DEBUG_MEMORY) * NumMalloc);
        if (!MyMallocStr.DebugMemory)
        {
            printf ("StartMyMalloc - could not allocate memory!\n");
            return FALSE;
        }
        printf ("***** MyMallocStart *****\n");
    }
    return TRUE;
}

MYMALLOC_API void MyMallocEnd()
{
    if (MyMallocStr.debugLeak == MYMALLOC_DEBUG)
    {
        PrintDebugMemory();
        free(MyMallocStr.DebugMemory);
        printf ("***** MyMallocEnd *****\n");
    }
}

MYMALLOC_API void *MyMalloc(__in size_t _Size, char* file, unsigned long line)
{
    void *pReturn = malloc(_Size);
//#ifdef _DEBUG
//    if (MyMallocStr.debugLeak != MYMALLOC_DEBUG)
//    {
//        printf ("$$$$$$$$$$$ MyMalloc %s (%d)\n", file, line);
//    }
//#endif
    if (MyMallocStr.debugLeak == MYMALLOC_DEBUG)
    {
        MyMallocStr.allocCount++;
        if (MyMallocStr.nextIndex == MyMallocStr.numAlloc)
        {
            MyMallocStr.allocExcessCount++;
        }
        else
        {
		    memset(&MyMallocStr.DebugMemory[MyMallocStr.nextIndex], 0, sizeof(DEBUG_MEMORY));
            MyMallocStr.DebugMemory[MyMallocStr.nextIndex].memAddr = pReturn;
		    MyMallocStr.DebugMemory[MyMallocStr.nextIndex].size = _Size;
            strcpy(MyMallocStr.DebugMemory[MyMallocStr.nextIndex].allocFile, file);
            MyMallocStr.DebugMemory[MyMallocStr.nextIndex].allocLine = line;
            MyMallocStr.nextIndex++;
        }
    } 
    return pReturn;
}

MYMALLOC_API void *MyCalloc(__in size_t _NumOfElements, __in size_t _SizeOfElements, char* file, unsigned long line)
{
    void *pReturn = calloc(_NumOfElements, _SizeOfElements);

    if (MyMallocStr.debugLeak == MYMALLOC_DEBUG)
    {
        MyMallocStr.allocCount++;
        if (MyMallocStr.nextIndex == MyMallocStr.numAlloc)
        {
		    MyMallocStr.allocExcessCount++;
    	}
        else
        {
		    memset(&MyMallocStr.DebugMemory[MyMallocStr.nextIndex], 0, sizeof(DEBUG_MEMORY));
            MyMallocStr.DebugMemory[MyMallocStr.nextIndex].memAddr = pReturn;
		    MyMallocStr.DebugMemory[MyMallocStr.nextIndex].size = _NumOfElements * _SizeOfElements;
            strcpy(MyMallocStr.DebugMemory[MyMallocStr.nextIndex].allocFile, file);
            MyMallocStr.DebugMemory[MyMallocStr.nextIndex].allocLine = line;
            MyMallocStr.nextIndex++;
        }
    }
    return pReturn;
}

MYMALLOC_API void MyFree(__inout_opt void * _Memory, char* file, unsigned long line)
{
    unsigned int i, j;
	int freed = 0;

	if (!_Memory)
	{
		MyMallocStr.freeZeroCount++;
		return;
	}
    free(_Memory);

    if (MyMallocStr.debugLeak == MYMALLOC_DEBUG)
    {
        MyMallocStr.freeCount++;

        for (i = 0; i < MyMallocStr.nextIndex; ++i)
        {
            if (MyMallocStr.DebugMemory[i].memAddr == _Memory)
            {
			    for (j = i; j < MyMallocStr.nextIndex-1; j++)
			    {
				    memcpy(&MyMallocStr.DebugMemory[j], &MyMallocStr.DebugMemory[j+1], sizeof(DEBUG_MEMORY));
			    }
			    memset(&MyMallocStr.DebugMemory[MyMallocStr.nextIndex-1], 0, sizeof(DEBUG_MEMORY));
			    MyMallocStr.nextIndex--;
			    freed = 1;
                break;
            }
        }
        if ((freed == 0) && (i == MyMallocStr.nextIndex))
        {
            MyMallocStr.DebugMemory[i].freeTime++;
            strcpy(MyMallocStr.DebugMemory[i].freeFile, file);
            MyMallocStr.DebugMemory[i].freeLine = line;
            MyMallocStr.DebugMemory[MyMallocStr.nextIndex++].memAddr = _Memory;
		    MyMallocStr.freeWithoutAllocCount++;
        }
        else if (i >= MyMallocStr.numAlloc)
        {
            MyMallocStr.freeExcessCount++;
        }
    }
}

void PrintDebugMemory()
{
    time_t rawtime;
    struct tm * timeinfo;
    unsigned int i;
    int j;
    char *allocFileName;
    char *freeFileName;
	unsigned long unfreedMemory;
    FILE *pFile;

    if (MyMallocStr.debugLeak != MYMALLOC_DEBUG)
    {
        return;
    }
    if (!(pFile = fopen(MyMallocStr.fileName, "a")))
    {
        printf ("PrintMyMallocStr.DebugMemory - ERROR in openning %s\n", DEBUG_MEMORY_LOG_FILE);
        return;
    }

    unfreedMemory = 0;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    fprintf (pFile, "\n\nTime and Date: %s\n", asctime (timeinfo) );
    fprintf (pFile,   "\n     Memory(Size)              AllocFile(Line)      FreeFile(Line)       FreeTime\n\n");
    for (i = 0; i < MyMallocStr.nextIndex; ++i)
    {
		allocFileName = "";
		freeFileName = "";

		if (MyMallocStr.DebugMemory[i].allocFile[0] != '\0')
		{
			for (j = (unsigned int)strlen(MyMallocStr.DebugMemory[i].allocFile) - 1; j >= 0; --j)
			{
				if ((MyMallocStr.DebugMemory[i].allocFile[j] == '\\') || (MyMallocStr.DebugMemory[i].allocFile[j] == '/'))
				{
					allocFileName = &MyMallocStr.DebugMemory[i].allocFile[j+1];
					break;
				}
			}
			unfreedMemory += ((unsigned long)MyMallocStr.DebugMemory[i].size);
        }
		if (MyMallocStr.DebugMemory[i].freeFile[0] != '\0')
		{
			for (j = (unsigned int)strlen(MyMallocStr.DebugMemory[i].freeFile) - 1; j >= 0; --j)
			{
				if ((MyMallocStr.DebugMemory[i].freeFile[j] == '\\') || (MyMallocStr.DebugMemory[i].freeFile[j] == '/'))
				{
					freeFileName = &MyMallocStr.DebugMemory[i].freeFile[j+1];
					break;
				}
			}
		}
		fprintf (pFile, "%-5d0x%p(%8d) %15s(%d) %15s(%d) %10d\n", i, MyMallocStr.DebugMemory[i].memAddr, MyMallocStr.DebugMemory[i].size,
						allocFileName, MyMallocStr.DebugMemory[i].allocLine, freeFileName, MyMallocStr.DebugMemory[i].freeLine, MyMallocStr.DebugMemory[i].freeTime);
    }
    fprintf (pFile, "\n\unfreedMemory       = %d\n\n", unfreedMemory);

    fprintf (pFile, "allocCount             = %d\n", MyMallocStr.allocCount);
    fprintf (pFile, "freeCount              = %d\n", MyMallocStr.freeCount);
    fprintf (pFile, "freeWithoutAllocCount  = %d\n", MyMallocStr.freeWithoutAllocCount);
	fprintf (pFile, "freeZeroCount          = %d\n", MyMallocStr.freeZeroCount);
	fprintf (pFile, "allocExcessCount       = %d\n", MyMallocStr.allocExcessCount);
	fprintf (pFile, "freeExcessCount        = %d\n", MyMallocStr.freeExcessCount);
    fclose(pFile);
}

