#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools

set -x

print_usage() {
	echo "Usage: CreateDockerHomegearBuild.sh DIST DISTVER ARCH"
	echo "  DIST:           The Linux distribution (debian, raspbian or ubuntu)"
	echo "  DISTVER:        The version of the distribution (e. g. for Ubuntu: trusty, utopic or vivid)"
	echo "  ARCH:           The CPU architecture (armhf, armel, arm64, i386, amd64 or mips)"
}

if [ $EUID -ne 0 ]; then
	echo "ERROR: This tool must be run as Root"
	exit 1
fi

if test -z $1; then
	echo "Please provide a valid Linux distribution (debian, raspbian or ubuntu)."	
	print_usage
	exit 1
fi

if test -z $2; then
	echo "Please provide a valid Linux distribution version (buster, focal, ...)."	
	print_usage
	exit 1
fi

if test -z $3; then
	echo "Please provide a valid CPU architecture."	
	print_usage
	exit 1
fi

dist=$1
dist="$(tr '[:lower:]' '[:upper:]' <<< ${dist:0:1})${dist:1}"
distlc="$(tr '[:upper:]' '[:lower:]' <<< ${dist:0:1})${dist:1}"
distver=$2
arch=$3
if [ "$dist" == "Ubuntu" ]; then
	repository="http://archive.ubuntu.com/ubuntu"
	if [ "$arch" == "armhf" ]; then
		repository="http://ports.ubuntu.com/ubuntu-ports"
	elif [ "$arch" == "armel" ]; then
		repository="http://ports.ubuntu.com/ubuntu-ports"
	elif [ "$arch" == "arm64" ]; then
		repository="http://ports.ubuntu.com/ubuntu-ports"
	elif [ "$arch" == "mips" ]; then
		repository="http://ports.ubuntu.com/ubuntu-ports"
	fi
elif [ "$dist" == "Debian" ]; then
	repository="http://ftp.debian.org/debian"
elif [ "$dist" == "Raspbian" ]; then
	repository="http://archive.raspbian.org/raspbian/"
else
	echo "Unknown distribution."
	print_usage
	exit 1
fi
dir="$(mktemp -d ${TMPDIR:-/var/tmp}/docker-mkimage.XXXXXXXXXX)"
rootfs=$dir/rootfs
mkdir $rootfs

debootstrap --no-check-gpg --foreign --arch=$arch $distver $rootfs $repository
if [ "$arch" == "armhf" ]; then
	cp /usr/bin/qemu-arm-static $rootfs/usr/bin/
elif [ "$arch" == "armel" ]; then
	cp /usr/bin/qemu-arm-static $rootfs/usr/bin/
elif [ "$arch" == "arm64" ]; then
	cp /usr/bin/qemu-aarch64-static $rootfs/usr/bin/
elif [ "$arch" == "mips" ]; then
	cp /usr/bin/qemu-mips-static $rootfs/usr/bin/
fi
LANG=C chroot $rootfs /debootstrap/debootstrap --second-stage

chroot $rootfs mount proc /proc -t proc

if [ "$dist" == "Ubuntu" ]; then
	echo "deb $repository $distver main restricted universe multiverse
	" > $rootfs/etc/apt/sources.list
elif [ "$dist" == "Debian" ]; then
	echo "deb http://ftp.debian.org/debian $distver main contrib
	" > $rootfs/etc/apt/sources.list
elif [ "$dist" == "Raspbian" ]; then
	echo "deb http://archive.raspbian.org/raspbian/ $distver main contrib
	" > $rootfs/etc/apt/sources.list
fi

# prevent init scripts from running during install/update
cat > "$rootfs/usr/sbin/policy-rc.d" <<'EOF'
#!/bin/sh
exit 101
EOF
chmod +x "$rootfs/usr/sbin/policy-rc.d"
# prevent upstart scripts from running during install/update
chroot $rootfs dpkg-divert --local --rename --add /sbin/initctl
cp -a "$rootfs/usr/sbin/policy-rc.d" "$rootfs/sbin/initctl"
sed -i 's/^exit.*/exit 0/' "$rootfs/sbin/initctl"

rm -f $rootfs/etc/apt/apt.conf.d/01autoremove-kernels

# Execute "apt-get clean" after every install (from https://raw.githubusercontent.com/docker/docker/master/contrib/mkimage/debootstrap)
aptGetClean='"rm -f /var/cache/apt/archives/*.deb /var/cache/apt/archives/partial/*.deb /var/cache/apt/*.bin || true";'
cat > "$rootfs/etc/apt/apt.conf.d/docker-clean" <<-EOF
DPkg::Post-Invoke { ${aptGetClean} };
APT::Update::Post-Invoke { ${aptGetClean} };

Dir::Cache::pkgcache "";
Dir::Cache::srcpkgcache "";
EOF

cat > "$rootfs/etc/apt/apt.conf.d/docker-no-languages" <<-'EOF'
Acquire::Languages "none";
EOF

cat > "$rootfs/etc/apt/apt.conf.d/docker-gzip-indexes" <<-'EOF'
Acquire::GzipIndexes "true";
Acquire::CompressionTypes::Order:: "gz";
EOF

cat > "$rootfs/etc/apt/preferences.d/homegear" <<-'EOF'
Package: *
Pin: origin homegear.eu
Pin-Priority: 999
EOF

# Retry downloading package three times
echo 'Acquire::Retries "3";' > "$rootfs/etc/apt/apt.conf.d/80-retries"

if [ "$distver" == "stretch" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y --allow-unauthenticated install debian-keyring debian-archive-keyring
	[ $? -ne 0 ] && exit 1
fi

if [ "$distver" == "jammy" ] || [ "$distver" == "focal" ] || [ "$distver" == "bionic" ] || [ "$distver" == "buster" ] || [ "$distver" == "bullseye" ]; then
	if [ "$arch" == "arm64" ]; then # Workaround for "syscall 277 error" in man-db
		export MAN_DISABLE_SECCOMP=1
	fi
fi

if [ "$distver" == "jammy" ] || [ "$distver" == "focal" ] || [ "$distver" == "bionic" ] || [ "$distver" == "buster" ] || [ "$distver" == "bullseye" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y install gnupg
	[ $? -ne 0 ] && exit 1
fi

chroot $rootfs apt-get update
if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "bullseye" ] || [ "$distver" == "vivid" ] || [ "$distver" == "wily" ] || [ "$distver" == "bionic" ] || [ "$distver" == "focal" ] || [ "$distver" == "jammy" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
	[ $? -ne 0 ] && exit 1
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
	[ $? -ne 0 ] && exit 1
fi
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install apt-transport-https ca-certificates curl
[ $? -ne 0 ] && exit 1
DEBIAN_FRONTEND=noninteractive chroot $rootfs sed -i 's/mozilla\/DST_Root_CA_X3.crt/!mozilla\/DST_Root_CA_X3.crt/g' /etc/ca-certificates.conf
DEBIAN_FRONTEND=noninteractive chroot $rootfs update-ca-certificates --fresh

repodist=$distlc
if [[ $distlc == "raspbian" ]]; then
  repodist="raspberry_pi_os"
fi
echo "deb https://apt.homegear.eu/${repodist}/${distver}/homegear/stable/ ${distver} main" > $rootfs/etc/apt/sources.list.d/homegear.list

wget -P $rootfs https://apt.homegear.eu/Release.key
chroot $rootfs apt-key add Release.key
rm $rootfs/Release.key

DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get update
# python: Needed by homegear-ui's npm on some systems
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install ssh wget unzip binutils debhelper devscripts automake autoconf libtool cmake sqlite3 libsqlite3-dev libncurses5-dev libssl-dev libparse-debcontrol-perl libgpg-error-dev php8-homegear-dev nodejs-homegear libxslt1-dev libedit-dev libqdbm-dev libcrypto++-dev libltdl-dev zlib1g-dev libtinfo-dev libgmp-dev libxml2-dev libzip-dev p7zip-full ntp libavahi-common-dev libavahi-client-dev libicu-dev libonig-dev libsodium-dev libpython3-dev python3-all python3-setuptools dh-python uuid-dev libgpgme-dev check
[ $? -ne 0 ] && exit 1

# {{{ Packages for doorctrl
if [[ "$dist" == "Raspbian" ]] && [[ "$distver" == "bullseye" ]]; then
  DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libasound2-dev libboost-dev libboost-thread-dev libboost-log-dev libboost-system-dev libboost-program-options-dev libgtk-3-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good gstreamer1.0-plugins-bad libavdevice58 libsdl2-2.0-0 libsdl2-2.0-0 libsdl2-2.0-0 libopencore-amrnb0 libopencore-amrwb0 libgpiod-dev libssl-dev libxss-dev
  [ $? -ne 0 ] && exit 1
fi
# }}}

if [ "$distver" == "bullseye" ] || [ "$distver" == "jammy" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libenchant-2-dev
	[ $? -ne 0 ] && exit 1
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libenchant-dev python
	[ $? -ne 0 ] && exit 1
fi

if [ "$distver" != "jammy" ] && [ "$distver" != "focal" ] && [ "$distver" != "bionic" ] && [ "$distver" != "buster" ] || [ "$distver" == "bullseye" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install insserv
	[ $? -ne 0 ] && exit 1
fi

if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "bullseye" ];  then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install default-libmysqlclient-dev dirmngr
	[ $? -ne 0 ] && exit 1
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libmysqlclient-dev
	[ $? -ne 0 ] && exit 1
fi

# {{{ GCC, GCrypt, GNUTLS, Curl
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libgcrypt20-dev libgnutls28-dev
if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "bullseye" ] || [ "$distver" == "bionic" ] || [ "$distver" == "focal" ] || [ "$distver" == "jammy" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libcurl4-gnutls-dev
	[ $? -ne 0 ] && exit 1
fi
# }}}

# {{{ Readline
if [ "$distver" == "jammy" ] || [ "$distver" == "focal" ] || [ "$distver" == "bullseye" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline8 libreadline-dev
	[ $? -ne 0 ] && exit 1
else
	if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "bionic" ]; then
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline7 libreadline-dev
		[ $? -ne 0 ] && exit 1
	else
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline6 libreadline6-dev
		[ $? -ne 0 ] && exit 1
	fi
fi
# }}}

# {{{ UI build dependencies
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install php-cli
[ $? -ne 0 ] && exit 1
# }}}

# {{{ PJPROJECT for doorctrl
if [[ "$dist" == "Raspbian" ]] && [[ "$distver" == "bullseye" ]]; then
  DEBIAN_FRONTEND=noninteractive chroot $rootfs /bin/bash -c 'cd /usr/src && wget https://github.com/pjsip/pjproject/archive/refs/tags/2.13.tar.gz && tar -zxf 2.13.tar.gz && rm 2.13.tar.gz && cd /usr/src/pjproject-2.13 && ./configure && make dep && make && make install'
  [ $? -ne 0 ] && exit 1
fi
# }}}

mkdir $rootfs/build
cat > "$rootfs/build/CreateDebianPackage.sh" <<-'EOF'
#!/bin/bash

distribution="<DIST>"
distributionVersion="<DISTVER>"
architecture="<ARCH>"
buildthreads="<BUILDTHREADS>"

function createPackage {
  echo "Building package ${1}..."
	fullversion=$(${1}-${2}/getVersion.sh)
	version=$(echo $fullversion | cut -d "-" -f 1)
	revision=$(echo $fullversion | cut -d "-" -f 2)
	revision=$(echo $revision | cut -d "." -f 1)
	if [ -z "$revision" ]; then
		echo "Error: Could not get revision."
		exit 1
	fi
	sourcePath=${3}-$version
	mv ${1}-${2} $sourcePath
	[ $? -ne 0 ] && exit 1
	cd $sourcePath
	[ $? -ne 0 ] && exit 1
	if [ -f ./bootstrap ]; then
	  ./bootstrap
	fi
	[ $? -ne 0 ] && exit 1
	cd ..
	if [ "$distribution" == "Debian" ]; then
		sed -i '/\/bin\/sh/a\
	if \[ "$(uname -m)" = "armv6l" \]; then\
	\techo "Wrong CPU instruction set. Are you trying to install the Debian package on Raspbian?"\
	\texit 1\
	fi' $sourcePath/debian/preinst
	fi
	sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
	if [ "$distributionVersion" != "stretch" ] && [ "$distributionVersion" != "buster" ] && [ "$distributionVersion" != "bullseye" ] && [ "$distributionVersion" != "bionic" ] && [ "$distributionVersion" != "focal" ] && [ "$distributionVersion" != "jammy" ]; then
		sed -i 's/, libcurl4-gnutls-dev//g' $sourcePath/debian/control
		sed -i 's/ --with-curl//g' $sourcePath/debian/rules
	fi
	date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
	echo "${3} (${fullversion}) ${distributionVersion}; urgency=low

  * See https://github.com/Homegear/${1}/issues?q=is%3Aissue+is%3Aclosed

 -- Dr. Sathya Laufer <s.laufer@homegear.email>  $date" > $sourcePath/debian/changelog
	tar -zcpf ${3}_$version.orig.tar.gz $sourcePath
	[ $? -ne 0 ] && exit 1
	cd $sourcePath
	[ $? -ne 0 ] && exit 1
	if [ $4 -eq 1 ]; then
		debuild --no-lintian -j${buildthreads} -us -uc -sd
		[ $? -ne 0 ] && exit 1
	else
		debuild --no-lintian -j${buildthreads} -us -uc
		[ $? -ne 0 ] && exit 1
	fi
	cd ..
	if [ $4 -eq 1 ]; then
		rm -f ${3}-dbgsym*.deb
		sed -i '/\.orig\.tar\.gz/d' ${3}_*.dsc
		dscfile=$(ls ${3}_*.dsc)
		sed -i "/-dbgsym_/d" ${3}_*.changes
		sed -i "/${dscfile}/d" ${3}_*.changes
		sed -i "/Checksums-Sha1:/a\ $(sha1sum ${3}_*.dsc | cut -d ' ' -f 1) $(stat --printf='%s' ${3}_*.dsc) ${dscfile}" ${3}_*.changes
		sed -i "/Checksums-Sha256:/a\ $(sha256sum ${3}_*.dsc | cut -d ' ' -f 1) $(stat --printf='%s' ${3}_*.dsc) ${dscfile}" ${3}_*.changes
		sed -i "/Files:/a\ $(md5sum ${3}_*.dsc | cut -d ' ' -f 1) $(stat --printf='%s' ${3}_*.dsc) misc optional ${dscfile}" ${3}_*.changes
	fi
	rm -Rf $sourcePath
}

function createPackageWithoutAutomake {
  echo "Building package ${1}..."
	fullversion=$(${1}-${2}/getVersion.sh)
	version=$(echo $fullversion | cut -d "-" -f 1)
	revision=$(echo $fullversion | cut -d "-" -f 2)
	revision=$(echo $revision | cut -d "." -f 1)
	if [ -z "$revision" ]; then
		echo "Error: Could not get revision."
		exit 1
	fi
	sourcePath=${3}-$version
	mv ${1}-${2} $sourcePath
	date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
	echo "${3} (${fullversion}) ${distributionVersion}; urgency=low

  * See https://github.com/Homegear/${1}/issues?q=is%3Aissue+is%3Aclosed

 -- Sathya Laufer <sathya@laufers.net>  $date" > $sourcePath/debian/changelog
	tar -zcpf ${3}_$version.orig.tar.gz $sourcePath
	[ $? -ne 0 ] && exit 1
	cd $sourcePath
	[ $? -ne 0 ] && exit 1
	debuild --no-lintian -us -uc
	[ $? -ne 0 ] && exit 1
	cd ..
	rm -Rf $sourcePath
}

cd /build

wget --https-only https://gitit.de/api/v4/projects/455/repository/archive.zip?sha=master -O master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
mv libc1-net-master* libc1-net-${1}

wget --https-only https://github.com/Homegear/libhomegear-base/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/libhomegear-node/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/libhomegear-ipc/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/python3-homegear/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm -f python3-homegear*/CMakeLists.txt
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/homegear-nodes-core/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/homegear-nodes-ui/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-HomeMaticBidCoS/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-HomeMaticWired/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-Insteon/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-MAX/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-PhilipsHue/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-Sonos/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-Kodi/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-Intertechno/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-Nanoleaf/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-CCU/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/Homegear-Velux-KLF200/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/dimmu311/Homegear-Loxone/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/homegear-influxdb/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/homegear-gateway/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/homegear-management/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/homegear-ui/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

wget --https-only https://github.com/Homegear/binrpc-client/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

if [[ -n $2 ]]; then
	wget --https-only https://gitit.de/api/v4/projects/2/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-AdminUI-${1}* Homegear-AdminUI-${1}

	wget --https-only https://gitit.de/api/v4/projects/11/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-easy-licensing-master* homegear-easy-licensing-${1}

	wget --https-only https://gitit.de/api/v4/projects/10/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-licensing-master* homegear-licensing-${1}

	wget --https-only https://gitit.de/api/v4/projects/6/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-nodes-extra-master* homegear-nodes-extra-${1}

	wget --https-only https://gitit.de/api/v4/projects/3/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-Beckhoff-${1}* Homegear-Beckhoff-${1}

	wget --https-only https://gitit.de/api/v4/projects/1/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-KNX-${1}* Homegear-KNX-${1}

	wget --https-only https://gitit.de/api/v4/projects/14/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-EnOcean-${1}* Homegear-EnOcean-${1}
	cd Homegear-EnOcean-${1}/misc/Device\ Description\ Files/
	wget --https-only https://github.com/Homegear/Homegear-EnOcean-XML/archive/master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	mv Homegear-EnOcean-XML-master/* .
	[ $? -ne 0 ] && exit 1
	rm -Rf Homegear-EnOcean-XML-master master.zip
	cd ../../..

	wget --https-only https://gitit.de/api/v4/projects/15/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-M-Bus-${1}* homegear-mbus-${1}

	wget --https-only https://gitit.de/api/v4/projects/4/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-Z-Wave-${1}* homegear-zwave-${1}

	wget --https-only https://gitit.de/api/v4/projects/5/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-Zigbee-${1}* homegear-zigbee-${1}

	wget --https-only https://gitit.de/api/v4/projects/114/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-abi-${1}* homegear-abi-${1}

	wget --https-only https://gitit.de/api/v4/projects/126/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-klafs-${1}* homegear-klafs-${1}

	wget --https-only https://gitit.de/api/v4/projects/13/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-WebSSH-${1}* homegear-webssh-${1}

	wget --https-only https://gitit.de/api/v4/projects/29/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-dc-connector-${1}* homegear-dc-connector-${1}

	wget --https-only https://gitit.de/api/v4/projects/125/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv mellonbot-${1}* mellonbot-${1}

	wget --https-only https://gitit.de/api/v4/projects/25/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-cloudconnect-${1}* homegear-cloudconnect-${1}

	wget --https-only https://gitit.de/api/v4/projects/467/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv c1-ssh-${1}* c1-ssh-${1}

  if [[ "$distribution" == "Raspbian" ]] && [[ "$distributionVersion" == "bullseye" ]]; then
    wget --https-only https://gitit.de/api/v4/projects/138/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
    [ $? -ne 0 ] && exit 1
    unzip ${1}.zip
    [ $? -ne 0 ] && exit 1
    rm ${1}.zip
    mv doorctrl-${1}* doorctrl-${1}

    wget --https-only https://gitit.de/api/v4/projects/332/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
    [ $? -ne 0 ] && exit 1
    unzip ${1}.zip
    [ $? -ne 0 ] && exit 1
    rm ${1}.zip
    mv ltp08-connector-${1}* ltp08-connector-${1}
	fi
fi

createPackage libc1-net $1 libc1-net 0
if test -f libc1-net*.deb; then
	dpkg -i libc1-net*.deb
else
	echo "Error building libc1-net."
	exit 1
fi

createPackage libhomegear-base $1 libhomegear-base 0
if test -f libhomegear-base*.deb; then
	dpkg -i libhomegear-base*.deb
else
	echo "Error building libhomegear-base."
	exit 1
fi

createPackage libhomegear-node $1 libhomegear-node 0
if test -f libhomegear-node*.deb; then
	dpkg -i libhomegear-node*.deb
else
	echo "Error building libhomegear-node."
	exit 1
fi

createPackage libhomegear-ipc $1 libhomegear-ipc 0
if test -f libhomegear-ipc*.deb; then
	dpkg -i libhomegear-ipc*.deb
else
	echo "Error building libhomegear-ipc."
	exit 1
fi

createPackageWithoutAutomake python3-homegear $1 python3-homegear 0

touch /tmp/HOMEGEAR_STATIC_INSTALLATION
createPackage Homegear $1 homegear 0
if test -f homegear*.deb; then
	dpkg --purge php8-homegear-dev
	dpkg -i homegear*.deb
else
	echo "Error building Homegear."
	exit 1
fi

createPackage homegear-nodes-core $1 homegear-nodes-core 0
createPackage homegear-nodes-ui $1 homegear-nodes-ui 0
createPackage Homegear-HomeMaticBidCoS $1 homegear-homematicbidcos 0
createPackage Homegear-HomeMaticWired $1 homegear-homematicwired 0
createPackage Homegear-Insteon $1 homegear-insteon 0
createPackage Homegear-MAX $1 homegear-max 0
createPackage Homegear-PhilipsHue $1 homegear-philipshue 0
createPackage Homegear-Sonos $1 homegear-sonos 0
createPackage Homegear-Kodi $1 homegear-kodi 0
createPackage Homegear-Intertechno $1 homegear-intertechno 0
createPackage Homegear-Nanoleaf $1 homegear-nanoleaf 0
createPackage Homegear-CCU $1 homegear-ccu 0
createPackage Homegear-Velux-KLF200 $1 homegear-velux-klf200 0
#createPackage Homegear-Loxone $1 homegear-loxone 0
createPackage homegear-influxdb $1 homegear-influxdb 0
createPackage homegear-gateway $1 homegear-gateway 0
createPackage homegear-management $1 homegear-management 0
createPackageWithoutAutomake homegear-ui $1 homegear-ui
createPackage binrpc-client $1 binrpc 0

if [[ -n $2 ]]; then
	sha256=`sha256sum /usr/bin/homegear | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if (sha256(homegearPath) != /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/std::string homegearPath(buffer.data(), size);/aif (sha256(homegearPath) != \"$sha256\") return false;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if (sha256(homegearPath) != /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/std::string homegearPath(buffer.data(), size);/aif (sha256(homegearPath) != \"$sha256\") return false;" homegear-licensing-${1}/src/Licensing.cpp

	sha256=`sha256sum /usr/lib/libhomegear-base.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if (sha256(baselibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if (baselibPath.empty()) return false;/aif (sha256(baselibPath) == \"$sha256\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if (sha256(baselibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if (baselibPath.empty()) return false;/aif (sha256(baselibPath) == \"$sha256\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	sha256=`sha256sum /usr/lib/libhomegear-node.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if (sha256(nodelibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if (nodelibPath.empty()) return false;/aif (sha256(nodelibPath) == \"$sha256\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if (sha256(nodelibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if (nodelibPath.empty()) return false;/aif (sha256(nodelibPath) == \"$sha256\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	sha256=`sha256sum /usr/lib/libhomegear-ipc.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if (sha256(ipclibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if (ipclibPath.empty()) return false;/aif (sha256(ipclibPath) == \"$sha256\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if (sha256(ipclibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if (ipclibPath.empty()) return false;/aif (sha256(ipclibPath) == \"$sha256\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	createPackageWithoutAutomake Homegear-AdminUI $1 homegear-adminui

	createPackage homegear-easy-licensing $1 homegear-easy-licensing 1
	createPackage homegear-licensing $1 homegear-licensing 1

	createPackage homegear-nodes-extra $1 homegear-nodes-extra 1
	createPackage Homegear-Beckhoff $1 homegear-beckhoff 1
	createPackage Homegear-KNX $1 homegear-knx 1
	createPackage Homegear-EnOcean $1 homegear-enocean 1
	createPackage homegear-mbus $1 homegear-mbus 1
	createPackage homegear-zwave $1 homegear-zwave 1
	createPackage homegear-zigbee $1 homegear-zigbee 1
	createPackage homegear-abi $1 homegear-abi 1
	createPackage homegear-klafs $1 homegear-klafs 1
	createPackage homegear-webssh $1 homegear-webssh 1
	createPackage homegear-dc-connector $1 homegear-dc-connector 1
	createPackage mellonbot $1 mellonbot 1
	createPackage homegear-cloudconnect $1 homegear-cloudconnect 1
	createPackage c1-ssh $1 c1-ssh 1
	if [[ "$distribution" == "Raspbian" ]] && [[ "$distributionVersion" == "bullseye" ]]; then
	  createPackage doorctrl $1 doorctrl 1
	  createPackage ltp08-connector $1 ltp08-connector 1
	fi
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackage.sh
sed -i "s/<ARCH>/${arch}/g" $rootfs/build/CreateDebianPackage.sh

cat > "$rootfs/build/CreateDebianPackageStable.sh" <<-'EOF'
#!/bin/bash

distribution="<DIST>"
distributionVersion="<DISTVER>"

/build/CreateDebianPackage.sh master $1

cd /build

if test -f libc1-net_*.deb && test -f libhomegear-base_*.deb && test -f libhomegear-node_*.deb && test -f libhomegear-ipc_*.deb && test -f python3-homegear_*.deb && test -f homegear_*.deb && test -f homegear-nodes-core_*.deb && test -f homegear-nodes-ui_*.deb && test -f homegear-homematicbidcos_*.deb && test -f homegear-homematicwired_*.deb && test -f homegear-insteon_*.deb && test -f homegear-max_*.deb && test -f homegear-philipshue_*.deb && test -f homegear-sonos_*.deb && test -f homegear-kodi_*.deb && test -f homegear-intertechno_*.deb && test -f homegear-nanoleaf_*.deb && test -f homegear-ccu_*.deb && test -f homegear-velux-klf200_*.deb && test -f homegear-influxdb_*.deb && test -f homegear-management_*.deb && test -f homegear-ui_*.deb && test -f binrpc_*.deb; then
	if [[ -n $1 ]]; then
		if test ! -f homegear-adminui_*.deb || test ! -f homegear-easy-licensing_*.deb || test ! -f homegear-licensing_*.deb || test ! -f homegear-nodes-extra_*.deb || test ! -f homegear-knx_*.deb || test ! -f homegear-enocean_*.deb || test ! -f homegear-mbus_*.deb || test ! -f homegear-zwave_*.deb || test ! -f homegear-zigbee_*.deb || test ! -f homegear-beckhoff_*.deb || test ! -f homegear-abi_*.deb || test ! -f homegear-klafs_*.deb || test ! -f homegear-webssh_*.deb || test ! -f homegear-dc-connector_*.deb || test ! -f mellonbot_*.deb || test ! -f homegear-cloudconnect_*.deb || test ! -f c1-ssh_*.deb; then
			echo "Error: Some or all packages from gitit.de could not be created."
			exit 1
		fi
		if [[ "$distribution" == "Raspbian" ]] && [[ "$distributionVersion" == "bullseye" ]]; then
			if test ! -f doorctrl_*.deb || test ! -f ltp08-connector_*.deb; then
				echo "Error: Some or all packages from gitit.de could not be created."
				exit 1
			fi
		fi
	fi
	if test -f /build/UploadRepository.sh; then
		/build/UploadRepository.sh
	fi
else
	echo "Error: Some or all packages could not be created."
	exit 1
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageStable.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackageStable.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageStable.sh

cat > "$rootfs/build/CreateDebianPackageTesting.sh" <<-'EOF'
#!/bin/bash

distribution="<DIST>"
distributionVersion="<DISTVER>"

/build/CreateDebianPackage.sh testing $1

cd /build

if test -f libc1-net_*.deb && test -f libhomegear-base_*.deb && test -f libhomegear-node_*.deb && test -f libhomegear-ipc_*.deb && test -f python3-homegear_*.deb && test -f homegear_*.deb && test -f homegear-nodes-core_*.deb && test -f homegear-nodes-ui_*.deb && test -f homegear-homematicbidcos_*.deb && test -f homegear-homematicwired_*.deb && test -f homegear-insteon_*.deb && test -f homegear-max_*.deb && test -f homegear-philipshue_*.deb && test -f homegear-sonos_*.deb && test -f homegear-kodi_*.deb && test -f homegear-intertechno_*.deb && test -f homegear-nanoleaf_*.deb && test -f homegear-ccu_*.deb && test -f homegear-velux-klf200_*.deb && test -f homegear-influxdb_*.deb && test -f homegear-management_*.deb && test -f homegear-ui_*.deb && test -f binrpc_*.deb; then
	if [[ -n $1 ]]; then
		if test ! -f homegear-adminui_*.deb || test ! -f homegear-easy-licensing_*.deb || test ! -f homegear-licensing_*.deb || test ! -f homegear-nodes-extra_*.deb || test ! -f homegear-knx_*.deb || test ! -f homegear-enocean_*.deb || test ! -f homegear-mbus_*.deb || test ! -f homegear-zwave_*.deb || test ! -f homegear-zigbee_*.deb || test ! -f homegear-beckhoff_*.deb || test ! -f homegear-abi_*.deb || test ! -f homegear-klafs_*.deb || test ! -f homegear-webssh_*.deb || test ! -f homegear-dc-connector_*.deb || test ! -f mellonbot_*.deb || test ! -f homegear-cloudconnect_*.deb || test ! -f c1-ssh_*.deb; then
			echo "Error: Some or all packages from gitit.de could not be created."
			exit 1
		fi
		if [[ "$distribution" == "Raspbian" ]] && [[ "$distributionVersion" == "bullseye" ]]; then
			if test ! -f doorctrl_*.deb || test ! -f ltp08-connector_*.deb; then
				echo "Error: Some or all packages from gitit.de could not be created."
				exit 1
			fi
		fi
	fi
	if test -f /build/UploadRepository.sh; then
		/build/UploadRepository.sh
	fi
else
	echo "Error: Some or all packages could not be created."
	exit 1
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageTesting.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackageTesting.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageTesting.sh

cat > "$rootfs/build/CreateDebianPackageNightly.sh" <<-'EOF'
#!/bin/bash

distribution="<DIST>"
distributionVersion="<DISTVER>"

/build/CreateDebianPackage.sh dev $1

cd /build

if test -f libc1-net_*.deb && test -f libhomegear-base_*.deb && test -f libhomegear-node_*.deb && test -f libhomegear-ipc_*.deb && test -f python3-homegear_*.deb && test -f homegear_*.deb && test -f homegear-nodes-core_*.deb && test -f homegear-nodes-ui_*.deb && test -f homegear-homematicbidcos_*.deb && test -f homegear-homematicwired_*.deb && test -f homegear-insteon_*.deb && test -f homegear-max_*.deb && test -f homegear-philipshue_*.deb && test -f homegear-sonos_*.deb && test -f homegear-kodi_*.deb && test -f homegear-intertechno_*.deb && test -f homegear-nanoleaf_*.deb && test -f homegear-ccu_*.deb && test -f homegear-velux-klf200_*.deb && test -f homegear-influxdb_*.deb && test -f homegear-management_*.deb && test -f homegear-ui_*.deb && test -f binrpc_*.deb; then
	if [[ -n $1 ]]; then
		if test ! -f homegear-adminui_*.deb || test ! -f homegear-easy-licensing_*.deb || test ! -f homegear-licensing_*.deb || test ! -f homegear-nodes-extra_*.deb || test ! -f homegear-knx_*.deb || test ! -f homegear-enocean_*.deb || test ! -f homegear-mbus_*.deb || test ! -f homegear-zwave_*.deb || test ! -f homegear-zigbee_*.deb || test ! -f homegear-beckhoff_*.deb || test ! -f homegear-abi_*.deb || test ! -f homegear-klafs_*.deb || test ! -f homegear-webssh_*.deb || test ! -f homegear-dc-connector_*.deb || test ! -f mellonbot_*.deb || test ! -f homegear-cloudconnect_*.deb || test ! -f c1-ssh_*.deb; then
			echo "Error: Some or all packages from gitit.de could not be created."
			exit 1
		fi
		if [[ "$distribution" == "Raspbian" ]] && [[ "$distributionVersion" == "bullseye" ]]; then
			if test ! -f doorctrl_*.deb || test ! -f ltp08-connector_*.deb; then
				echo "Error: Some or all packages from gitit.de could not be created."
				exit 1
			fi
		fi
	fi
	if test -f /build/UploadRepository.sh; then
		/build/UploadRepository.sh
	fi
else
	echo "Error: Some or all packages could not be created."
	exit 1
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [ -n "$HOMEGEARBUILD_THREADS" ]; then
	sed -i "s/<BUILDTHREADS>/${HOMEGEARBUILD_THREADS}/g" /build/CreateDebianPackage.sh
else
	sed -i "s/<BUILDTHREADS>/1/g" /build/CreateDebianPackage.sh
fi
if [[ -z "$HOMEGEARBUILD_SERVERNAME" || -z "$HOMEGEARBUILD_SERVERPORT" || -z "$HOMEGEARBUILD_SERVERUSER" || -z "$HOMEGEARBUILD_SERVERPATH" || -z "$HOMEGEARBUILD_SERVERCERT" ]]; then
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageStable.sh\" or \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
	exit 0
else
	HOMEGEARBUILD_SERVERPATH=${HOMEGEARBUILD_SERVERPATH%/}
	echo "Testing connection..."
	mkdir -p /root/.ssh
	echo "$HOMEGEARBUILD_SERVERCERT" > /root/.ssh/id_ed25519
	chmod 400 /root/.ssh/id_ed25519
	sed -i -- 's/\\n/\n/g' /root/.ssh/id_ed25519
	if [ -n "$HOMEGEARBUILD_SERVER_KNOWNHOST_ENTRY" ]; then
		echo "$HOMEGEARBUILD_SERVER_KNOWNHOST_ENTRY" > /root/.ssh/known_hosts
		sed -i -- 's/\\n/\n/g' /root/.ssh/known_hosts
	fi
	ssh -p $HOMEGEARBUILD_SERVERPORT ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME} "echo \"It works :-)\""
fi

echo "#!/bin/bash

set -x

cd /build
if [ \$(ls /build | grep -c \"\\.changes\$\") -ne 0 ]; then
	path=\`mktemp -p / -u\`".tar.gz"
	echo \"<DIST>\" > distribution
	tar -zcpf \${path} homegear* lib* doorbell* doorctrl* ltp08* c1-ssh* mellon* python3-homegear* binrpc* distribution
	if test -f \${path}; then
		mv \${path} \${path}.uploading
		filename=\$(basename \$path)
		scp -P $HOMEGEARBUILD_SERVERPORT \${path}.uploading ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME}:${HOMEGEARBUILD_SERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $HOMEGEARBUILD_SERVERPORT ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME} \"mv ${HOMEGEARBUILD_SERVERPATH}/\${filename}.uploading ${HOMEGEARBUILD_SERVERPATH}/\${filename}\"
			if [ \$? -eq 0 ]; then
				echo "Packages uploaded successfully."
				exit 0
			else
				exit \$?
			fi
		fi
	fi
fi
" > /build/UploadRepository.sh
chmod 755 /build/UploadRepository.sh

rm /FirstStart.sh

if [ "$HOMEGEARBUILD_TYPE" = "stable" ]; then
	/build/CreateDebianPackageStable.sh ${HOMEGEARBUILD_API_KEY}
elif [ "$HOMEGEARBUILD_TYPE" = "testing" ]; then
	/build/CreateDebianPackageTesting.sh ${HOMEGEARBUILD_API_KEY}
elif [ "$HOMEGEARBUILD_TYPE" = "nightly" ]; then
	/build/CreateDebianPackageNightly.sh ${HOMEGEARBUILD_API_KEY}
else
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageStable.sh\" or \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
fi
EOF
chmod 755 $rootfs/FirstStart.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/FirstStart.sh

#read -p "Copy additional files into ${rootfs} and check that all packages were installed ok then hit [Enter] to continue..."

chroot $rootfs apt-get clean
chroot $rootfs umount /proc
rm -Rf $rootfs/var/lib/apt/lists/*
rm -Rf $rootfs/dev
rm -Rf $rootfs/proc
mkdir $rootfs/dev
mkdir $rootfs/proc

tar --numeric-owner -caf "$dir/rootfs.tar.xz" -C "$rootfs" --transform='s,^./,,' .

cat > "$dir/Dockerfile" <<'EOF'
FROM scratch
ADD rootfs.tar.xz /
CMD ["/FirstStart.sh"]
EOF

rm -Rf $rootfs

docker build -t homegear/build:${distlc}-${distver}-${arch} "$dir"
if [ "$dist" == "Raspbian" ]; then
	docker tag -f homegear/build:${distlc}-${distver}-${arch} homegear/build:${distlc}-${distver}
fi

rm -Rf $dir
