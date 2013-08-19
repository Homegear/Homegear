#include "BidCoSMessages.h"
#include "HelperFunctions.h"

void BidCoSMessages::add(std::shared_ptr<BidCoSMessage> message)
{
	try
	{
		_messages.push_back(message);
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

std::shared_ptr<BidCoSMessage> BidCoSMessages::find(int32_t direction, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet == nullptr) return std::shared_ptr<BidCoSMessage>();
		int32_t subtypeMax = -1;
		std::shared_ptr<BidCoSMessage>* elementToReturn = nullptr;
		for(uint32_t i = 0; i < _messages.size(); i++)
		{
			if(_messages[i]->getDirection() == direction && _messages[i]->typeIsEqual(packet))
			{
				if((signed)_messages[i]->subtypeCount() > subtypeMax)
				{
					elementToReturn = &_messages[i];
					subtypeMax = _messages[i]->subtypeCount();
				}
			}
		}
		if(elementToReturn == nullptr) return std::shared_ptr<BidCoSMessage>(); else return *elementToReturn;
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return nullptr;
}

std::shared_ptr<BidCoSMessage> BidCoSMessages::find(int32_t direction, int32_t messageType, std::vector<std::pair<uint32_t, int32_t>> subtypes)
{
	try
	{
		for(uint32_t i = 0; i < _messages.size(); i++)
		{
			if(_messages[i]->getDirection() == direction && _messages[i]->typeIsEqual(messageType, &subtypes)) return _messages[i];
		}
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
    return std::shared_ptr<BidCoSMessage>();
}
