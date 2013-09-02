#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools
# made possible by:
#   Klaus M Pfeiffer (http://blog.kmp.or.at/2012/05/build-your-own-raspberry-pi-image/)
#   Alex Bradbury (http://asbradbury.org/projects/spindle/)

deb_mirror="http://archive.raspbian.org/raspbian/"
deb_local_mirror=$deb_mirror
deb_release="wheezy"

bootsize="64M"
buildenv="/root/rpi"
rootfs="${buildenv}/rootfs"
bootfs="${rootfs}/boot"

mydate=`date +%Y%m%d`

if [ $EUID -ne 0 ]; then
  echo "ERROR: This tool must be run as Root"
  exit 1
fi

echo "Creating image..."
mkdir -p $buildenv
image="${buildenv}/rpi_homegear_${deb_release}_${mydate}.img"
dd if=/dev/zero of=$image bs=1MB count=1000
device=`losetup -f --show $image`
echo "Image $image Created and mounted as $device"

fdisk $device << EOF
n
p
1

+$bootsize
t
c
n
p
2


w
EOF


losetup -d $device
device=`kpartx -va $image | sed -E 's/.*(loop[0-9])p.*/\1/g' | head -1`
echo "--- kpartx device ${device}"
device="/dev/mapper/${device}"
bootp=${device}p1
rootp=${device}p2

mkfs.vfat $bootp
mkfs.ext4 $rootp

mkdir -p $rootfs

mount $rootp $rootfs

cd $rootfs
debootstrap --no-check-gpg --foreign --arch=armhf $deb_release $rootfs $deb_local_mirror

cp /usr/bin/qemu-arm-static usr/bin/
LANG=C chroot $rootfs /debootstrap/debootstrap --second-stage

mount $bootp $bootfs

echo "deb $deb_local_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

echo "blacklist i2c-bcm2708" > etc/modprobe.d/raspi-blacklist.conf

echo "dwc_otg.lpm_enable=0 console=ttyUSB0,115200 kgdboc=ttyUSB0,115200 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait" > boot/cmdline.txt

echo "proc            /proc           proc    defaults        0       0
/dev/mmcblk0p1  /boot           vfat    defaults        0       2
/dev/mmcblk0p2  /           ext4    defaults        0       1
" > etc/fstab

echo "homegearpi" > etc/hostname

echo "auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
" > etc/network/interfaces

echo "console-common    console-data/keymap/policy      select  Select keymap from full list
console-common  console-data/keymap/full        select  us
" > debconf.set

wget http://homegear.eu/downloads/homegear_current_armhf.deb

echo "#!/bin/bash
debconf-set-selections /debconf.set
rm -f /debconf.set
apt-get update
apt-get -y install locales console-common ntp openssh-server git-core binutils ca-certificates sudo
wget http://goo.gl/1BOfJ -O /usr/bin/rpi-update
chmod +x /usr/bin/rpi-update
rpi-update
useradd --create-home --shell /bin/bash --user-group pi
echo \"pi:raspberry\" | chpasswd
echo \"root:raspberry\" | chpasswd
echo \"pi ALL=(ALL) NOPASSWD: ALL\" >> /etc/sudoers
sed -i -e 's/KERNEL\!=\"eth\*|/KERNEL\!=\"/' /lib/udev/rules.d/75-persistent-net-generator.rules
dpkg-divert --add --local /lib/udev/rules.d/75-persistent-net-generator.rules
dpkg -i /homegear_current_armhf.deb
service homegear stop
service ssh stop
rm -f third-stage
" > third-stage
chmod +x third-stage
LANG=C chroot $rootfs /third-stage

rm homegear_current_armhf.deb

mkdir /scripts
chown root:root /scripts
chmod 750 /scripts

echo "#!/bin/bash
return=`ps -A | grep homegear -c`
if [ $return -lt 1 ] && test -e /var/run/homegear/homegear.pid; then
        LOGDIR=/var/log/homegear
        if test -e $LOGDIR/core; then
                COREDIR=$LOGDIR/coredumps
                mkdir -p $COREDIR
                DIR=0
                while test -d $COREDIR/$DIR; do
                        ((DIR++))
                done
                COREDIR=$COREDIR/$DIR
                mkdir -p $COREDIR
                mv $LOGDIR/core $COREDIR
                mv $LOGDIR/homegear.log $COREDIR
                mv $LOGDIR/homegear.err $COREDIR
                cp /usr/bin/homegear $COREDIR
        fi

        /etc/init.d/homegear restart
fi" > /scripts/checkServices.sh
chown root:root /scripts/checkServices.sh
chmod 750 /scripts/checkServices.sh

echo "*/10 *  *       *       *       /scripts/checkServices.sh 2>&1 |/usr/bin/logger -t CheckServices" >> /var/spool/cron/crontabs/root

echo "deb $deb_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

echo "#!/bin/bash
apt-get clean
rm -f cleanup
" > cleanup
chmod +x cleanup
LANG=C chroot $rootfs /cleanup

cd

read -p "Copy your boot files into ${bootfs} then hit [Enter] to continue..."

umount $bootp
umount $rootp

kpartx -d $image

rm -Rf $rootfs

echo "Created Image: $image"

echo "Done."
