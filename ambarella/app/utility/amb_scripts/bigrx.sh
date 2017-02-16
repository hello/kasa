#!/bin/sh

sysctl -w net.core.rmem_max=10485760
sysctl -w net.core.rmem_default=1048576
sysctl -w net.ipv4.udp_mem="4096 1048576 10485760"
sysctl -w net.ipv4.udp_rmem_min=1048576
sysctl -w net.ipv4.route.flush=1

