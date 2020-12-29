#!/bin/bash

set -x

echo "Customizing image..."

rootfs=$1

cat > "$rootfs/homegear-customizations" <<'EOF'
echo "deb https://apt.homegear.eu/Raspbian/ buster/" >> /etc/apt/sources.list.d/homegear.list
wget https://apt.homegear.eu/Release.key
apt-key add - < Release.key
rm Release.key

result=`id -u homegear 2>/dev/null`
adduser --system --no-create-home --shell /bin/false --group homegear >/dev/null 2>&1
usermod -a -G dialout homegear 2>/dev/null
usermod -a -G gpio homegear 2>/dev/null
usermod -a -G spi homegear 2>/dev/null

mkdir -p /var/lib/homegear/db
chown -R homegear:homegear /var/lib/homegear

sed -i "/^exit 0/d" /lib/systemd/scripts/setup-tmpfs.sh
sed -i "s/modprobe zram num_devices=2/modprobe zram num_devices=3/g" /lib/systemd/scripts/setup-tmpfs.sh
cat >> "/lib/systemd/scripts/setup-tmpfs.sh" <<'EOG'

# Homegear
echo 20971520 > /sys/block/zram2/disksize
mkfs.ext4 /dev/zram2
mount /dev/zram2 /var/lib/homegear/db
chmod 770 /var/lib/homegear/db
chown homegear:homegear /var/lib/homegear/db

mkdir /var/log/homegear
mkdir /var/log/homegear-dc-connector
mkdir /var/log/homegear-influxdb
mkdir /var/log/homegear-webssh
mkdir /var/log/homegear-management
chown homegear:homegear /var/log/homegear*

touch /var/log/mosquitto.log
chown mosquitto:mosquitto /var/log/mosquitto.log

mkdir -p /var/tmp/homegear
chown homegear:homegear /var/tmp/homegear
chmod 770 /var/tmp/homegear

[ -d /data ] && [ ! -d /data/homegear-data ] && mkdir /data/homegear-data
[ -d /data/homegear-data ] && [ ! -d /data/homegear-data/node-blue ] && mkdir /data/homegear-data/node-blue
[ -d /data/homegear-data ] && [ ! -d /data/homegear-data/families ] && mkdir /data/homegear-data/families
[ -d /data/homegear-data ] && chown -R homegear:homegear /data/homegear-data/*
[ -f /data/homegear-data/db.sql ] && cp -a /data/homegear-data/db.sql /var/lib/homegear/db/ && rm -f /data/homegear-data/db.sql

exit 0
EOG
EOF
chmod +x $rootfs/homegear-customizations
LANG=C chroot $rootfs /homegear-customizations
rm $rootfs/homegear-customizations

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
    reboot
fi

mount -o remount,rw /
mount -o remount,rw /boot

export NCURSES_NO_UTF8_ACS=1
export DIALOG_OUTPUT=1

# Todo: Change this check to polling the microcontroller once possible
hardwareRevision=$(cat /proc/cpuinfo | grep Revision | cut -d ':' -f 2 | xargs)
if [ ${#hardwareRevision} -eq 6 ]; then
    hardwareRevision=${hardwareRevision:3:2}

    if [ "$hardwareRevision" == "10" ] || [ "$hardwareRevision" == "0a" ]; then
        # Settings for compute module 3+ and compute module 3
        echo "" >> /boot/config.txt
        echo "# UART1 is not working without constant core frequency" >> /boot/config.txt
        echo "core_freq=400" >> /boot/config.txt
        echo "# Reduce CPU speed of HG Pro Core => reduce temperature" >> /boot/config.txt
        echo "arm_freq=400" >> /boot/config.txt
        echo "# Check if really necessary for UART1 to work" >> /boot/config.txt
        echo "force_turbo=1" >> /boot/config.txt
        echo "" >> /boot/config.txt
        echo "# GPIOs, welche am LAN-Chip hängen" >> /boot/config.txt
        echo "gpio=12=op,dh" >> /boot/config.txt
        echo "gpio=44=op,dh" >> /boot/config.txt
        echo "" >> /boot/config.txt
        echo "dtoverlay=bcm2710-rpi-cm3" >> /boot/config.txt
        echo "dtoverlay=uart0,txd0_pin=32,rxd0_pin=33,pin_func=7" >> /boot/config.txt
        echo "dtoverlay=uart1,txd1_pin=40,rxd1_pin=41" >> /boot/config.txt
    fi
fi

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

ntpdate pool.ntp.org | dialog --title "Date" --progressbox "Waiting for NTP to set date." 10 50
if [ $? -ne 0 ]; then
    dialog --no-cancel --stdout --title "No internet" --no-tags --pause "Could not set date. Rebooting in 10 seconds..." 10 50 10
    reboot
fi

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

# Create Homegear data directory before installing Homegear so it can be detected in Homegear's postinst script.
mkdir -p /data/homegear-data
chown homegear:homegear /data/homegear-data

TTY_X=$(($(stty size | awk '{print $2}')-6))
TTY_Y=$(($(stty size | awk '{print $1}')-6))
apt-get -y install homegear homegear-management homegear-webssh homegear-adminui homegear-nodes-core homegear-nodes-extra homegear-homematicbidcos homegear-homematicwired homegear-insteon homegear-max homegear-philipshue homegear-sonos homegear-kodi homegear-ipcam homegear-beckhoff homegear-knx homegear-enocean homegear-intertechno homegear-ccu homegear-zwave homegear-nanoleaf | dialog --title "System setup" --progressbox "Installing Homegear..." $TTY_Y $TTY_X
if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
    TTY_X=$(($(stty size | awk '{print $2}')-6))
    TTY_Y=$(($(stty size | awk '{print $1}')-6))
    apt-get -y -f install  | dialog --title "System setup" --progressbox "Installing dependencies..." $TTY_Y $TTY_X
fi

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

mkdir -p /data/homegear-data/node-blue/node-red
cp /var/lib/homegear/node-blue/data/node-red/settings.js /data/homegear-data/node-blue/node-red/
chown -R homegear:homegear /data/homegear-data
sed -i 's/\/var\/lib\/homegear\/node-blue\/data\/node-red/\/data\/homegear-data\/node-blue\/node-red/g' /data/homegear-data/node-blue/node-red/settings.js

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