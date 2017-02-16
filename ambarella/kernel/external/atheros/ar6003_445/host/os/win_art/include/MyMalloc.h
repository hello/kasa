/*
 * Copyright 2010 Atheros Communications, Inc., All Rights Reserved.
*/

// MyMemory.h

#ifndef __INCmymemoryh
#define __INCmymemoryh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#ifdef _WINDOWS
//#ifdef MYMALLOC_EXPORTS
//#define MYMALLOC_API __declspec( dllexport )
//#else
//#define MYMALLOC_API __declspec( dllimport )
//#endif  // #ifdef MAUILIB_EXPORTS
//#else	// #ifdef _WINDOWS
#define MYMALLOC_API  
//#endif	// #ifdef WIN32

#ifndef NULL
#define NULL	0
#endif
#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

#define MYMALLOC_MAX_FILE_NAME		256

#define A_MALLOC(a)                 MyMalloc(a, __FILE__, __LINE__)
#define A_CALLOC(a, b)              MyCalloc(a, b, __FILE__, __LINE__)
#define A_FREE(a)                   MyFree(a, __FILE__, __LINE__)
#define A_MALLOC_NOWAIT(a)          MyMalloc(a, __FILE__, __LINE__)

typedef struct DebugMemory_t {
    void            *memAddr;
	__in size_t     size;
    char            allocFile[MYMALLOC_MAX_FILE_NAME];
    unsigned long   allocLine;
    char            freeFile[MYMALLOC_MAX_FILE_NAME];
    unsigned long   freeLine;
    unsigned long   freeTime;
} DEBUG_MEMORY;

typedef struct MyMalloc_t
{
    unsigned int debugLeak;
    unsigned int allocCount;
    unsigned int freeCount;
    unsigned int freeWithoutAllocCount;
    unsigned int freeZeroCount;
    unsigned int allocExcessCount;
    unsigned int freeExcessCount;
    unsigned int numAlloc;
    unsigned int nextIndex;
    DEBUG_MEMORY *DebugMemory;
    char         fileName[MYMALLOC_MAX_FILE_NAME];
} MY_MALLOC;

MYMALLOC_API int MyMallocStart(unsigned int debugLeak, unsigned long NumMalloc, char *fileName);
MYMALLOC_API void MyMallocEnd();
MYMALLOC_API void *MyMalloc(__in size_t _Size, char* file, unsigned long line);
MYMALLOC_API void *MyCalloc(__in size_t _NumOfElements, __in size_t _SizeOfElements, char* file, unsigned long line);
MYMALLOC_API void MyFree(__inout_opt void * _Memory, char* file, unsigned long line);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCmymemoryh */
