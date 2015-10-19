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
	echo "Please provide a valid Linux distribution version (wheezy, trusty, ...)."	
	print_usage
	exit 1
fi

if test -z $3; then
	echo "Please provide a valid CPU architecture."	
	print_usage
	exit 1
fi

scriptdir="$( cd "$(dirname $0)" && pwd )"
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

echo "deb-src http://packages.dotdeb.org jessie all
	" > $rootfs/etc/apt/sources.list.d/php7-src.list

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

wget -P $rootfs http://www.dotdeb.org/dotdeb.gpg
chroot $rootfs apt-key add dotdeb.gpg
rm $rootfs/dotdeb.gpg
chroot $rootfs apt-get update
#Fix debootstrap base package errors
chroot $rootfs apt-get -y -f install
chroot $rootfs apt-get -y install ca-certificates binutils debhelper devscripts ssh equivs nano

mkdir $rootfs/PHPBuild
chroot $rootfs bash -c "cd /PHPBuild && apt-get source php7.0"
cd $rootfs/PHPBuild
tar -xf php7*debian.tar.xz
cd ..
sed -i '/.*libcurl4-openssl-dev | libcurl-dev,.*/d' $rootfs/PHPBuild/debian/control
if [ "$distver" == "wheezy" ]; then
	sed -i 's/apache2-dev.*,//g' $rootfs/PHPBuild/debian/control
	sed -i '/.*dh-apache2,.*/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*dh-systemd.*,.*/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*libc-client-dev,.*/d' $rootfs/PHPBuild/debian/control
else
	sed -i '/.*libgcrypt11-dev,.*/d' $rootfs/PHPBuild/debian/control
	sed -i '/.*libxml2-dev/a\\t       libgcrypt20-dev,' $rootfs/PHPBuild/debian/control
fi
chroot $rootfs bash -c "cd /PHPBuild && mk-build-deps debian/control"
chroot $rootfs bash -c "cd /PHPBuild && dpkg -i php*-build-deps_*.deb"
chroot $rootfs apt-get -y -f install
# On some architechtures install hangs and a process needs to be manually killed. So repeat the installation.
chroot $rootfs apt-get -y -f install
# For some reason php*-build-deps... gets removed. Install it again.
chroot $rootfs bash -c "cd /PHPBuild && dpkg -i php*-build-deps_*.deb"

# Create links necessary to build PHP
rm -Rf $rootfs/PHPBuild/*
cp -R "$scriptdir/debian" $rootfs/PHPBuild
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

target="$($rootfs gcc -v 2>&1)"
strpos="${target%%--target=*}"
strpos=${#strpos}
target=${target:strpos}
target=$(echo $target | cut -d "=" -f 2 | cut -d " " -f 1)
ln -s /usr/include/qdbm /usr/include/qdbm/include
ln -s /usr/include/$target/ /usr/include/$target/include

cd /PHPBuild
apt-get update
apt-get source php7.0
tar -xf php7*.orig.tar.gz
rm php7*.tar.xz
rm php7*.dsc
cd php7*
cd ext
wget https://github.com/krakjoe/pthreads/archive/v3.0.8.tar.gz
tar -zxf v3.0.8.tar.gz
rm v3.0.8.tar.gz
mv pthreads-3.0.8 pthreads
sed -i 's/{ZEND_STRL("cli")}/{ZEND_STRL("homegear")}/g' pthreads/php_pthreads.c
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
sed -i 's/enableval=$enable_embed;/enableval=static;/g' configure
cd ..
mv php7* php7-homegear-dev-${version}
tar -zcpf php7-homegear-dev_${version}.orig.tar.gz php7-homegear-dev-${version}
cd php7*
debuild -us -uc
if test -f /PHPBuild/Upload.sh; then
	/PHPBuild/Upload.sh
fi
EOF
chmod 755 $rootfs/PHPBuild/CreatePHPPackages.sh
sed -i "s/<DISTVER>/${distver}/g" $rootfs/PHPBuild/CreatePHPPackages.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
if [ -n "$PHPBUILD_SHELL" ]; then
	echo "Container setup successful. You can now execute \"/PHPBuild/CreatePHPPackages.sh\"."
	/bin/bash
	exit 0
fi
if [[ -z "$PHPBUILD_SERVERNAME" || -z "$PHPBUILD_SERVERPORT" || -z "$PHPBUILD_SERVERUSER" || -z "$PHPBUILD_SERVERPATH" || -z "$PHPBUILD_SERVERCERT" ]]; then
	while :
	do
		echo "Setting up SSH package uploading:"
		while :
		do
			read -p "Please specify the server name to upload to: " PHPBUILD_SERVERNAME
			if [ -n "$PHPBUILD_SERVERNAME" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the SSH port number of the server: " PHPBUILD_SERVERPORT
			if [ -n "$PHPBUILD_SERVERPORT" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the user name to use to login into the server: " PHPBUILD_SERVERUSER
			if [ -n "$PHPBUILD_SERVERUSER" ]; then
				break
			fi
		done
		while :
		do
			read -p "Please specify the path on the server to upload packages to: " PHPBUILD_SERVERPATH
			PHPBUILD_SERVERPATH=${PHPBUILD_SERVERPATH%/}
			if [ -n "$PHPBUILD_SERVERPATH" ]; then
				break
			fi
		done
		echo "Paste your certificate:"
		IFS= read -d '' -n 1 PHPBUILD_SERVERCERT
		while IFS= read -d '' -n 1 -t 2 c
		do
		    PHPBUILD_SERVERCERT+=$c
		done
		echo
		echo
		echo "Testing connection..."
		mkdir -p /root/.ssh
		echo "$PHPBUILD_SERVERCERT" > /root/.ssh/id_rsa
		chmod 400 /root/.ssh/id_rsa
		ssh -p $PHPBUILD_SERVERPORT ${PHPBUILD_SERVERUSER}@${PHPBUILD_SERVERNAME} "echo \"It works :-)\""
		echo
		echo -e "Server name:\t$PHPBUILD_SERVERNAME"
		echo -e "Server port:\t$PHPBUILD_SERVERPORT"
		echo -e "Server user:\t$PHPBUILD_SERVERUSER"
		echo -e "Server path:\t$PHPBUILD_SERVERPATH"
		echo -e "Certificate:\t"
		echo "$PHPBUILD_SERVERCERT"
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
	tar -zcpf \${path} php* lib* distribution
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

docker build -t homegear/phpbuild:${distlc}-${distver}-$arch "$dir"

rm -Rf $dir
