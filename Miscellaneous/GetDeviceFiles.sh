#!/bin/bash
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
rm -Rf /tmp/HomegearDeviceTypes
mkdir /tmp/HomegearDeviceTypes
wget -P /tmp/HomegearDeviceTypes/ http://www.eq-3.de/Downloads/Software/Konfigurationsadapter/Konfigurationsadapter_USB/HM-CFG-USB_Usersoftware_V1_511_eQ-3_131018.zip
unzip -d /tmp/HomegearDeviceTypes /tmp/HomegearDeviceTypes/HM-CFG-USB_Usersoftware_V1_511_eQ-3_131018.zip
mv /tmp/HomegearDeviceTypes/Setup_BidCos-Service.exe /tmp/HomegearDeviceTypes/Setup_BidCos-Service.exe.7z
7z x -o/tmp/HomegearDeviceTypes/ /tmp/HomegearDeviceTypes/Setup_BidCos-Service.exe.7z
patch -d /tmp/HomegearDeviceTypes/OFFLINE/7083BBE7/ADD945A7 -p1 < $SCRIPTDIR/DeviceTypePatch.patch
mkdir -p /etc/homegear/Device\ types
mv /tmp/HomegearDeviceTypes/OFFLINE/7083BBE7/ADD945A7/* /etc/homegear/Device\ types
rm -Rf /tmp/HomegearDeviceTypes
chown -R root:root /etc/homegear/Device\ types
chmod 755 /etc/homegear/Device\ types
chmod 644 /etc/homegear/Device\ types/*
rm -f /etc/homegear/Device\ types/rf_cmm.xml
