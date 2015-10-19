#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -f /var/lib/homegear/modules/*
cd $SCRIPTDIR
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8
cp homegear-miscellaneous/src/.libs/mod_miscellaneous.so /var/lib/homegear/modules

cd $SCRIPTDIR/../homegear-homematicbidcos
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install

#cd $SCRIPTDIR/../homegear-homematicwired
#CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install

#cd $SCRIPTDIR/../homegear-insteon
#CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install

#cd $SCRIPTDIR/../homegear-max
#CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install

#cd $SCRIPTDIR/../homegear-philipshue
#CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install

cd $SCRIPTDIR/../homegear-sonos
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make -j8 && make install
