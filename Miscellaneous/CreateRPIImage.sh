#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools
# made possible by:
#   Klaus M Pfeiffer (http://blog.kmp.or.at/2012/05/build-your-own-raspberry-pi-image/)
#   Alex Bradbury (http://asbradbury.org/projects/spindle/)

deb_mirror="http://mirrordirector.raspbian.org/raspbian/"
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

#Setup network settings
echo "homegearpi" > etc/hostname
echo -e "127.0.0.1\thomegearpi" >> etc/hosts

echo "auto lo
iface lo inet loopback
iface lo inet6 loopback

auto eth0
iface eth0 inet dhcp
iface eth0 inet6 auto
" > etc/network/interfaces
#End network settings

echo "console-common    console-data/keymap/policy      select  Select keymap from full list
console-common  console-data/keymap/full        select  us
" > debconf.set

echo "#!/bin/bash
debconf-set-selections /debconf.set
rm -f /debconf.set
mkdir -p /etc/apt/sources.list.d/
echo \"deb http://homegear.eu/packages/Raspbian/ wheezy/\" >> /etc/apt/sources.list.d/homegear.list
wget http://homegear.eu/packages/Release.key
apt-key add - < Release.key
rm Release.key
apt-get update
apt-get -y install locales console-common ntp openssh-server git-core binutils curl ca-certificates sudo parted unzip p7zip-full php5-cli php5-xmlrpc libxml2-utils keyboard-configuration liblzo2-dev python-lzo libgcrypt11 libgcrypt11-dev libgpg-error0 libgpg-error-dev libgnutlsxx27 libgnutls-dev
update-ca-certificates --fresh
wget http://goo.gl/1BOfJ -O /usr/bin/rpi-update
chmod +x /usr/bin/rpi-update
mkdir -p /lib/modules/$(uname -r)
rpi-update
rm -Rf /boot.bak
useradd --create-home --shell /bin/bash --user-group pi
echo \"pi:raspberry\" | chpasswd
echo \"root:raspberry\" | chpasswd
echo \"pi ALL=(ALL) NOPASSWD: ALL\" >> /etc/sudoers
sed -i -e 's/KERNEL\!=\"eth\*|/KERNEL\!=\"/' /lib/udev/rules.d/75-persistent-net-generator.rules
dpkg-divert --add --local /lib/udev/rules.d/75-persistent-net-generator.rules
dpkg-reconfigure locales
read -p \"Ready to install Homegear. Hit [Enter] to continue...\"
touch /tmp/HOMEGEAR_STATIC_INSTALLATION
apt-get -y install homegear
rm /tmp/HOMEGEAR_STATIC_INSTALLATION
service homegear stop
echo \"*               soft    core            unlimited\" >> /etc/security/limits.d/homegear
service ssh stop
service ntp stop
rm -rf /var/log/homegear/*
rm -f third-stage
" > third-stage
chmod +x third-stage
LANG=C chroot $rootfs /third-stage

#Install raspi-config
wget https://raw.github.com/asb/raspi-config/master/raspi-config
mv raspi-config usr/bin
chown root:root usr/bin/raspi-config
chmod 755 usr/bin/raspi-config
#End install raspi-config

#Create scripts directory
mkdir scripts
chown root:root scripts
chmod 750 scripts

#Add homegear monitor script
echo "#!/bin/bash
return=`ps -A | grep homegear -c`
if [ \$return -lt 1 ] && test -e /var/run/homegear/homegear.pid; then
        LOGDIR=/var/log/homegear
        if test -e \$LOGDIR/core; then
                COREDIR=\$LOGDIR/coredumps
                mkdir -p \$COREDIR
                DIR=0
                while test -d \$COREDIR/\$DIR; do
                        ((DIR++))
                done
                COREDIR=\$COREDIR/\$DIR
                mkdir -p \$COREDIR
                mv \$LOGDIR/core \$COREDIR
                mv \$LOGDIR/homegear.log \$COREDIR
                mv \$LOGDIR/homegear.err \$COREDIR
                cp /usr/bin/homegear \$COREDIR
        fi

        /etc/init.d/homegear restart
fi" > scripts/checkServices.sh
chown root:root scripts/checkServices.sh
chmod 750 scripts/checkServices.sh

#Add homegear monitor script to crontab
echo "*/10 *  *       *       *       /scripts/checkServices.sh 2>&1 |/usr/bin/logger -t CheckServices" >> var/spool/cron/crontabs/root

#Create Raspberry Pi boot config
echo "arm_freq=900
core_freq=250
sdram_freq=450
over_voltage=2" > boot/config.txt
chown root:root boot/config.txt
chmod 755 boot/config.txt
#End Raspberry Pi boot config

echo "deb $deb_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

#First-start script
echo "#!/bin/bash
sed -i '$ d' /home/pi/.bashrc >/dev/null
echo \"************************************************************\"
echo \"************************************************************\"
echo \"************* Welcome to your Homegear system! *************\"
echo \"************************************************************\"
echo \"************************************************************\"
echo \"Downloading device description files...\"
/var/lib/homegear/GetDeviceFiles.sh 1
[ $? -ne 0 ] && echo \"Download of device description files failed. Please execute '/var/lib/homegear/GetDeviceFiles.sh' manually. Homegear won't work until the files are downloaded.\"
echo \"Downloading firmware updates...\"
/var/lib/homegear/firmware/GetFirmwareUpdates.sh
[ $? -ne 0 ] && echo \"Download of firmware updates failed. Please execute '/var/lib/homegear/firmware/GetFirmwareUpdates.sh' manually.\"
echo \"Generating new SSH host keys. This might take a while.\"
rm /etc/ssh/ssh_host* >/dev/null
ssh-keygen -A >/dev/null
echo \"Updating your system...\"
apt-get update
apt-get -y upgrade
echo \"Generating new SSL keys and Diffie-Hellman parameters for Homegear. This might take a long time...\"
rm -f /etc/homegear/homegear.key
rm -f /etc/homegear/homegear.crt
openssl genrsa -out /etc/homegear/homegear.key 2048
openssl req -batch -new -key /etc/homegear/homegear.key -out /etc/homegear/homegear.csr
openssl x509 -req -in /etc/homegear/homegear.csr -signkey /etc/homegear/homegear.key -out /etc/homegear/homegear.crt
rm /etc/homegear/homegear.csr
chown homegear:homegear /etc/homegear/homegear.key
chmod 400 /etc/homegear/homegear.key
openssl dhparam -check -text -5 1024 -out /etc/homegear/dh1024.pem
chown homegear:homegear /etc/homegear/dh1024.pem
chmod 400 /etc/homegear/dh1024.pem
echo \"Starting raspi-config...\"
raspi-config
rm /scripts/firstStart.sh
rm -Rf /var/log/homegear/*
reboot" > scripts/firstStart.sh
chown root:root scripts/firstStart.sh
chmod 755 scripts/firstStart.sh

echo "sudo /scripts/firstStart.sh" >> home/pi/.bashrc
#End first-start script

#Bash profile
echo "let upSeconds=\"\$(/usr/bin/cut -d. -f1 /proc/uptime)\"
let secs=\$((\${upSeconds}%60))
let mins=\$((\${upSeconds}/60%60))
let hours=\$((\${upSeconds}/3600%24))
let days=\$((\${upSeconds}/86400))
UPTIME=\`printf \"%d days, %02dh %02dm %02ds\" \"\$days\" \"\$hours\" \"\$mins\" \"\$secs\"\`

echo \"\$(tput setaf 4)\$(tput bold)
                   dd
                  dddd
                dddddddd
              dddddddddddd
               dddddddddd                   \$(tput setaf 7)Welcome to your Homegear system!\$(tput setaf 4)
               dddddddddd                   \$(tput setaf 7)\`uname -srmo\`\$(tput setaf 4)
               d\$(tput setaf 6).:dddd:.\$(tput setaf 4)d\$(tput setaf 6)
    .:ool:,,:oddddddddddddo:,,:loo:.        \$(tput sgr0)Uptime.............: \${UPTIME}\$(tput setaf 6)\$(tput bold)
    oddddddddddddddddddddddddddddddo        \$(tput sgr0)Homegear Version...: \$(homegear -v | head -1 | cut -d \" \" -f 3)\$(tput setaf 6)\$(tput bold)
    .odddddddd| \$(tput setaf 7)Homegear\$(tput setaf 6) |ddddddddo.
     lddddddddddddddddddddddddddddl
    lddddddddddddddddddddddddddddddl
  .:dddddddddddddc.''.cddddddddddddd:.
:odddddddddddddo.      .odddddddddddddo:
ddddddddddddddd,        ,ddddddddddddddd


\$(tput sgr0)\"

# if running bash
if [ -n \"\$BASH_VERSION\" ]; then
    # include .bashrc if it exists
    if [ -f \"\$HOME/.bashrc\" ]; then
        . \"\$HOME/.bashrc\"
    fi
fi

# set PATH so it includes user's private bin if it exists
if [ -d \"\$HOME/bin\" ] ; then
    PATH=\"\$HOME/bin:\$PATH\"
fi" > home/pi/.bash_profile
#End bash profile

echo "#!/bin/bash
apt-get clean
rm -f cleanup
" > cleanup
chmod +x cleanup
LANG=C chroot $rootfs /cleanup

cd

read -p "Copy additional files into ${rootfs} or ${bootfs} then hit [Enter] to continue..."

umount $bootp
umount $rootp

kpartx -d $image

mv $image .
rm -Rf $buildenv

echo "Created Image: $image"

echo "Done."
