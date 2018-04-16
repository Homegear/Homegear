#!/bin/bash
if [ ! -z $HOMEGEARUSER ] && [ ! -z $HOMEGEARGROUP ]; then
        RUNASUSER=$HOMEGEARUSER
        RUNASGROUP=$HOMEGEARGROUP

else
        RUNASUSER=homegear
        RUNASGROUP=homegear
fi

echo $RUNASUSER
echo $RUNASGROUP
