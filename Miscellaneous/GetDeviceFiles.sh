#!/bin/bash
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#TMPDIR="/tmp"
#FREESPACE=`df -P $TMPDIR | tail -1 | awk '{print $4}'`
#if [[ "$FREESPACE" -lt 4000000 ]]; then
#        echo "There is not enough free space on /tmp. Please specify a location to store temporary files. They will be $
#        read TMPDIR;
#fi
#echo $TMPDIR;
rm -Rf /tmp/HomegearDeviceTypes
[ $? -ne 0 ] && exit 1
mkdir /tmp/HomegearDeviceTypes
[ $? -ne 0 ] && exit 1
wget -P /tmp/HomegearDeviceTypes/ http://www.eq-3.de/Downloads/Software/HM-CCU1-Firmware_Updates/1.513/hm-ccu-1513a.zip
[ $? -ne 0 ] && exit 1
unzip -d /tmp/HomegearDeviceTypes /tmp/HomegearDeviceTypes/hm-ccu-1513a.zip hm-ccu-firmware-1.513.img
[ $? -ne 0 ] && exit 1
tar -zxf /tmp/HomegearDeviceTypes/hm-ccu-firmware-1.513.img -C /tmp/HomegearDeviceTypes
[ $? -ne 0 ] && exit 1
tar -zxf /tmp/HomegearDeviceTypes/root_fs.tar.gz -C /tmp/HomegearDeviceTypes
[ $? -ne 0 ] && exit 1
rm -f /tmp/HomegearDeviceTypes/firmware/rftypes/rf_cmm.xml
rm -f /tmp/HomegearDeviceTypes/firmware/hs485types/hmw_central.xml
rm -f /tmp/HomegearDeviceTypes/firmware/hs485types/hmw_generic.xml
mv /tmp/HomegearDeviceTypes/firmware/hs485types/* /tmp/HomegearDeviceTypes/firmware/rftypes/
[ $? -ne 0 ] && exit 1
for i in /tmp/HomegearDeviceTypes/firmware/rftypes/*.xml; do
	content=`cat $i`
	echo $content | xmllint --noblanks - > $i
	[ $? -ne 0 ] && exit 1
done
for i in /tmp/HomegearDeviceTypes/firmware/rftypes/*.xml; do
	content=`cat $i`
	echo $content | xmllint --format - > $i
	[ $? -ne 0 ] && exit 1
done
patch -d /tmp/HomegearDeviceTypes/firmware/rftypes -p1 < $SCRIPTDIR/DeviceTypePatch.patch
[ $? -ne 0 ] && exit 1
mkdir -p /etc/homegear/devices/0
[ $? -ne 0 ] && exit 1
mkdir -p /etc/homegear/devices/1
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceTypes/firmware/rftypes/rf_* /etc/homegear/devices/0
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceTypes/firmware/rftypes/hmw_* /etc/homegear/devices/1
[ $? -ne 0 ] && exit 1
rm -Rf /tmp/HomegearDeviceTypes
chown -R root:root /etc/homegear/devices
chmod 755 /etc/homegear/devices
chmod 755 /etc/homegear/devices/0
chmod 644 /etc/homegear/devices/0/*
chmod 755 /etc/homegear/devices/1
chmod 644 /etc/homegear/devices/1/*
