rm -f lib/Debug/*
rm -f lib/Modules/Debug/*
rm -f bin/Debug/homegear
./premake4 --platform=rpi gmake
make config=debug_rpi
./premake4 gmake
