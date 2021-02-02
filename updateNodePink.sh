#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -Rf "$SCRIPTDIR/misc/State Directory/node-blue/node-red/"
mkdir -p "$SCRIPTDIR/misc/State Directory/node-blue/node-red/"
cd "$SCRIPTDIR/misc/State Directory/node-blue/"
git clone git@gitit.de:ib-company/ibs-home/homegear-addons/node-blue.git node-red
rm -Rf node-red/packages/node_modules/@node-red/editor-client/
rm -Rf node-red/.git
