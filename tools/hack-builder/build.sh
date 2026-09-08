#!/usr/bin/env bash

gcc bytearray.c -c -o bytearray.o
g++ -g builder.cc bytearray.o -o hack-builder
