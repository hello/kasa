#ifndef __7Z_UTF_TO_STR_H
#define __7Z_UTF_TO_STR_H

#include "7zAlloc.h"
#include "7zTypes.h"
#include "7zBuf.h"

#ifndef _WIN32
#define _USE_UTF8
#endif

#ifdef __cplusplus
extern "C" {
#endif
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

SRes Utf16_To_Char(CBuf *buf,
                   const UInt16 *s
#ifndef _USE_UTF8
                 , UINT codePage
#endif
);

int apend_utf16_to_Cstring(char *str, UInt16 *temp);
#ifdef __cplusplus
}
#endif

#endif //__7Z_UTF_TO_STR_H
