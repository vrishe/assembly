#!/bin/sh

DLL=$1
if [ -z "$DLL" ]; then
  DLL="Analytics.dll"
fi

mkdir build 2>/dev/null

APP=./build/assembly
if [[ "$OSTYPE" == "cygwin" ]]; then
	APP="$APP.exe"
	CC_FLAGS=-static

	cp `which cygwin1.dll` ./build
fi
