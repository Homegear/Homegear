#!/bin/bash
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
wget -P $SCRIPTDIR http://www.eq-3.de/Downloads/Software/Firmware/hm_cc_rt_dn_update_V1_2_007_131202.tar.gz
[ $? -ne 0 ] && exit 1
tar -zxf $SCRIPTDIR/hm_cc_rt_dn_update_V1_2_007_131202.tar.gz -C $SCRIPTDIR
[ $? -ne 0 ] && exit 1
mv $SCRIPTDIR/hm_cc_rt_dn_update_V1_2_007_131202.eq3 $SCRIPTDIR/0000.00000095.fw
rm $SCRIPTDIR/hm_cc_rt_dn_update_V1_2_007_131202.tar.gz
rm $SCRIPTDIR/changelog.txt
[ $? -ne 0 ] && exit 1
echo "12" > $SCRIPTDIR/0000.00000095.version
[ $? -ne 0 ] && exit 1
chown homegear:homegear $SCRIPTDIR/*.fw
chown homegear:homegear $SCRIPTDIR/*.version
chmod 444 $SCRIPTDIR/*.fw
chmod 444 $SCRIPTDIR/*.version
