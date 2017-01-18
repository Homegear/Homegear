#!/bin/bash
if [ -n "$1" ]; then
	BUILDTHREADS=$1
else
	BUILDTHREADS=2
fi
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR
rm -Rf autom4te.cache
./bootstrap || exit 1
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib || exit 1
make -j${BUILDTHREADS} || exit 1
strip -s homegear-miscellaneous/src/.libs/mod_miscellaneous.so
strip -s src/homegear
make install
