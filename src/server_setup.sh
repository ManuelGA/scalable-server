#!/bin/bash

# File : firewall.sh
# Author : Manuel Gonzales
# Date : Feb 14, 2016
# Server Setup

#open file descriptors
ulimit -n 256000

#maxx listening connections
echo 1024 > /proc/sys/net/core/somaxconn
sysctl -w net.core.somaxconn=1024