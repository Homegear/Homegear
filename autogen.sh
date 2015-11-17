#!/bin/sh
autoreconf --install || exit 1
./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib
