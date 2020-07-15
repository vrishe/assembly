#!/bin/sh

. ./prepare_.sh
g++ -x c++ --std=c++17 $CC_FLAGS -mpopcnt -O2 -o build/assembly assembly.cpp\
	&& $APP "$DLL"