

#ifndef _MW_DEFINES_H_
#define _MW_DEFINES_H_

extern int G_mw_log_level;

/*********************************
 *
 *	for debug
 *
 *********************************/


#include "stdio.h"

#define	MW_PRINT(mylog, LOG_LEVEL, format, args...)		do {		\
							if (mylog >= LOG_LEVEL) {			\
								printf(format, ##args);			\
							}									\
						} while (0)

#define MW_ERROR(format, args...)			MW_PRINT(G_mw_log_level, MW_ERROR_LEVEL, "!!! [%s: %d]" format "\n", __FILE__, __LINE__, ##args)
#define MW_MSG(format, args...)			MW_PRINT(G_mw_log_level, MW_MSG_LEVEL, ">>> " format, ##args)
#define MW_INFO(format, args...)			MW_PRINT(G_mw_log_level, MW_INFO_LEVEL, "::: " format, ##args)
#define MW_DEBUG(format, args...)		MW_PRINT(G_mw_log_level, MW_DEBUG_LEVEL, "=== " format, ##args)



/*********************************
 *
 *	Predefined expression
 *
 *********************************/

#define		Q9_BASE			(512000000)

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, n)	( ((x)+(n)-1u) & ~((n)-1u) )
#endif

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(x) ({				\
		int __x = (x);			\
		(__x < 0) ? -__x : __x;		\
            })
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))
#endif


#endif // _MW_DEFINES_H_

