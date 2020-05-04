#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools

set -x

print_usage() {
	echo "Usage: CreateDockerPHPBuild.sh DIST DISTVER ARCH"
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
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install ca-certificates binutils debhelper devscripts ssh equivs nano git

if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ];  then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install default-libmysqlclient-dev dirmngr
else
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libmysqlclient-dev
fi

if [ "$distver" == "stretch" ] || [ "$distver" == "buster" ] || [ "$distver" == "jessie" ] || [ "$distver" == "wheezy" ] || [ "$distver" == "xenial" ] || [ "$distver" == "bionic" ] || [ "$distver" == "focal" ]; then
	DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y install libcurl4-gnutls-dev
fi

if [ "$distver" == "focal" ] || [ "$distver" == "bionic" ] || [ "$distver" == "buster" ]; then
	echo "deb-src http://ppa.launchpad.net/ondrej/php/ubuntu bionic main" > $rootfs/etc/apt/sources.list.d/php7-src.list
elif [ "$distver" == "trusty" ]; then
	echo "deb-src http://ppa.launchpad.net/ondrej/php/ubuntu trusty main" > $rootfs/etc/apt/sources.list.d/php7-src.list
else
	echo "deb-src http://ppa.launchpad.net/ondrej/php/ubuntu xenial main" > $rootfs/etc/apt/sources.list.d/php7-src.list
fi

cat > "$rootfs/php-gpg.key" <<'EOF'
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v1

mI0ESX35nAEEALKDCUDVXvmW9n+T/+3G1DnTpoWh9/1xNaz/RrUH6fQKhHr568F8
hfnZP/2CGYVYkW9hxP9LVW9IDvzcmnhgIwK+ddeaPZqh3T/FM4OTA7Q78HSvR81m
Jpf2iMLm/Zvh89ZsmP2sIgZuARiaHo8lxoTSLtmKXsM3FsJVlusyewHfABEBAAG0
H0xhdW5jaHBhZCBQUEEgZm9yIE9uZMWZZWogU3Vyw72ItgQTAQIAIAUCSX35nAIb
AwYLCQgHAwIEFQIIAwQWAgMBAh4BAheAAAoJEE9OoKrlJnpsQjYD/jW1NlIFAlT6
EvF2xfVbkhERii9MapjaUsSso4XLCEmZdEGX54GQ01svXnrivwnd/kmhKvyxCqiN
LDY/dOaK8MK//bDI6mqdKmG8XbP2vsdsxhifNC+GH/OwaDPvn1TyYB653kwyruCG
FjEnCreZTcRUu2oBQyolORDl+BmF4DjL
=R5tk
-----END PGP PUBLIC KEY BLOCK-----
EOF
chroot $rootfs apt-key add /php-gpg.key
rm $rootfs/php-gpg.key
chroot $rootfs apt-get update

mkdir $rootfs/PHPBuild
chroot $rootfs bash -c "cd /PHPBuild && apt-get source php7.4"
cd $rootfs/PHPBuild
tar -xf php7*debian.tar.xz
cd ..
sed -i '/.*libcurl4-openssl-dev | libcurl-dev,.*/d' $rootfs/PHPBuild/debian/control
sed -i 's/libtidy-dev.*,/libtidy-dev,/g' $rootfs/PHPBuild/debian/control
sed -i 's/libzip-dev.*,/libzip-dev,/g' $rootfs/PHPBuild/debian/control
sed -i '/.*libsystemd-daemon-dev,.*/d' $rootfs/PHPBuild/debian/control
sed -i '/.*libxml2-dev/a\\t       libicu-dev,' $rootfs/PHPBuild/debian/control
if [ "$distver" == "wheezy" ]; then
	sed -i 's/apache2-dev.*,//g' $rootfs/PHPBuild/debian/control
	sed -i '/.*dh-apache2,.*/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*dh-systemd.*,.*/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*libc-client-dev,.*/d' $rootfs/PHPBuild/debian/control
	sed -i 's/libgd-dev.*,/libgd2-xpm-dev,/g' $rootfs/PHPBuild/debian/control
else
	sed -i 's/libpcre2-dev .*,/libpcre2-dev,/g' $rootfs/PHPBuild/debian/control
	sed -i '/.*libgcrypt11-dev,.*/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*libgcrypt20-dev .*,/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*libxml2-dev/a\\t       libgcrypt20-dev,' $rootfs/PHPBuild/debian/control
fi
chroot $rootfs bash -c "cd /PHPBuild && mk-build-deps debian/control"
DEBIAN_FRONTEND=noninteractive chroot $rootfs bash -c "cd /PHPBuild && dpkg -i php*-build-deps_*.deb"
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
# On some architechtures install hangs and a process needs to be manually killed. So repeat the installation.
DEBIAN_FRONTEND=noninteractive chroot $rootfs apt-get -y -f install
# For some reason php*-build-deps... gets removed. Install it again.
chroot $rootfs bash -c "cd /PHPBuild && dpkg -i php*-build-deps_*.deb"

# Create links necessary to build PHP
rm -Rf $rootfs/PHPBuild/*
cp -R "$scriptdir/debian" $rootfs/PHPBuild || exit 1
if [ "$distver" == "trusty" ]; then
	sed -i 's/, libcurl4-gnutls-dev//g' $rootfs/PHPBuild/debian/control
	sed -i 's/--with-curl //g' $rootfs/PHPBuild/debian/rules
fi
if [ "$arch" == "armel" ]; then
	sed -i 's/--with-mysqli=mysqlnd //g' $rootfs/PHPBuild/debian/rules
fi
cat > "$rootfs/PHPBuild/SetTarget.sh" <<-'EOF'
#!/bin/bash
target="$(gcc -v 2>&1)"
strpos="${target%%--target=*}"
strpos=${#strpos}
target=${target:strpos}
target=$(echo $target | cut -d "=" -f 2 | cut -d " " -f 1)
sed -i "s/%target%/$target/g" debian/rules
EOF
chmod 755 $rootfs/PHPBuild/SetTarget.sh
cat > "$rootfs/PHPBuild/CreatePHPPackages.sh" <<-'EOF'
#!/bin/bash

set -x

if [[ $1 -lt 1 ]]; then
        echo "Please provide a revision number."
        exit 1
fi

distribution="<DISTVER>"

rm -Rf /PHPBuild/php*
rm -Rf /PHPBuild/lib*

cd /PHPBuild
apt-get update
apt-get source php7.4
rm php7*.tar.xz
rm php7*.dsc
cd php7*
cd ext
if test ! -f ext_skel.in; then
	touch ext_skel.in
fi
git clone https://github.com/krakjoe/parallel.git parallel
cd parallel
git checkout release
cd ..
# Add Homegear to allowed OPcode cache modules
sed -i 's/strcmp(sapi_module.name, "cli") == 0/strcmp(sapi_module.name, "homegear") == 0/g' opcache/ZendAccelerator.c
sed -i '/.*case ZEND_HANDLE_STREAM:.*/a\\t\t\tif (strcmp(sapi_module.name, "homegear") == 0) { if (zend_get_stream_timestamp(file_handle->filename, &statbuf) != SUCCESS) return 0; else break; }' opcache/ZendAccelerator.c
cd ..
autoconf
version=`head -n 1 debian/changelog | cut -d "(" -f 2 | cut -d ")" -f 1 | cut -d "+" -f 1 | cut -d "-" -f 1`
revision=`head -n 1 debian/changelog | cut -d "(" -f 2 | cut -d ")" -f 1 | cut -d "+" -f 1 | cut -d "-" -f 2 | cut -d "~" -f 1`
rm -Rf debian
cp -R /PHPBuild/debian .
/PHPBuild/SetTarget.sh
date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
echo "php7-homegear-dev (${version}-${revision}~${1}) ${distribution}; urgency=medium

  * Rebuild for Homegear.

 -- Sathya Laufer <sathya@laufers.net>  $date
" | cat - debian/changelog > debian/changelog2
mv debian/changelog2 debian/changelog
cd ..
mv php7* php7-homegear-dev-${version}
tar -zcpf php7-homegear-dev_${version}.orig.tar.gz php7-homegear-dev-${version}
cd php7-homegear-dev-*
debuild -us -uc
cd /
rm -Rf /PHPBuild/php7-homegear-dev-${version}
if test -f /PHPBuild/Upload.sh; then
	/PHPBuild/Upload.sh
fi
EOF
chmod 755 $rootfs/PHPBuild/CreatePHPPackages.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/PHPBuild/CreatePHPPackages.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [[ -z "$PHPBUILD_SERVERNAME" || -z "$PHPBUILD_SERVERPORT" || -z "$PHPBUILD_SERVERUSER" || -z "$PHPBUILD_SERVERPATH" || -z "$PHPBUILD_SERVERCERT" ]]; then
	echo "Container setup successful. You can now execute \"/PHPBuild/CreatePHPPackages.sh\"."
	/bin/bash
	exit 0
else
	PHPBUILD_SERVERPATH=${PHPBUILD_SERVERPATH%/}
	echo "Testing connection..."
	mkdir -p /root/.ssh
	echo "$PHPBUILD_SERVERCERT" > /root/.ssh/id_rsa
	chmod 400 /root/.ssh/id_rsa
	sed -i -- 's/\\n/\n/g' /root/.ssh/id_rsa
	if [ -n "$PHPBUILD_SERVER_KNOWNHOST_ENTRY" ]; then
		echo "$PHPBUILD_SERVER_KNOWNHOST_ENTRY" > /root/.ssh/known_hosts
		sed -i -- 's/\\n/\n/g' /root/.ssh/known_hosts
	fi
	ssh -p $PHPBUILD_SERVERPORT ${PHPBUILD_SERVERUSER}@${PHPBUILD_SERVERNAME} "echo \"It works :-)\""
fi

echo "#!/bin/bash

set -x

cd /PHPBuild
if [ \$(ls /PHPBuild | grep -c \"\\.changes\$\") -ne 0 ]; then
	path=\`mktemp -p / -u\`".tar.gz"
	echo \"<DIST>\" > distribution
	tar -zcpf \${path} php* distribution
	if test -f \${path}; then
		mv \${path} \${path}.uploading
		filename=\$(basename \$path)
		scp -P $PHPBUILD_SERVERPORT \${path}.uploading ${PHPBUILD_SERVERUSER}@${PHPBUILD_SERVERNAME}:${PHPBUILD_SERVERPATH}
		if [ \$? -eq 0 ]; then
			ssh -p $PHPBUILD_SERVERPORT ${PHPBUILD_SERVERUSER}@${PHPBUILD_SERVERNAME} \"mv ${PHPBUILD_SERVERPATH}/\${filename}.uploading ${PHPBUILD_SERVERPATH}/\${filename}\"
			if [ \$? -eq 0 ]; then
				echo "Packages uploaded successfully."
				exit 0
			else
				exit \$?
			fi
		fi
	fi
fi
" > /PHPBuild/Upload.sh
chmod 755 /PHPBuild/Upload.sh
rm /FirstStart.sh
echo
if [ -n "$PHPBUILD_REVISION" ]; then
	/PHPBuild/CreatePHPPackages.sh $PHPBUILD_REVISION
else
	echo "Container setup successful. You can now execute \"/PHPBuild/CreatePHPPackages.sh\"."
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

docker build -t homegear/phpbuild:${distlc}-${distver}-$arch "$dir"

rm -Rf $dir
