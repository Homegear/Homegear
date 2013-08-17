#!/bin/bash
result=`id -u $1 2>/dev/null`
if [ "0$result" -eq "0" ]; then
	echo "user exists"
else
	echo "user does not exist"
fi
