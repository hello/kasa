/*!
 @addtogroup  mmaptest
 @{
 @file        mmaptest.c
 @brief       
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#define SRCBUF_SIZE	(257)
#define TARGET_BOARD


#define MMAP_FILE_PATH	"/dev/mem"
#define IAV_DRIVER_PATH	"/dev/iav"

// physical address for mmap
static const unsigned long g_mapaddress_outside = 0x0E000000;	// os管理外領域
static const unsigned long g_mapaddress_inside = 0x0C000000;	// os管理内領域


// size for mmap
static const unsigned long g_mapsize = 3 * 1024 * 1024;

static int createMmap( void** ppBuf, int* pFd, unsigned long );
static int deleteMmap( void* pBufAdr, int fd);
static size_t getMmapSize( size_t size );



int main(int argc, char *argv[])
{
	void* pBuf, *pBuf2;
	int fd, fd2;

	// mmap for inside memory
	if( 0 != createMmap( &pBuf, &fd, g_mapaddress_inside )) {
		printf( "createMmap error\n");
		return -1;
	}
	printf( "createMmap(inside) ok.\n");
	
		// mmap for outside memory
	if( 0 != createMmap( &pBuf2, &fd2, g_mapaddress_outside )) {
		printf( "createMmap error\n");
		return -1;
	}
	printf( "createMmap(outside) ok.\n");


	// copy from stack
	char* testdata = (char*)malloc( 256 );
	memset( testdata, 0xcc, 256 );

	char testdata2[256];
	memset( testdata2, 0xdd, 256 );

	/************** test for heap ***********/

	printf( "memcpy from heap to heap with alignment\n");

	// memcpy from mmap area to heap with alignment --- ok.
	memcpy( testdata, testdata2, 8 );

	printf( "ok\n");

	sleep( 1 );

	printf( "memcpy from heap to heap without alignment \n");

	// memcpy from mmap area to heap without alignment --- ok.
	memcpy( testdata, testdata2 + 5, 8 );

	printf( "ok\n");

	sleep( 1 );


	/************** test for mmap (inside os) ***********/

	printf( "memcpy from mmap area(inside os) to heap with alignment\n");

	// memcpy from mmap area to heap with alignment --- ok.
	memcpy( testdata, pBuf, 8 );

	printf( "ok\n");

	sleep( 1 );

	printf( "memcpy from mmap area(inside os) to heap without alignment \n");

	// memcpy from mmap area to heap without alignment --- ok.
	memcpy( testdata, pBuf + 5, 8 );

	printf( "ok\n");

	sleep( 1 );
	
	/************** test for mmap (outside os) ***********/

	printf( "memcpy from mmap area(outside os) to heap with alignment\n");

	// memcpy from mmap area to heap with alignment --- ok.
	memcpy( testdata, pBuf2, 8 );

	printf( "ok\n");

	sleep( 1 );

	printf( "memcpy from mmap area(outside os) to heap without alignment \n");

	// memcpy from mmap area to heap without alignment --- hangup.
	memcpy( testdata, pBuf2 + 5, 8 );

	printf( "ok\n");

	sleep( 1 );


	
	free( testdata );
//	free( testdata2);

	deleteMmap( pBuf, fd );
	deleteMmap( pBuf2, fd );

	return 0;
}



static int createMmap( void** ppBuf, int* pFd, unsigned long mapaddress )
{
	int fd;
	long sizeBuf;
	char buf[ 64 ];

	if(( fd = open( MMAP_FILE_PATH, O_RDWR )) == -1 )
	{
		strerror_r( errno, buf, 64 );
		printf( "open error (errno=%d, flag=%d): %s\n", errno, O_RDWR, buf );
		return -1;
	}

	sizeBuf = getMmapSize( g_mapsize );

	*ppBuf = mmap( NULL, sizeBuf, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, mapaddress );

	if( *ppBuf == ( void *)-1 )
	{
		strerror_r( errno, buf, 64 );
		printf( "%s: mmap: %s\n", __FUNCTION__, buf );
		close( fd );
		return -2;
	}

	printf( "mmap from 0x%lx to %p, size %ld\n", mapaddress, *ppBuf, sizeBuf );

	*pFd = fd;


	return 0;
}


static int deleteMmap( void* pBufAdr, int fd)
{
	long size;
	int ret = 0;
	char buf[ 64 ];

	// null check
	if( NULL == pBufAdr ) {
		return -1;
	}


	size = getMmapSize( g_mapsize );

	if( munmap( pBufAdr, size ) == -1 )
	{
		strerror_r( errno, buf, 64 );
		printf( "%s: munmap buf: %s\n", __FUNCTION__, buf );
		ret = -2;
	}

	if( close( fd ) == -1 )
	{
		strerror_r( errno, buf, 64 );
		printf( "dev/exmem close: %s\n", buf );
	}

	printf( "munmap at %p.\n", pBufAdr );

	return ret;
}


static size_t getMmapSize( size_t size )
{
	size_t pageSize;
	int pageNum;

	pageSize = sysconf( _SC_PAGESIZE );
	pageNum = (size % pageSize == 0)?( size / pageSize ):( size / pageSize + 1);
	return pageNum * pageSize;
}


/*! @} */

