#!/bin/bash

wget https://github.com/hfedcba/Homegear/archive/master.zip
unzip master.zip
rm master.zip
version=$(head -n 1 Homegear-master/Version.h | cut -d " " -f3 | tr -d '"')
sourcePath=homegear-$version
mv Homegear-master $sourcePath
rm -Rf $sourcePath/.* 1>/dev/null 2>&2
rm -Rf $sourcePath/obj
rm -Rf $sourcePath/bin
sed -i 's/make config=release/make config=debug/g' $sourcePath/debian/rules
sed -i 's/$(CURDIR)\/bin\/Release\/homegear/$(CURDIR)\/bin\/Debug\/homegear/g' $sourcePath/debian/rules
tar -zcpf homegear_$version.orig.tar.gz $sourcePath
cd $sourcePath
dch -v $version-1 -M
debuild -us -uc
cd ..
rm -Rf $sourcePath
rm homegear_$version-?_*.build
rm homegear_$version-?_*.changes
rm homegear_$version-?.debian.tar.gz
rm homegear_$version-?.dsc
rm homegear_$version.orig.tar.gz
