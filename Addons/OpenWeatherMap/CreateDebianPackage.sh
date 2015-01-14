#!/bin/bash
if test -z $1
then
  echo "Please specify a version number (i. e. 1.0.0)."
  exit 0;
fi
if [[ $2 -lt 1 ]]
then
	echo "Please provide a revision number."
	exit 0;
fi
if test -z $3
then
  echo "Please specify a distribution (i. e. wheezy)."
  exit 0;
fi
version=$1

rm -Rf homegear-openweathermap*
mkdir homegear-openweathermap-$version
cp -R HM-XMLRPC-Client *.php *.xml debian homegear-openweathermap-$version
date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
echo "homegear-openweathermap ($version-$2) $3; urgency=low

  * See https://github.com/Homegear/Homegear/issues?q=is%3Aissue+is%3Aclosed

 -- Sathya Laufer <sathya@laufers.net>  $date" > homegear-openweathermap-$version/debian/changelog
tar -zcpf homegear-openweathermap_$version.orig.tar.gz homegear-openweathermap-$version
cd homegear-openweathermap-$version
debuild -us -uc
