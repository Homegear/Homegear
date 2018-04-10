#!/bin/bash

if [ -z $1 ]; then
	echo "Please provide a file name for the backup."
	exit 1
fi

if [ ! -d /etc/openvpn ]; then
	mkdir /etc/openvpn
fi

service homegear reload

if [ -d /data/homegear-data ]; then
	tar -zcpf $1 /etc/homegear /etc/openvpn /data/homegear-data/db.sql* /data/homegear-data/node-blue /data/homegear-data/families /var/lib/homegear/audio /var/lib/homegear/scripts /var/lib/homegear/phpinclude /var/lib/homegear/www
else
	tar -zcpf $1 /etc/homegear /etc/openvpn /var/lib/homegear/db.sql.bak* /var/lib/homegear/node-blue/data /var/lib/homegear/families /var/lib/homegear/audio /var/lib/homegear/scripts /var/lib/homegear/phpinclude /var/lib/homegear/www
fi

chown homegear:homegear $1
