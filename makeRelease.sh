SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -f $SCRIPTDIR/lib/Release/*
rm -f $SCRIPTDIR/lib/Modules/Release/*
rm -f $SCRIPTDIR/bin/Release/homegear
$SCRIPTDIR/premake4 gmake
cd $SCRIPTDIR
make config=release
FILES=$SCRIPTDIR/lib/Modules/Release/*
for f in $FILES; do
	f2=`echo $f | sed 's#.*/##' | sed 's/^lib/mod_/'`
	mv $f $SCRIPTDIR/lib/Modules/Release/$f2
done
