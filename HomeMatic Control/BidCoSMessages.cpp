#include "BidCoSMessages.h"

BidCoSMessages::BidCoSMessages()
{
    //ctor
}

BidCoSMessages::~BidCoSMessages()
{
    //dtor
}

void BidCoSMessages::add(BidCoSMessage message)
{
	try
	{
		_messages.push_back(message);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

BidCoSMessage* BidCoSMessages::find(int32_t direction, BidCoSPacket* packet)
{
	try
	{
		int32_t subtypeMax = -1;
		BidCoSMessage* elementToReturn = nullptr;
		for(int i = 0; i < (signed)_messages.size(); i++)
		{
			if(_messages[i].getDirection() == direction && _messages[i].typeIsEqual(packet))
			{
				if(_messages[i].subtypeCount() > subtypeMax)
				{
					elementToReturn = &_messages[i];
					subtypeMax = _messages[i].subtypeCount();
				}
			}
		}
		return elementToReturn;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return nullptr;
}

BidCoSMessage* BidCoSMessages::find(int32_t direction, int32_t messageType, std::vector<std::pair<int32_t, int32_t>> subtypes)
{
	try
	{
		for(int i = 0; i < (signed)_messages.size(); i++)
		{
			if(_messages[i].getDirection() == direction && _messages[i].typeIsEqual(messageType, &subtypes)) return &_messages[i];
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
    return nullptr;
}
