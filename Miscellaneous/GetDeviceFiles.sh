#!/bin/sh
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
FIRMWAREDIR=/tmp/HomegearTemp/rootfs/rootfs.ubi/125662337/root/firmware
NODELETE=0
if [ "$#" -eq "1" ] && [ "$1" -eq "1" ]; then
    NODELETE=1
fi
#TMPDIR="/tmp"
#FREESPACE=`df -P $TMPDIR | tail -1 | awk '{print $4}'`
#if [[ "$FREESPACE" -lt 4000000 ]]; then
#        echo "There is not enough free space on /tmp. Please specify a location to store temporary files. They will be $
#        read TMPDIR;
#fi
#echo $TMPDIR;
rm -Rf /tmp/HomegearTemp
[ $? -ne 0 ] && exit 1
mkdir /tmp/HomegearTemp
[ $? -ne 0 ] && exit 1
wget -P /tmp/HomegearTemp/ http://www.eq-3.de/Downloads/Software/HM-CCU2-Firmware_Updates/HM-CCU2-2.11.6/HM-CCU-2.11.6.tar.gz
[ $? -ne 0 ] && exit 1
tar -zxf /tmp/HomegearTemp/HM-CCU-2.11.6.tar.gz -C /tmp/HomegearTemp
[ $? -ne 0 ] && exit 1
rm -f /tmp/HomegearTemp/HM-CCU-2.11.6.tar.gz

echo "Downloading UBI Reader..."
echo "(C) 2013 Jason Pruitt (Jason Pruitt), see https://github.com/jrspruitt/ubi_reader"
wget -P /tmp/HomegearTemp/ https://github.com/jrspruitt/ubi_reader/archive/v2_ui.tar.gz
[ $? -ne 0 ] && exit 1
tar -zxf /tmp/HomegearTemp/v2_ui.tar.gz -C /tmp/HomegearTemp
[ $? -ne 0 ] && exit 1

/tmp/HomegearTemp/ubi_reader-2_ui/extract_files.py -o /tmp/HomegearTemp/rootfs /tmp/HomegearTemp/rootfs.ubi
[ $? -ne 0 ] && exit 1

rm -f $FIRMWAREDIR/rftypes/rf_cmm.xml
rm -f $FIRMWAREDIR/hs485types/hmw_central.xml
rm -f $FIRMWAREDIR/hs485types/hmw_generic.xml
mv $FIRMWAREDIR/hs485types/* $FIRMWAREDIR/rftypes/
[ $? -ne 0 ] && exit 1
for i in $FIRMWAREDIR/rftypes/*.xml; do
	content=`cat $i`
	echo $content | xmllint --noblanks - > $i
	[ $? -ne 0 ] && exit 1
done
for i in $FIRMWAREDIR/rftypes/*.xml; do
	content=`cat $i`
	echo $content | xmllint --format - > $i
	[ $? -ne 0 ] && exit 1
done
patch -d $FIRMWAREDIR/rftypes -p1 < $SCRIPTDIR/DeviceTypePatch.patch
[ $? -ne 0 ] && exit 1
mkdir -p /etc/homegear/devices/0
[ $? -ne 0 ] && exit 1
mkdir -p /etc/homegear/devices/1
[ $? -ne 0 ] && exit 1
mv $FIRMWAREDIR/rftypes/rf_* /etc/homegear/devices/0
[ $? -ne 0 ] && exit 1
mv $FIRMWAREDIR/rftypes/hmw_* /etc/homegear/devices/1
[ $? -ne 0 ] && exit 1

if [ "$NODELETE" -eq "0" ]; then
	rm -Rf /tmp/HomegearTemp
fi
chown -R root:root /etc/homegear/devices
chmod 755 /etc/homegear/devices
chmod 755 /etc/homegear/devices/0
chmod 644 /etc/homegear/devices/0/*
chmod 755 /etc/homegear/devices/1
chmod 644 /etc/homegear/devices/1/*
