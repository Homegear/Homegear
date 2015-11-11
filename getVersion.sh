#!/bin/sh
dir=`mktemp -d`
cat > "$dir/libhomegear-base-version.cpp" <<-'EOF'
#include "homegear-base/BaseLib.h"

int main(int argc, char** argv)
{
	std::cout << BaseLib::Obj::version() << std::endl;
	return 0;
}
EOF
g++ -std=c++11 -o $dir/libhomegear-base-version $dir/libhomegear-base-version.cpp -lhomegear-base -lgcrypt -lgnutls
chmod 755 $dir/libhomegear-base-version
$dir/libhomegear-base-version
rm -Rf $dir
