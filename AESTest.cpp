#include "AESTest.h"

namespace BidCoS
{

AESTest::AESTest()
{


}

AESTest::~AESTest()
{

}

void AESTest::auth()
{
	int32_t senderAddress = 0xAAAAAA;
	int32_t receiverAddress = 0xBBBBBB;
	int32_t someAddress = 0xCCCCCC;

	std::string var_44 = std::string(0x1, 0x49);
	std::string var_48 = BaseLib::HelperFunctions::getHexString(senderAddress);
	var_44.append(var_48);
	var_48 = BaseLib::HelperFunctions::getHexString(receiverAddress);
	var_44.append(var_48);
	var_48 = BaseLib::HelperFunctions::getHexString(someAddress);
	var_44.append(var_48);
	var_44.append(0x5, 0x0);
	var_44.append(0x1, 0x5);
	//Encrypt
}
} /* namespace BidCoS */
