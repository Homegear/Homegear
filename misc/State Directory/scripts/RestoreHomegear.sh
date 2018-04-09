#!/bin/bash

if [ -z $1 ]; then
	echo "Please provide the file name of the backup."
	exit 1
fi

service homegear stop
TEMPDIR=$(mktemp -d)

tar -zxf $1 -C $TEMPDIR
if test ! -d $TEMPDIR/etc/homegear || test ! -f $TEMPDIR/etc/homegear/main.conf; then
	rm -Rf $TEMPDIR
	exit 1
fi

if test ! -f $TEMPDIR/data/homegear-data/db.sql.bak0 && test ! -f $TEMPDIR/var/lib/homegear/db.sql.bak0; then
	rm -Rf $TEMPDIR
	exit 1
fi

if [ -d $TEMPDIR/data/homegear-data ]; then
	mv $TEMPDIR/data/homegear-data/db.sql* $TEMPDIR/var/lib/homegear/
	mv $TEMPDIR/data/homegear-data/families $TEMPDIR/var/lib/homegear/
	rm -Rf $TEMPDIR/var/lib/homegear/node-blue/data
	mv $TEMPDIR/data/homegear-data/node-blue $TEMPDIR/var/lib/homegear/node-blue/data
fi

TIME=$(date +%s)

# Backup old data
mv /etc/homegear/ /etc/homegear.bak${TIME}
cp -a $TEMPDIR/etc/homegear /etc/
mv /etc/openvpn /etc/openvpn.bak${TIME}
cp -a $TEMPDIR/etc/openvpn /etc/

cp -a /var/lib/homegear /var/lib/homegear.bak${TIME}
rm -Rf /var/lib/homegear/audio
rm -Rf /var/lib/homegear/phpinclude
rm -Rf /var/lib/homegear/scripts
rm -Rf /var/lib/homegear/www

if [ -d /data/homegear-data];
	mv /data/homegear-data /data/homegear-data.bak${TIME}

	cp -a $TEMPDIR/data/homegear-data /data/
	cp -a $TEMPDIR/var/lib/homegear/db.sql* /data/homegear-data/
	cp -a $TEMPDIR/var/lib/homegear/families /data/homegear-data/
	cp -a $TEMPDIR/var/lib/homegear/node-blue/data /data/homegear-data/node-blue
	rm -Rf $TEMPDIR/var/lib/homegear/db.sql*
	rm -Rf $TEMPDIR/var/lib/homegear/families
	rm -Rf $TEMPDIR/var/lib/homegear/node-blue
else
	rm -f /var/lib/homegear/db.sql*
	rm -Rf /var/lib/homegear/families
	rm -Rf /var/lib/homegear/node-blue/data
fi

cp -a $TEMPDIR/var/lib/homegear/* /var/lib/homegear/

rm -Rf $TEMPDIR
