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
echo "deb http://homegear.eu/packages/Raspbian/ wheezy/
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
chroot $rootfs apt-get -y install ssh unzip ca-certificates binutils debhelper devscripts sqlite3 libsqlite3-dev libreadline6 libreadline6-dev libncurses5-dev libssl-dev libparse-debcontrol-perl libgcrypt11-dev libgpg-error-dev libgnutls-dev php5-dev libphp5-embed libssl-dev libcrypto++-dev g++-4.7 gcc-4.7

rm $rootfs/usr/bin/g++
rm $rootfs/usr/bin/gcc
ln -s g++-4.7 $rootfs/usr/bin/g++
ln -s gcc-4.7 $rootfs/usr/bin/gcc

mkdir $rootfs/build
cat > "$rootfs/build/CreateDebianPackageNightly.sh" <<-'EOF'
#!/bin/bash

cd /build
wget https://github.com/Homegear/Homegear/archive/master.zip
unzip master.zip
rm master.zip
version=$(head -n 1 Homegear-master/Version.h | cut -d " " -f3 | tr -d '"')
sourcePath=homegear-$version
mv Homegear-master $sourcePath
cd $sourcePath
revision=0
wgetCount=0
while [ $revision -eq 0 ] && [ $wgetCount -le 5 ]; do
	rm -f contributors
	wget https://api.github.com/repos/Homegear/Homegear/stats/contributors
	lines=`grep -Po '"total":.*[0-9]' contributors`
	for l in $lines; do
		    if [ "$l" != "\"total\":" ]; then
		            revision=$(($revision + $l))
		    fi
	done
	wgetCount=$(($wgetCount + 1))
done
if [ $revision -eq 0 ]; then
	echo "Error: Could not get revision from GitHub."
	exit 1
fi
cd ..
rm -Rf $sourcePath/.* 1>/dev/null 2>&2
rm -Rf $sourcePath/obj
rm -Rf $sourcePath/bin
sed -i 's/dh_auto_build -- config=release/dh_auto_build -- config=debug/g' $sourcePath/debian/rules
sed -i 's/$(CURDIR)\/bin\/Release\/homegear/$(CURDIR)\/bin\/Debug\/homegear/g' $sourcePath/debian/rules
sed -i 's/$(CURDIR)\/lib\/Modules\/Release\//$(CURDIR)\/lib\/Modules\/Debug\//g' $sourcePath/debian/rules
sed -i 's/libgcrypt20-dev/libgcrypt11-dev/g' $sourcePath/debian/control
sed -i 's/libgnutls28-dev/libgnutls-dev/g' $sourcePath/debian/control
sed -i 's/libgcrypt20/libgcrypt11/g' $sourcePath/debian/control
sed -i 's/libgnutlsxx28/libgnutlsxx27/g' $sourcePath/debian/control
tar -zcpf homegear_$version.orig.tar.gz $sourcePath
cd $sourcePath
dch -v $version-$revision -M "Version $version."
debuild -us -uc
cd ..
rm -Rf $sourcePath
rm homegear_$version-$revision_*.build
rm homegear_$version-$revision_*.changes
rm homegear_$version-$revision.debian.tar.?z
rm homegear_$version-$revision.dsc
rm homegear_$version.orig.tar.gz
mv homegear_$version-$revision_*.deb homegear.deb
if test -f homegear.deb; then
	isodate=`date +%Y%m%d`
EOF
echo "	mv homegear.deb homegear_\$[isodate]_raspbian_wheezy_$arch.deb
	/build/Upload.sh
fi" >> $rootfs/build/CreateDebianPackageNightly.sh
chmod 755 $rootfs/build/CreateDebianPackageNightly.sh

cat > "$rootfs/FirstStart.sh" <<-'EOF'
#!/bin/bash
sed -i '$ d' /root/.bashrc >/dev/null
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

docker build -t homegear/build:raspbian-wheezy "$dir"

rm -Rf $dir
