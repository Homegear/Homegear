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

#include "Central.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

Central::Central(BaseLib::Obj* baseLib, LogicalDevice* me)
{
	_baseLib = baseLib;
	_me = me;
}

Central::~Central()
{
}

int32_t Central::physicalAddress()
{
	return _me->getAddress();
}

//RPC methods
std::shared_ptr<RPC::Variable> Central::getAllValues(uint64_t peerID, bool returnWriteOnly)
{
	try
	{
		std::shared_ptr<RPC::Variable> array(new RPC::Variable(RPC::VariableType::rpcArray));

		if(peerID > 0)
		{
			std::shared_ptr<Peer> peer = _me->getPeer(peerID);
			if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
			std::shared_ptr<RPC::Variable> values = peer->getAllValues(returnWriteOnly);
			if(!values) return RPC::Variable::createError(-32500, "Unknown application error. Values is nullptr.");
			if(values->errorStruct) return values;
			array->arrayValue->push_back(values);
		}
		else
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				//getAllValues really needs a lot of resources, so wait a little bit after each device
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				std::shared_ptr<RPC::Variable> values = (*i)->getAllValues(returnWriteOnly);
				if(!values || values->errorStruct) continue;
				array->arrayValue->push_back(values);
			}
		}

		return array;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getDeviceDescription(std::string serialNumber, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");

		return peer->getDeviceDescription(channel, std::map<std::string, bool>());
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getDeviceDescription(uint64_t id, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(id));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");

		return peer->getDeviceDescription(channel, std::map<std::string, bool>());
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::Variable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::Variable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<Peer> sender(_me->getPeer(senderSerialNumber));
		std::shared_ptr<Peer> receiver(_me->getPeer(receiverSerialNumber));
		if(!sender) return RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return RPC::Variable::createError(-2, "Receiver device not found.");
		return sender->getLinkInfo(senderChannel, receiver->getID(), receiverChannel);
	}
	catch(const std::exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel)
{
	try
	{
		if(senderID == 0) return RPC::Variable::createError(-2, "Sender id is not set.");
		if(receiverID == 0) return RPC::Variable::createError(-2, "Receiver id is not set.");
		std::shared_ptr<Peer> sender(_me->getPeer(senderID));
		std::shared_ptr<Peer> receiver(_me->getPeer(receiverID));
		if(!sender) return RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return RPC::Variable::createError(-2, "Receiver device not found.");
		return sender->getLinkInfo(senderChannel, receiver->getID(), receiverChannel);
	}
	catch(const std::exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getLinkPeers(std::string serialNumber, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return peer->getLinkPeers(channel, false);
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getLinkPeers(uint64_t peerID, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(peerID));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return peer->getLinkPeers(channel, true);
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getLinks(std::string serialNumber, int32_t channel, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return getLinks(0, -1, flags);
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return getLinks(peer->getID(), channel, flags);
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getLinks(uint64_t peerID, int32_t channel, int32_t flags)
{
	try
	{
		std::shared_ptr<RPC::Variable> array(new RPC::Variable(RPC::VariableType::rpcArray));
		std::shared_ptr<RPC::Variable> element(new RPC::Variable(RPC::VariableType::rpcArray));
		if(peerID == 0)
		{
			try
			{
				std::vector<std::shared_ptr<Peer>> peers;
				//Copy all peers first, because getLinks takes very long and we don't want to lock _peersMutex too long
				_me->getPeers(peers);

				for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					//listDevices really needs a lot of ressources, so wait a little bit after each device
					std::this_thread::sleep_for(std::chrono::milliseconds(3));
					element = (*i)->getLink(channel, flags, true);
					array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
			catch(const std::exception& ex)
			{
				_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		else
		{
			std::shared_ptr<Peer> peer(_me->getPeer(peerID));
			if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
			element = peer->getLink(channel, flags, false);
			array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		/*if(serialNumber == "BidCoS-RF" && (channel == 0 || channel == -1) && type == RPC::ParameterSet::Type::Enum::master)
		{
			std::shared_ptr<RPC::Variable> paramset(new RPC::Variable(RPC::VariableType::rpcStruct));
			paramset->structValue->insert(RPC::RPCStructElement("AES_KEY", std::shared_ptr<RPC::Variable>(new RPC::Variable(1))));
			return paramset;
		}*/
		if(serialNumber == _me->getSerialNumber() && (channel == 0 || channel == -1) && type == RPC::ParameterSet::Type::Enum::master)
		{
			std::shared_ptr<RPC::Variable> paramset(new RPC::Variable(RPC::VariableType::rpcStruct));
			return paramset;
		}
		else
		{
			std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
			if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
			uint64_t remoteID = 0;
			if(!remoteSerialNumber.empty())
			{
				std::shared_ptr<Peer> remotePeer(_me->getPeer(remoteSerialNumber));
				if(!remotePeer)
				{
					if(remoteSerialNumber != _me->getSerialNumber()) return RPC::Variable::createError(-3, "Remote peer is unknown.");
				}
				else remoteID = remotePeer->getID();
			}
			return peer->getParamset(channel, type, remoteID, remoteChannel);
		}
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getParamset(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(peerID));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return peer->getParamset(channel, type, remoteID, remoteChannel);
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(serialNumber == _me->getSerialNumber() && (channel == 0 || channel == -1) && type == RPC::ParameterSet::Type::Enum::master)
		{
			std::shared_ptr<RPC::Variable> descriptions(new RPC::Variable(RPC::VariableType::rpcStruct));
			return descriptions;
		}
		else
		{
			std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
			uint64_t remoteID = 0;
			if(!remoteSerialNumber.empty())
			{
				std::shared_ptr<Peer> remotePeer(_me->getPeer(remoteSerialNumber));
				if(remotePeer) remoteID = remotePeer->getID();
			}
			if(peer) return peer->getParamsetDescription(channel, type, remoteID, remoteChannel);
			return RPC::Variable::createError(-2, "Unknown device.");
		}
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getParamsetDescription(uint64_t id, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(id));
		if(peer) return peer->getParamsetDescription(channel, type, remoteID, remoteChannel);
		return RPC::Variable::createError(-2, "Unknown device.");
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(serialNumber == _me->getSerialNumber())
		{
			if(channel > 0) return RPC::Variable::createError(-2, "Unknown channel.");
			if(type != RPC::ParameterSet::Type::Enum::master) return RPC::Variable::createError(-3, "Unknown parameter set.");
			return std::shared_ptr<RPC::Variable>(new RPC::Variable(std::string("rf_homegear_central_master")));
		}
		else
		{
			std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
			uint64_t remoteID = 0;
			if(!remoteSerialNumber.empty())
			{
				std::shared_ptr<Peer> remotePeer(_me->getPeer(remoteSerialNumber));
				if(remotePeer) remoteID = remotePeer->getID();
			}
			if(peer) return peer->getParamsetId(channel, type, remoteID, remoteChannel);
			return RPC::Variable::createError(-2, "Unknown device.");
		}
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getParamsetId(uint64_t peerID, uint32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(peerID));
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return peer->getParamsetId(channel, type, remoteID, remoteChannel);
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getPeerID(int32_t filterType, std::string filterValue)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::Variable> ids(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		if(filterType == 1) //Serial number
		{
			std::shared_ptr<Peer> peer = _me->getPeer(filterValue);
			if(peer) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)peer->getID())));
		}
		else if(filterType == 2) //Physical address
		{
			int32_t address = Math::getNumber(filterValue);
			if(address != 0)
			{
				std::shared_ptr<Peer> peer = _me->getPeer(address);
				if(peer) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)peer->getID())));
			}
		}
		else if(filterType == 3) //Type id
		{
			uint32_t type = (uint32_t)Math::getNumber(filterValue);
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if((*i)->getDeviceType().type() == type) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		else if(filterType == 4) //Type string
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if((*i)->getTypeString() == filterValue) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		else if(filterType == 5) //Name
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if((*i)->getName().find(filterValue) != std::string::npos) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		else if(filterType == 6) //Pending config
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if((*i)->serviceMessages->getConfigPending()) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		else if(filterType == 7) //Unreachable
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if((*i)->serviceMessages->getUnreach()) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		else if(filterType == 8) //Reachable
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if(!(*i)->serviceMessages->getUnreach()) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		else if(filterType == 9) //Low battery
		{
			std::vector<std::shared_ptr<Peer>> peers;
			//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
			_me->getPeers(peers);

			for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				if(!*i) continue;
				if((*i)->serviceMessages->getLowbat()) ids->arrayValue->push_back(std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)(*i)->getID())));
			}
		}
		return ids;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getPeerID(int32_t address)
{
	try
	{
		std::shared_ptr<Peer> peer = _me->getPeer(address);
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)peer->getID()));
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getPeerID(std::string serialNumber)
{
	try
	{
		std::shared_ptr<Peer> peer = _me->getPeer(serialNumber);
		if(!peer) return RPC::Variable::createError(-2, "Unknown device.");
		return std::shared_ptr<RPC::Variable>(new RPC::Variable((int32_t)peer->getID()));
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getServiceMessages(bool returnID)
{
	try
	{
		std::vector<std::shared_ptr<Peer>> peers;
		//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
		_me->getPeers(peers);

		std::shared_ptr<RPC::Variable> serviceMessages(new RPC::Variable(RPC::VariableType::rpcArray));
		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			if(!*i) continue;
			//getServiceMessages really needs a lot of ressources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<RPC::Variable> messages = (*i)->getServiceMessages(returnID);
			if(!messages->arrayValue->empty()) serviceMessages->arrayValue->insert(serviceMessages->arrayValue->end(), messages->arrayValue->begin(), messages->arrayValue->end());
		}
		return serviceMessages;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getValue(std::string serialNumber, uint32_t channel, std::string valueKey, bool requestFromDevice, bool asynchronous)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(peer) return peer->getValue(channel, valueKey, requestFromDevice, asynchronous);
		return RPC::Variable::createError(-2, "Unknown device.");
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::getValue(uint64_t id, uint32_t channel, std::string valueKey, bool requestFromDevice, bool asynchronous)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(id));
		if(peer) return peer->getValue(channel, valueKey, requestFromDevice, asynchronous);
		return RPC::Variable::createError(-2, "Unknown device.");
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::listDevices(bool channels, std::map<std::string, bool> fields)
{
	return listDevices(channels, fields, std::shared_ptr<std::set<std::uint64_t>>());
}

std::shared_ptr<RPC::Variable> Central::listDevices(bool channels, std::map<std::string, bool> fields, std::shared_ptr<std::set<uint64_t>> knownDevices)
{
	try
	{
		std::shared_ptr<RPC::Variable> array(new RPC::Variable(RPC::VariableType::rpcArray));

		std::vector<std::shared_ptr<Peer>> peers;
		//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
		_me->getPeers(peers, knownDevices);

		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			//listDevices really needs a lot of resources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::Variable>>> descriptions = (*i)->getDeviceDescriptions(channels, fields);
			if(!descriptions) continue;
			for(std::vector<std::shared_ptr<RPC::Variable>>::iterator j = descriptions->begin(); j != descriptions->end(); ++j)
			{
				array->arrayValue->push_back(*j);
			}
		}

		return array;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::reportValueUsage(std::string serialNumber)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(!peer) return RPC::Variable::createError(-2, "Peer not found.");
		return peer->reportValueUsage();
	}
	catch(const std::exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::rssiInfo()
{
	try
	{
		std::shared_ptr<RPC::Variable> response(new RPC::Variable(RPC::VariableType::rpcStruct));

		std::vector<std::shared_ptr<Peer>> peers;
		//Copy all peers first, because rssiInfo takes very long and we don't want to lock _peersMutex too long
		_me->getPeers(peers);

		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			//rssiInfo really needs a lot of resources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<RPC::Variable> element = (*i)->rssiInfo();
			if(!element || element->errorStruct) continue;
			response->structValue->insert(RPC::RPCStructElement((*i)->getSerialNumber(), element));
		}

		return response;
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::setId(uint64_t oldPeerID, uint64_t newPeerID)
{
	try
	{
		if(oldPeerID == 0 || oldPeerID >= 0x40000000) return RPC::Variable::createError(-100, "The current peer ID is invalid.");
		std::shared_ptr<Peer> peer(_me->getPeer(oldPeerID));
		if(!peer) return RPC::Variable::createError(-2, "Peer not found.");
		std::shared_ptr<RPC::Variable> result = peer->setId(newPeerID);
		if(result->errorStruct) return result;
		_me->setPeerID(oldPeerID, newPeerID);
		return std::shared_ptr<RPC::Variable>(new RPC::Variable(RPC::VariableType::rpcVoid));
	}
	catch(const std::exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::Variable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::Variable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<Peer> sender(_me->getPeer(senderSerialNumber));
		std::shared_ptr<Peer> receiver(_me->getPeer(receiverSerialNumber));
		if(!sender) return RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return RPC::Variable::createError(-2, "Receiver device not found.");
		std::shared_ptr<RPC::Variable> result1 = sender->setLinkInfo(senderChannel, receiver->getID(), receiverChannel, name, description);
		std::shared_ptr<RPC::Variable> result2 = receiver->setLinkInfo(receiverChannel, sender->getID(), senderChannel, name, description);
		if(result1->errorStruct) return result1;
		if(result2->errorStruct) return result2;
		return std::shared_ptr<RPC::Variable>(new RPC::Variable(RPC::VariableType::rpcVoid));
	}
	catch(const std::exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::setLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(senderID == 0) return RPC::Variable::createError(-2, "Sender id is not set.");
		if(receiverID == 0) return RPC::Variable::createError(-2, "Receiver id is not set.");
		std::shared_ptr<Peer> sender(_me->getPeer(senderID));
		std::shared_ptr<Peer> receiver(_me->getPeer(receiverID));
		if(!sender) return RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return RPC::Variable::createError(-2, "Receiver device not found.");
		std::shared_ptr<RPC::Variable> result1 = sender->setLinkInfo(senderChannel, receiver->getID(), receiverChannel, name, description);
		std::shared_ptr<RPC::Variable> result2 = receiver->setLinkInfo(receiverChannel, sender->getID(), senderChannel, name, description);
		if(result1->errorStruct) return result1;
		if(result2->errorStruct) return result2;
		return std::shared_ptr<RPC::Variable>(new RPC::Variable(RPC::VariableType::rpcVoid));
	}
	catch(const std::exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::setName(uint64_t id, std::string name)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(id));
		if(peer)
		{
			peer->setName(name);
			return std::shared_ptr<RPC::Variable>(new RPC::Variable(RPC::VariableType::rpcVoid));
		}
		return RPC::Variable::createError(-2, "Unknown device.");
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::Variable> value)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(serialNumber));
		if(peer) return peer->setValue(channel, valueKey, value);
		return RPC::Variable::createError(-2, "Unknown device.");
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::Variable> Central::setValue(uint64_t id, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::Variable> value)
{
	try
	{
		std::shared_ptr<Peer> peer(_me->getPeer(id));
		if(peer) return peer->setValue(channel, valueKey, value);
		return RPC::Variable::createError(-2, "Unknown device.");
	}
	catch(const std::exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::Variable::createError(-32500, "Unknown application error.");
}
//End RPC methods

}
}
