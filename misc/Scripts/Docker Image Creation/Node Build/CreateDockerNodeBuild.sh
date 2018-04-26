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

if [ "$distver" == "bionic" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y install gnupg
fi

chroot $rootfs apt-get update
if [ "$distver" == "stretch" ] || [ "$distver" == "vivid" ] || [ "$distver" == "wily" ] || [ "$distver" == "xenial" ] || [ "$distver" == "bionic" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
fi
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install apt-transport-https

echo "deb https://homegear.eu/packages/$dist/ $distver/
" > $rootfs/etc/apt/sources.list.d/homegear.list

chroot $rootfs mount proc /proc -t proc
wget -P $rootfs https://homegear.eu/packages/Release.key
chroot $rootfs apt-key add Release.key
rm $rootfs/Release.key
chroot $rootfs umount /proc

chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install ssh unzip ca-certificates binutils debhelper devscripts automake autoconf libtool sqlite3 insserv libsqlite3-dev libncurses5-dev libssl-dev libparse-debcontrol-perl libgpg-error-dev php7-homegear-dev libxslt1-dev libedit-dev libenchant-dev libqdbm-dev libcrypto++-dev libltdl-dev zlib1g-dev libtinfo-dev libgmp-dev libxml2-dev libzip-dev p7zip-full ntp libavahi-common-dev libavahi-client-dev

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
	if [ "$distver" == "stretch" ] || [ "$distver" == "wheezy" ] || [ "$distver" == "jessie" ] || [ "$distver" == "xenial" ] || [ "$distver" == "bionic" ]; then
		DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libcurl4-gnutls-dev
	fi
fi
# }}}

# {{{ Readline
if [ "$distver" == "stretch" ] || [ "$distver" == "bionic" ]; then
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
	if [ "$distributionVersion" != "stretch" ] && [ "$distributionVersion" != "jessie" ] && [ "$distributionVersion" != "wheezy" ] && [ "$distributionVersion" != "xenial" ] && [ "$distributionVersion" != "bionic" ]; then
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
		dscfile=$(ls ${3}_*.dsc)
		sed -i "/-dbgsym_/d" ${3}_*.changes
		sed -i "/${dscfile}/d" ${3}_*.changes
		sed -i "/Checksums-Sha1:/a\ $(sha1sum ${3}_*.dsc | cut -d ' ' -f 1) $(stat --printf='%s' ${3}_*.dsc) ${dscfile}" ${3}_*.changes
		sed -i "/Checksums-Sha256:/a\ $(sha256sum ${3}_*.dsc | cut -d ' ' -f 1) $(stat --printf='%s' ${3}_*.dsc) ${dscfile}" ${3}_*.changes
		sed -i "/Files:/a\ $(md5sum ${3}_*.dsc | cut -d ' ' -f 1) $(stat --printf='%s' ${3}_*.dsc) misc optional ${dscfile}" ${3}_*.changes
	fi
	rm -Rf $sourcePath
}

cd /build

wget --https-only https://github.com/Homegear/homegear-nodes-optional/archive/${1}.zip
[ $? -ne 0 ] && exit 1
unzip ${1}.zip
[ $? -ne 0 ] && exit 1
rm ${1}.zip

cd homegear-nodes-optional*
./buildPackages <DISTVER> <ARCH>
if [ "<ARCH>" == "amd64" ]; then
	./buildPackages <DISTVER> all
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackage.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackage.sh
sed -i "s/<ARCH>/${arch}/g" $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/build/CreateDebianPackageNightly.sh" <<-'EOF'
#!/bin/bash

TEMPDIR=$(mktemp -d)
cd $TEMPDIR
wget https://homegear.eu/downloads/nightlies/libhomegear-node_current_<DIST>_<DISTVER>_<ARCH>.deb || exit 1
dpkg -i libhomegear-node_current_<DIST>_<DISTVER>_<ARCH>.deb
if [ $? -ne 0 ]; then
	apt-get -y -f install || exit 1
	dpkg -i libhomegear-node_current_<DIST>_<DISTVER>_<ARCH>.deb || exit 1
fi

/build/CreateDebianPackage.sh dev $1

if test -f node-blue-*.deb; then
	if test -f /build/UploadNightly.sh; then
		/build/UploadNightly.sh
	fi
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<DIST>/${dist}/g" $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreateDebianPackageNightly.sh
sed -i "s/<ARCH>/${arch}/g" $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/build/CreateDebianPackageStable.sh" <<-'EOF'
#!/bin/bash

chroot $rootfs apt-get update
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libhomegear-node

/build/CreateDebianPackage.sh master $1

if test -f node-blue-*.deb; then
	if test -f /build/UploadStable.sh; then
		/build/UploadStable.sh
	fi
fi
EOF
chmod 755 $rootfs/build/CreateDebianPackageStable.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [ -n "$NODEBUILD_THREADS" ]; then
	sed -i "s/<BUILDTHREADS>/${NODEBUILD_THREADS}/g" /build/CreateDebianPackage.sh
else
	sed -i "s/<BUILDTHREADS>/1/g" /build/CreateDebianPackage.sh
fi
if [ -n "$NODEBUILD_SHELL" ]; then
	echo "Container setup successful. You can now execute \"/build/CreateDebianPackageStable.sh\" or \"/build/CreateDebianPackageNightly.sh\"."
	/bin/bash
	exit 0
fi
if [[ -z "$NODEBUILD_SERVERNAME" || -z "$NODEBUILD_SERVERPORT" || -z "$NODEBUILD_SERVERUSER" || -z "$NODEBUILD_STABLESERVERPATH" || -z "$NODEBUILD_NIGHTLYSERVERPATH" || -z "$NODEBUILD_SERVERCERT" ]]; then
	while :
	do
		echo "Setting up SSH package uploading:"
		while :
		do
			read -p "Please specify the server name to upload to: " NODEBUILD_SERVERNAME
			if [ -n "$NODEBUILD_SERVERNAME" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the SSH port number of the server: " NODEBUILD_SERVERPORT
			if [ -n "$NODEBUILD_SERVERPORT" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the user name to use to login into the server: " NODEBUILD_SERVERUSER
			if [ -n "$NODEBUILD_SERVERUSER" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the path on the server to upload stable packages to: " NODEBUILD_STABLESERVERPATH
			NODEBUILD_STABLESERVERPATH=${NODEBUILD_STABLESERVERPATH%/}
			if [ -n "$NODEBUILD_STABLESERVERPATH" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the path on the server to upload nightly packages to: " NODEBUILD_NIGHTLYSERVERPATH
			NODEBUILD_NIGHTLYSERVERPATH=${NODEBUILD_NIGHTLYSERVERPATH%/}
			if [ -n "$NODEBUILD_NIGHTLYSERVERPATH" ]; then
				break
			fi
		done
		echo "Paste your certificate:"
		IFS= read -d '' -n 1 NODEBUILD_SERVERCERT
		while IFS= read -d '' -n 1 -t 2 c
		do
		    NODEBUILD_SERVERCERT+=$c
		done
		echo
		echo
		echo "Testing connection..."
		mkdir -p /root/.ssh
		echo "$NODEBUILD_SERVERCERT" > /root/.ssh/id_rsa
		chmod 400 /root/.ssh/id_rsa
		ssh -p $NODEBUILD_SERVERPORT ${NODEBUILD_SERVERUSER}@${NODEBUILD_SERVERNAME} "echo \"It works :-)\""
		echo
		echo -e "Server name:\t$NODEBUILD_SERVERNAME"
		echo -e "Server port:\t$NODEBUILD_SERVERPORT"
		echo -e "Server user:\t$NODEBUILD_SERVERUSER"
		echo -e "Server path (stable):\t$NODEBUILD_STABLESERVERPATH"
		echo -e "Server path (nightlies):\t$NODEBUILD_NIGHTLYSERVERPATH"
		echo -e "Certificate:\t"
		echo "$NODEBUILD_SERVERCERT"
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
	NODEBUILD_STABLESERVERPATH=${NODEBUILD_STABLESERVERPATH%/}
	NODEBUILD_NIGHTLYSERVERPATH=${NODEBUILD_NIGHTLYSERVERPATH%/}
	echo "Testing connection..."
	mkdir -p /root/.ssh
	echo "$NODEBUILD_SERVERCERT" > /root/.ssh/id_rsa
	chmod 400 /root/.ssh/id_rsa
	sed -i -- 's/\\n/\n/g' /root/.ssh/id_rsa
	if [ -n "$NODEBUILD_SERVER_KNOWNHOST_ENTRY" ]; then
		echo "$NODEBUILD_SERVER_KNOWNHOST_ENTRY" > /root/.ssh/known_hosts
		sed -i -- 's/\\n/\n/g' /root/.ssh/known_hosts
	fi
	ssh -p $NODEBUILD_SERVERPORT ${NODEBUILD_SERVERUSER}@${NODEBUILD_SERVERNAME} "echo \"It works :-)\""
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
		scp -P $NODEBUILD_SERVERPORT \${path}.uploading ${NODEBUILD_SERVERUSER}@${NODEBUILD_SERVERNAME}:${NODEBUILD_STABLESERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $NODEBUILD_SERVERPORT ${NODEBUILD_SERVERUSER}@${NODEBUILD_SERVERNAME} \"mv ${NODEBUILD_STABLESERVERPATH}/\${filename}.uploading ${NODEBUILD_STABLESERVERPATH}/\${filename}\"
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
		scp -P $NODEBUILD_SERVERPORT \${path}.uploading ${NODEBUILD_SERVERUSER}@${NODEBUILD_SERVERNAME}:${NODEBUILD_NIGHTLYSERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $NODEBUILD_SERVERPORT ${NODEBUILD_SERVERUSER}@${NODEBUILD_SERVERNAME} \"mv ${NODEBUILD_NIGHTLYSERVERPATH}/\${filename}.uploading ${NODEBUILD_NIGHTLYSERVERPATH}/\${filename}\"
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

if [ "$NODEBUILD_TYPE" = "stable" ]; then
	/build/CreateDebianPackageStable.sh ${NODEBUILD_API_KEY}
elif [ "$NODEBUILD_TYPE" = "nightly" ]; then
	/build/CreateDebianPackageNightly.sh ${NODEBUILD_API_KEY}
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

docker build -t homegear/nodebuild:${distlc}-${distver}-${arch} "$dir"
if [ "$dist" == "Raspbian" ]; then
	docker tag -f homegear/nodebuild:${distlc}-${distver}-${arch} homegear/nodebuild:${distlc}-${distver}
fi

rm -Rf $dir
