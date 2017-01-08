#!/bin/bash
if [ -n "$1" ]; then
        BUILDTHREADS=$1
else
        BUILDTHREADS=2
fi

SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR
make clean
make distclean
rm -Rf autom4te.cache
./bootstrap || exit 1
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib || exit 1
cov-build --dir cov-int make -j${BUILDTHREADS}
tar -czvf homegear.tgz cov-int
rm -Rf cov-int
make clean
make distclean
