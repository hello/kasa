/*============================================================================
  bandwidth 0.23, a benchmark to estimate memory transfer bandwidth.
  Copyright (C) 2005-2010 by Zack T Smith.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  The author may be reached at fbui@comcast.net.
 *===========================================================================*/

//---------------------------------------------------------------------------
// Change log
// 0.18	Grand unified version supports x86/intel64/arm, linux/win32/winmo.
// 0.19	Now have 128-bit writer that goes to cache AND one that bypasses.
// 0.20	Added my bmplib and graphing of output. Also added --slow option.
// 0.21	Adds random testing. Min chunk size = 256 B. Allows non-2^n chunks.
// 0.22	Adds register-to-register and register-to/from-stack transfers.
// 0.23	Adds vector-to-vector and register-to-vector transfers, & Mac support.
// 0.24	Adds network bandwidth tests from this PC to specified others.
//---------------------------------------------------------------------------

#ifndef _DEFS_H
#define _DEFS_H

#define VERSION "0.24a"
#define VERSION_W L"0.24a"
#define APPNAME L"Bandwidth WinMo "VERSION_W

#ifndef bool
typedef char bool;
enum { true = 1, false = 0 };
#endif

#define NETWORK_DEFAULT_PORTNUM (49000)
#define NETWORK_CHUNK_SIZE (8192)

extern int Reader (void *ptr, unsigned long size, unsigned long loops);
extern int RandomReader (void *ptr, unsigned long n_chunks, unsigned long loops);

extern int Writer (void *ptr, unsigned long size, unsigned long loops, unsigned long value);
extern int RandomWriter (void *ptr, unsigned long size, unsigned long loops, unsigned long value);

extern int RegisterToRegister (unsigned long);

extern int StackReader (unsigned long);
extern int StackWriter (unsigned long);

#ifndef __arm__
extern int RegisterToVector (unsigned long);	// SSE2
extern int Register8ToVector (unsigned long);	// SSE2
extern int Register16ToVector (unsigned long);	// SSE2
extern int Register32ToVector (unsigned long);	// SSE2
extern int Register64ToVector (unsigned long);	// SSE2

extern int VectorToVector (unsigned long);	// SSE2

extern int VectorToRegister (unsigned long);	// SSE2
extern int Vector8ToRegister (unsigned long);	// SSE2
extern int Vector16ToRegister (unsigned long);	// SSE2
extern int Vector32ToRegister (unsigned long);	// SSE2
extern int Vector64ToRegister (unsigned long);	// SSE2

extern int CopySSE (void*, void*, unsigned long, unsigned long);	// SSE2
extern int Copy (void*, void*, unsigned long, unsigned long);	

extern int ReaderSSE2 (void *ptr, unsigned long, unsigned long);
extern int RandomReaderSSE2 (unsigned long **ptr, unsigned long, unsigned long);

extern int WriterSSE2 (void *ptr, unsigned long, unsigned long, unsigned long);
extern int RandomWriterSSE2(unsigned long **ptr, unsigned long, unsigned long, unsigned long);

extern int WriterSSE2_bypass (void *ptr, unsigned long, unsigned long, unsigned long);
extern int RandomWriterSSE2_bypass (unsigned long **ptr, unsigned long, unsigned long, unsigned long);

extern int has_sse2 ();
#endif

#define FBLOOPS_R 400
#define FBLOOPS_W 800
#define FB_SIZE (640*480*2)

#endif

