#!/bin/bash

# required build environment packages: binfmt-support qemu qemu-user-static debootstrap kpartx lvm2 dosfstools

set -x

if [ $EUID -ne 0 ]; then
	echo "ERROR: This tool must be run as Root"
	exit 1
fi

arch="armhf"

dir="$(mktemp -d ${TMPDIR:-/var/tmp}/docker-mkimage.XXXXXXXXXX)"
rootfs=$dir/rootfs
mkdir $rootfs

debootstrap --no-check-gpg --foreign --arch=$arch wheezy $rootfs http://mirrordirector.raspbian.org/raspbian/
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

echo "deb http://mirrordirector.raspbian.org/raspbian/ wheezy main contrib non-free
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
sed -i 's/^Architecture: all.*/Architecture: any/' "debian/control"
DEB_BUILD_OPTIONS=nocheck debuild
rm -Rf /PHPBuild/*/
/PHPBuild/Upload.sh
EOF
chmod 755 $rootfs/PHPBuild/CreatePHPPackages.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
while :
do
	echo "Setting up SSH package uploading:"
	while :
	do
		read -p "Please specify the server name to upload to: " servername
		if [ -n "$servername" ]; then
			break
		fi
	done
	while :
	do
		read -p "Please specify the SSH port number of the server: " serverport
		if [ -n "$serverport" ]; then
			break
		fi
	done
	while :
	do
		read -p "Please specify the user name to use to login into the server: " serveruser
		if [ -n "$serveruser" ]; then
			break
		fi
	done
	while :
	do
		read -p "Please specify the path on the server to upload packages to: " serverpath
		serverpath=${serverpath%/}
		if [ -n "$serverpath" ]; then
			break
		fi
	done
	echo "Paste your certificate:"
	IFS= read -d '' -n 1 certificate
	while IFS= read -d '' -n 1 -t 2 c
	do
	    certificate+=$c
	done
	echo
	echo
	echo "Testing connection..."
	mkdir -p /root/.ssh
	echo "$certificate" > /root/.ssh/id_rsa
	chmod 400 /root/.ssh/id_rsa
	ssh -p $serverport ${serveruser}@${servername} "echo \"It works :-)\""
	echo
	echo -e "Server name:\t$servername"
	echo -e "Server port:\t$serverport"
	echo -e "Server user:\t$serveruser"
	echo -e "Server path:\t$serverpath"
	echo -e "Certificate:\t"
	echo "$certificate"
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
		scp -P $serverport \${path}.uploading ${serveruser}@${servername}:${serverpath}
		if [ \$? -eq 0 ]; then
			ssh -p $serverport ${serveruser}@${servername} \"mv ${serverpath}/\${filename}.uploading ${serverpath}/\${filename}\"
			if [ \$? -eq 0 ]; then
				echo "Packages uploaded successfully."
			fi
		fi
	fi
fi
" > /PHPBuild/Upload.sh
chmod 755 /PHPBuild/Upload.sh
rm /FirstStart.sh
echo
echo "Container setup successful. You can now execute \"/PHPBuild/CreatePHPPackages.sh\"."
EOF
chmod 755 $rootfs/FirstStart.sh
echo "/FirstStart.sh" >> $rootfs/root/.bashrc

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
CMD ["/bin/bash"]
EOF

rm -Rf $rootfs

docker build -t homegear/phpbuild:raspbian-wheezy-$arch "$dir"

rm -Rf $dir
