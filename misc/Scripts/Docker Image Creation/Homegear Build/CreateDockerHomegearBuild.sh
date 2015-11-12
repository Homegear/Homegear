#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools

set -x

print_usage() {
	echo "Usage: CreateDockerHomegearBuild.sh DIST DISTVER ARCH"
	echo "  DIST:           The Linux distribution (debian, raspbian or ubuntu)"
	echo "  DISTVER:		The version of the distribution (e. g. for Ubuntu: trusty, utopic or vivid)"
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

echo "deb http://homegear.eu/packages/$dist/ $distver/
" > $rootfs/etc/apt/sources.list.d/homegear.list

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

wget -P $rootfs http://homegear.eu/packages/Release.key
chroot $rootfs apt-key add Release.key
rm $rootfs/Release.key

chroot $rootfs apt-get update
if [ "$distver" == "vivid" ] || [ "$distver" == "wily" ]; then
	chroot $rootfs apt-get -y install python3
	chroot $rootfs apt-get -y -f install
fi
chroot $rootfs apt-get -y install ssh unzip ca-certificates binutils debhelper devscripts automake autoconf libtool sqlite3 libsqlite3-dev libreadline6 libreadline6-dev libncurses5-dev libssl-dev libparse-debcontrol-perl libgpg-error-dev php7-homegear-dev libxslt1-dev libedit-dev libmcrypt-dev libenchant-dev libqdbm-dev libcrypto++-dev libltdl-dev zlib1g-dev libtinfo-dev libgmp-dev libxml2-dev

if [ "$distver" == "wheezy" ]; then
	chroot $rootfs apt-get -y install libgcrypt11-dev libgnutls-dev g++-4.7 gcc-4.7
	rm $rootfs/usr/bin/g++
	rm $rootfs/usr/bin/gcc
	ln -s g++-4.7 $rootfs/usr/bin/g++
	ln -s gcc-4.7 $rootfs/usr/bin/gcc
else
	chroot $rootfs apt-get -y install libgcrypt20-dev libgnutls28-dev
fi

mkdir $rootfs/build
cat > "$rootfs/build/CreateDebianPackageNightly.sh" <<-'EOF'
#!/bin/bash

distribution="<DISTVER>"

cd /build

wget https://github.com/Homegear/libhomegear-base/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear-HomeMaticBidCoS/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear-HomeMaticWired/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear-Insteon/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear-MAX/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear-PhilipsHue/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip
wget https://github.com/Homegear/Homegear-Sonos/archive/master.zip
[ $? -ne 0 ] && exit 1
unzip master.zip
[ $? -ne 0 ] && exit 1
rm master.zip

# {{{ libhomegear-base
fullversion=$(libhomegear-base-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
if [ $revision -eq 0 ]; then
	echo "Error: Could not get revision from GitHub."
	exit 1
fi
sourcePath=libhomegear-base-$version
mv libhomegear-base-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf libhomegear-base_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm libhomegear-base_$version-$revision_*.build
rm libhomegear-base_$version-$revision_*.changes
rm libhomegear-base_$version-$revision.debian.tar.?z
rm libhomegear-base_$version-$revision.dsc
rm libhomegear-base_$version.orig.tar.gz
mv libhomegear-base*.deb libhomegear-base.deb
if test -f libhomegear-base.deb; then
	dpkg -i libhomegear-base.deb
else
	echo "Error building libhomegear-base."
	exit 1
fi
# }}}

# {{{ homegear
fullversion=$(Homegear-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-$version
mv Homegear-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg homegear-miscellaneous/m4 homegear-miscellaneous/cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
rm -f homegear-miscellaneous/config.status homegear-miscellaneous/config.log
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear_$version-$revision_*.build
rm homegear_$version-$revision_*.changes
rm homegear_$version-$revision.debian.tar.?z
rm homegear_$version-$revision.dsc
rm homegear_$version.orig.tar.gz
mv homegear_$version-$revision_*.deb homegear.deb
# }}}

# {{{ homegear-homematicbidcos
fullversion=$(Homegear-HomeMaticBidCoS-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-homematicbidcos-$version
mv Homegear-HomeMaticBidCoS-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear-homematicbidcos_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear-homematicbidcos_$version-$revision_*.build
rm homegear-homematicbidcos_$version-$revision_*.changes
rm homegear-homematicbidcos_$version-$revision.debian.tar.?z
rm homegear-homematicbidcos_$version-$revision.dsc
rm homegear-homematicbidcos_$version.orig.tar.gz
mv homegear-homematicbidcos*.deb homegear-homematicbidcos.deb
# }}}

# {{{ homegear-homematicwired
fullversion=$(Homegear-HomeMaticWired-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-homematicwired-$version
mv Homegear-HomeMaticWired-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear-homematicwired_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear-homematicwired_$version-$revision_*.build
rm homegear-homematicwired_$version-$revision_*.changes
rm homegear-homematicwired_$version-$revision.debian.tar.?z
rm homegear-homematicwired_$version-$revision.dsc
rm homegear-homematicwired_$version.orig.tar.gz
mv homegear-homematicwired*.deb homegear-homematicwired.deb
# }}}

# {{{ homegear-insteon
fullversion=$(Homegear-Insteon-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-insteon-$version
mv Homegear-Insteon-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear-insteon_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear-insteon_$version-$revision_*.build
rm homegear-insteon_$version-$revision_*.changes
rm homegear-insteon_$version-$revision.debian.tar.?z
rm homegear-insteon_$version-$revision.dsc
rm homegear-insteon_$version.orig.tar.gz
mv homegear-insteon*.deb homegear-insteon.deb
# }}}

# {{{ homegear-max
fullversion=$(Homegear-MAX-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-max-$version
mv Homegear-MAX-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear-max_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear-max_$version-$revision_*.build
rm homegear-max_$version-$revision_*.changes
rm homegear-max_$version-$revision.debian.tar.?z
rm homegear-max_$version-$revision.dsc
rm homegear-max_$version.orig.tar.gz
mv homegear-max*.deb homegear-max.deb
# }}}

# {{{ homegear-philipshue
fullversion=$(Homegear-PhilipsHue-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-philipshue-$version
mv Homegear-PhilipsHue-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear-philipshue_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear-philipshue_$version-$revision_*.build
rm homegear-philipshue_$version-$revision_*.changes
rm homegear-philipshue_$version-$revision.debian.tar.?z
rm homegear-philipshue_$version-$revision.dsc
rm homegear-philipshue_$version.orig.tar.gz
mv homegear-philipshue*.deb homegear-philipshue.deb
# }}}

# {{{ homegear-sonos
fullversion=$(Homegear-Sonos-master/getVersion.sh)
version=$(echo $fullversion | cut -d "-" -f 1)
revision=$(echo $fullversion | cut -d "-" -f 2)
sourcePath=homegear-sonos-$version
mv Homegear-Sonos-master $sourcePath
cd $sourcePath
rm -Rf .* m4 cfg 1>/dev/null 2>&2
libtoolize
aclocal
autoconf
automake --add-missing
make distclean
cd ..
sed -i "s/<BASELIBVER>/$version-$revision/g" $sourcePath/debian/control
if [ "$distribution" == "wheezy" ]; then
	sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
	sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
	sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
	sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
fi
tar -zcpf homegear-sonos_$version.orig.tar.gz $sourcePath
cd $sourcePath
./configure
dch -v $version-$revision -M "Version $version."
debuild -j4 -us -uc
cd ..
rm -Rf $sourcePath
rm homegear-sonos_$version-$revision_*.build
rm homegear-sonos_$version-$revision_*.changes
rm homegear-sonos_$version-$revision.debian.tar.?z
rm homegear-sonos_$version-$revision.dsc
rm homegear-sonos_$version.orig.tar.gz
mv homegear-sonos*.deb homegear-sonos.deb
# }}}

if test -f libhomegear-base.deb && test -f homegear.deb && test -f homegear-homematicbidcos.deb && test -f homegear-homematicwired.deb && test -f homegear-insteon.deb && test -f homegear-max.deb && test -f homegear-philipshue.deb && test -f homegear-sonos.deb; then
	isodate=`date +%Y%m%d`
EOF
echo "	mv libhomegear-base.deb libhomegear-base_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear.deb homegear_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-homematicbidcos.deb homegear-homematicbidcos_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-homematicwired.deb homegear-homematicwired_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-insteon.deb homegear-insteon_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-max.deb homegear-max_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-philipshue.deb homegear-philipshue_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-sonos.deb homegear-sonos_\$[isodate]_${distlc}_${distver}_${arch}.deb
	if test -f /build/Upload.sh; then
		/build/Upload.sh
	fi
fi" >> $rootfs/build/CreateDebianPackageNightly.sh
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [ -n "$HOMEGEARBUILD_SHELL" ]; then
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
	exit 0
fi
if [[ -z "$HOMEGEARBUILD_SERVERNAME" || -z "$HOMEGEARBUILD_SERVERPORT" || -z "$HOMEGEARBUILD_SERVERUSER" || -z "$HOMEGEARBUILD_SERVERPATH" || -z "$HOMEGEARBUILD_SERVERCERT" ]]; then
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
			read -p "Please specify the path on the server to upload packages to: " HOMEGEARBUILD_SERVERPATH
			HOMEGEARBUILD_SERVERPATH=${HOMEGEARBUILD_SERVERPATH%/}
			if [ -n "$HOMEGEARBUILD_SERVERPATH" ]; then
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
		echo -e "Server path:\t$HOMEGEARBUILD_SERVERPATH"
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
	HOMEGEARBUILD_SERVERPATH=${HOMEGEARBUILD_SERVERPATH%/}
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
if [ \$(ls /build | grep -c \"\\.deb\$\") -ne 0 ]; then
	path=\`mktemp -p / -u\`".tar.gz"
	tar -zcpf \${path} *.deb
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
" > /build/Upload.sh
chmod 755 /build/Upload.sh
rm /FirstStart.sh
echo
if [ -n "$HOMEGEARBUILD_REVISION" ]; then
	/build/CreateDebianPackageNightly.sh $HOMEGEARBUILD_REVISION
else
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
fi
EOF
chmod 755 $rootfs/FirstStart.sh

read -p "Copy additional files into ${rootfs} and check that all packages were installed ok then hit [Enter] to continue..."

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
	docker tab homegear/build:${distlc}-${distver}-${arch} homegear/build:${distlc}-${distver}
fi

rm -Rf $dir
