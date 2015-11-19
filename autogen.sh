#!/bin/sh
libtoolize || exit 1
aclocal || exit 1
autoheader || exit 1
automake --add-missing || exit 1
autoconf || exit 1

# It is not recommended to run configure in this script
# Thanks to Oli: https://forum.homegear.eu/viewtopic.php?p=2532#p2532, https://www.sourceware.org/autobook/autobook/autobook_43.html
#./configure --prefix=/usr --localstatedir=/var --sysconfdir=/etc --libdir=/usr/lib || exit 1
