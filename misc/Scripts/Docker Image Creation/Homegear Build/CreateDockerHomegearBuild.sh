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

chroot $rootfs apt-get update
if [ "$distver" == "vivid" ] || [ "$distver" == "wily" ]; then
	chroot $rootfs apt-get -y install python3
	chroot $rootfs apt-get -y -f install
fi
chroot $rootfs apt-get -y install apt-transport-https

echo "deb https://homegear.eu/packages/$dist/ $distver/
" > $rootfs/etc/apt/sources.list.d/homegear.list

wget -P $rootfs https://homegear.eu/packages/Release.key
chroot $rootfs apt-key add Release.key
rm $rootfs/Release.key

chroot $rootfs apt-get update
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
	tar -zcpf ${3}_$version.orig.tar.gz $sourcePath
	cd $sourcePath
	dch -v $version-$revision -M "Version $version."
	debuild -j${buildthreads} -us -uc
	cd ..
	rm -Rf $sourcePath
}

cd /build

wget https://github.com/Homegear/libhomegear-base/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear-HomeMaticBidCoS/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear-HomeMaticWired/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear-Insteon/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear-MAX/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear-PhilipsHue/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip
wget https://github.com/Homegear/Homegear-Sonos/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

createPackage libhomegear-base $1 libhomegear-base
if test -f libhomegear-base*.deb; then
	dpkg -i libhomegear-base*.deb
else
	echo "Error building libhomegear-base."
	exit 1
fi

createPackage Homegear $1 homegear
createPackage Homegear-HomeMaticBidCoS $1 homegear-homematicbidcos
createPackage Homegear-HomeMaticWired $1 homegear-homematicwired
createPackage Homegear-Insteon $1 homegear-insteon
createPackage Homegear-MAX $1 homegear-max
createPackage Homegear-PhilipsHue $1 homegear-philipshue
createPackage Homegear-Sonos $1 homegear-sonos
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

/build/CreateDebianPackage.sh master

cd /build

cleanUp libhomegear-base
cleanUp homegear
cleanUp homegear-homematicbidcos
cleanUp homegear-homematicwired
cleanUp homegear-insteon
cleanUp homegear-max
cleanUp homegear-philipshue
cleanUp homegear-sonos

EOF
echo "if test -f libhomegear-base.deb && test -f homegear.deb && test -f homegear-homematicbidcos.deb && test -f homegear-homematicwired.deb && test -f homegear-insteon.deb && test -f homegear-max.deb && test -f homegear-philipshue.deb && test -f homegear-sonos.deb; then
	isodate=`date +%Y%m%d`
	mv libhomegear-base.deb libhomegear-base_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear.deb homegear_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-homematicbidcos.deb homegear-homematicbidcos_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-homematicwired.deb homegear-homematicwired_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-insteon.deb homegear-insteon_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-max.deb homegear-max_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-philipshue.deb homegear-philipshue_\$[isodate]_${distlc}_${distver}_${arch}.deb
	mv homegear-sonos.deb homegear-sonos_\$[isodate]_${distlc}_${distver}_${arch}.deb
	if test -f /build/UploadNightly.sh; then
		/build/UploadNightly.sh
	fi
fi" >> $rootfs/build/CreateDebianPackageNightly.sh
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/build/CreateDebianPackageStable.sh" <<-'EOF'
#!/bin/bash

/build/CreateDebianPackage.sh 0.6

if test -f /build/UploadStable.sh; then
	/build/UploadStable.sh
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
	/build/CreateDebianPackageStable.sh
elif [ "$HOMEGEARBUILD_TYPE" = "nightly" ]; then
	/build/CreateDebianPackageNightly.sh
else
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageStable.sh\" or \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
fi
EOF
chmod 755 $rootfs/FirstStart.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/FirstStart.sh

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
	docker tag homegear/build:${distlc}-${distver}-${arch} homegear/build:${distlc}-${distver}
fi

rm -Rf $dir
