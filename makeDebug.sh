#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
CPPFLAGS=-DDEBUG CXXFLAGS="-g -O0" && make && make install