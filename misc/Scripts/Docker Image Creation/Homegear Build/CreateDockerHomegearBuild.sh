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
	echo "Please provide a valid Linux distribution version (wheezy, trusty, ...)."	
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
	repository="http://mirrordirector.raspbian.org/raspbian/"
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

if [ "$dist" == "Ubuntu" ]; then
	echo "deb $repository $distver main restricted universe multiverse
	" > $rootfs/etc/apt/sources.list
elif [ "$dist" == "Debian" ]; then
	echo "deb http://ftp.debian.org/debian $distver main contrib
	" > $rootfs/etc/apt/sources.list
elif [ "$dist" == "Raspbian" ]; then
	echo "deb http://mirrordirector.raspbian.org/raspbian/ $distver main contrib
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

if [ "$distver" == "stretch" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y --allow-unauthenticated install debian-keyring debian-archive-keyring
fi

chroot $rootfs apt-get update
if [ "$distver" == "stretch" ] || [ "$distver" == "vivid" ] || [ "$distver" == "wily" ] || [ "$distver" == "xenial" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
fi
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install apt-transport-https

echo "deb https://homegear.eu/packages/$dist/ $distver/
" > $rootfs/etc/apt/sources.list.d/homegear.list

wget -P $rootfs https://homegear.eu/packages/Release.key
chroot $rootfs apt-key add Release.key
rm $rootfs/Release.key

chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install ssh unzip ca-certificates binutils debhelper devscripts automake autoconf libtool sqlite3 libsqlite3-dev libncurses5-dev libssl-dev libparse-debcontrol-perl libgpg-error-dev php7-homegear-dev libxslt1-dev libedit-dev libenchant-dev libqdbm-dev libcrypto++-dev libltdl-dev zlib1g-dev libtinfo-dev libgmp-dev libxml2-dev libmodbus-dev libzip-dev p7zip-full ntp

if [ "$distver" == "stretch" ];  then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install default-libmysqlclient-dev dirmngr
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libmysqlclient-dev
fi

# {{{ GCC, GCrypt, GNUTLS, Curl
if [ "$distver" == "wheezy" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libgcrypt11-dev libgnutls-dev g++-4.7 gcc-4.7 libcurl4-gnutls-dev
	rm $rootfs/usr/bin/g++
	rm $rootfs/usr/bin/gcc
	ln -s g++-4.7 $rootfs/usr/bin/g++
	ln -s gcc-4.7 $rootfs/usr/bin/gcc
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libgcrypt20-dev libgnutls28-dev
	if [ "$distver" == "stretch" ] || [ "$distver" == "wheezy" ] || [ "$distver" == "jessie" ] || [ "$distver" == "xenial" ]; then
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libcurl4-gnutls-dev
	fi
fi
# }}}

# {{{ Readline
if [ "$distver" == "stretch" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline7 libreadline-dev
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline6 libreadline6-dev
fi
# }}}

mkdir $rootfs/build
cat > "$rootfs/build/CreateDebianPackage.sh" <<-'EOF'
#!/bin/bash

distribution="<DIST>"
distributionVersion="<DISTVER>"
buildthreads="<BUILDTHREADS>"

function createPackage {
	fullversion=$(${1}-${2}/getVersion.sh)
	version=$(echo $fullversion | cut -d "-" -f 1)
	revision=$(echo $fullversion | cut -d "-" -f 2)
	if [ $revision -eq 0 ]; then
		echo "Error: Could not get revision."
		exit 1
	fi
	sourcePath=${3}-$version
	mv ${1}-${2} $sourcePath
	cd $sourcePath
	./bootstrap
	cd ..
	if [ "$distribution" == "Debian" ]; then
		sed -i '/\/bin\/sh/a\
	if \[ "$(uname -m)" = "armv6l" \]; then\
	\techo "Wrong CPU instruction set. Are you trying to install the Debian package on Raspbian?"\
	\texit 1\
	fi' $sourcePath/debian/preinst
	fi
	sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
	if [ "$distributionVersion" == "wheezy" ]; then
		sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
		sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
		sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
		sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
	fi
	if [ "$distributionVersion" != "stretch" ] && [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "wheezy" ] && [ "$distributionVersion" != "xenial" ]; then
		sed -i 's/, libcurl4-gnutls-dev//g' $sourcePath/debian/control
		sed -i 's/ --with-curl//g' $sourcePath/debian/rules
	fi
	date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
	echo "${3} (${fullversion}) ${distributionVersion}; urgency=low

  * See https://github.com/Homegear/${1}/issues?q=is%3Aissue+is%3Aclosed

 -- Sathya Laufer <sathya@laufers.net>  $date" > $sourcePath/debian/changelog
	tar -zcpf ${3}_$version.orig.tar.gz $sourcePath
	cd $sourcePath
	if [ $4 -eq 1 ]; then
		debuild -j${buildthreads} -us -uc -sd
	else
		debuild -j${buildthreads} -us -uc
	fi
	cd ..
	if [ $4 -eq 1 ]; then
		rm -f ${3}-dbgsym*.deb
		sed -i '/\.orig\.tar\.gz/d' ${3}_*.dsc
	fi
	rm -Rf $sourcePath
}

cd /build

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

wget --https-only https://github.com/Homegear/Homegear-IPCam/archive/${1}.zip
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

wget --https-only https://github.com/Homegear/homegear-influxdb/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

if [[ -n $2 ]]; then
	wget --https-only https://gitit.de/Homegear-Addons/homegear-easy-licensing/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-easy-licensing-${1}* homegear-easy-licensing-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-licensing/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-licensing-${1}* homegear-licensing-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-nodes-extra/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-nodes-extra-${1}* homegear-nodes-extra-${1}

	wget --https-only https://gitit.de/Homegear-Addons/Homegear-Beckhoff/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-Beckhoff-${1}* Homegear-Beckhoff-${1}

	wget --https-only https://gitit.de/Homegear-Addons/Homegear-KNX/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv Homegear-KNX-${1}* Homegear-KNX-${1}

	wget --https-only https://gitit.de/Homegear-Addons/Homegear-EnOcean/repository/dev/archive.zip?private_token=${2} -O ${1}
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

	wget --https-only https://gitit.de/Homegear-Addons/homegear-easycam/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-easycam-${1}* homegear-easycam-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-easyled/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-easyled-${1}* homegear-easyled-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-easyled2/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-easyled2-${1}* homegear-easyled2-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-rsl/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-rsl-${1}* homegear-rsl-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-rs2w/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-rs2w-${1}* homegear-rs2w-${1}

	wget --https-only https://gitit.de/Homegear-Addons/homegear-gateway/repository/dev/archive.zip?private_token=${2} -O ${1}
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
	mv homegear-gateway-${1}* homegear-gateway-${1}
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

touch /tmp/HOMEGEAR_STATIC_INSTALLATION
createPackage Homegear $1 homegear 0
if test -f homegear*.deb; then
	dpkg -i homegear*.deb
else
	echo "Error building Homegear."
	exit 1
fi

createPackage homegear-nodes-core $1 homegear-nodes-core 0
createPackage Homegear-HomeMaticBidCoS $1 homegear-homematicbidcos 0
createPackage Homegear-HomeMaticWired $1 homegear-homematicwired 0
createPackage Homegear-Insteon $1 homegear-insteon 0
createPackage Homegear-MAX $1 homegear-max 0
createPackage Homegear-PhilipsHue $1 homegear-philipshue 0
createPackage Homegear-Sonos $1 homegear-sonos 0
createPackage Homegear-Kodi $1 homegear-kodi 0
createPackage Homegear-IPCam $1 homegear-ipcam 0
createPackage Homegear-Intertechno $1 homegear-intertechno 0
createPackage Homegear-Nanoleaf $1 homegear-nanoleaf 0
createPackage homegear-influxdb $1 homegear-influxdb 0
if [[ -n $2 ]]; then
	sha512=`sha512sum /usr/bin/homegear | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if(sha512(homegearPath) != /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/std::string homegearPath(buffer, size);/aif(sha512(homegearPath) != \"$sha512\") return false;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if(sha512(homegearPath) != /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/std::string homegearPath(buffer, size);/aif(sha512(homegearPath) != \"$sha512\") return false;" homegear-licensing-${1}/src/Licensing.cpp

	sha512=`sha512sum /usr/lib/libhomegear-base.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if(sha512(baselibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if(baselibPath.empty()) return false;/aif(sha512(baselibPath) == \"$sha512\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if(sha512(baselibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if(baselibPath.empty()) return false;/aif(sha512(baselibPath) == \"$sha512\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	createPackage homegear-easy-licensing $1 homegear-easy-licensing 1
	createPackage homegear-licensing $1 homegear-licensing 1

	createPackage homegear-nodes-extra $1 homegear-nodes-extra 1
	createPackage Homegear-Beckhoff $1 homegear-beckhoff 1
	createPackage Homegear-KNX $1 homegear-knx 1
	createPackage Homegear-EnOcean $1 homegear-enocean 1
	createPackage homegear-easycam $1 homegear-easycam 1
	createPackage homegear-easyled $1 homegear-easyled 1
	createPackage homegear-easyled2 $1 homegear-easyled2 1
	createPackage homegear-rsl $1 homegear-rsl 1
	createPackage homegear-rs2w $1 homegear-rs2w 1
	createPackage homegear-gateway $1 homegear-gateway 1
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackage.sh

cat > "$rootfs/build/CreateDebianPackageNightly.sh" <<-'EOF'
#!/bin/bash

function cleanUp {
	rm ${1}_*.build
	rm ${1}_*.changes
	rm ${1}_*.debian.tar.?z
	rm ${1}_*.dsc
	rm ${1}_*.orig.tar.gz
	mv ${1}_*.deb ${1}.deb
}

function cleanUp2 {
	rm ${1}_*.build
	rm ${1}_*.changes
	rm ${1}_*.debian.tar.?z
	rm ${1}_*.dsc
	rm ${1}_*.orig.tar.gz
	rm ${1}-dbgsym*.deb
	mv ${1}_*.deb ${1}.deb
}

/build/CreateDebianPackage.sh dev $1

cd /build

cleanUp libhomegear-base
cleanUp libhomegear-node
cleanUp libhomegear-ipc
cleanUp homegear
cleanUp homegear-nodes-core
cleanUp homegear-homematicbidcos
cleanUp homegear-homematicwired
cleanUp homegear-insteon
cleanUp homegear-max
cleanUp homegear-philipshue
cleanUp homegear-sonos
cleanUp homegear-kodi
cleanUp homegear-ipcam
cleanUp homegear-intertechno
cleanUp homegear-nanoleaf
cleanUp homegear-influxdb
if [[ -n $1 ]]; then
	cleanUp2 homegear-easy-licensing
	cleanUp2 homegear-licensing

	cleanUp2 homegear-nodes-extra
	cleanUp2 homegear-beckhoff
	cleanUp2 homegear-knx
	cleanUp2 homegear-enocean
	cleanUp2 homegear-easycam
	cleanUp2 homegear-easyled
	cleanUp2 homegear-easyled2
	cleanUp2 homegear-rsl
	cleanUp2 homegear-rs2w
	cleanUp2 homegear-gateway
fi

sed -i '/\.orig\.tar\.gz/d' *.dsc
EOF
echo "if test -f libhomegear-base.deb && test -f libhomegear-node.deb && test -f libhomegear-ipc.deb && test -f homegear.deb && test -f homegear-nodes-core.deb && test -f homegear-homematicbidcos.deb && test -f homegear-homematicwired.deb && test -f homegear-insteon.deb && test -f homegear-max.deb && test -f homegear-philipshue.deb && test -f homegear-sonos.deb && test -f homegear-kodi.deb && test -f homegear-ipcam.deb && test -f homegear-intertechno.deb && test -f homegear-nanoleaf.deb && test -f homegear-influxdb.deb; then
	isodate=\`date +%Y%m%d\`
	mv libhomegear-base.deb libhomegear-base_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv libhomegear-node.deb libhomegear-node_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv libhomegear-ipc.deb libhomegear-ipc_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear.deb homegear_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-nodes-core.deb homegear-nodes-core_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-homematicbidcos.deb homegear-homematicbidcos_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-homematicwired.deb homegear-homematicwired_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-insteon.deb homegear-insteon_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-max.deb homegear-max_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-philipshue.deb homegear-philipshue_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-sonos.deb homegear-sonos_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-kodi.deb homegear-kodi_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-ipcam.deb homegear-ipcam_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-intertechno.deb homegear-intertechno_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-nanoleaf.deb homegear-nanoleaf_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-influxdb.deb homegear-influxdb_\$[isodate]_${distlc}_${distver}_${arch}.deb
	if [[ -n \$1 ]]; then
		mv homegear-easy-licensing.deb homegear-easy-licensing_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-licensing.deb homegear-licensing_\$[isodate]_${distlc}_${distver}_${arch}.deb

		mv homegear-nodes-extra.deb homegear-nodes-extra_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-beckhoff.deb homegear-beckhoff_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-knx.deb homegear-knx_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-enocean.deb homegear-enocean_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-easycam.deb homegear-easycam_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-easyled.deb homegear-easyled_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-easyled2.deb homegear-easyled2_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-rsl.deb homegear-rsl_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-rs2w.deb homegear-rs2w_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-gateway.deb homegear-gateway_\$[isodate]_${distlc}_${distver}_${arch}.deb
	fi
	if test -f /build/UploadNightly.sh; then
		/build/UploadNightly.sh
	fi
fi" >> $rootfs/build/CreateDebianPackageNightly.sh
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/build/CreateDebianPackageStable.sh" <<-'EOF'
#!/bin/bash

/build/CreateDebianPackage.sh master $1

cd /build

if test -f libhomegear-base*.deb && test -f libhomegear-node*.deb && test -f libhomegear-ipc*.deb && test -f homegear_*.deb && test -f homegear-nodes-core*.deb && test -f homegear-homematicbidcos*.deb && test -f homegear-homematicwired*.deb && test -f homegear-insteon*.deb && test -f homegear-max*.deb && test -f homegear-philipshue*.deb && test -f homegear-sonos*.deb && test -f homegear-kodi*.deb && test -f homegear-ipcam*.deb && test -f homegear-intertechno*.deb && test -f homegear-nanoleaf*.deb && test -f homegear-influxdb*.deb; then
	if test -f /build/UploadStable.sh; then
		/build/UploadStable.sh
	fi
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageStable.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [ -n "$HOMEGEARBUILD_THREADS" ]; then
	sed -i "s/<BUILDTHREADS>/${HOMEGEARBUILD_THREADS}/g" /build/CreateDebianPackage.sh
else
	sed -i "s/<BUILDTHREADS>/1/g" /build/CreateDebianPackage.sh
fi
if [ -n "$HOMEGEARBUILD_SHELL" ]; then
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageStable.sh\" or \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
	exit 0
fi
if [[ -z "$HOMEGEARBUILD_SERVERNAME" || -z "$HOMEGEARBUILD_SERVERPORT" || -z "$HOMEGEARBUILD_SERVERUSER" || -z "$HOMEGEARBUILD_STABLESERVERPATH" || -z "$HOMEGEARBUILD_NIGHTLYSERVERPATH" || -z "$HOMEGEARBUILD_SERVERCERT" ]]; then
	while :
	do
		echo "Setting up SSH package uploading:"
		while :
		do
			read -p "Please specify the server name to upload to: " HOMEGEARBUILD_SERVERNAME
			if [ -n "$HOMEGEARBUILD_SERVERNAME" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the SSH port number of the server: " HOMEGEARBUILD_SERVERPORT
			if [ -n "$HOMEGEARBUILD_SERVERPORT" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the user name to use to login into the server: " HOMEGEARBUILD_SERVERUSER
			if [ -n "$HOMEGEARBUILD_SERVERUSER" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the path on the server to upload stable packages to: " HOMEGEARBUILD_STABLESERVERPATH
			HOMEGEARBUILD_STABLESERVERPATH=${HOMEGEARBUILD_STABLESERVERPATH%/}
			if [ -n "$HOMEGEARBUILD_STABLESERVERPATH" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the path on the server to upload nightly packages to: " HOMEGEARBUILD_NIGHTLYSERVERPATH
			HOMEGEARBUILD_NIGHTLYSERVERPATH=${HOMEGEARBUILD_NIGHTLYSERVERPATH%/}
			if [ -n "$HOMEGEARBUILD_NIGHTLYSERVERPATH" ]; then
				break
			fi
		done
		echo "Paste your certificate:"
		IFS= read -d '' -n 1 HOMEGEARBUILD_SERVERCERT
		while IFS= read -d '' -n 1 -t 2 c
		do
		    HOMEGEARBUILD_SERVERCERT+=$c
		done
		echo
		echo
		echo "Testing connection..."
		mkdir -p /root/.ssh
		echo "$HOMEGEARBUILD_SERVERCERT" > /root/.ssh/id_rsa
		chmod 400 /root/.ssh/id_rsa
		ssh -p $HOMEGEARBUILD_SERVERPORT ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME} "echo \"It works :-)\""
		echo
		echo -e "Server name:\t$HOMEGEARBUILD_SERVERNAME"
		echo -e "Server port:\t$HOMEGEARBUILD_SERVERPORT"
		echo -e "Server user:\t$HOMEGEARBUILD_SERVERUSER"
		echo -e "Server path (stable):\t$HOMEGEARBUILD_STABLESERVERPATH"
		echo -e "Server path (nightlies):\t$HOMEGEARBUILD_NIGHTLYSERVERPATH"
		echo -e "Certificate:\t"
		echo "$HOMEGEARBUILD_SERVERCERT"
		while :
		do
			read -p "Is this information correct [y/n]: " correct
			if [ -n "$correct" ]; then
				break
			fi
		done
		if [ "$correct" = "y" ]; then
			break
		fi
	done
else
	HOMEGEARBUILD_STABLESERVERPATH=${HOMEGEARBUILD_STABLESERVERPATH%/}
	HOMEGEARBUILD_NIGHTLYSERVERPATH=${HOMEGEARBUILD_NIGHTLYSERVERPATH%/}
	echo "Testing connection..."
	mkdir -p /root/.ssh
	echo "$HOMEGEARBUILD_SERVERCERT" > /root/.ssh/id_rsa
	chmod 400 /root/.ssh/id_rsa
	sed -i -- 's/\\n/\n/g' /root/.ssh/id_rsa
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
	tar -zcpf \${path} homegear* lib* distribution
	if test -f \${path}; then
		mv \${path} \${path}.uploading
		filename=\$(basename \$path)
		scp -P $HOMEGEARBUILD_SERVERPORT \${path}.uploading ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME}:${HOMEGEARBUILD_STABLESERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $HOMEGEARBUILD_SERVERPORT ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME} \"mv ${HOMEGEARBUILD_STABLESERVERPATH}/\${filename}.uploading ${HOMEGEARBUILD_STABLESERVERPATH}/\${filename}\"
			if [ \$? -eq 0 ]; then
				echo "Packages uploaded successfully."
				exit 0
			else
				exit \$?
			fi
		fi
	fi
fi
" > /build/UploadStable.sh
chmod 755 /build/UploadStable.sh

echo "#!/bin/bash

set -x

cd /build
if [ \$(ls /build | grep -c \"\\.deb\$\") -ne 0 ]; then
	path=\`mktemp -p / -u\`".tar.gz"
	tar -zcpf \${path} *.deb
	if test -f \${path}; then
		mv \${path} \${path}.uploading
		filename=\$(basename \$path)
		scp -P $HOMEGEARBUILD_SERVERPORT \${path}.uploading ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME}:${HOMEGEARBUILD_NIGHTLYSERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $HOMEGEARBUILD_SERVERPORT ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME} \"mv ${HOMEGEARBUILD_NIGHTLYSERVERPATH}/\${filename}.uploading ${HOMEGEARBUILD_NIGHTLYSERVERPATH}/\${filename}\"
			if [ \$? -eq 0 ]; then
				echo "Packages uploaded successfully."
				exit 0
			else
				exit \$?
			fi
		fi
	fi
fi
" > /build/UploadNightly.sh
chmod 755 /build/UploadNightly.sh
rm /FirstStart.sh

if [ "$HOMEGEARBUILD_TYPE" = "stable" ]; then
	/build/CreateDebianPackageStable.sh ${HOMEGEARBUILD_API_KEY}
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
