#!/bin/sh

rm -f commits
wget -q https://api.github.com/repos/Homegear/Homegear/commits
sha=`grep -m 1 -Po '"sha":.*([0-9a-fA-F]*)' commits | cut -d '"' -f 4`
rm -f commits
echo "\"$sha\""
