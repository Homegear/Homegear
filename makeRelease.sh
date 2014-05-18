rm -f lib/Release/*
rm -f lib/Modules/Release/*
rm -f bin/Release/homegear
./premake4 gmake
make config=release
