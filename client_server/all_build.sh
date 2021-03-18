#!/bin/sh
set -e
make -f Makefile
echo $PWD
gcc -Wall server_combined.c $PWD/snappy.o $PWD/util.o $PWD/map.o -o server -lrt -g
gcc client_combined.c -o client -lrt
