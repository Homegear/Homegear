rm -f lib/Debug/*
rm -f lib/Modules/Debug/*
rm -f bin/Debug/homegear
./premake4 gmake
make config=debug
