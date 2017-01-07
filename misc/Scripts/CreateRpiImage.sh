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
dd if=/dev/zero of=$image bs=1MB count=2000
[ $? -ne 0 ] && exit 1
device=`losetup -f --show $image`
[ $? -ne 0 ] && exit 1
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
sleep 1

losetup -d $device
[ $? -ne 0 ] && exit 1
sleep 1

device=`kpartx -va $image | sed -E 's/.*(loop[0-9])p.*/\1/g' | head -1`
[ $? -ne 0 ] && exit 1
sleep 1

echo "--- kpartx device ${device}"
device="/dev/mapper/${device}"
bootp=${device}p1
rootp=${device}p2

mkfs.vfat $bootp
mkfs.ext4 $rootp

mkdir -p $rootfs
[ $? -ne 0 ] && exit 1

mount $rootp $rootfs
[ $? -ne 0 ] && exit 1

cd $rootfs
debootstrap --no-check-gpg --foreign --arch=armhf $deb_release $rootfs $deb_local_mirror
[ $? -ne 0 ] && exit 1

cp /usr/bin/qemu-arm-static usr/bin/
[ $? -ne 0 ] && exit 1
LANG=C chroot $rootfs /debootstrap/debootstrap --second-stage
[ $? -ne 0 ] && exit 1

mount $bootp $bootfs
[ $? -ne 0 ] && exit 1

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
set -x
debconf-set-selections /debconf.set
rm -f /debconf.set
apt update
ls -l /etc/apt/sources.list.d
cat /etc/apt/sources.list
apt -y install apt-transport-https ca-certificates
update-ca-certificates --fresh
mkdir -p /etc/apt/sources.list.d/
echo \"deb https://homegear.eu/packages/Raspbian/ jessie/\" >> /etc/apt/sources.list.d/homegear.list
wget http://homegear.eu/packages/Release.key
apt-key add - < Release.key
rm Release.key
apt update
apt -y install locales console-common ntp openssh-server git-core binutils curl sudo parted unzip p7zip-full php5-cli php5-xmlrpc libxml2-utils keyboard-configuration liblzo2-dev python-lzo libgcrypt20 libgcrypt20-dev libgpg-error0 libgpg-error-dev libgnutlsxx28 libgnutls28-dev lua5.2 libmysqlclient-dev libcurl4-gnutls-dev libenchant1c2a libltdl7 libmcrypt4 libxslt1.1 libmodbus5 tmux
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

#Install Java and OpenHAB
if [ $OPENHAB -eq 1 ]; then
	echo "#!/bin/bash
read -p \"Ready to install Java. Please provide the download link to the current ARM package (http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html): \" JAVAPACKAGE
wget --header \"Cookie: oraclelicense=accept-securebackup-cookie\" \$JAVAPACKAGE
tar -zxf jdk*.tar.gz -C /opt
rm jdk*.tar.gz
JDK=\`ls /opt/ | grep jdk\`
update-alternatives --install /usr/bin/javac javac /opt/\${JDK}/bin/javac 1
update-alternatives --install /usr/bin/java java /opt/\${JDK}/bin/java 1
update-alternatives --config javac
update-alternatives --config java
wget -qO - 'https://bintray.com/user/downloadSubjectPublicKey?username=openhab' | sudo apt-key add -
echo \"deb http://dl.bintray.com/openhab/apt-repo stable main\" > /etc/apt/sources.list.d/openhab.list
rm -f fifth-stage
" > fifth-stage
	chmod +x fifth-stage
	chroot $rootfs /fifth-stage
fi
#End install Java and OpenHAB

#Install raspi-config
wget https://raw.githubusercontent.com/RPi-Distro/raspi-config/master/raspi-config
mv raspi-config usr/bin
chown root:root usr/bin/raspi-config
chmod 755 usr/bin/raspi-config
#End install raspi-config

#Create scripts directory
mkdir scripts
chown root:root scripts
chmod 750 scripts

#Create Raspberry Pi boot config
echo "arm_freq=900
core_freq=250
sdram_freq=450
over_voltage=2
enable_uart=1
dtparam=spi=on
dtparam=i2c_arm=on" > boot/config.txt
chown root:root boot/config.txt
chmod 755 boot/config.txt
#End Raspberry Pi boot config

echo "deb $deb_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

cat > "etc/init.d/tmpfslog.sh" <<-'EOF'
#!/bin/bash
### BEGIN INIT INFO
# Provides:          tmpfslog
# Required-Start:    $local_fs
# Required-Stop:     $local_fs
# X-Start-Before:    $syslog
# X-Stop-After:      $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start/stop logfile saving
### END INIT INFO
#
# varlog        This init.d script is used to start logfile saving and restore.
#

varlogSave=/var/log.save/
[ ! -d $varlogSave ] && mkdir -p $varlogSave

PATH=/sbin:/usr/sbin:/bin:/usr/bin

case $1 in
    start)
        echo "*** Starting tmpfs file restore: varlog."
        if [ -z "$(grep /var/log /proc/mounts)" ]; then
            echo "*** mounting /var/log"
            cp -Rpu /var/log/* $varlogSave
            rm -Rf /var/log/*
            varlogsize=$(grep /var/log /etc/fstab|awk {'print $4'}|cut -d"=" -f2)
            [ -z "$varlogsize" ] && varlogsize="100M"
            mount -t tmpfs tmpfs /var/log -o defaults,size=$varlogsize
            chmod 755 /var/log
        fi
        cp -Rpu ${varlogSave}* /var/log/
    ;;
    stop)
        echo "*** Stopping tmpfs file saving: varlog."
        rm -Rf ${varlogSave}*
        cp -Rpu /var/log/* $varlogSave >/dev/null 2>&1
        sync
        umount -f /var/log/
    ;;
  reload)
    echo "*** Stopping tmpfs file saving: varlog."
    	rm -Rf ${varlogSave}*
        cp -Rpu /var/log/* $varlogSave >/dev/null 2>&1
        sync
  ;;
    *)
        echo "Usage: $0 {start|stop}"
    ;;
esac

exit 0
EOF
chown root:root etc/init.d/tmpfslog.sh
chmod 755 etc/init.d/tmpfslog.sh

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
if [ \$(nproc --all) -ge 4 ]; then
  echo \"dwc_otg.lpm_enable=0 console=ttyUSB0,115200 kgdboc=ttyUSB0,115200 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline isolcpus=2,3 rootwait\" > /boot/cmdline.txt
  sed -i 's/varlogsize=\"100M\"/varlogsize=\"250M\"/g' /etc/init.d/tmpfslog.sh
fi
insserv tmpfslog.sh
echo \"Updating your system...\"
apt update
apt -y upgrade
apt -y install homegear homegear-homematicbidcos homegear-homematicwired homegear-insteon homegear-max homegear-philipshue homegear-sonos homegear-kodi homegear-ipcam homegear-beckhoff homegear-knx homegear-enocean homegear-intertechno
service homegear stop" >> scripts/firstStart.sh
if [ $OPENHAB -eq 1 ]; then
  echo "apt -y install openhab-runtime openhab-addon-action-homematic openhab-addon-binding-homematic
  apt-get -y -f install
  cp /etc/openhab/configurations/openhab_default.cfg /etc/openhab/configurations/openhab.cfg
  sed -i \"s/^# homematic:host=/homematic:host=127.0.0.1/\" /etc/openhab/configurations/openhab.cfg
  sed -i \"s/^# homematic:callback.host=/homematic:callback.host=127.0.0.1/\" /etc/openhab/configurations/openhab.cfg
  systemctl enable openhab" >> scripts/firstStart.sh
fi
echo "echo \"Starting raspi-config...\"
raspi-config
rm /scripts/firstStart.sh
rm -Rf /var/log/homegear/*
echo \"Rebooting...\"
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
