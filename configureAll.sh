#!/bin/bash
function configure()
{
	cd $1
	libtoolize
	aclocal
	autoconf
	automake --add-missing
	./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib
}

SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -f /var/lib/homegear/modules/*
configure $SCRIPTDIR
configure $SCRIPTDIR/../libhomegear-base
configure $SCRIPTDIR/../homegear-homematicbidcos
configure $SCRIPTDIR/../homegear-homematicwired
configure $SCRIPTDIR/../homegear-insteon
configure $SCRIPTDIR/../homegear-max
configure $SCRIPTDIR/../homegear-philipshue
configure $SCRIPTDIR/../homegear-sonos
