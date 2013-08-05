#include "BidCoSMessage.h"

BidCoSMessage::BidCoSMessage()
{
}

BidCoSMessage::BidCoSMessage(int32_t messageType, HomeMaticDevice* device, int32_t access, void (HomeMaticDevice::*messageHandler)(int32_t, std::shared_ptr<BidCoSPacket>)) : _messageType(messageType), _device(device), _access(access), _messageHandlerIncoming(messageHandler)
{
    _direction = DIRECTIONIN;
}

BidCoSMessage::BidCoSMessage(int32_t messageType, HomeMaticDevice* device, int32_t access, int32_t accessPairing, void (HomeMaticDevice::*messageHandler)(int32_t, std::shared_ptr<BidCoSPacket>)) : _messageType(messageType), _device(device), _access(access), _accessPairing(accessPairing), _messageHandlerIncoming(messageHandler)
{
    _direction = DIRECTIONIN;
}

BidCoSMessage::BidCoSMessage(int32_t messageType, int32_t controlByte, HomeMaticDevice* device, void (HomeMaticDevice::*messageHandler)(int32_t, int32_t, std::shared_ptr<BidCoSPacket>)) : _messageType(messageType), _controlByte(controlByte), _device(device), _messageHandlerOutgoing(messageHandler)
{
    _direction = DIRECTIONOUT;
}

BidCoSMessage::~BidCoSMessage()
{
}

void BidCoSMessage::invokeMessageHandlerIncoming(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_device == nullptr || _messageHandlerIncoming == nullptr || packet == nullptr) return;
		((_device)->*(_messageHandlerIncoming))(packet->messageCounter(), packet);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

void BidCoSMessage::invokeMessageHandlerOutgoing(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_device == nullptr || _messageHandlerOutgoing == nullptr || packet == nullptr) return;
		//Actually the message counter implementation is not correct. See https://sathya.de/HMCWiki/index.php/Examples:Message_Counter
		_device->messageCounter()->at(packet->senderAddress())++;
		((_device)->*(_messageHandlerOutgoing))(_device->messageCounter()->at(packet->senderAddress()), _controlByte, packet);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

bool BidCoSMessage::typeIsEqual(int32_t messageType, std::vector<std::pair<uint32_t, int32_t> >* subtypes)
{
	try
	{
		if(_messageType != messageType) return false;
		if(subtypes->size() != _subtypes.size()) return false;
		for(uint32_t i = 0; i < subtypes->size(); i++)
		{
			if(subtypes->at(i).first != _subtypes.at(i).first || subtypes->at(i).second != _subtypes.at(i).second) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return false;
}


bool BidCoSMessage::typeIsEqual(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_messageType != packet->messageType()) return false;
		std::vector<uint8_t>* payload = packet->payload();
		if(_subtypes.empty()) return true;
		for(std::vector<std::pair<uint32_t, int32_t>>::const_iterator i = _subtypes.begin(); i != _subtypes.end(); ++i)
		{
			if(i->first >= payload->size()) return false;
			if(payload->at(i->first) != i->second) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return false;
}

bool BidCoSMessage::typeIsEqual(std::shared_ptr<BidCoSMessage> message)
{
	try
	{
		if(_messageType != message->getMessageType()) return false;
		if(_subtypes.empty()) return true;
		if(message->subtypeCount() != _subtypes.size()) return false;
		std::vector<std::pair<uint32_t, int32_t> >* subtypes = message->getSubtypes();
		for(uint32_t i = 0; i < _subtypes.size(); i++)
		{
			if(subtypes->at(i).first != _subtypes.at(i).first || subtypes->at(i).second != _subtypes.at(i).second) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return false;
}

bool BidCoSMessage::typeIsEqual(std::shared_ptr<BidCoSMessage> message, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(message->getMessageType() != packet->messageType()) return false;
		std::vector<std::pair<uint32_t, int32_t>>* subtypes = message->getSubtypes();
		std::vector<uint8_t>* payload = packet->payload();
		if(subtypes == nullptr || subtypes->size() == 0) return true;
		for(std::vector<std::pair<uint32_t, int32_t> >::const_iterator i = subtypes->begin(); i != subtypes->end(); ++i)
		{
			if(i->first >= payload->size()) return false;
			if(payload->at(i->first) != i->second) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return false;
}

bool BidCoSMessage::checkAccess(std::shared_ptr<BidCoSPacket> packet, std::shared_ptr<BidCoSQueue> queue)
{
	try
	{
		if(_device == nullptr || !packet) return false;

		int32_t access = _device->isInPairingMode() ? _accessPairing : _access;
		Peer* currentPeer = _device->isInPairingMode() ? ((queue && queue->peer && queue->peer->address == packet->senderAddress()) ? queue->peer.get() : nullptr) : nullptr;
		if(!currentPeer) currentPeer = ((_device->getPeers()->find(packet->senderAddress()) == _device->getPeers()->end()) ? nullptr : _device->getPeers()->at(packet->senderAddress()).get());
		if(access == NOACCESS) return false;
		if(queue && !queue->isEmpty())
		{
			if(queue->front()->getType() == QueueEntryType::PACKET || (queue->front()->getType() == QueueEntryType::MESSAGE && !typeIsEqual(queue->front()->getMessage())))
			{
				queue->pop(); //Popping takes place here to be able to process resent messages.
				if(!queue->isEmpty() && queue->front()->getType() == QueueEntryType::MESSAGE && !typeIsEqual(queue->front()->getMessage())) return false;
			}
		}
		if(access & FULLACCESS) return true;
		if((access & ACCESSDESTISME) && packet->destinationAddress() != _device->address())
		{
			//std::cout << "Access denied, because the destination address is not me: " << packet->hexString() << std::endl;
			return false;
		}
		if((access & ACCESSUNPAIRING) && queue != nullptr && queue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			return true;
		}
		if((access & ACCESSPAIREDTOSENDER) && currentPeer == nullptr)
		{
			//std::cout << "Access denied, because device is not paired with sender: " << packet->hexString() << std::endl;
			return false;
		}
		if((access & ACCESSCENTRAL) && _device->getCentralAddress() != packet->senderAddress())
		{
			//std::cout << "Access denied, because it is only granted to a paired central: " << packet->hexString() << std::endl;
			return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	return false;
}
