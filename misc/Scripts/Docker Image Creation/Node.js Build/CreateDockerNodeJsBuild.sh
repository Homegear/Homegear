#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools

set -x

print_usage() {
	echo "Usage: CreateDockerNodeJsBuild.sh DIST DISTVER ARCH"
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
	echo "Please provide a valid Linux distribution version (buster, bionic, ...)."	
	print_usage
	exit 1
fi

if test -z $3; then
	echo "Please provide a valid CPU architecture."	
	print_usage
	exit 1
fi

scriptdir="$( cd "$(dirname $0)" && pwd )"
if test ! -d "${scriptdir}/debian"; then
	echo "Directory \"debian\" does not exist in the directory of this script file."
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
	echo "deb $repository $distver main restricted universe multiverse" > $rootfs/etc/apt/sources.list
elif [ "$dist" == "Debian" ]; then
	echo "deb http://ftp.debian.org/debian $distver main contrib" > $rootfs/etc/apt/sources.list
elif [ "$dist" == "Raspbian" ]; then
	echo "deb http://archive.raspbian.org/raspbian/ $distver main contrib" > $rootfs/etc/apt/sources.list
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

#Fix debootstrap base package errors
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install

if [ "$distver" == "stretch" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y --allow-unauthenticated install debian-keyring debian-archive-keyring
fi

if [ "$distver" == "focal" ] || [ "$distver" == "bionic" ] || [ "$distver" == "buster" ]; then
	if [ "$arch" == "arm64" ]; then # Workaround for "syscall 277 error" in man-db
		export MAN_DISABLE_SECCOMP=1
	fi
fi

if [ "$distver" == "focal" ] || [ "$distver" == "bionic" ]; then
	chroot $rootfs apt-get update
	chroot $rootfs apt-get -y install gnupg
fi

chroot $rootfs apt-get update
if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "vivid" ] || [ "$distver" == "wily" ] || [ "$distver" == "xenial" ] || [ "$distver" == "bionic" ] || [ "$distver" == "focal" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install python3
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
fi
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install ca-certificates binutils debhelper devscripts ssh equivs nano python python3-distutils wget

if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ];  then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install dirmngr
fi

if [ "$distver" == "focal" ]; then
	#When using the default "fakeroot-sysv" PHP package creation fails
	chroot $rootfs update-alternatives --install /usr/bin/fakeroot fakeroot /usr/bin/fakeroot-tcp 100
fi


# Create links necessary to build PHP
mkdir -p $rootfs/build
cp -R "$scriptdir/debian" $rootfs/build || exit 1
cat > "$rootfs/build/CreatePackage.sh" <<-'EOF'
#!/bin/bash

set -x

if [[ -n $1 ]]; then
        echo "Please provide a version number."
        exit 1
fi

if [[ $2 -lt 1 ]]; then
        echo "Please provide a revision number."
        exit 1
fi

distribution="<DISTVER>"
version=${1}
revision=${2}

cd /build
wget https://github.com/nodejs/node/archive/v${version}.tar.gz || exit 1
tar -zxf v${version}.tar.gz || exit 1
cd node*
cp -R /build/debian .
date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
echo "nodejs-homegear (${version}-${revision}) ${distribution}; urgency=medium

  * Rebuild for Homegear.

 -- Dr. Sathya Laufer <s.laufer@homegear.email>  $date
" | cat - debian/changelog > debian/changelog2
mv debian/changelog2 debian/changelog
cd ..
mv node* nodejs-homegear-${version}
tar -zcpf nodejs-homegear_${version}.orig.tar.gz nodejs-homegear-${version}
cd nodejs-homegear-*
debuild --no-lintian -us -uc
cd /
rm -Rf /build/nodejs-homegear-${version}
if test -f /build/Upload.sh; then
	/build/Upload.sh
fi
EOF
chmod 755 $rootfs/build/CreatePackage.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/build/CreatePackage.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [[ -z "$SERVERNAME" || -z "$SERVERPORT" || -z "$SERVERUSER" || -z "$SERVERPATH" || -z "$SERVERCERT" ]]; then
	echo "Container setup successful. You can now execute \"/PHPBuild/CreatePHPPackages.sh\"."
	/bin/bash
	exit 0
else
	SERVERPATH=${SERVERPATH%/}
	echo "Testing connection..."
	mkdir -p /root/.ssh
	echo "$SERVERCERT" > /root/.ssh/id_rsa
	chmod 400 /root/.ssh/id_rsa
	sed -i -- 's/\\n/\n/g' /root/.ssh/id_rsa
	if [ -n "$SERVER_KNOWNHOST_ENTRY" ]; then
		echo "$SERVER_KNOWNHOST_ENTRY" > /root/.ssh/known_hosts
		sed -i -- 's/\\n/\n/g' /root/.ssh/known_hosts
	fi
	ssh -p $SERVERPORT ${SERVERUSER}@${SERVERNAME} "echo \"It works :-)\""
fi

echo "#!/bin/bash

set -x

cd /build
if [ \$(ls /build | grep -c \"\\.changes\$\") -ne 0 ]; then
	path=\`mktemp -p / -u\`".tar.gz"
	echo \"<DIST>\" > distribution
	tar -zcpf \${path} node* distribution
	if test -f \${path}; then
		mv \${path} \${path}.uploading
		filename=\$(basename \$path)
		scp -P $SERVERPORT \${path}.uploading ${SERVERUSER}@${SERVERNAME}:${SERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $SERVERPORT ${SERVERUSER}@${SERVERNAME} \"mv ${SERVERPATH}/\${filename}.uploading ${SERVERPATH}/\${filename}\"
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
if [ -n "$VERSION" ] && [ -n "$REVISION" ]; then
	/build/CreatePackage.sh $VERSION $REVISION
else
	echo "Container setup successful. You can now execute \"/build/CreatePackage.sh\"."
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

docker build -t homegear/nodejsbuild:${distlc}-${distver}-$arch "$dir"

rm -Rf $dir
