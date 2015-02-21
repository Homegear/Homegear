#!/bin/bash

# Required build environment packages: qemu qemu-user-static debootstrap binfmt-support

print_usage() {
	echo "Usage: CreateBuildChroot.sh DEBIAN_MIRROR DEBIAN_RELEASE ARCH ROOTFS"
	echo "  DEBIAN_MIRROR:  The mirror URL. E. g.: http://ftp.debian.org/debian"
	echo "  DEBIAN_RELEASE: The debian version. E. g.: wheezy"
	echo "  ARCH:           The CPU architecture. E. g. armhf"
    echo "  ROOTFS:         The directory to install Debian to"
}

if [ $EUID -ne 0 ]; then
  echo "ERROR: This tool must be run as Root"
  exit 1
fi

if test -z $1
then
	echo "Please specify a valid Debian mirror."
	print_usage
	exit 0;
fi

if test -z $2
then
	echo "Please specify the Debian release."
	print_usage
	exit 0;
fi

if test -z $3
then
	echo "Please provide a valid CPU architecture."	
	print_usage
	exit 0;
fi

if test -z $4
then
	echo "Please provide a valid installation directory."
	print_usage
	exit 0;
fi

deb_mirror=$1
deb_release=$2
arch=$3

rootfs=$4

cd $rootfs
debootstrap --no-check-gpg --foreign --arch=$arch $deb_release $rootfs $deb_mirror

if [ "$arch" == "armhf" ]; then
	cp /usr/bin/qemu-arm-static usr/bin/
elif [ "$arch" == "armel" ]; then
	cp /usr/bin/qemu-arm-static usr/bin/
elif [ "$arch" == "arm64" ]; then
	cp /usr/bin/qemu-aarch64-static usr/bin/
elif [ "$arch" == "i386" ]; then
	cp /usr/bin/qemu-i386-static usr/bin/
elif [ "$arch" == "amd64" ]; then
	cp /usr/bin/qemu-x86_64-static usr/bin/
elif [ "$arch" == "mips" ]; then
	cp /usr/bin/qemu-mips-static usr/bin/
fi
LANG=C chroot $rootfs /debootstrap/debootstrap --second-stage

echo "deb $deb_mirror $deb_release main contrib non-free
" > etc/apt/sources.list

echo "proc            /proc           proc    defaults        0       0" > etc/fstab

# Prevent init scripts from running
LANG=C chroot $rootfs dpkg-divert --local --rename --add /sbin/initctl
echo "#!/bin/sh
exit 0
" > sbin/initctl
chmod 755 sbin/initctl
echo "#!/bin/sh
exit 101
" > usr/sbin/policy-rc.d
chmod 755 usr/sbin/policy-rc.d

LANG=C chroot $rootfs apt-get update
LANG=C chroot $rootfs apt-get -y install locales console-common ntp openssh-server git-core binutils curl ca-certificates sudo parted unzip p7zip-full php5-cli php5-xmlrpc libxml2-utils keyboard-configuration liblzo2-dev python-lzo libgcrypt11 libgpg-error0 libgpg-error-dev libgnutlsxx27 libgnutls-dev binutils debhelper devscripts build-essential sqlite3 libsqlite3-dev libreadline6 libreadline6-dev libncurses-dev libssl-dev libparse-debcontrol-perl libgcrypt11-dev g++

echo "#!/bin/bash
if [[ \$1 -lt 1 ]]
then
	echo \"Please provide a revision number.\"
	exit 0;
fi
if test -z \$2
then
  echo \"Please specify branch.\"
  exit 0;
fi
wget https://github.com/Homegear/Homegear/archive/\$2.zip
unzip \$2.zip
rm \$2.zip
version=\$(head -n 1 Homegear-\$2/Version.h | cut -d \" \" -f3 | tr -d '\"')
sourcePath=homegear-\$version
mv Homegear-\$2 \$sourcePath
rm -Rf \$sourcePath/.* 1>/dev/null 2>&2
rm -Rf \$sourcePath/obj
rm -Rf \$sourcePath/bin
tar -zcpf homegear_\$version.orig.tar.gz \$sourcePath
cd \$sourcePath
dch -v \$version-\$1 -M
debuild -us -uc
cd ..
rm -Rf \$sourcePath
rm homegear_\$version-?_*.build
rm homegear_\$version-?_*.changes
rm homegear_\$version-?.debian.tar.gz
rm homegear_\$version-?.dsc
rm homegear_\$version.orig.tar.gz
" > CreateDebianPackage.sh
chmod 755 CreateDebianPackage.sh

LANG=C chroot $rootfs dpkg-reconfigure locales

echo "**************************************************************"
echo "**************************************************************"
echo "Installation completed. You can execute commands by prefixing"
echo "them with \"chroot $rootfs\"."
echo "**************************************************************"
echo "**************************************************************"
