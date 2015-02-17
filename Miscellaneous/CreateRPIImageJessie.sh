#!/bin/bash

set -x

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools
# made possible by:
#   Klaus M Pfeiffer (http://blog.kmp.or.at/2012/05/build-your-own-raspberry-pi-image/)
#   Alex Bradbury (http://asbradbury.org/projects/spindle/)

read -p "Do you want to include openHAB and Java in the image [y/N]: " OPENHAB
if [ "$OPENHAB" = "y" ]; then
	OPENHAB=1
	echo "Creating image with openHAB and Java..."
else
	OPENHAB=0
	echo "Creating image without openHAB and Java..."
fi

deb_mirror="http://mirrordirector.raspbian.org/raspbian/"
deb_local_mirror=$deb_mirror
deb_release="jessie"

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
dd if=/dev/zero of=$image bs=1MB count=1900
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
echo \"deb http://homegear.eu/packages/Raspbian/ jessie/\" >> /etc/apt/sources.list.d/homegear.list
wget http://homegear.eu/packages/Release.key
apt-key add - < Release.key
rm Release.key
apt-get update
apt-get -y install locales console-common ntp openssh-server git-core binutils curl ca-certificates sudo parted unzip p7zip-full php5-cli php5-xmlrpc libxml2-utils keyboard-configuration liblzo2-dev python-lzo libgcrypt20 libgcrypt20-dev libgpg-error0 libgpg-error-dev libgnutlsxx28 libgnutls28-dev
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
service ssh stop
service ntp stop
rm -rf /var/log/homegear/*
rm -f third-stage
" > third-stage
chmod +x third-stage
LANG=C chroot $rootfs /third-stage

#Create SPI and I2C device tree blob
echo "// Enable the i2c-1, spidev-0 & spidev-1 devices
/dts-v1/;
/plugin/;

/ {
   compatible = \"brcm,bcm2708,bcm2836\";

   fragment@0 {
      target = <&i2c0>;
      __overlay__ {
         status = \"okay\";
      };
   };

   fragment@1 {
      target = <&i2c1>;
      __overlay__ {
         status = \"okay\";
      };
   };

   fragment@2 {
      target = <&spi0>;
      __overlay__ {
         status = \"okay\";
      };
   };
};
" > enable-i2c-spi-overlay.dts

echo "#!/bin/bash
apt-get -y install bison build-essential flex
mkdir /tmp/dtc
wget -P /tmp/dtc https://github.com/RobertCNelson/dtc/archive/dtc-fixup-65cc4d2.zip
unzip /tmp/dtc/dtc-fixup-65cc4d2.zip -d /tmp/dtc
cd /tmp/dtc/dtc-dtc-fixup-65cc4d2
make PREFIX=/usr/ CC=gcc CROSS_COMPILE=all
make PREFIX=/usr/ install
cd /
dtc -@ -I dts -O dtb -o /boot/overlays/enable-i2c-spi-overlay.dtb /enable-i2c-spi-overlay.dts
rm -Rf /tmp/dtc
rm /enable-i2c-spi-overlay.dts
dpkg --purge bison build-essential flex
apt-get -y autoremove
rm -f fourth-stage
" > fourth-stage
chmod +x fourth-stage
chroot $rootfs /fourth-stage
#End create SPI and I2C device tree blob

#Install Java and OpenHAB
if [ $OPENHAB -eq 1 ]; then
	echo "#!/bin/bash
read -p \"Ready to install Java. Please provide the download link to the current ARM package (http://www.oracle.com/technetwork/java/javase/downloads/jdk8-arm-downloads-2187472.html): \" JAVAPACKAGE
wget --header \"Cookie: oraclelicense=accept-securebackup-cookie\" \$JAVAPACKAGE
tar -zxf jdk*.tar.gz -C /opt
rm jdk*.tar.gz
JDK=\`ls /opt/ | grep jdk\`
update-alternatives --install /usr/bin/javac javac /opt/\${JDK}/bin/javac 1
update-alternatives --install /usr/bin/java java /opt/\${JDK}/bin/java 1
update-alternatives --config javac
update-alternatives --config java
read -p \"Ready to install OpenHAB. Please provide the current version number (e. g. 1.6.1): \" OPENHABVERSION
echo \"deb http://repository-openhab.forge.cloudbees.com/release/\${OPENHABVERSION}/apt-repo/ /\" > /etc/apt/sources.list.d/openhab.list
apt-get update
apt-get -y --force-yes install openhab-runtime openhab-addon-action-homematic openhab-addon-binding-homematic
cp /etc/openhab/configurations/openhab_default.cfg /etc/openhab/configurations/openhab.cfg
sed -i \"s/^# homematic:host=/homematic:host=127.0.0.1/\" /etc/openhab/configurations/openhab.cfg
sed -i \"s/^# homematic:callback.host=/homematic:callback.host=127.0.0.1/\" /etc/openhab/configurations/openhab.cfg
rm -f fifth-stage
" > fifth-stage
	chmod +x fifth-stage
	chroot $rootfs /fifth-stage
fi
#End install Java and OpenHAB

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
over_voltage=2
device_tree_overlay=overlays/enable-i2c-spi-overlay.dtb" > boot/config.txt
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
echo \"************************************************************\"" > scripts/firstStart.sh
if [ $OPENHAB -eq 1 ]; then
	echo "read -p \"The Oracle Java Development Kit 8 (JDK 8) is installed on this system. By pressing [Enter] you accept the \\\"Oracle Binary Code License Agreement for Java SE\\\" (http://www.oracle.com/technetwork/java/javase/terms/license/index.html)...\"" >> scripts/firstStart.sh
fi
echo "echo \"Generating new SSH host keys. This might take a while.\"
rm /etc/ssh/ssh_host* >/dev/null
ssh-keygen -A >/dev/null
echo \"Updating your system...\"
apt-get update
apt-get -y upgrade
apt-get -y install homegear
echo \"Starting raspi-config...\"
raspi-config
rm /scripts/firstStart.sh
rm -Rf /var/log/homegear/*
reboot" >> scripts/firstStart.sh
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

if test -e /usr/bin/homegear; then
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
fi

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
