#ifndef _AMBA_DATA_TYPES_H_
#define _AMBA_DATA_TYPES_H_
#ifndef NULL
#define NULL (void*)0
#endif
typedef unsigned char	UINT8;
typedef unsigned short	UINT16;
typedef unsigned int	UINT32;
typedef unsigned int	UINT;

typedef unsigned long long	UINT64;
typedef signed char	INT8;
typedef signed short	INT16;
typedef signed int	INT32;
typedef signed int	INT;

typedef signed long long	INT64;

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef unsigned char  	    u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned short 	    u16;/**< UNSIGNED 16-bit data type */
typedef unsigned int   	   u32;	/**< UNSIGNED 32-bit data type */
typedef unsigned long long u64; /**< UNSIGNED 64-bit data type */
typedef signed char         s8;	/**< SIGNED 8-bit data type */
typedef signed short       s16;	/**< SIGNED 16-bit data type */
typedef signed int         s32;	/**< SIGNED 32-bit data type */

#endif
