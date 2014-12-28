/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "InsteonMessage.h"
#include "GD.h"

namespace Insteon
{
InsteonMessage::InsteonMessage()
{
}

InsteonMessage::InsteonMessage(int32_t messageType, int32_t messageSubtype, InsteonPacketFlags flags, InsteonDevice* device, int32_t access, void (InsteonDevice::*messageHandler)(std::shared_ptr<InsteonPacket>)) : _messageType(messageType), _messageSubtype(messageSubtype), _messageFlags(flags), _device(device), _access(access), _messageHandlerIncoming(messageHandler)
{
    _direction = DIRECTIONIN;
}

InsteonMessage::InsteonMessage(int32_t messageType, int32_t messageSubtype, InsteonPacketFlags flags, InsteonDevice* device, int32_t access, int32_t accessPairing, void (InsteonDevice::*messageHandler)(std::shared_ptr<InsteonPacket>)) : _messageType(messageType), _messageSubtype(messageSubtype), _messageFlags(flags), _device(device), _access(access), _accessPairing(accessPairing), _messageHandlerIncoming(messageHandler)
{
    _direction = DIRECTIONIN;
}

InsteonMessage::InsteonMessage(int32_t messageType, int32_t messageSubtype, InsteonPacketFlags flags, InsteonDevice* device, void (InsteonDevice::*messageHandler)(int32_t, std::shared_ptr<InsteonPacket>)) : _messageType(messageType), _messageSubtype(messageSubtype), _messageFlags(flags), _device(device), _messageHandlerOutgoing(messageHandler)
{
    _direction = DIRECTIONOUT;
}

InsteonMessage::~InsteonMessage()
{
}

void InsteonMessage::invokeMessageHandlerIncoming(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		if(_device == nullptr || _messageHandlerIncoming == nullptr || packet == nullptr) return;
		((_device)->*(_messageHandlerIncoming))(packet);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void InsteonMessage::invokeMessageHandlerOutgoing(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		if(_device == nullptr || _messageHandlerOutgoing == nullptr || packet == nullptr) return;
		((_device)->*(_messageHandlerOutgoing))(_messageSubtype, packet);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

bool InsteonMessage::typeIsEqual(int32_t messageType, int32_t messageSubtype, InsteonPacketFlags flags, std::vector<std::pair<uint32_t, int32_t> >* subtypes)
{
	try
	{
		if(_messageType != messageType || (_messageSubtype > -1 && messageSubtype > -1 && _messageSubtype != messageSubtype) || _messageFlags != flags) return false;
		if(subtypes->size() != _subtypes.size()) return false;
		for(uint32_t i = 0; i < subtypes->size(); i++)
		{
			if(subtypes->at(i).first != _subtypes.at(i).first || subtypes->at(i).second != _subtypes.at(i).second) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}


bool InsteonMessage::typeIsEqual(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		if(_messageType != packet->messageType() || (_messageSubtype > -1 && packet->messageSubtype() > -1 && _messageSubtype != packet->messageSubtype()) || _messageFlags != packet->flags()) return false;
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
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

bool InsteonMessage::typeIsEqual(std::shared_ptr<InsteonMessage> message)
{
	try
	{
		if(_messageType != message->getMessageType() || _messageFlags != message->getMessageFlags()) return false;
		if(message->getMessageSubtype() > -1 && _messageSubtype > -1 && message->getMessageSubtype() != _messageSubtype) return false;
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
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

bool InsteonMessage::typeIsEqual(std::shared_ptr<InsteonMessage> message, std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		if(message->getMessageType() != packet->messageType() || message->getMessageFlags() != packet->flags()) return false;
		if(message->getMessageSubtype() > -1 && packet->messageSubtype() > -1 && message->getMessageSubtype() != packet->messageSubtype()) return false;
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
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

bool InsteonMessage::checkAccess(std::shared_ptr<InsteonPacket> packet, std::shared_ptr<PacketQueue> queue)
{
	try
	{
		if(_device == nullptr || !packet) return false;

		int32_t access = _device->isInPairingMode() ? _accessPairing : _access;
		if(access == NOACCESS) return false;
		if(queue && !queue->isEmpty() && packet->destinationAddress() == _device->getAddress())
		{
			if(!_device->isInPairingMode() && queue->getQueueType() == PacketQueueType::PAIRING) access = _accessPairing;
			if(queue->front()->getType() == QueueEntryType::PACKET)
			{
				std::shared_ptr<InsteonPacket> backup = queue->front()->getPacket();
				queue->pop(); //Popping takes place here to be able to process resent messages.
				if(!queue->isEmpty() && queue->front()->getType() == QueueEntryType::MESSAGE && !typeIsEqual(queue->front()->getMessage()))
				{
					GD::out.printDebug("Debug: Readding message to queue, because the received packet does not match.");
					queue->pushFront(backup);
					queue->processCurrentQueueEntry(true);
					return false;
				}
			}
		}
		if(access & FULLACCESS) return true;
		if((access & ACCESSDESTISME) && packet->destinationAddress() != _device->getAddress())
		{
			//GD::out.printMessage( "Access denied, because the destination address is not me: " << packet->hexString() << std::endl;
			return false;
		}
		if((access & ACCESSUNPAIRING) && queue != nullptr && queue->getQueueType() == PacketQueueType::UNPAIRING)
		{
			return true;
		}
		if(access & ACCESSPAIREDTOSENDER)
		{
			std::shared_ptr<InsteonPeer> currentPeer;
			if(_device->isInPairingMode() && queue && queue->peer && queue->peer->getAddress() == packet->senderAddress()) currentPeer = queue->peer;
			if(!currentPeer) currentPeer = _device->getPeer(packet->senderAddress());
			if(!currentPeer) return false;
		}
		if((access & ACCESSCENTRAL) && _device->getCentralAddress() != packet->senderAddress())
		{
			//GD::out.printMessage( "Access denied, because it is only granted to a paired central: " << packet->hexString() << std::endl;
			return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}
}
