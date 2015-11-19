#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR
./bootstrap || exit 1
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib || exit 1
make && make install
