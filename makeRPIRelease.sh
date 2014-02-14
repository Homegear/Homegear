rm -f bin/Release/homegear
./premake4 --platform=rpi gmake
make config=release_rpi
./premake4 gmake
