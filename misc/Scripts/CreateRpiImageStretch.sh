#!/bin/bash

set -x

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools
# made possible by:
#   Klaus M Pfeiffer (http://blog.kmp.or.at/2012/05/build-your-own-raspberry-pi-image/)
#   Alex Bradbury (http://asbradbury.org/projects/spindle/)


deb_mirror="http://mirrordirector.raspbian.org/raspbian/"
deb_local_mirror=$deb_mirror
deb_release="stretch"

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
dd if=/dev/zero of=$image bs=1MB count=1536
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

# {{{ Only for stretch - correct errors
chroot $rootfs apt-get clean
rm -rf $rootfs/var/lib/apt/lists/*
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y --allow-unauthenticated install debian-keyring debian-archive-keyring
wget http://archive.raspbian.org/raspbian.public.key && chroot $rootfs apt-key add raspbian.public.key && rm raspbian.public.key
wget http://archive.raspberrypi.org/debian/raspberrypi.gpg.key && chroot $rootfs apt-key add raspberrypi.gpg.key && rm raspberrypi.gpg.key
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
# }}}

echo "deb $deb_local_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

echo "blacklist i2c-bcm2708" > $rootfs/etc/modprobe.d/raspi-blacklist.conf

echo "dwc_otg.lpm_enable=0 console=ttyUSB0,115200 console=tty1 kgdboc=ttyUSB0,115200 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait" > boot/cmdline.txt

rm -f $rootfs/etc/fstab
cat > "$rootfs/etc/fstab" <<'EOF'
proc            /proc                       proc            defaults                                                            0       0
/dev/mmcblk0p1  /boot                       vfat            defaults,noatime,ro                                                 0       2
/dev/mmcblk0p2  /                           ext4            defaults,noatime,ro                                                 0       1
tmpfs           /run                        tmpfs           defaults,nosuid,mode=1777,size=50M                                  0       0
#tmpfs           /var/<your directory>       tmpfs           defaults,nosuid,mode=1777,size=50M                                  0       0
EOF

#Setup network settings
echo "homegearpi" > etc/hostname
echo "127.0.0.1       localhost
127.0.1.1       homegearpi
::1             localhost ip6-localhost ip6-loopback
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters
" > etc/hosts

echo "# interfaces(5) file used by ifup(8) and ifdown(8)

# Please note that this file is written to be used with dhcpcd
# For static IP, consult /etc/dhcpcd.conf and 'man dhcpcd.conf'

# Include files from /etc/network/interfaces.d:
source-directory /etc/network/interfaces.d
" > etc/network/interfaces
#End network settings

echo "console-common    console-data/keymap/policy      select  Select keymap from full list
console-common  console-data/keymap/full        select  us
" > debconf.set

cat > "$rootfs/third-stage" <<'EOF'
#!/bin/bash
set -x
debconf-set-selections /debconf.set
rm -f /debconf.set
apt-get update
ls -l /etc/apt/sources.list.d
cat /etc/apt/sources.list
apt-get -y install apt-transport-https ca-certificates
update-ca-certificates --fresh
mkdir -p /etc/apt/sources.list.d/
echo "deb http://archive.raspberrypi.org/debian/ stretch main ui" > /etc/apt/sources.list.d/raspi.list
echo "deb https://apt.homegear.eu/Raspbian/ stretch/" >> /etc/apt/sources.list.d/homegear.list
wget https://apt.homegear.eu/Release.key
apt-key add - < Release.key
rm Release.key
wget http://archive.raspberrypi.org/debian/raspberrypi.gpg.key
apt-key add - < raspberrypi.gpg.key
rm raspberrypi.gpg.key
apt-get update
apt-get -y install libraspberrypi0 libraspberrypi-bin locales console-common dhcpcd5 ntp ntpdate resolvconf openssh-server git-core binutils curl libcurl3-gnutls sudo parted unzip p7zip-full libxml2-utils keyboard-configuration python-lzo libgcrypt20 libgpg-error0 libgnutlsxx28 lua5.2 libenchant1c2a libltdl7 libxslt1.1 libmodbus5 tmux dialog whiptail
# Wireless packets
apt-get -y install bluez-firmware firmware-atheros firmware-libertas firmware-realtek firmware-ralink firmware-brcm80211 wireless-tools wpasupplicant
wget http://goo.gl/1BOfJ -O /usr/bin/rpi-update
chmod +x /usr/bin/rpi-update
mkdir -p /lib/modules/$(uname -r)
rpi-update
rm -Rf /boot.bak
useradd --create-home --shell /bin/bash --user-group pi
echo "pi:raspberry" | chpasswd
result=`id -u homegear 2>/dev/null`
if [ "0$result" -eq "0" ]; then
    adduser --system --no-create-home --shell /bin/false --group homegear >/dev/null 2>&1
    usermod -a -G dialout homegear 2>/dev/null
    usermod -a -G gpio homegear 2>/dev/null
    usermod -a -G spi homegear 2>/dev/null
fi
echo "pi ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
dpkg-reconfigure locales
service ssh stop
service ntp stop
systemctl disable serial-getty@ttyAMA0.service
systemctl disable serial-getty@serial0.service
systemctl disable serial-getty@ttyS0.service

rm -rf /var/log/homegear/*
EOF
chmod +x $rootfs/third-stage
LANG=C chroot $rootfs /third-stage
rm -f $rootfs/third-stage

mkdir -p $rootfs/lib/systemd/scripts
cat > "$rootfs/lib/systemd/scripts/setup-tmpfs.sh" <<'EOF'
#!/bin/bash

modprobe zram num_devices=3

echo 268435456 > /sys/block/zram0/disksize
echo 20971520 > /sys/block/zram1/disksize
echo 134217728 > /sys/block/zram2/disksize

mkfs.ext4 /dev/zram0
mkfs.ext4 /dev/zram1
mkfs.ext4 /dev/zram2

mount /dev/zram0 /var/log
mount /dev/zram1 /var/lib/homegear/db
mount /dev/zram2 /var/tmp

chmod 777 /var/log
chmod 770 /var/lib/homegear/db
chown homegear:homegear /var/lib/homegear/db
chmod 777 /var/tmp

mkdir /var/tmp/lock
chmod 777 /var/tmp/lock
mkdir /var/tmp/dhcp
chmod 755 /var/tmp/dhcp
mkdir /var/tmp/spool
chmod 755 /var/tmp/spool
mkdir /var/tmp/systemd
chmod 755 /var/tmp/systemd
touch /var/tmp/systemd/random-seed
chmod 600 /var/tmp/systemd/random-seed
mkdir -p /var/spool/cron/crontabs
chmod 731 /var/spool/cron/crontabs
chmod +t /var/spool/cron/crontabs

mkdir -p /var/tmp/homegear
chown homegear:homegear /var/tmp/homegear
chmod 770 /var/tmp/homegear

[ -d /data ] && [ ! -d /data/homegear-data ] && mkdir /data/homegear-data
[ -d /data/homegear-data ] && [ ! -d /data/homegear-data/node-blue ] && mkdir /data/homegear-data/node-blue
[ -d /data/homegear-data ] && [ ! -d /data/homegear-data/families ] && mkdir /data/homegear-data/families
[ -d /data/homegear-data ] && chown -R homegear:homegear /data/homegear-data/*
[ -f /data/homegear-data/db.sql ] && cp -a /data/homegear-data/db.sql /var/lib/homegear/db/ && rm -f /data/homegear-data/db.sql

exit 0
EOF
    chmod 750 $rootfs/lib/systemd/scripts/setup-tmpfs.sh

    cat > "$rootfs/lib/systemd/system/setup-tmpfs.service" <<'EOF'
[Unit]
Description=setup-tmpfs
DefaultDependencies=no
After=-.mount run.mount
Before=systemd-random-seed.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/lib/systemd/scripts/setup-tmpfs.sh
TimeoutSec=30s

[Install]
WantedBy=sysinit.target
EOF

cat > "$rootfs/fourth-stage" <<'EOF'
rm -Rf /tmp
ln -s /var/tmp /tmp
mkdir /data
rm -Rf /var/spool
ln -s /var/tmp/spool /var/spool
rm -Rf /var/lib/dhcp
ln -s /var/tmp/dhcp /var/lib/dhcp
rm -Rf /var/lock
ln -s /var/tmp/lock /var/lock
rm -Rf /var/lib/systemd
ln -s /var/tmp/systemd /var/lib/systemd

mkdir -p /var/lib/homegear/db
chown -R homegear:homegear /var/lib/homegear

# {{{ debian-fixup fixes
    ln -sf /proc/mounts /etc/mtab
# }}}

systemctl enable setup-tmpfs
sed -i "s/#SystemMaxUse=/SystemMaxUse=1M/g" /etc/systemd/journald.conf
sed -i "s/#RuntimeMaxUse=/RuntimeMaxUse=1M/g" /etc/systemd/journald.conf
EOF
chmod +x $rootfs/fourth-stage
LANG=C chroot $rootfs /fourth-stage
rm $rootfs/fourth-stage

#Install raspi-config
wget https://raw.githubusercontent.com/RPi-Distro/raspi-config/master/raspi-config
mv raspi-config usr/bin
chown root:root usr/bin/raspi-config
chmod 755 usr/bin/raspi-config
#End install raspi-config

#Create Raspberry Pi boot config
echo "# For more options and information see
# http://www.raspberrypi.org/documentation/configuration/config-txt.md
# Some settings may impact device functionality. See link above for details

# uncomment if you get no picture on HDMI for a default "safe" mode
#hdmi_safe=1

# uncomment this if your display has a black border of unused pixels visible
# and your display can output without overscan
#disable_overscan=1

# uncomment the following to adjust overscan. Use positive numbers if console
# goes off screen, and negative if there is too much border
#overscan_left=16
#overscan_right=16
#overscan_top=16
#overscan_bottom=16

# uncomment to force a console size. By default it will be display's size minus
# overscan.
#framebuffer_width=1280
#framebuffer_height=720

# uncomment if hdmi display is not detected and composite is being output
#hdmi_force_hotplug=1

# uncomment to force a specific HDMI mode (this will force VGA)
#hdmi_group=1
#hdmi_mode=1

# uncomment to force a HDMI mode rather than DVI. This can make audio work in
# DMT (computer monitor) modes
#hdmi_drive=2

# uncomment to increase signal to HDMI, if you have interference, blanking, or
# no display
#config_hdmi_boost=4

# uncomment for composite PAL
#sdtv_mode=2

#uncomment to overclock the arm. 700 MHz is the default.
#arm_freq=800

# Uncomment some or all of these to enable the optional hardware interfaces
#dtparam=i2c_arm=on
#dtparam=i2s=on
#dtparam=spi=on

# Uncomment this to enable the lirc-rpi module
#dtoverlay=lirc-rpi

# Additional overlays and parameters are documented /boot/overlays/README

# Enable audio (loads snd_bcm2835)
dtparam=audio=on

enable_uart=1
dtparam=spi=on
dtparam=i2c_arm=on
gpu_mem=16" > boot/config.txt
chown root:root boot/config.txt
chmod 755 boot/config.txt
#End Raspberry Pi boot config

echo "deb $deb_mirror $deb_release main contrib non-free rpi
" > etc/apt/sources.list

cat > "$rootfs/setupPartitions.sh" <<-'EOF'
#!/bin/bash

stage_one()
{
	export LC_ALL=C
	export LANG=C
    bytes=$(fdisk -l | grep mmcblk0 | head -1 | cut -d "," -f 2 | cut -d " " -f 2)
    gigabytes=$(($bytes / 1073741824))
    maxrootpartitionsize=$(($gigabytes - 1))

    rootpartitionsize=""
    while [ -z $rootpartitionsize ] || ! [[ $rootpartitionsize =~ ^[1-9][0-9]*$ ]] || [[ $rootpartitionsize -gt $maxrootpartitionsize ]]; do
        rootpartitionsize=$(dialog --stdout --title "Partitioning" --no-tags --no-cancel --inputbox "Enter new size of readonly root partition in gigabytes. The minimum partition size is 2 GB. The maximum partition size is $maxrootpartitionsize GB. We recommend only using as much space as really necessary so damaged sectors can be placed outside of the used space. It is also recommended to use a SD card as large as possible." 15 50 "2")
        if ! [[ $rootpartitionsize =~ ^[1-9][0-9]*$ ]] || [[ $rootpartitionsize -lt 2 ]]; then
            dialog --title "Partitioning" --msgbox "Please enter a valid size in gigabytes (without unit). E. g. \"2\" or \"4\". Not \"2G\"." 10 50
        fi
    done

    maxdatapartitionsize=$(($gigabytes - $rootpartitionsize))
    datapartitionsize=""
    while [ -z $datapartitionsize ] || ! [[ $datapartitionsize =~ ^[1-9][0-9]*$ ]] || [[ $datapartitionsize -gt $maxdatapartitionsize ]]; do
        datapartitionsize=$(dialog --stdout --title "Partitioning" --no-tags --no-cancel --inputbox "Enter size of writeable data partition in gigabytes. The maximum partition size is $maxdatapartitionsize GB. We recommend only using as much space as really necessary so damaged sectors can be placed outside of the used space. It is also recommended to use a SD card as large as possible." 15 50 "2")
        if ! [[ $datapartitionsize =~ ^[1-9][0-9]*$ ]]; then
            dialog --title "Partitioning" --msgbox "Please enter a valid size in gigabytes (without unit). E. g. \"2\" or \"4\". Not \"2G\"." 10 50
        fi
    done

    fdisk /dev/mmcblk0 << EOC
d
2
n
p
2

+${rootpartitionsize}G
n
p
3

+${datapartitionsize}G
w
EOC

    rm -f /partstageone
    touch /partstagetwo
    sync
    dialog --no-cancel --stdout --title "Partition setup" --no-tags --pause "Rebooting in 10 seconds..." 10 50 10
    reboot
}

stage_two()
{
    TTY_X=$(($(stty size | awk '{print $2}')-6))
    TTY_Y=$(($(stty size | awk '{print $1}')-6))
    resize2fs /dev/mmcblk0p2 | dialog --title "Partition setup" --progressbox "Resizing root partition..." $TTY_Y $TTY_X
    mkfs.ext4 -F /dev/mmcblk0p3 | dialog --title "Partition setup" --progressbox "Creating data partition..." $TTY_Y $TTY_X

    sed -i '/\/dev\/mmcblk0p2/a\
\/dev\/mmcblk0p3  \/data                       ext4            defaults,noatime,commit=600             0       1' /etc/fstab
    mount -o defaults,noatime,commit=600 /dev/mmcblk0p3 /data
    sed -i '/^After=/ s/$/ data.mount/' /lib/systemd/system/setup-tmpfs.service
    systemctl daemon-reload
    rm -f /partstagetwo
}

[ -f /partstagetwo ] && stage_two
[ -f /partstageone ] && stage_one
EOF
touch $rootfs/partstageone
chmod 755 $rootfs/setupPartitions.sh

#First-start script
cat > "$rootfs/firstStart.sh" <<-'EOF'
#!/bin/bash

scriptCount=`/bin/ps -ejH -1 | /bin/grep firstStart.sh | /bin/grep -c /firstStart`
if [ $scriptCount -gt 3 ]; then
        echo "First start script is already running... Not executing it again..."
        exit 1
fi

echo "************************************************************"
echo "************************************************************"
echo "************* Welcome to your Homegear system! *************"
echo "************************************************************"
echo "************************************************************"

sleep 2

ping -c 4 apt.homegear.eu 1>/dev/null 2>/dev/null
if [ $? -ne 0 ]; then
    dialog --no-cancel --stdout --title "No internet" --no-tags --pause "Your device doesn't seem to have a working internet connection. Rebooting in 10 seconds..." 10 50 10
    poweroff
fi

mount -o remount,rw /
mount -o remount,rw /boot

export NCURSES_NO_UTF8_ACS=1
export DIALOG_OUTPUT=1

MAC="homegearpi-""$( ifconfig | grep -m 1 ether | sed "s/^.*ether [0-9a-f:]\{9\}\([0-9a-f:]*\) .*$/\1/;s/:/-/g" )"
echo "$MAC" > "/etc/hostname"
grep -q $MAC /etc/hosts
if [ $? -eq 1 ]; then
    sed -i "s/127.0.1.1       homegearpi/127.0.1.1       $MAC/g" /etc/hosts
fi
hostname $MAC

/setupPartitions.sh
if [ -f /partstageone ] || [ -f /partstagetwo ]; then
    exit 0
fi
rm -f /setupPartitions.sh

service ntp stop
ntpdate pool.ntp.org | dialog --title "Date" --progressbox "Waiting for NTP to set date." 10 50
service ntp start

password1=""
password2=""
while [[ -z $password1 ]] || [[ $password1 != $password2 ]]; do
    password1=""
    while [[ -z $password1 ]]; do
        password1=$(dialog --stdout --title "New passwords" --no-tags --no-cancel --insecure --passwordbox "Please enter a new password for user \"pi\"" 10 50)
    done
    password2=$(dialog --stdout --title "New passwords" --no-tags --no-cancel --insecure --passwordbox "Please enter the same password again" 10 50)

    if [[ $password1 == $password2 ]]; then
        mount -o remount,rw /
        echo -e "${password1}\n${password2}" | passwd pi
    else
        dialog --title "Error changing password" --msgbox "The entered passwords did not match." 10 50
    fi
done
unset password1
unset passowrd2

# Detect Pi 3
if [ "$boardRev" == "a02082" ] || [ "$boardRev" == "a22082" ]; then
  echo "dwc_otg.lpm_enable=0 console=ttyUSB0,115200 console=tty1 kgdboc=ttyUSB0,115200 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait" > /boot/cmdline.txt
  echo "dtoverlay=pi3-miniuart-bt" >> /boot/config.txt
fi

rm /etc/ssh/ssh_host* >/dev/null
ssh-keygen -A | dialog --title "System setup" --progressbox "Generating new SSH host keys. This might take a while." 10 50

TTY_X=$(($(stty size | awk '{print $2}')-6))
TTY_Y=$(($(stty size | awk '{print $1}')-6))
apt-get update | dialog --title "System update (1/2)" --progressbox "Updating system..." $TTY_Y $TTY_X
[[ ${PIPESTATUS[0]} -ne 0 ]] && mount -o remount,rw / && apt-get update | dialog --title "System update (1/2)" --progressbox "Updating system..." $TTY_Y $TTY_X

TTY_X=$(($(stty size | awk '{print $2}')-6))
TTY_Y=$(($(stty size | awk '{print $1}')-6))
apt-get -y dist-upgrade | dialog --title "System update (2/2)" --progressbox "Updating system..." $TTY_Y $TTY_X

TTY_X=$(($(stty size | awk '{print $2}')-6))
TTY_Y=$(($(stty size | awk '{print $1}')-6))
apt-get -y install homegear homegear-management homegear-webssh homegear-adminui homegear-nodes-core homegear-nodes-extra homegear-homematicbidcos homegear-homematicwired homegear-insteon homegear-max homegear-philipshue homegear-sonos homegear-kodi homegear-ipcam homegear-beckhoff homegear-knx homegear-enocean homegear-intertechno homegear-ccu2 homegear-zwave | dialog --title "System setup" --progressbox "Installing Homegear..." $TTY_Y $TTY_X
if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
    TTY_X=$(($(stty size | awk '{print $2}')-6))
    TTY_Y=$(($(stty size | awk '{print $1}')-6))
    apt-get -y -f install  | dialog --title "System setup" --progressbox "Installing dependencies..." $TTY_Y $TTY_X
fi

mkdir -p /data/homegear-data
chown homegear:homegear /data/homegear-data
sed -i 's/debugLevel = 4/debugLevel = 3/g' /etc/homegear/main.conf
sed -i 's/tempPath = \/var\/lib\/homegear\/tmp/tempPath = \/var\/tmp\/homegear/g' /etc/homegear/main.conf
sed -i 's/# databasePath =/databasePath = \/var\/lib\/homegear\/db/g' /etc/homegear/main.conf
sed -i 's/# writeableDataPath =/writeableDataPath =/g' /etc/homegear/main.conf
sed -i 's/# databaseBackupPath =/databaseBackupPath = \/data\/homegear-data/g' /etc/homegear/main.conf
sed -i 's/familyDataPath = \/var\/lib\/homegear\/families/familyDataPath = \/data\/homegear-data\/families/g' /etc/homegear/main.conf
sed -i 's/nodeBlueDataPath = \/var\/lib\/homegear\/node-blue\/data/nodeBlueDataPath = \/data\/homegear-data\/node-blue/g' /etc/homegear/main.conf
sed -i 's/databaseMemoryJournal = false/databaseMemoryJournal = true/g' /etc/homegear/main.conf
sed -i 's/databaseWALJournal = true/databaseWALJournal = false/g' /etc/homegear/main.conf
sed -i 's/databaseSynchronous = true/databaseSynchronous = false/g' /etc/homegear/main.conf

sed -i 's/session.save_path = "\/var\/lib\/homegear\/tmp\/php"/session.save_path = "\/var\/tmp\/homegear\/php"/g' /etc/homegear/php.ini

echo "" >> /etc/homegear/homegear-start.sh
echo "# Delete backuped db.sql." >> /etc/homegear/homegear-start.sh
echo "[ -f /data/homegear-data/db.sql ] && [ -f /var/lib/homegear/db/db.sql ] && rm -f /data/homegear-data/db.sql" >> /etc/homegear/homegear-start.sh
echo "exit 0" >> /etc/homegear/homegear-start.sh

echo "[ -f /var/lib/homegear/db/db.sql ] && [ -d /data/homegear-data ] && cp -a /var/lib/homegear/db/db.sql /data/homegear-data/" >> /etc/homegear/homegear-stop.sh
echo "[ -d /data/homegear-data ] && chown homegear:homegear /data/homegear-data/*" >> /etc/homegear/homegear-stop.sh
echo "sync" >> /etc/homegear/homegear-stop.sh

echo "3 *  * * *   root    /bin/systemctl reload homegear 2>&1 |/usr/bin/logger -t homegear-reload" > /etc/cron.d/homegear

chown -R homegear:homegear /var/lib/homegear/www

# Create database and defaultPassword.txt while file system is writeable
systemctl restart homegear

echo ""
echo "Starting raspi-config..."
PATH="$PATH:/opt/vc/bin:/opt/vc/sbin"
mount -o remount,rw /boot
raspi-config
rm /firstStart.sh
sed -i '/sudo \/firstStart.sh/d' /home/pi/.bashrc
dialog --no-cancel --stdout --title "Setup finished" --no-tags --pause "Rebooting in 10 seconds..." 10 50 10
reboot
EOF
chown root:root $rootfs/firstStart.sh
chmod 755 $rootfs/firstStart.sh

echo "sudo /firstStart.sh" >> $rootfs/home/pi/.bashrc
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

echo \"\"
echo \"* To change data on the root partition (e. g. to update the system),\"
echo \"  enter:\"
echo \"\"
echo \"  mount -o remount,rw /\"
echo \"\"
echo \"  When you are done, execute\"
echo \"\"
echo \"  mount -o remount,ro /\"
echo \"\"
echo \"  to make the root partition readonly again.\"
echo \"* You can store data on \\\"/data\\\". It is recommended to only backup\"
echo \"  data to this directory. During operation data should be written to a\"
echo \"  temporary file system. By default these are \\\"/var/log\\\" and \"
echo \"  \\\"/var/tmp\\\". You can add additional mounts in \\\"/etc/fstab\\\".\"
echo \"* Remember to backup all data to \\\"/data\\\" before rebooting.\"
echo \"\"

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
rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
rm -f cleanup
" > cleanup
chmod +x cleanup
LANG=C chroot $rootfs /cleanup

cd

umount $bootp
umount $rootp

kpartx -d $image

mv $image .
rm -Rf $buildenv

echo "Created Image: $image"

echo "Done."
