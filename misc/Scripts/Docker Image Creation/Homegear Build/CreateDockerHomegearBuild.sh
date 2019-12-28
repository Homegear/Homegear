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

if [ "$distver" == "stretch" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y --allow-unauthenticated install debian-keyring debian-archive-keyring
fi

if [ "$distver" == "bionic" ] || [ "$distver" == "buster" ]; then
	if [ "$arch" == "arm64" ]; then # Workaround for "syscall 277 error" in man-db
		export MAN_DISABLE_SECCOMP=1
	fi
fi

if [ "$distver" == "bionic" ] || [ "$distver" == "buster" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y install gnupg
fi

if [ "$distver" == "jessie" ]; then
	wget -P $rootfs https://packages.sury.org/php/apt.gpg
	chroot $rootfs apt-key add apt.gpg
	rm $rootfs/apt.gpg

	echo "deb https://packages.sury.org/php $distver main" > $rootfs/etc/apt/sources.list.d/php7.list
fi

chroot $rootfs apt-get update
if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "vivid" ] || [ "$distver" == "wily" ] || [ "$distver" == "xenial" ] || [ "$distver" == "bionic" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
fi
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install apt-transport-https ca-certificates curl
DEBIAN_FRONTEND=noninteractive chroot $rootfs 

echo "deb https://homegear.eu/packages/$dist/ $distver/" > $rootfs/etc/apt/sources.list.d/homegear.list

wget -P $rootfs https://homegear.eu/packages/Release.key
chroot $rootfs apt-key add Release.key
rm $rootfs/Release.key

wget -P $rootfs https://deb.nodesource.com/gpgkey/nodesource.gpg.key
chroot $rootfs apt-key add nodesource.gpg.key
rm $rootfs/nodesource.gpg.key

if [ "$distver" != "buster" ] || [ "$arch" == "i386" ]; then
	if [ "$arch" == "i386" ]; then
		# 10.x and later are not available for i386
		echo "deb https://deb.nodesource.com/node_9.x $distver main" > $rootfs/etc/apt/sources.list.d/nodesource.list
	else
		echo "deb https://deb.nodesource.com/node_12.x $distver main" > $rootfs/etc/apt/sources.list.d/nodesource.list
	fi
fi

DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install ssh wget unzip binutils debhelper devscripts automake autoconf libtool sqlite3 libsqlite3-dev libncurses5-dev libssl-dev libparse-debcontrol-perl libgpg-error-dev php7-homegear-dev libxslt1-dev libedit-dev libenchant-dev libqdbm-dev libcrypto++-dev libltdl-dev zlib1g-dev libtinfo-dev libgmp-dev libxml2-dev libzip-dev p7zip-full ntp libavahi-common-dev libavahi-client-dev libicu-dev libpython3-dev python3-all python3-setuptools

if [ "$distver" == "buster" ] && [ "$arch" != "i386" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install npm
else
	if [ "$arch" == "i386" ]; then
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install nodejs=9.11.2-1nodesource1
	else
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install nodejs
	fi
fi

# Fix npm uid/gid issue (effect: npm install doesn't work)
if test -d $rootfs/usr/lib/node_modules/npm/node_modules/uid-number; then
	echo "module.exports = function uidNumber(uid, gid, cb) {cb(null, 0, 0)}" > $rootfs/usr/lib/node_modules/npm/node_modules/uid-number/uid-number.js
fi
if test -d $rootfs/usr/lib/nodejs/npm/node_modules/uid-number; then
	echo "module.exports = function uidNumber(uid, gid, cb) {cb(null, 0, 0)}" > $rootfs/usr/lib/nodejs/npm/node_modules/uid-number/uid-number.js
fi

if [ "$distver" != "bionic" ] && [ "$distver" != "buster" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install insserv
fi

if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ];  then
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
	if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "wheezy" ] || [ "$distver" == "jessie" ] || [ "$distver" == "xenial" ] || [ "$distver" == "bionic" ]; then
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libcurl4-gnutls-dev
	fi
fi
# }}}

# {{{ Readline
if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "bionic" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline7 libreadline-dev
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libreadline6 libreadline6-dev
fi
# }}}

# {{{ UI build dependencies
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install php-cli
DEBIAN_FRONTEND=noninteractive chroot $rootfs npm -g install babel-cli
DEBIAN_FRONTEND=noninteractive chroot $rootfs ln -s /usr/local/bin/babel /usr/bin/babel
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
	[ $? -ne 0 ] && exit 1
	cd $sourcePath
	[ $? -ne 0 ] && exit 1
	./bootstrap
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
	if [ "$distributionVersion" == "wheezy" ]; then
		sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
		sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
		sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
		sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
	fi
	if [ "$distributionVersion" != "stretch" ] && [ "$distributionVersion" != "buster" ] && [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "wheezy" ] && [ "$distributionVersion" != "xenial" ] && [ "$distributionVersion" != "bionic" ]; then
		sed -i 's/, libcurl4-gnutls-dev//g' $sourcePath/debian/control
		sed -i 's/ --with-curl//g' $sourcePath/debian/rules
	fi
	date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
	echo "${3} (${fullversion}) ${distributionVersion}; urgency=low

  * See https://github.com/Homegear/${1}/issues?q=is%3Aissue+is%3Aclosed

 -- Sathya Laufer <sathya@laufers.net>  $date" > $sourcePath/debian/changelog
	tar -zcpf ${3}_$version.orig.tar.gz $sourcePath
	[ $? -ne 0 ] && exit 1
	cd $sourcePath
	[ $? -ne 0 ] && exit 1
	if [ $4 -eq 1 ]; then
		debuild -j${buildthreads} -us -uc -sd
		[ $? -ne 0 ] && exit 1
	else
		debuild -j${buildthreads} -us -uc
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
	fullversion=$(${1}-${2}/getVersion.sh)
	version=$(echo $fullversion | cut -d "-" -f 1)
	revision=$(echo $fullversion | cut -d "-" -f 2)
	if [ $revision -eq 0 ]; then
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
	debuild -us -uc
	[ $? -ne 0 ] && exit 1
	cd ..
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

wget --https-only https://github.com/Homegear/Homegear-CCU/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

if [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "xenial" ]; then
	wget --https-only https://github.com/Homegear/Homegear-Velux-KLF200/archive/${1}.zip
	[ $? -ne 0 ] && exit 1
	unzip ${1}.zip
	[ $? -ne 0 ] && exit 1
	rm ${1}.zip
fi

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

	if [ "$distributionVersion" != "jessie" ]; then
		wget --https-only https://gitit.de/api/v4/projects/3/repository/archive.zip?sha=${1}\&private_token=${2} -O ${1}.zip
		[ $? -ne 0 ] && exit 1
		unzip ${1}.zip
		[ $? -ne 0 ] && exit 1
		rm ${1}.zip
		mv Homegear-Beckhoff-${1}* Homegear-Beckhoff-${1}
	fi

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

	wget --https-only https://gitit.de/api/v4/projects/9/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-easycam-master* homegear-easycam-${1}

	wget --https-only https://gitit.de/api/v4/projects/7/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-easyled-master* homegear-easyled-${1}

	wget --https-only https://gitit.de/api/v4/projects/8/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-easyled2-master* homegear-easyled2-${1}

	wget --https-only https://gitit.de/api/v4/projects/21/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-rsl-master* homegear-rsl-${1}

	wget --https-only https://gitit.de/api/v4/projects/20/repository/archive.zip?sha=master\&private_token=${2} -O master.zip
	[ $? -ne 0 ] && exit 1
	unzip master.zip
	[ $? -ne 0 ] && exit 1
	rm master.zip
	mv homegear-rs2w-master* homegear-rs2w-${1}

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
	dpkg --purge php7-homegear-dev
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
createPackage Homegear-CCU $1 homegear-ccu 0
if [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "xenial" ]; then
	createPackage Homegear-Velux-KLF200 $1 homegear-velux-klf200 0
fi
createPackage homegear-influxdb $1 homegear-influxdb 0
createPackage homegear-gateway $1 homegear-gateway 0
createPackage homegear-management $1 homegear-management 0
createPackageWithoutAutomake homegear-ui $1 homegear-ui
createPackage binrpc-client $1 binrpc 0

if [[ -n $2 ]]; then
	sha256=`sha256sum /usr/bin/homegear | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if(sha256(homegearPath) != /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/std::string homegearPath(buffer, size);/aif(sha256(homegearPath) != \"$sha256\") return false;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if(sha256(homegearPath) != /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/std::string homegearPath(buffer, size);/aif(sha256(homegearPath) != \"$sha256\") return false;" homegear-licensing-${1}/src/Licensing.cpp

	sha256=`sha256sum /usr/lib/libhomegear-base.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if(sha256(baselibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if(baselibPath.empty()) return false;/aif(sha256(baselibPath) == \"$sha256\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if(sha256(baselibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if(baselibPath.empty()) return false;/aif(sha256(baselibPath) == \"$sha256\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	sha256=`sha256sum /usr/lib/libhomegear-node.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if(sha256(nodelibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if(nodelibPath.empty()) return false;/aif(sha256(nodelibPath) == \"$sha256\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if(sha256(nodelibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if(nodelibPath.empty()) return false;/aif(sha256(nodelibPath) == \"$sha256\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	sha256=`sha256sum /usr/lib/libhomegear-ipc.so.1 | awk '{print toupper($0)}' | cut -d ' ' -f 1`
	sed -i '/if(sha256(ipclibPath) == /d' homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i "/if(ipclibPath.empty()) return false;/aif(sha256(ipclibPath) == \"$sha256\") return true;" homegear-easy-licensing-${1}/src/EasyLicensing.cpp
	sed -i '/if(sha256(ipclibPath) == /d' homegear-licensing-${1}/src/Licensing.cpp
	sed -i "/if(ipclibPath.empty()) return false;/aif(sha256(ipclibPath) == \"$sha256\") return true;" homegear-licensing-${1}/src/Licensing.cpp

	createPackageWithoutAutomake Homegear-AdminUI $1 homegear-adminui

	createPackage homegear-easy-licensing $1 homegear-easy-licensing 1
	createPackage homegear-licensing $1 homegear-licensing 1

	createPackage homegear-nodes-extra $1 homegear-nodes-extra 1
	if [ "$distributionVersion" != "jessie" ]; then
		createPackage Homegear-Beckhoff $1 homegear-beckhoff 1
	fi
	createPackage Homegear-KNX $1 homegear-knx 1
	createPackage Homegear-EnOcean $1 homegear-enocean 1
	createPackage homegear-easycam $1 homegear-easycam 1
	createPackage homegear-easyled $1 homegear-easyled 1
	createPackage homegear-easyled2 $1 homegear-easyled2 1
	createPackage homegear-rsl $1 homegear-rsl 1
	createPackage homegear-rs2w $1 homegear-rs2w 1
	createPackage homegear-mbus $1 homegear-mbus 1
	createPackage homegear-zwave $1 homegear-zwave 1
	createPackage homegear-zigbee $1 homegear-zigbee 1
	createPackage homegear-webssh $1 homegear-webssh 1
	createPackage homegear-dc-connector $1 homegear-dc-connector 1
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackage.sh

cat > "$rootfs/build/CreateDebianPackageNightly.sh" <<-'EOF'
#!/bin/bash

distributionVersion="<DISTVER>"

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
cleanUp python3-homegear
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
cleanUp homegear-ccu
if [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "xenial" ]; then
	cleanUp homegear-velux-klf200
fi
cleanUp homegear-influxdb
cleanUp homegear-gateway
cleanUp homegear-management
cleanUp homegear-ui
cleanUp binrpc
if [[ -n $1 ]]; then
	cleanUp homegear-adminui

	cleanUp2 homegear-easy-licensing
	cleanUp2 homegear-licensing

	cleanUp2 homegear-nodes-extra
	if [ "$distributionVersion" != "jessie" ]; then
		cleanUp2 homegear-beckhoff
	fi
	cleanUp2 homegear-knx
	cleanUp2 homegear-enocean
	cleanUp2 homegear-easycam
	cleanUp2 homegear-easyled
	cleanUp2 homegear-easyled2
	cleanUp2 homegear-rsl
	cleanUp2 homegear-rs2w
	cleanUp2 homegear-mbus
	cleanUp2 homegear-zwave
	cleanUp2 homegear-zigbee
	cleanUp2 homegear-webssh
	cleanUp2 homegear-dc-connector
fi
EOF
echo "if test -f libhomegear-base.deb && test -f libhomegear-node.deb && test -f libhomegear-ipc.deb && test -f python3-homegear.deb && test -f homegear.deb && test -f homegear-nodes-core.deb && test -f homegear-homematicbidcos.deb && test -f homegear-homematicwired.deb && test -f homegear-insteon.deb && test -f homegear-max.deb && test -f homegear-philipshue.deb && test -f homegear-sonos.deb && test -f homegear-kodi.deb && test -f homegear-ipcam.deb && test -f homegear-intertechno.deb && test -f homegear-nanoleaf.deb && test -f homegear-ccu.deb && test -f homegear-influxdb.deb && test -f homegear-gateway.deb && test -f homegear-management.deb && test -f homegear-ui.deb && test -f binrpc.deb; then
	isodate=\`date +%Y%m%d\`
	mv libhomegear-base.deb libhomegear-base_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv libhomegear-node.deb libhomegear-node_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv libhomegear-ipc.deb libhomegear-ipc_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv python3-homegear.deb python3-homegear_\$[isodate]_${distlc}_${distver}_${arch}.deb
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
	mv homegear-ccu.deb homegear-ccu_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-influxdb.deb homegear-influxdb_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-gateway.deb homegear-gateway_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-management.deb homegear-management_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-ui.deb homegear-ui_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv binrpc.deb binrpc_\$[isodate]_${distlc}_${distver}_${arch}.deb

	if [ \"\$distributionVersion\" != \"jessie\" ] && [ \"\$distributionVersion\" != \"xenial\" ]; then
		if test ! -f homegear-velux-klf200.deb; then
			echo \"Error: Some or all packages from GitHub could not be created.\"
			exit 1
		fi

		mv homegear-velux-klf200.deb homegear-velux-klf200_\$[isodate]_${distlc}_${distver}_${arch}.deb
	fi

	if [[ -n \$1 ]]; then
		if test ! -f homegear-adminui.deb || test ! -f homegear-easy-licensing.deb || test ! -f homegear-licensing.deb || test ! -f homegear-nodes-extra.deb || test ! -f homegear-knx.deb || test ! -f homegear-enocean.deb || test ! -f homegear-easycam.deb || test ! -f homegear-easyled.deb || test ! -f homegear-easyled2.deb || test ! -f homegear-rsl.deb || test ! -f homegear-rs2w.deb || test ! -f homegear-mbus.deb || test ! -f homegear-zwave.deb || test ! -f homegear-zigbee.deb || test ! -f homegear-webssh.deb || test ! -f homegear-dc-connector.deb; then
			echo \"Error: Some or all packages from gitit.de could not be created.\"
			exit 1
		fi

		mv homegear-adminui.deb homegear-adminui_\$[isodate]_${distlc}_${distver}_${arch}.deb

		mv homegear-easy-licensing.deb homegear-easy-licensing_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-licensing.deb homegear-licensing_\$[isodate]_${distlc}_${distver}_${arch}.deb

		mv homegear-nodes-extra.deb homegear-nodes-extra_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-knx.deb homegear-knx_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-enocean.deb homegear-enocean_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-easycam.deb homegear-easycam_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-easyled.deb homegear-easyled_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-easyled2.deb homegear-easyled2_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-rsl.deb homegear-rsl_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-rs2w.deb homegear-rs2w_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-mbus.deb homegear-mbus_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-zwave.deb homegear-zwave_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-zigbee.deb homegear-zigbee_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-webssh.deb homegear-webssh_\$[isodate]_${distlc}_${distver}_${arch}.deb
		mv homegear-dc-connector.deb homegear-dc-connector_\$[isodate]_${distlc}_${distver}_${arch}.deb

		if [ \"\$distributionVersion\" != \"jessie\" ]; then
			if test ! -f homegear-beckhoff.deb; then
				echo \"Error: Some or all packages from gitit.de could not be created.\"
				exit 1
			fi

			mv homegear-beckhoff.deb homegear-beckhoff_\$[isodate]_${distlc}_${distver}_${arch}.deb
		fi
	fi
	if test -f /build/UploadNightly.sh; then
		/build/UploadNightly.sh
	fi
fi" >> $rootfs/build/CreateDebianPackageNightly.sh
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/build/CreateDebianPackageStable.sh" <<-'EOF'
#!/bin/bash

distributionVersion="<DISTVER>"

/build/CreateDebianPackage.sh master $1

cd /build

if test -f libhomegear-base_*.deb && test -f libhomegear-node_*.deb && test -f libhomegear-ipc_*.deb && test -f python3-homegear_*.deb && test -f homegear_*.deb && test -f homegear-nodes-core_*.deb && test -f homegear-homematicbidcos_*.deb && test -f homegear-homematicwired_*.deb && test -f homegear-insteon_*.deb && test -f homegear-max_*.deb && test -f homegear-philipshue_*.deb && test -f homegear-sonos_*.deb && test -f homegear-kodi_*.deb && test -f homegear-ipcam_*.deb && test -f homegear-intertechno_*.deb && test -f homegear-nanoleaf_*.deb && test -f homegear-ccu_*.deb && test -f homegear-influxdb_*.deb && test -f homegear-management_*.deb && test -f homegear-ui_*.deb && test -f binrpc_*.deb; then
	if [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "xenial" ]; then
		if test ! -f homegear-velux-klf200_*.deb; then
			echo "Error: Some or all packages from GitHub could not be created."
			exit 1
		fi
	fi

	if [[ -n $1 ]]; then
		if test ! -f homegear-adminui_*.deb || test ! -f homegear-easy-licensing_*.deb || test ! -f homegear-licensing_*.deb || test ! -f homegear-nodes-extra_*.deb || test ! -f homegear-knx_*.deb || test ! -f homegear-enocean_*.deb || test ! -f homegear-easycam_*.deb || test ! -f homegear-easyled_*.deb || test ! -f homegear-easyled2_*.deb || test ! -f homegear-rsl_*.deb || test ! -f homegear-rs2w_*.deb || test ! -f homegear-mbus_*.deb || test ! -f homegear-zwave_*.deb || test ! -f homegear-zigbee_*.deb || test ! -f homegear-webssh_*.deb || test ! -f homegear-dc-connector_*.deb; then
			echo "Error: Some or all packages from gitit.de could not be created."
			exit 1
		fi

		if [ "$distributionVersion" != "jessie" ]; then
			if test ! -f homegear-beckhoff_*.deb; then
				echo "Error: Some or all packages from gitit.de could not be created."
				exit 1
			fi
		fi
	fi
	if test -f /build/UploadRepository.sh; then
		/build/UploadRepository.sh
	fi
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageStable.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageStable.sh

cat > "$rootfs/build/CreateDebianPackageTesting.sh" <<-'EOF'
#!/bin/bash

distributionVersion="<DISTVER>"

/build/CreateDebianPackage.sh testing $1

cd /build

if test -f libhomegear-base_*.deb && test -f libhomegear-node_*.deb && test -f libhomegear-ipc_*.deb && test -f python3-homegear_*.deb && test -f homegear_*.deb && test -f homegear-nodes-core_*.deb && test -f homegear-homematicbidcos_*.deb && test -f homegear-homematicwired_*.deb && test -f homegear-insteon_*.deb && test -f homegear-max_*.deb && test -f homegear-philipshue_*.deb && test -f homegear-sonos_*.deb && test -f homegear-kodi_*.deb && test -f homegear-ipcam_*.deb && test -f homegear-intertechno_*.deb && test -f homegear-nanoleaf_*.deb && test -f homegear-ccu_*.deb && test -f homegear-influxdb_*.deb && test -f homegear-management_*.deb && test -f homegear-ui_*.deb && test -f binrpc_*.deb; then
	if [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "xenial" ]; then
		if test ! -f homegear-velux-klf200_*.deb; then
			echo "Error: Some or all packages from GitHub could not be created."
			exit 1
		fi
	fi

	if [[ -n $1 ]]; then
		if test ! -f homegear-adminui_*.deb || test ! -f homegear-easy-licensing_*.deb || test ! -f homegear-licensing_*.deb || test ! -f homegear-nodes-extra_*.deb || test ! -f homegear-knx_*.deb || test ! -f homegear-enocean_*.deb || test ! -f homegear-easycam_*.deb || test ! -f homegear-easyled_*.deb || test ! -f homegear-easyled2_*.deb || test ! -f homegear-rsl_*.deb || test ! -f homegear-rs2w_*.deb || test ! -f homegear-mbus_*.deb || test ! -f homegear-zwave_*.deb || test ! -f homegear-zigbee_*.deb || test ! -f homegear-webssh_*.deb || test ! -f homegear-dc-connector_*.deb; then
			echo "Error: Some or all packages from gitit.de could not be created."
			exit 1
		fi

		if [ "$distributionVersion" != "jessie" ]; then
			if test ! -f homegear-beckhoff_*.deb; then
				echo "Error: Some or all packages from gitit.de could not be created."
				exit 1
			fi
		fi
	fi
	if test -f /build/UploadRepository.sh; then
		/build/UploadRepository.sh
	fi
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageTesting.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageTesting.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [ -n "$HOMEGEARBUILD_THREADS" ]; then
	sed -i "s/<BUILDTHREADS>/${HOMEGEARBUILD_THREADS}/g" /build/CreateDebianPackage.sh
else
	sed -i "s/<BUILDTHREADS>/1/g" /build/CreateDebianPackage.sh
fi
if [[ -z "$HOMEGEARBUILD_SERVERNAME" || -z "$HOMEGEARBUILD_SERVERPORT" || -z "$HOMEGEARBUILD_SERVERUSER" || -z "$HOMEGEARBUILD_REPOSITORYSERVERPATH" || -z "$HOMEGEARBUILD_NIGHTLYSERVERPATH" || -z "$HOMEGEARBUILD_SERVERCERT" ]]; then
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageStable.sh\" or \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
	exit 0
else
	HOMEGEARBUILD_REPOSITORYSERVERPATH=${HOMEGEARBUILD_REPOSITORYSERVERPATH%/}
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
	tar -zcpf \${path} homegear* lib* python3-homegear* distribution
	if test -f \${path}; then
		mv \${path} \${path}.uploading
		filename=\$(basename \$path)
		scp -P $HOMEGEARBUILD_SERVERPORT \${path}.uploading ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME}:${HOMEGEARBUILD_REPOSITORYSERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $HOMEGEARBUILD_SERVERPORT ${HOMEGEARBUILD_SERVERUSER}@${HOMEGEARBUILD_SERVERNAME} \"mv ${HOMEGEARBUILD_REPOSITORYSERVERPATH}/\${filename}.uploading ${HOMEGEARBUILD_REPOSITORYSERVERPATH}/\${filename}\"
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
