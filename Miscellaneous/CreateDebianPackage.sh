#!/bin/bash
if [[ $1 -lt 1 ]]
then
	echo "Please provide a revision number."
	exit 0;
fi
if test -z $2
then
  echo "Please specify branch."
  exit 0;
fi
wget https://github.com/hfedcba/Homegear/archive/$2.zip
unzip $2.zip
rm $2.zip
version=$(head -n 1 Homegear-$2/Version.h | cut -d " " -f3 | tr -d '"')
sourcePath=homegear-$version
mv Homegear-$2 $sourcePath
rm -Rf $sourcePath/.* 1>/dev/null 2>&2
rm -Rf $sourcePath/obj
rm -Rf $sourcePath/bin
tar -zcpf homegear_$version.orig.tar.gz $sourcePath
cd $sourcePath
dch -v $version-$1 -M
debuild -us -uc
cd ..
rm -Rf $sourcePath
rm homegear_$version-?_*.build
rm homegear_$version-?_*.changes
rm homegear_$version-?.debian.tar.gz
rm homegear_$version-?.dsc
rm homegear_$version.orig.tar.gz
