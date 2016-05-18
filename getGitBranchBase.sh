#!/bin/sh
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"

found=0
count=0
for f in $SCRIPTDIR/../libhomegear-base*; do
	if [ -d "$f" ]; then
		found=1
	fi
	count=$((count+1))
done

if [ $count -gt 1 ] || [ $found -eq 0 ]; then
	echo "\"-\""
	exit 0
fi

cd $SCRIPTDIR/../libhomegear-base*/
if [ ! -d .git ]; then
	echo "\"-\""
else
	branch=`git rev-parse --abbrev-ref HEAD`
	echo "\"$branch\""
fi
