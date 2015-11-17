#!/bin/sh
libtoolize || exit 1
autoreconf --install || exit 1
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib || exit 1
