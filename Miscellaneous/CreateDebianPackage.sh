#!/bin/bash

wget https://github.com/hfedcba/Homegear/archive/master.zip
unzip master.zip
rm master.zip
version=$(head -n 1 Homegear-master/Version.h | cut -d " " -f3 | tr -d '"')
sourcePath=homegear-$version
mv Homegear-master $sourcePath
rm -Rf $sourcePath/.*
rm -Rf $sourcePath/obj
rm -Rf $sourcePath/bin
tar -zcpf homegear_$version.orig.tar.gz $sourcePath
cd $sourcePath && debuild -us -uc
