#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -f /usr/share/homegear/modules/*
if [ "$1" = "--with-base" ]; then
	cd $SCRIPTDIR/libhomegear-base
	CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && sudo make install
elif [ -n "$1" ]; then
	echo "Usage:"
	echo "  --with-base:	Compile base lib"
	exit 0
fi
cd $SCRIPTDIR/homegear
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8
cd $SCRIPTDIR/homegear-miscellaneous
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install
cd $SCRIPTDIR/../homegear-sonos
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install
cd $SCRIPTDIR/../homegear-homematicbidcos
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install
cd $SCRIPTDIR/../homegear-homematicwired
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install
