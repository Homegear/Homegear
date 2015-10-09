#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -f /var/lib/homegear/modules/*
cd $SCRIPTDIR
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../libhomegear-base
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../homegear-homematicbidcos
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../homegear-homematicwired
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../homegear-insteon
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../homegear-max
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../homegear-philipshue
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
cd $SCRIPTDIR/../homegear-sonos
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc
