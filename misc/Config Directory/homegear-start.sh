#!/bin/bash

# This script is executed before Homegear starts.

# Set permissions on interfaces and directories, export GPIOs.
/usr/bin/homegear -u homegear -g homegear -p /var/run/homegear/homegear.pid -pre
