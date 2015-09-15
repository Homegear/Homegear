#!/bin/bash

# Installation instructions:
# 1. Make this script executable: chmod 700 homegearMonitor.sh
# 2. Add the script to your root's crontab:
#	- crontab -e
#	- Add the line:
#	  */2 *  *       *       *       /path/to/your/HomegearMonitor.sh 2>&1 |/usr/bin/logger -t HomegearMonitor

return=`ps -A | grep homegear -c`
if [ $return -lt 1 ] && test -e /var/run/homegear/homegear.pid; then
        LOGDIR=/var/log/homegear
        if test -e $LOGDIR/core; then
                COREDIR=$LOGDIR/coredumps
                mkdir -p $COREDIR
                DIR=0
                while test -d $COREDIR/$DIR; do
                        ((DIR++))
                done
                COREDIR=$COREDIR/$DIR
                mkdir -p $COREDIR
				mv $LOGDIR/core $COREDIR
				mv $LOGDIR/homegear.log $COREDIR
				mv $LOGDIR/homegear.err $COREDIR
                cp /var/lib/homegear/modules/* $COREDIR
				cp /usr/bin/homegear $COREDIR
        fi

        /etc/init.d/homegear restart
fi

