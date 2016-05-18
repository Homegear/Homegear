#!/bin/sh
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
cd $SCRIPTDIR
if [ ! -d .git ]; then
	echo "\"-\""
else
	branch=`git rev-parse --abbrev-ref HEAD`
	echo "\"$branch\""
fi
