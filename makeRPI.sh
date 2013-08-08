./premake4 --platform=rpi gmake
make config=release_rpi
make config=profiling_rpi
./premake4 gmake
