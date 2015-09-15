#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR/libhomegear-base
make clean
./configure && make && make install
cd $SCRIPTDIR/homegear
make clean
./configure && make && make install
cd $SCRIPTDIR/homegear-miscellaneous
make clean
./configure && make && make install
