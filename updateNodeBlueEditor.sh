#!/bin/bash
SCRIPTDIR="$( cd "$(dirname $0)" && pwd )"

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
