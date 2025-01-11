#!/bin/sh
mkdir -p build
gcc -Wall -Wextra -o build/bootstrap bootstrap/unity.c
