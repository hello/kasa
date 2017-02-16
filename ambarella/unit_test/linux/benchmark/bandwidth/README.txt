
This is the Readme file for my program
called "bandwidth".

Bandwidth is a benchmark that attempts to measure
primarily memory bandwidth. In December 2010 and
as of release 0.24a, I have extended 'bandwidth'
to measure network bandwidth as well.

It's useful because hardware specifications are 
sometimes incomplete or misleading.

--------------------------------------------------
MEMORY BANDWIDTH 

Bandwidth performs sequential and random
reads and writes of varying sizes. This permits 
you to see in the numbers how each type of memory 
is performing.  So for instance when bandwidth
writes a 256-byte chunk, you know that because
caches are normally write-back, this chunk
will reside entirely in the L1 cache. Whereas
a 512 kB chunk will mainly reside in L2.

You could run a non-artificial benchmark and 
observe that a general performance number is lower 
on that machine, but that conceals the cause. 
So the purpose of this program is to help you 
pinpoint the cause of a performance problem,
and determine whether it is memory related.
It also tells you the best-case scenario i.e.
the maximum bandwidth achieved using sequential,
128-bit memory accesses.

Version 0.24 adds network bandwidth testing.

Version 0.23 adds:
- Mac OS/X 64-bit support.
- Vector-to-vector register transfer test.
- Main register to/from vector register transfer test.
- Main register byte/word/dword/qword to/from 
  vector register test (pinsr*, pextr* instructions).
- Memory copy test using SSE2.
- Automatic checks under Linux for SSE2 & SSE4.

Version 0.22 adds:
- Register-to-register transfer test.
- Register-to/from-stack transfer tests.

Version 0.21 adds:
- Standardized memory chunks to always be
  a multiple of 256-byte mini-chunks.
- Random memory accesses, in which each 
  256-byte mini-chunk accessed is accessed 
  in a random order, but also, inside each 
  mini-chunk the 32/64/128 data are accessed
  pseudo-randomly as well. 
- Now 'bandwidth' includes chunk sizes that 
  are not powers of 2, which increases 
  data points around the key chunk sizes 
  corresponding to common L1 and L2 cache 
  sizes.
- Command-line options:
	--quick for 0.25 seconds per test.
	--slow for 20 seconds per test.
	--title for adding a graph title.

Version 0.20 added graphing, with the graph
stored in a BMP image file. It also adds the
--slow option for more precise runs.

Version 0.19 added a second 128-bit SSE writer
routine that bypasses the caches, in addition
to the one that doesn't.

Version 0.18 was my Grand Unified bandwidth
benchmark that brought together support for
four operating systems:
	- Linux
	- Windows Mobile
	- 32-bit Windows
	- Mac OS/X 64-bit
and three processor architectures:
	- x86
	- Intel64
	- ARM 
I've written custom assembly routines for
each architecture.

Total run time for the default speed, which
has 5 seconds per test, is about 35 minutes.

--------------------------------------------------
NETWORK BANDWIDTH (beginning with release 0.24a)

In December 2010, I extended bandwidth to measure
network bandwidth, which is useful for testing
your home or work network setup, and in theory
could be used to test larger networks as well.

The network test is pretty simple. It sends chunks
of data of varying sizes to whatever computers
(nodes) that you specify. Each of those must be
running 'bandwidth' in transponder mode.

The chunks of data range of 8 kB up to 32 MB.

Sample output:
	output/Network-Mac-Linux-Win32.txt

How to start a transponder:
	./bandwidth-mac64 --transponder

Example invocation of the test leader:
	./bandwidth64 --network 192.168.1.104

Areas for improvement:

At present, the output of this test is not graphed.

At present, the 'leader' in the test interacts
with the specified nodes by sending data but
not by receiving it.

At present, the specified nodes do not interact
with one another as part of the test.

At present, it is not known whether the network
code will work on ARM devices.
I've only tested it on 
	Linux 32-bit
	Mac OS/X 32- and 64-bit
	Win/Cygwin 32-bit.

At present, it uses port 49000 but later
the port will be specifiable.

--------------------------------------------------
This program is provided without any warranty
and AS-IS. See the file COPYING for details.

Zack Smith
fbui@comcast.net
December 2010

