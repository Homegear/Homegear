#!/bin/sh
dir=`mktemp -d`
cat > "$dir/libhomegear-ipc-version.cpp" <<-'EOF'
#include <homegear-ipc/IIpcClient.h>
#include <iostream>

int main(int argc, char** argv)
{
	std::cout << '"' << Ipc::IIpcClient::version() << '"' << std::endl;
	return 0;
}
EOF
g++ -std=c++11 -o $dir/libhomegear-ipc-version $dir/libhomegear-ipc-version.cpp -lhomegear-ipc -lgcrypt -lgnutls -lpthread -latomic -latomic
chmod 755 $dir/libhomegear-ipc-version
$dir/libhomegear-ipc-version
rm -Rf $dir
