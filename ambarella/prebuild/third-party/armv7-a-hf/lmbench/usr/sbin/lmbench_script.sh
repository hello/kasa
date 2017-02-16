#!/bin/sh
echo ""
echo "NOTE:	This script implements part of the entire lmbench test."
echo "	If you want to complete all the lmbench test,"
echo "	please see third-party/lmbench/readme."

cd /usr/sbin/lmbench
echo ""

echo ""
echo "1. BANDWIDTH MEASUREMENTS"
echo "   Result Format:  Size(MB)  Speed(MB/s)"
echo ""

echo "File read bandwidth with open/close"
./bw_file_rd 1M open2close sample.txt
echo ""
echo "File read bandwidth with io only"
./bw_file_rd 1M io_only sample.txt
echo ""
echo "Memory read bandwidth(1): 4 byte read, 32 byte stride"
./bw_mem 1M rd
echo ""
echo "Memory write bandwidth(1): 4 byte write, 32 byte stride"
./bw_mem 1M wr
echo ""
echo "Memory read/write bandwidth: 4 byte read followed by 4 byte write to same place, 32 byte stride"
./bw_mem 1M rdwr
echo ""
echo "Memory copy bandwidth(1): 4 byte read then 4 byte write to different place, 32 byte stride"
./bw_mem 1M cp
echo ""
echo "Memory write bandwidth(2): write every 4 byte word"
./bw_mem 1M fwr
echo ""
echo "Memory read bandwidth(2): read every 4 byte word"
./bw_mem 1M frd
echo ""
echo "Memory copy bandwidth(2): copy every 4 byte word"
./bw_mem 1M fcp
echo ""
echo "Memory Operation: bzero"
./bw_mem 1M bzero
echo ""
echo "Memory Operation: bcopy"
./bw_mem 1M bcopy
echo ""
echo "Memory Operation: mmap with open/close file"
./bw_mmap_rd 1M open2close sample.txt
echo ""
echo "Memory Operation: mmap with io only"
./bw_mmap_rd 1M mmap_only sample.txt
echo ""
echo "Tcp bandwidth"
./bw_tcp localhost
echo ""
echo "Socket bandwidth"
./bw_unix
echo ""

echo ""
echo ""
echo "2. LATENCY MEASUREMENTS"
echo ""

echo "Latency for ls command"
./lat_cmd ls
echo ""
echo "Latency for connect"
./lat_connect localhost
echo ""
echo "Latency for context switch"
./lat_ctx -s 128K processes 2
echo ""
echo "latency for DRAM page"
./lat_dram_page -M 1M
echo ""
echo "Latency for file locking"
./lat_fcntl
echo ""
echo "Latency for creating and removing files: "
echo "(Result Format: file size, created files number, creations per second, removals per second)"
./lat_fs
echo ""
echo "Latency for memory read latency"
./lat_mem_rd 1M
echo ""
echo "Latency for mmap"
cp ./sample.txt /tmp
./lat_mmap 1M /tmp/sample.txt
rm -rf /tmp/sample.txt
echo ""
echo "Latency for basic CPU operations"
./lat_ops
echo ""
echo "Latency for the cost of pagefaulting pages from a file"
./lat_pagefault sample.txt
echo ""
echo "Latency for interprocess communication through pipes"
./lat_pipe
echo ""
echo "Latency for process creation"
cp ./hello /tmp
./lat_proc fork
./lat_proc exec
./lat_proc shell
./lat_proc procedure
echo ""
echo "latency for semaphore"
./lat_sem
echo ""
echo "Latency for doing a select on TCP"
./lat_select tcp
echo ""
echo "Latency for install and catch signals"
./lat_sig  install
./lat_sig  catch
./lat_sig  prot sample.txt
echo ""
echo "Latency for system calls"
./lat_syscall fstat sample.txt
./lat_syscall stat sample.txt
./lat_syscall open sample.txt
./lat_syscall write sample.txt
./lat_syscall read sample.txt
./lat_syscall null
echo ""
echo "Latency for interprocess communication via tcp/udp/UNIX sockets"
./lat_tcp localhost
./lat_udp localhost
./lat_unix
echo ""
echo "Latency for sleep"
./lat_usleep -u usleep 100
./lat_usleep -u nanosleep 100
./lat_usleep -u select 100
./lat_usleep -u itimer 100
echo ""

echo ""
echo ""
echo "3. OTHER MEASUREMENTS"
echo ""

echo "Cache line size"
./line
echo ""
echo "lmdd test"
./lmdd if=internal of=/tmp/file count=1000 fsync=1
echo ""
echo "CPU frequency"
./mhz
echo ""
echo "Sleep test - 3 secs"
./msleep 3000
echo "sleep test complete"
echo ""
echo "Memory parallelism test"
./par_mem -M 1M
echo ""
echo "Basic CPU operation parallelism test"
./par_ops
echo ""
echo "John McCalpin's STREAM test"
./stream -M 128K
echo ""
echo "TLB size and latency test - 1M"
./tlb -M 1M



