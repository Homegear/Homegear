#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
./configure && make && make install