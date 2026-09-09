#!/usr/bin/env bash

gcc bytearray.c -c -o bytearray.o
g++ -Wall -g builder.cc bytearray.o -o hack-builder 
