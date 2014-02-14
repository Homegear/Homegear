rm -f bin/Debug/homegear
./premake4 --platform=rpi gmake
make config=debug_rpi
./premake4 gmake
