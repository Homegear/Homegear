#!/bin/sh
dir=`mktemp -d`
cat > "$dir/libhomegear-node-version.cpp" <<-'EOF'
#include <homegear-node/INode.h>
#include <iostream>

int main(int argc, char** argv)
{
	std::cout << '"' << Flows::INode::version() << '"' << std::endl;
	return 0;
}
EOF
g++ -std=c++11 -o $dir/libhomegear-node-version $dir/libhomegear-node-version.cpp -lhomegear-node -lgcrypt -lgnutls -lpthread -latomic -latomic
chmod 755 $dir/libhomegear-node-version
$dir/libhomegear-node-version
rm -Rf $dir
