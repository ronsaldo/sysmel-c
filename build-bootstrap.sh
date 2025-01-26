#!/bin/sh
mkdir -p build
gcc -Wall -Wextra -g -O2 -o build/bootstrap bootstrap/unity.c
