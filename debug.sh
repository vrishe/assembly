#!/bin/sh

. ./prepare_.sh
g++ -x c++ --std=c++17 $CC_FLAGS -mpopcnt -g -o build/assembly assembly.cpp\
	&& gdb --args $APP "$DLL"