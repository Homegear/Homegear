#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools

set -x

if [ $EUID -ne 0 ]; then
	echo "ERROR: This tool must be run as Root"
	exit 1
fi

arch=armhf

dir="$(mktemp -d ${TMPDIR:-/var/tmp}/docker-mkimage.XXXXXXXXXX)"
rootfs=$dir/rootfs
mkdir $rootfs

debootstrap --no-check-gpg --foreign --arch=$arch jessie $rootfs http://mirrordirector.raspbian.org/raspbian/
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

echo "deb http://mirrordirector.raspbian.org/raspbian/ jessie main contrib non-free
" > $rootfs/etc/apt/sources.list
echo "deb-src http://packages.dotdeb.org wheezy-php56-zts all
" > $rootfs/etc/apt/sources.list.d/php5-zts-src.list


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
chroot $rootfs apt-get -y install ca-certificates binutils debhelper devscripts ssh equivs

mkdir $rootfs/PHPBuild
chroot $rootfs bash -c "cd /PHPBuild && apt-get source php5"
cd $rootfs/PHPBuild
tar -zxf php5_5*.debian.tar.gz
cd ..
sed -i '/.*libt1-dev,.*/d' $rootfs/PHPBuild/debian/control
sed -i '/.*libxml2-dev/a\\t       libxpm-dev,' $rootfs/PHPBuild/debian/control
chroot $rootfs bash -c "cd /PHPBuild && mk-build-deps debian/control"
chroot $rootfs bash -c "cd /PHPBuild && dpkg -i php5-build-deps_*.deb"
chroot $rootfs apt-get -y -f install
chroot $rootfs apt-get -y -f install
rm -Rf $rootfs/PHPBuild/*
cat > "$rootfs/PHPBuild/CreatePHPPackages.sh" <<-'EOF'
#!/bin/bash

set -x

if [[ $1 -lt 1 ]]; then
        echo "Please provide a revision number."
        exit 1
fi

distribution=`lsb_release -a 2>/dev/null | grep "^Codename:" | cut -d $'\t' -f 2`

rm -Rf /PHPBuild/php*
rm -Rf /PHPBuild/lib*

cd /PHPBuild
apt-get update
apt-get source php5
tar -xf php5_5*.orig.tar.xz
tar -zxf php5_5*.debian.tar.gz
mv debian php-5*
cd php-5*
version=`head -n 1 debian/changelog | cut -d "(" -f 2 | cut -d ")" -f 1 | cut -d "+" -f 1 | cut -d "-" -f 1`
revision=`head -n 1 debian/changelog | cut -d "(" -f 2 | cut -d ")" -f 1 | cut -d "+" -f 1 | cut -d "-" -f 2 | cut -d "~" -f 1`
date=`LANG=en_US.UTF-8 date +"%a, %d %b %Y %T %z"`
echo "php5 (${version}-${revision}~homegear.${1}) ${distribution}; urgency=medium

  * Rebuild for Homegear.

 -- Sathya Laufer <sathya@laufers.net>  $date
" | cat - debian/changelog > debian/changelog2
mv debian/changelog2 debian/changelog
sed -i 's/^Architecture: all.*/Architecture: any/' debian/control
sed -i '/.*libt1-dev,.*/d' debian/control
sed -i '/.*libxml2-dev/a\\t       libxpm-dev,' debian/control
sed -i '/.*--with-t1lib.*/d' debian/rules
DEB_BUILD_OPTIONS=nocheck debuild
rm -Rf /PHPBuild/*/
/PHPBuild/Upload.sh
EOF
chmod 755 $rootfs/PHPBuild/CreatePHPPackages.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
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
	echo \"Raspbian\" > distribution
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

docker build -t homegear/phpbuild:raspbian-jessie "$dir"

rm -Rf $dir
