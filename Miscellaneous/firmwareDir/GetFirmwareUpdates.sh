#!/bin/bash
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
wget -P $SCRIPTDIR http://www.eq-3.de/Downloads/Software/Firmware/hm_cc_rt_dn_update_V1_2_007_131202.tar.gz
[ $? -ne 0 ] && exit 1
tar -zxf $SCRIPTDIR/hm_cc_rt_dn_update_V1_2_007_131202.tar.gz -C $SCRIPTDIR
[ $? -ne 0 ] && exit 1
mv $SCRIPTDIR/hm_cc_rt_dn_update_V1_2_007_131202.eq3 $SCRIPTDIR/0000.00000095.fw
[ $? -ne 0 ] && exit 1
rm $SCRIPTDIR/hm_cc_rt_dn_update_V1_2_007_131202.tar.gz
rm $SCRIPTDIR/changelog.txt
echo "12" > $SCRIPTDIR/0000.00000095.version
[ $? -ne 0 ] && exit 1


rm -Rf /tmp/HomegearDeviceFirmware
[ $? -ne 0 ] && exit 1
mkdir /tmp/HomegearDeviceFirmware
[ $? -ne 0 ] && exit 1
wget -P /tmp/HomegearDeviceFirmware/ http://www.eq-3.de/Downloads/Software/HM-CCU1-Firmware_Updates/1.514/hm-ccu-1.514.zip
[ $? -ne 0 ] && exit 1
unzip -d /tmp/HomegearDeviceFirmware /tmp/HomegearDeviceFirmware/hm-ccu-1.514.zip hm-ccu-firmware-1.514.img
[ $? -ne 0 ] && exit 1
tar -zxf /tmp/HomegearDeviceFirmware/hm-ccu-firmware-1.514.img -C /tmp/HomegearDeviceFirmware
[ $? -ne 0 ] && exit 1
tar -zxf /tmp/HomegearDeviceFirmware/root_fs.tar.gz -C /tmp/HomegearDeviceFirmware
[ $? -ne 0 ] && exit 1

mv /tmp/HomegearDeviceFirmware/firmware/hmw_io_4_fm_hw0.hex $SCRIPTDIR/0001.00001000.fw
[ $? -ne 0 ] && exit 1
echo "0306" > $SCRIPTDIR/0001.00001000.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_lc_sw2_dr_hw0.hex $SCRIPTDIR/0001.00001100.fw
[ $? -ne 0 ] && exit 1
echo "0306" > $SCRIPTDIR/0001.00001100.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_io12_sw7_dr_hw0.hex $SCRIPTDIR/0001.00001200.fw
[ $? -ne 0 ] && exit 1
echo "0306" > $SCRIPTDIR/0001.00001200.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_lc_dim1l_dr_hw0.hex $SCRIPTDIR/0001.00001400.fw
[ $? -ne 0 ] && exit 1
echo "0303" > $SCRIPTDIR/0001.00001400.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_lc_bl1_dr_hw0.hex $SCRIPTDIR/0001.00001500.fw
[ $? -ne 0 ] && exit 1
echo "0306" > $SCRIPTDIR/0001.00001500.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_io_sr_fm_hw0_unstable.hex $SCRIPTDIR/0001.00001600.fw
[ $? -ne 0 ] && exit 1
echo "0001" > $SCRIPTDIR/0001.00001600.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_sen_sc_12_dr_hw0.hex $SCRIPTDIR/0001.00001900.fw
[ $? -ne 0 ] && exit 1
echo "0301" > $SCRIPTDIR/0001.00001900.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_sen_sc_12_fm_hw0.hex $SCRIPTDIR/0001.00001A00.fw
[ $? -ne 0 ] && exit 1
echo "0301" > $SCRIPTDIR/0001.00001A00.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_io_12_fm_hw0.hex $SCRIPTDIR/0001.00001B00.fw
[ $? -ne 0 ] && exit 1
echo "0300" > $SCRIPTDIR/0001.00001B00.version
[ $? -ne 0 ] && exit 1
mv /tmp/HomegearDeviceFirmware/firmware/hmw_io12_sw14_dr_hw0.hex $SCRIPTDIR/0001.00001C00.fw
[ $? -ne 0 ] && exit 1
echo "0100" > $SCRIPTDIR/0001.00001C00.version
[ $? -ne 0 ] && exit 1
rm -Rf /tmp/HomegearDeviceFirmware


chown homegear:homegear $SCRIPTDIR/*.fw
chown homegear:homegear $SCRIPTDIR/*.version
chmod 444 $SCRIPTDIR/*.fw
chmod 444 $SCRIPTDIR/*.version