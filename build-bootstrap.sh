#!/bin/sh
mkdir -p build
gcc -Wall -Wextra -g -o build/bootstrap bootstrap/unity.c
