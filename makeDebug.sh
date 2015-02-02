SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -f $SCRIPTDIR/lib/Debug/*
rm -f $SCRIPTDIR/lib/Modules/Debug/*
rm -f $SCRIPTDIR/bin/Debug/homegear
$SCRIPTDIR/premake4 gmake
cd $SCRIPTDIR
make config=debug
FILES=$SCRIPTDIR/lib/Modules/Debug/*
for f in $FILES; do
	f2=`echo $f | sed 's#.*/##' | sed 's/^lib/mod_/'`
	mv $f $SCRIPTDIR/lib/Modules/Debug/$f2
done
