#!/bin/bash
if [ "$1" = "stop" ]; then
	echo "Stopping tracking make files...";
	git update-index --assume-unchanged premake4.lua
	git update-index --assume-unchanged Makefile
	git update-index --assume-unchanged *.make
elif [ "$1" = "start" ]; then
	echo "Starting tracking make files...";
	git update-index --no-assume-unchanged premake4.lua
	git update-index --no-assume-unchanged Makefile
	git update-index --no-assume-unchanged *.make
else
	echo "This script enable's or disables git tracking of make files so you can change them without uploading the changes to the repository."
	echo "Usage (execute in repository's root directory): gitTrackMakefiles.sh [start|stop]"
fi

exit 0
