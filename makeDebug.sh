#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
if [ "$1" = "--with-base" ]; then
	cd $SCRIPTDIR/libhomegear-base
	CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make && sudo make install
elif [ -n "$1" ]; then
	echo "Usage:"
	echo "  --with-base:	Compile base lib"
	exit 0
fi
cd $SCRIPTDIR/homegear
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make
cd $SCRIPTDIR/homegear-miscellaneous
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make && make install
