#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"
rm -Rf "$SCRIPTDIR/misc/State Directory/node-blue/node-red/"
mkdir -p "$SCRIPTDIR/misc/State Directory/node-blue/node-red/"
cd "$SCRIPTDIR/misc/State Directory/node-blue/"
git clone git@gitit.de:ib-company/ibs-home/homegear-addons/node-blue.git node-red
rm -Rf node-red/packages/node_modules/@node-red/editor-client/
rm -Rf node-red/.git

if [[ ! -d "$SCRIPTDIR/../node-blue" ]]; then
	echo "Node-BLUE is not available in \"../node-blue\""
	exit 1
fi

cd "$SCRIPTDIR/../node-blue"
grunt build
rm -Rf "$SCRIPTDIR/misc/State Directory/node-blue/www/*"
cp -a packages/node_modules/@node-red/editor-client/public/* "$SCRIPTDIR/misc/State Directory/node-blue/www/"
rm -f "$SCRIPTDIR/misc/State Directory/node-blue/www/red/red.js"
rm -f "$SCRIPTDIR/misc/State Directory/node-blue/www/red/main.js"
sed -i 's/<script src="red\/red.js"><\/script>/<script src="red\/red.min.js"><\/script>/g' "$SCRIPTDIR/misc/State Directory/node-blue/www/index.php"
