/* Copyright 2013-2014 Sathya Laufer
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

class CallbackFunctionParameter;

#include "BidCoSPeer.h"
#include "BidCoSQueue.h"
#include "PendingBidCoSQueues.h"
#include "Devices/HomeMaticCentral.h"
#include "../Base/BaseLib.h"
#include "GD.h"

namespace BidCoS
{
std::shared_ptr<HomeMaticCentral> BidCoSPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = std::dynamic_pointer_cast<HomeMaticCentral>(BaseLib::Obj::family->getCentral());
		return _central;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<HomeMaticCentral>();
}

std::shared_ptr<HomeMaticDevice> BidCoSPeer::getDevice(int32_t address)
{
	try
	{
		std::shared_ptr<HomeMaticDevice> device(std::dynamic_pointer_cast<HomeMaticDevice>(BaseLib::Obj::family->get(address)));
		return device;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<HomeMaticDevice>();
}

void BidCoSPeer::initializeCentralConfig()
{
	try
	{
		if(!rpcDevice)
		{
			BaseLib::Output::printWarning("Warning: Tried to initialize HomeMatic BidCoS peer's central config without xmlrpcDevice being set.");
			return;
		}
		raiseCreateSavepoint("bidCoSPeerConfig" + std::to_string(_peerID));
		BaseLib::Systems::RPCConfigurationParameter parameter;
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			if(i->second->parameterSets.find(BaseLib::RPC::ParameterSet::Type::master) != i->second->parameterSets.end() && i->second->parameterSets[BaseLib::RPC::ParameterSet::Type::master])
			{
				std::shared_ptr<BaseLib::RPC::ParameterSet> masterSet = i->second->parameterSets[BaseLib::RPC::ParameterSet::Type::master];
				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = masterSet->parameters.begin(); j != masterSet->parameters.end(); ++j)
				{
					if(!(*j)->id.empty() && configCentral[i->first].find((*j)->id) == configCentral[i->first].end())
					{
						parameter = BaseLib::Systems::RPCConfigurationParameter();
						parameter.rpcParameter = *j;
						parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
						configCentral[i->first][(*j)->id] = parameter;
						saveParameter(0, BaseLib::RPC::ParameterSet::Type::master, i->first, (*j)->id, parameter.data);
					}
				}
			}
			if(i->second->parameterSets.find(BaseLib::RPC::ParameterSet::Type::values) != i->second->parameterSets.end() && i->second->parameterSets[BaseLib::RPC::ParameterSet::Type::values])
			{
				std::shared_ptr<BaseLib::RPC::ParameterSet> valueSet = i->second->parameterSets[BaseLib::RPC::ParameterSet::Type::values];
				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = valueSet->parameters.begin(); j != valueSet->parameters.end(); ++j)
				{
					if(!(*j)->id.empty() && valuesCentral[i->first].find((*j)->id) == valuesCentral[i->first].end())
					{
						parameter = BaseLib::Systems::RPCConfigurationParameter();
						parameter.rpcParameter = *j;
						parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
						valuesCentral[i->first][(*j)->id] = parameter;
						saveParameter(0, BaseLib::RPC::ParameterSet::Type::values, i->first, (*j)->id, parameter.data);
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    raiseReleaseSavepoint("bidCoSPeerConfig" + std::to_string(_peerID));
}

void BidCoSPeer::initializeLinkConfig(int32_t channel, int32_t remoteAddress, int32_t remoteChannel, bool useConfigFunction)
{
	try
	{
		raiseCreateSavepoint("bidCoSPeerLinkConfig" + std::to_string(_peerID));
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		if(rpcDevice->channels[channel]->parameterSets.find(BaseLib::RPC::ParameterSet::Type::link) == rpcDevice->channels[channel]->parameterSets.end()) return;
		std::shared_ptr<BaseLib::RPC::ParameterSet> linkSet = rpcDevice->channels[channel]->parameterSets[BaseLib::RPC::ParameterSet::Type::link];
		//This line creates an empty link config. This is essential as the link config must exist, even if it is empty.
		std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>* linkConfig = &linksCentral[channel][remoteAddress][remoteChannel];
		BaseLib::Systems::RPCConfigurationParameter parameter;
		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = linkSet->parameters.begin(); j != linkSet->parameters.end(); ++j)
		{
			if(!(*j)->id.empty() && linkConfig->find((*j)->id) == linkConfig->end())
			{
				parameter = BaseLib::Systems::RPCConfigurationParameter();
				parameter.rpcParameter = *j;
				parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
				linkConfig->insert(std::pair<std::string, BaseLib::Systems::RPCConfigurationParameter>((*j)->id, parameter));
				saveParameter(0, BaseLib::RPC::ParameterSet::Type::link, channel, (*j)->id, parameter.data, remoteAddress, remoteChannel);
			}
		}
		if(useConfigFunction) applyConfigFunction(channel, remoteAddress, remoteChannel);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    raiseReleaseSavepoint("bidCoSPeerLinkConfig" + std::to_string(_peerID));
}

void BidCoSPeer::applyConfigFunction(int32_t channel, int32_t peerAddress, int32_t remoteChannel)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		if(rpcDevice->channels[channel]->parameterSets.find(BaseLib::RPC::ParameterSet::Type::link) == rpcDevice->channels[channel]->parameterSets.end()) return;
		std::shared_ptr<BaseLib::RPC::ParameterSet> linkSet = rpcDevice->channels[channel]->parameterSets[BaseLib::RPC::ParameterSet::Type::link];
		std::shared_ptr<HomeMaticCentral> central = getCentral();
		if(!central) return; //Shouldn't happen
		std::shared_ptr<BidCoSPeer> remotePeer(central->getPeer(peerAddress));
		if(!remotePeer || !remotePeer->rpcDevice) return; //Shouldn't happen
		if(remotePeer->rpcDevice->channels.find(remoteChannel) == remotePeer->rpcDevice->channels.end()) return;
		std::shared_ptr<BaseLib::RPC::DeviceChannel> remoteRPCChannel = remotePeer->rpcDevice->channels.at(remoteChannel);
		int32_t groupedWith = remotePeer->getChannelGroupedWith(remoteChannel);
		std::string function;
		if(groupedWith == -1 && !remoteRPCChannel->function.empty()) function = remoteRPCChannel->function;
		if(groupedWith > -1)
		{
			if(groupedWith > remoteChannel && !remoteRPCChannel->pairFunction1.empty()) function = remoteRPCChannel->pairFunction1;
			else if(groupedWith < remoteChannel && !remoteRPCChannel->pairFunction2.empty()) function = remoteRPCChannel->pairFunction2;
		}
		if(function.empty()) return;
		if(linkSet->defaultValues.find(function) == linkSet->defaultValues.end()) return;
		BaseLib::Output::printInfo("Info: Peer " + std::to_string(_peerID) + ": Applying channel function " + function + ".");
		for(BaseLib::RPC::DefaultValue::iterator j = linkSet->defaultValues.at(function).begin(); j != linkSet->defaultValues.at(function).end(); ++j)
		{
			BaseLib::Systems::RPCConfigurationParameter* parameter = &linksCentral[channel][peerAddress][remoteChannel][j->first];
			if(!parameter->rpcParameter) continue;
			parameter->data = parameter->rpcParameter->convertToPacket(j->second);
			saveParameter(parameter->databaseID, parameter->data);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

BidCoSPeer::~BidCoSPeer()
{
	try
	{
		dispose();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
	}
	catch(const std::exception& ex)
    {
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::setPhysicalInterfaceID(std::string id)
{
	if(GD::physicalInterfaces.find(id) != GD::physicalInterfaces.end() && GD::physicalInterfaces.at(id))
	{
		_physicalInterfaceID = id;
		_physicalInterface = GD::physicalInterfaces.at(_physicalInterfaceID);
		std::shared_ptr<HomeMaticDevice> virtualDevice = getHiddenPeerDevice();
		if(virtualDevice) virtualDevice->setPhysicalInterfaceID(id);
	}
}

BidCoSPeer::BidCoSPeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(parentID, centralFeatures, eventHandler)
{
	try
	{
		_team.address = 0;
		if(centralFeatures)
		{
			pendingBidCoSQueues.reset(new PendingBidCoSQueues());
		}
		_physicalInterface = GD::defaultPhysicalInterface;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

BidCoSPeer::BidCoSPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(id, address, serialNumber, parentID, centralFeatures, eventHandler)
{
	_physicalInterface = GD::defaultPhysicalInterface;
}

void BidCoSPeer::worker()
{
	if(!_centralFeatures || _disposing) return;
	std::vector<uint32_t> positionsToDelete;
	std::vector<std::shared_ptr<VariableToReset>> variablesToReset;
	int32_t wakeUpIndex = 0;
	int32_t index;
	int64_t time;
	try
	{
		time = BaseLib::HelperFunctions::getTime();
		if(!_variablesToReset.empty())
		{
			index = 0;
			_variablesToResetMutex.lock();
			for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
			{
				if((*i)->resetTime + 3000 <= time)
				{
					variablesToReset.push_back(*i);
					positionsToDelete.push_back(index);
				}
				index++;
			}
			_variablesToResetMutex.unlock();
			for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = variablesToReset.begin(); i != variablesToReset.end(); ++i)
			{
				if((*i)->isDominoEvent)
				{
					BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral.at((*i)->channel).at((*i)->key);
					parameter->data = (*i)->data;
					saveParameter(parameter->databaseID, parameter->data);
					std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {(*i)->key});
					std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>> { valuesCentral.at((*i)->channel).at((*i)->key).rpcParameter->convertFromPacket((*i)->data) });
					BaseLib::Output::printInfo("Info: Domino event: " + (*i)->key + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string((*i)->channel) + " was reset.");
					raiseRPCEvent(_peerID, (*i)->channel, _serialNumber + ":" + std::to_string((*i)->channel), valueKeys, rpcValues);
				}
				else
				{
					std::shared_ptr<BaseLib::RPC::RPCVariable> rpcValue = valuesCentral.at((*i)->channel).at((*i)->key).rpcParameter->convertFromPacket((*i)->data);
					setValue((*i)->channel, (*i)->key, rpcValue);
				}
			}
			_variablesToResetMutex.lock();
			for(std::vector<uint32_t>::reverse_iterator i = positionsToDelete.rbegin(); i != positionsToDelete.rend(); ++i)
			{
				std::vector<std::shared_ptr<VariableToReset>>::iterator element = _variablesToReset.begin() + *i;
				if(element < _variablesToReset.end()) _variablesToReset.erase(element);
			}
			_variablesToResetMutex.unlock();
			positionsToDelete.clear();
			variablesToReset.clear();
		}
		if(rpcDevice) serviceMessages->checkUnreach(rpcDevice->cyclicTimeout, getLastPacketReceived());
		if(serviceMessages->getConfigPending() && (!pendingBidCoSQueues || pendingBidCoSQueues->empty()))
		{
			serviceMessages->setConfigPending(false);
		}
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_variablesToResetMutex.unlock();
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_variablesToResetMutex.unlock();
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_variablesToResetMutex.unlock();
	}
}

std::string BidCoSPeer::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			stringStream << "channel count\t\tPrint the number of channels of this peer" << std::endl;
			stringStream << "config print\t\tPrints all configuration parameters and their values" << std::endl;
			stringStream << "queues info\t\tPrints information about the pending BidCoS packet queues" << std::endl;
			stringStream << "queues clear\t\tClears pending BidCoS packet queues" << std::endl;
			stringStream << "team info\t\tPrints information about this peers team" << std::endl;
			stringStream << "peers list\t\tLists all peers paired to this peer" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 13, "channel count") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints this peer's number of channels." << std::endl;
						stringStream << "Usage: channel count" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Peer has " << rpcDevice->channels.size() << " channels." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "config print") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints all configuration parameters of this peer. The values are in BidCoS packet format." << std::endl;
						stringStream << "Usage: config print" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			return printConfig();
		}
		else if(command.compare(0, 11, "queues info") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints information about the pending BidCoS queues." << std::endl;
						stringStream << "Usage: queues info" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Number of Pending queues:\t\t\t" << pendingBidCoSQueues->size() << std::endl;
			if(!pendingBidCoSQueues->empty() && !pendingBidCoSQueues->front()->isEmpty())
			{
				stringStream << "First pending queue type:\t\t\t" << (int32_t)pendingBidCoSQueues->front()->getQueueType() << std::endl;
				if(pendingBidCoSQueues->front()->front()->getPacket()) stringStream << "First packet of first pending queue:\t\t" << pendingBidCoSQueues->front()->front()->getPacket()->hexString() << std::endl;
				stringStream << "Type of first entry of first pending queue:\t" << (int32_t)pendingBidCoSQueues->front()->front()->getType() << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "queues clear") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command clears all pending BidCoS queues." << std::endl;
						stringStream << "Usage: queues clear" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			pendingBidCoSQueues->clear();
			stringStream << "All pending BidCoSQueues were deleted." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 9, "team info") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints information about this peers team." << std::endl;
						stringStream << "Usage: team info" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			if(_team.serialNumber.empty()) stringStream << "This peer doesn't support teams." << std::endl;
			else stringStream << "Team address: 0x" << std::hex << _team.address << std::dec << " Team serial number: " << _team.serialNumber << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 10, "peers list") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command lists all peers paired to this peer." << std::endl;
						stringStream << "Usage: peers list" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			if(_peers.empty())
			{
				stringStream << "No peers are paired to this peer." << std::endl;
				return stringStream.str();
			}
			for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
			{
				for(std::vector<std::shared_ptr<BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					stringStream << "Channel: " << i->first << "\tAddress: 0x" << std::hex << (*j)->address << "\tRemote channel: " << std::dec << (*j)->channel << "\tSerial number: " << (*j)->serialNumber << std::endl << std::dec;
				}
			}
			return stringStream.str();
		}
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

void BidCoSPeer::addPeer(int32_t channel, std::shared_ptr<BasicPeer> peer)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == peer->address && (*i)->channel == peer->channel)
			{
				_peers[channel].erase(i);
				break;
			}
		}
		_peers[channel].push_back(peer);
		initializeLinkConfig(channel, peer->address, peer->channel, true);
		savePeers();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}



std::shared_ptr<BasicPeer> BidCoSPeer::getPeer(int32_t channel, std::string serialNumber, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->serialNumber.empty())
			{
				std::shared_ptr<HomeMaticCentral> central = getCentral();
				if(central)
				{
					std::shared_ptr<BidCoSPeer> peer(central->getPeer((*i)->address));
					if(peer) (*i)->serialNumber = peer->getSerialNumber();
				}
			}
			if((*i)->serialNumber == serialNumber && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<HomeMaticDevice> BidCoSPeer::getHiddenPeerDevice()
{
	try
	{
		//This is pretty dirty, but as all hidden peers should be the same device, this should always work
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator j = _peers.begin(); j != _peers.end(); ++j)
		{
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = j->second.begin(); i != j->second.end(); ++i)
			{
				if((*i)->hidden) return getDevice((*i)->address);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<HomeMaticDevice>();
}

std::shared_ptr<BasicPeer> BidCoSPeer::getHiddenPeer(int32_t channel)
{
	try
	{
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->hidden) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> BidCoSPeer::getPeer(int32_t channel, int32_t address, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == address && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

void BidCoSPeer::removePeer(int32_t channel, int32_t address, int32_t remoteChannel)
{
	try
	{
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == address && (*i)->channel == remoteChannel)
			{
				_peers[channel].erase(i);
				if(linksCentral[channel].find(address) != linksCentral[channel].end() && linksCentral[channel][address].find(remoteChannel) != linksCentral[channel][address].end()) linksCentral[channel][address].erase(linksCentral[channel][address].find(remoteChannel));
				_databaseMutex.lock();
				BaseLib::Database::DataRow data;
				data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(_peerID)));
				data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((int32_t)BaseLib::RPC::ParameterSet::Type::Enum::link)));
				data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(channel)));
				data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(address)));
				data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(remoteChannel)));
				raiseDeletePeerParameter(data);
				_databaseMutex.unlock();
				savePeers();
				return;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void BidCoSPeer::save(bool savePeer, bool variables, bool centralConfig)
{
	try
	{
		Peer::save(savePeer, variables, centralConfig);
		if(!variables)
		{
			saveNonCentralConfig();
			saveVariablesToReset();
			savePendingQueues();
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::deletePairedVirtualDevice(int32_t address)
{
	try
	{
		std::shared_ptr<HomeMaticDevice> device(getDevice(address));
		if(device && !device->isCentral())
		{
			BaseLib::Obj::family->remove(device->getID());
			device->reset();
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::deletePairedVirtualDevices()
{
	try
	{
		std::map<int32_t, bool> deleted;
		std::shared_ptr<HomeMaticDevice> device;
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!*j) continue;
				if((*j)->hidden && deleted.find((*j)->address) == deleted.end())
				{
					device = getDevice((*j)->address);
					if(device && !device->isCentral())
					{
						BaseLib::Obj::family->remove(device->getID());
						deleted[(*j)->address] = true;
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::serializePeers(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _peers.size());
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeInteger(encodedData, i->second.size());
			for(std::vector<std::shared_ptr<BasicPeer>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!*j) continue;
				encoder.encodeInteger(encodedData, (*j)->address);
				encoder.encodeInteger(encodedData, (*j)->channel);
				encoder.encodeString(encodedData, (*j)->serialNumber);
				encoder.encodeBoolean(encodedData, (*j)->hidden);
				encoder.encodeString(encodedData, (*j)->linkName);
				encoder.encodeString(encodedData, (*j)->linkDescription);
				encoder.encodeInteger(encodedData, (*j)->data.size());
				encodedData.insert(encodedData.end(), (*j)->data.begin(), (*j)->data.end());
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::unserializePeers(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t peersSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < peersSize; i++)
		{
			uint32_t channel = decoder.decodeInteger(serializedData, position);
			uint32_t peerCount = decoder.decodeInteger(serializedData, position);
			for(uint32_t j = 0; j < peerCount; j++)
			{
				std::shared_ptr<BasicPeer> basicPeer(new BasicPeer());
				basicPeer->address = decoder.decodeInteger(serializedData, position);
				basicPeer->channel = decoder.decodeInteger(serializedData, position);
				basicPeer->serialNumber = decoder.decodeString(serializedData, position);
				basicPeer->hidden = decoder.decodeBoolean(serializedData, position);
				_peers[channel].push_back(basicPeer);
				basicPeer->linkName = decoder.decodeString(serializedData, position);
				basicPeer->linkDescription = decoder.decodeString(serializedData, position);
				uint32_t dataSize = decoder.decodeInteger(serializedData, position);
				if(position + dataSize <= serializedData->size()) basicPeer->data.insert(basicPeer->data.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
				position += dataSize;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::serializeNonCentralConfig(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, config.size());
		for(std::unordered_map<int32_t, int32_t>::const_iterator i = config.begin(); i != config.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeInteger(encodedData, i->second);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::unserializeNonCentralConfig(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		config.clear();
		BaseLib::BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t configSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < configSize; i++)
		{
			int32_t index = decoder.decodeInteger(serializedData, position);
			config[index] = decoder.decodeInteger(serializedData, position);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::serializeVariablesToReset(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder;
		_variablesToResetMutex.lock();
		encoder.encodeInteger(encodedData, _variablesToReset.size());
		for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
		{
			encoder.encodeInteger(encodedData, (*i)->channel);
			encoder.encodeString(encodedData, (*i)->key);
			encoder.encodeInteger(encodedData, (*i)->data.size());
			encodedData.insert(encodedData.end(), (*i)->data.begin(), (*i)->data.end());
			encoder.encodeInteger(encodedData, (*i)->resetTime / 1000);
			encoder.encodeBoolean(encodedData, (*i)->isDominoEvent);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _variablesToResetMutex.unlock();
}

void BidCoSPeer::unserializeVariablesToReset(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		_variablesToResetMutex.lock();
		_variablesToReset.clear();
		_variablesToResetMutex.unlock();
		BaseLib::BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t variablesToResetSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < variablesToResetSize; i++)
		{
			std::shared_ptr<VariableToReset> variable(new VariableToReset());
			variable->channel = decoder.decodeInteger(serializedData, position);
			variable->key = decoder.decodeString(serializedData, position);
			uint32_t dataSize = decoder.decodeInteger(serializedData, position);
			if(position + dataSize <= serializedData->size()) variable->data.insert(variable->data.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
			position += dataSize;
			variable->resetTime = ((int64_t)decoder.decodeInteger(serializedData, position)) * 1000;
			variable->isDominoEvent = decoder.decodeBoolean(serializedData, position);
			try
			{
				_variablesToResetMutex.lock();
				_variablesToReset.push_back(variable);
			}
			catch(const std::exception& ex)
			{
				BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_variablesToResetMutex.unlock();
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::saveVariables()
{
	try
	{
		if(_peerID == 0 || isTeam()) return;
		saveVariable(0, _firmwareVersion);
		saveVariable(1, _remoteChannel);
		saveVariable(2, _localChannel);
		saveVariable(3, (int32_t)_deviceType.type());
		saveVariable(4, _countFromSysinfo);
		saveVariable(5, _messageCounter);
		saveVariable(6, _pairingComplete);
		saveVariable(7, _teamChannel);
		saveVariable(8, _team.address);
		saveVariable(9, _team.channel);
		saveVariable(10, _team.serialNumber);
		saveVariable(11, _team.data);
		savePeers(); //12
		saveNonCentralConfig(); //13
		saveVariablesToReset(); //14
		saveServiceMessages(); //15
		savePendingQueues(); //16
		if(_aesKeyIndex > 0 || _aesKeySendIndex > 0)
		{
			saveVariable(17, _aesKeyIndex);
			saveVariable(18, _aesKeySendIndex);
		}
		saveVariable(19, _physicalInterfaceID);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::savePeers()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializePeers(serializedData);
		saveVariable(12, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::saveNonCentralConfig()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeNonCentralConfig(serializedData);
		saveVariable(13, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::saveVariablesToReset()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeVariablesToReset(serializedData);
		saveVariable(14, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::savePendingQueues()
{
	try
	{
		if(!_centralFeatures || !pendingBidCoSQueues) return;
		std::vector<uint8_t> serializedData;
		pendingBidCoSQueues->serialize(serializedData);
		saveVariable(16, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool BidCoSPeer::pendingQueuesEmpty()
{
	if(!pendingBidCoSQueues) return true;
	return pendingBidCoSQueues->empty();
}

void BidCoSPeer::enqueuePendingQueues()
{
	try
	{
		std::shared_ptr<HomeMaticCentral> central = getCentral();
		if(central)
		{
			BaseLib::Output::printInfo("Info: Queue is not finished (peer: " + std::to_string(_peerID) + "). Retrying...");
			central->enqueuePendingQueues(_address);
		}
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void BidCoSPeer::loadVariables(HomeMaticDevice* device)
{
	try
	{
		_databaseMutex.lock();
		BaseLib::Database::DataTable rows = raiseGetPeerVariables();
		for(BaseLib::Database::DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 0:
				_firmwareVersion = row->second.at(3)->intValue;
				break;
			case 1:
				_remoteChannel = row->second.at(3)->intValue;
				break;
			case 2:
				_localChannel = row->second.at(3)->intValue;
				break;
			case 3:
				_deviceType = BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, row->second.at(3)->intValue);
				if(_deviceType.type() == (uint32_t)DeviceType::none)
				{
					BaseLib::Output::printError("Error loading HomeMatic BidCoS peer " + std::to_string(_peerID) + ": Device type unknown: 0x" + BaseLib::HelperFunctions::getHexString(row->second.at(3)->intValue) + " Firmware version: " + std::to_string(_firmwareVersion));
				}
				break;
			case 4:
				_countFromSysinfo = row->second.at(3)->intValue;
				break;
			case 5:
				_messageCounter = row->second.at(3)->intValue;
				break;
			case 6:
				_pairingComplete = (bool)row->second.at(3)->intValue;
				break;
			case 7:
				_teamChannel = row->second.at(3)->intValue;
				break;
			case 8:
				_team.address = row->second.at(3)->intValue;
				break;
			case 9:
				_team.channel = row->second.at(3)->intValue;
				break;
			case 10:
				_team.serialNumber = row->second.at(4)->textValue;
				break;
			case 11:
				_team.data.insert(_team.data.begin(), row->second.at(5)->binaryValue->begin(), row->second.at(5)->binaryValue->end());
				break;
			case 12:
				unserializePeers(row->second.at(5)->binaryValue);
				break;
			case 13:
				unserializeNonCentralConfig(row->second.at(5)->binaryValue);
				break;
			case 14:
				unserializeVariablesToReset(row->second.at(5)->binaryValue);
				break;
			case 15:
				if(_centralFeatures)
				{
					serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_peerID, _serialNumber, this));
					serviceMessages->unserialize(row->second.at(5)->binaryValue);
				}
				break;
			case 16:
				if(_centralFeatures && device)
				{
					pendingBidCoSQueues.reset(new PendingBidCoSQueues());
					pendingBidCoSQueues->unserialize(row->second.at(5)->binaryValue, this, device);
				}
				break;
			case 17:
				_aesKeyIndex = row->second.at(3)->intValue;
				break;
			case 18:
				_aesKeySendIndex = row->second.at(3)->intValue;
				break;
			case 19:
				_physicalInterfaceID = row->second.at(4)->textValue;
				if(GD::physicalInterfaces.find(_physicalInterfaceID) != GD::physicalInterfaces.end()) _physicalInterface = GD::physicalInterfaces.at(_physicalInterfaceID);
				break;
			}
		}
		if(_centralFeatures)
		{
			if(!serviceMessages)
			{
				serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_peerID, _serialNumber, this));
			}
			if(!pendingBidCoSQueues) pendingBidCoSQueues.reset(new PendingBidCoSQueues());
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	_databaseMutex.unlock();
}

bool BidCoSPeer::load(BaseLib::Systems::LogicalDevice* device)
{
	try
	{
		loadVariables((HomeMaticDevice*)device);

		rpcDevice = BaseLib::Obj::ins->rpcDevices.find(_deviceType, _firmwareVersion, _countFromSysinfo);
		if(!rpcDevice)
		{
			BaseLib::Output::printError("Error loading HomeMatic BidCoS peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		std::string entry;
		uint32_t pos = 0;
		uint32_t dataSize;
		loadConfig();
		initializeCentralConfig();

		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				initializeLinkConfig(i->first, (*j)->address, (*j)->channel, false);
			}
		}

		checkAESKey();

		return true;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void BidCoSPeer::checkAESKey(bool onlyPushing)
{
	try
	{
		if(!rpcDevice || !rpcDevice->supportsAES || !_centralFeatures) return;
		bool aesEnabled = false;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			if(i->second.find("AES_ACTIVE") != i->second.end() && !i->second.at("AES_ACTIVE").data.empty() && (bool)i->second.at("AES_ACTIVE").data.at(0))
			{
				aesEnabled = true;
				break;
			}
		}
		if(!aesEnabled) return;
		if(_aesKeyIndex == _physicalInterface->getCurrentRFKeyIndex())
		{
			BaseLib::Output::printDebug("Debug: AES key of peer " + std::to_string(_peerID) + " is current.");
			return;
		}
		BaseLib::Output::printDebug("Info: Updating AES key of peer " + std::to_string(_peerID) + ".");
		if(_aesKeyIndex > _physicalInterface->getCurrentRFKeyIndex())
		{
			BaseLib::Output::printError("Error: Can't update AES key of peer " + std::to_string(_peerID) + ". Peer's AES key index is larger than the key index defined in physicalinterfaces.conf.");
			return;
		}
		if(_aesKeyIndex > 0 && _physicalInterface->getCurrentRFKeyIndex() - _aesKeyIndex > 1)
		{
			BaseLib::Output::printError("Error: Can't update AES key of peer " + std::to_string(_peerID) + ". AES key seems to be updated more than once since this peer's config was last updated (key indexes differ by more than 1).");
			return;
		}
		if(pendingBidCoSQueues->find(BidCoSQueueType::SETAESKEY)) return;

		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(_physicalInterface, BidCoSQueueType::SETAESKEY));
		queue->noSending = true;
		std::vector<uint8_t> payload;
		std::shared_ptr<HomeMaticCentral> central = getCentral();
		bool firstPacket = true;

		payload.push_back(1);
		payload.push_back(_aesKeySendIndex);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter, 0xA0, 0x04, central->getAddress(), _address, payload));
		queue->push(configPacket);
		queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		setMessageCounter(_messageCounter + 1);

		payload.push_back(1);
		payload.push_back(_aesKeySendIndex + 1);
		configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x04, central->getAddress(), _address, payload));
		queue->push(configPacket);
		queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		setMessageCounter(_messageCounter + 1);

		pendingBidCoSQueues->push(queue);
		if(serviceMessages) serviceMessages->setConfigPending(true);
		if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::always) || (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst))
		{
			if(!onlyPushing) getCentral()->enqueuePendingQueues(_address);
		}
		else
		{
			BaseLib::Output::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string BidCoSPeer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "No RPC parameter";
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		stringStream << "VALUES" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "No RPC parameter";
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		stringStream << "LINK" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>>>::const_iterator i = linksCentral.begin(); i != linksCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t" << "Address: " << std::hex << "0x" << j->first << std::endl;
				stringStream << "\t\t{" << std::endl;
				for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					stringStream << "\t\t\t" << "Remote channel: " << std::dec << k->first << std::endl;
					stringStream << "\t\t\t{" << std::endl;
					for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator l = k->second.begin(); l != k->second.end(); ++l)
					{
						stringStream << "\t\t\t\t[" << l->first << "]: ";
						if(!l->second.rpcParameter) stringStream << "No RPC parameter";
						for(std::vector<uint8_t>::const_iterator m = l->second.data.begin(); m != l->second.data.end(); ++m)
						{
							stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*m << " ";
						}
						stringStream << std::endl;
					}
					stringStream << "\t\t\t}" << std::endl;
				}
				stringStream << "\t\t}" << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;
		return stringStream.str();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

IBidCoSInterface::PeerInfo BidCoSPeer::getPeerInfo()
{
	try
	{
		IBidCoSInterface::PeerInfo peerInfo;
		peerInfo.address = _address;
		if(!rpcDevice) return peerInfo;
		if(serviceMessages->getConfigPending() && (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::wakeUp))
		{
			peerInfo.wakeUp = true;
		}
		if(_physicalInterface->aesSupported() && rpcDevice->supportsAES)
		{
			peerInfo.keyIndex = _aesKeyIndex;
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				if(!i->second) continue;
				if(configCentral.find(i->first) == configCentral.end() || configCentral.at(i->first).find("AES_ACTIVE") == configCentral.at(i->first).end())
				{
					peerInfo.aesChannels[i->first] = i->second->aesDefault;
				}
				else
				{
					peerInfo.aesChannels[i->first] = (bool)configCentral.at(i->first).at("AES_ACTIVE").data.at(0);
				}
			}
		}
		return peerInfo;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return IBidCoSInterface::PeerInfo();
}

void BidCoSPeer::onConfigPending(bool configPending)
{
	try
	{
		Peer::onConfigPending(configPending);

		if(configPending)
		{
			if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::wakeUp) && _physicalInterface->autoResend())
			{
				BaseLib::Output::printDebug("Debug: Setting physical device's wake up flag.");
				_physicalInterface->addPeer(getPeerInfo());
			}
		}
		else
		{
			if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::wakeUp) && _physicalInterface->autoResend())
			{
				BaseLib::Output::printDebug("Debug: Removing physical device's wake up flag.");
				_physicalInterface->addPeer(getPeerInfo());
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int32_t BidCoSPeer::getChannelGroupedWith(int32_t channel)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return -1;
		if(rpcDevice->channels.at(channel)->paired)
		{
			uint32_t firstGroupChannel = 0;
			//Find first channel of group
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				if(i->second->paired)
				{
					firstGroupChannel = i->first;
					break;
				}
			}
			uint32_t groupedWith = 0;
			if((channel - firstGroupChannel) % 2 == 0) groupedWith = channel + 1; //Grouped with next channel
			else groupedWith = channel - 1; //Grouped with last channel
			if(rpcDevice->channels.find(groupedWith) != rpcDevice->channels.end())
			{
				return groupedWith;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return -1;
}

int32_t BidCoSPeer::getNewFirmwareVersion()
{
	try
	{
		std::string filenamePrefix = BaseLib::HelperFunctions::getHexString((int32_t)BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, 4) + "." + BaseLib::HelperFunctions::getHexString(_deviceType.type(), 8);
		std::string versionFile(BaseLib::Obj::ins->settings.firmwarePath() + filenamePrefix + ".version");
		if(!BaseLib::HelperFunctions::fileExists(versionFile)) return 0;
		std::string versionHex = BaseLib::HelperFunctions::getFileContent(versionFile);
		return BaseLib::HelperFunctions::getNumber(versionHex, true);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return 0;
}

bool BidCoSPeer::firmwareUpdateAvailable()
{
	try
	{
		return _firmwareVersion > 0 && _firmwareVersion < getNewFirmwareVersion();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return false;
}

void BidCoSPeer::getValuesFromPacket(std::shared_ptr<BidCoSPacket> packet, std::vector<FrameValues>& frameValues)
{
	try
	{
		if(!rpcDevice) return;
		//equal_range returns all elements with "0" or an unknown element as argument
		if(packet->messageType() == 0 || rpcDevice->framesByMessageType.find(packet->messageType()) == rpcDevice->framesByMessageType.end()) return;
		std::pair<std::multimap<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator,std::multimap<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByMessageType.equal_range((uint32_t)packet->messageType());
		if(range.first == rpcDevice->framesByMessageType.end()) return;
		std::multimap<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator i = range.first;
		do
		{
			FrameValues currentFrameValues;
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame(i->second);
			if(!frame) continue;
			if(frame->direction == BaseLib::RPC::DeviceFrame::Direction::Enum::fromDevice && packet->senderAddress() != _address && (!hasTeam() || packet->senderAddress() != _team.address)) continue;
			if(frame->direction == BaseLib::RPC::DeviceFrame::Direction::Enum::toDevice && packet->destinationAddress() != _address) continue;
			if(packet->payload()->empty()) continue;
			if(frame->subtype > -1 && frame->subtypeIndex >= 9 && (signed)packet->payload()->size() > (frame->subtypeIndex - 9) && packet->payload()->at(frame->subtypeIndex - 9) != (unsigned)frame->subtype) continue;
			int32_t channelIndex = frame->channelField;
			int32_t channel = -1;
			if(channelIndex >= 9 && (signed)packet->payload()->size() > (channelIndex - 9)) channel = packet->payload()->at(channelIndex - 9);
			if(channel > -1 && frame->channelFieldSize < 1.0) channel &= (0xFF >> (8 - std::lround(frame->channelFieldSize * 10) % 10));
			if(frame->fixedChannel > -1) channel = frame->fixedChannel;
			currentFrameValues.frameID = frame->id;

			for(std::vector<BaseLib::RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				std::vector<uint8_t> data;
				if(j->size > 0 && j->index > 0)
				{
					if(((int32_t)j->index) - 9 >= (signed)packet->payload()->size()) continue;
					data = packet->getPosition(j->index, j->size, -1);

					if(j->constValue > -1)
					{
						int32_t intValue = 0;
						BaseLib::HelperFunctions::memcpyBigEndian(intValue, data);
						if(intValue != j->constValue) break; else continue;
					}
				}
				else if(j->constValue > -1)
				{
					BaseLib::HelperFunctions::memcpyBigEndian(data, j->constValue);
				}
				else continue;
				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
				{
					if((*k)->physicalParameter->valueID != j->param) continue;
					currentFrameValues.parameterSetType = (*k)->parentParameterSet->type;
					bool setValues = false;
					if(currentFrameValues.paramsetChannels.empty()) //Fill paramsetChannels
					{
						int32_t startChannel = (channel < 0) ? 0 : channel;
						int32_t endChannel;
						//When fixedChannel is -2 (means '*') cycle through all channels
						if(frame->fixedChannel == -2)
						{
							startChannel = 0;
							endChannel = (rpcDevice->channels.end()--)->first;
						}
						else endChannel = startChannel;
						for(uint32_t l = startChannel; l <= endChannel; l++)
						{
							if(rpcDevice->channels.find(l) == rpcDevice->channels.end()) continue;
							if(rpcDevice->channels.at(l)->parameterSets.find(currentFrameValues.parameterSetType) == rpcDevice->channels.at(l)->parameterSets.end()) continue;
							if(!rpcDevice->channels.at(l)->parameterSets.at(currentFrameValues.parameterSetType)->getParameter((*k)->id)) continue;
							currentFrameValues.paramsetChannels.push_back(l);
							currentFrameValues.values[(*k)->id].channels.push_back(l);
							setValues = true;
						}
					}
					else //Use paramsetChannels
					{
						for(std::list<uint32_t>::const_iterator l = currentFrameValues.paramsetChannels.begin(); l != currentFrameValues.paramsetChannels.end(); ++l)
						{
							if(rpcDevice->channels.find(*l) == rpcDevice->channels.end()) continue;
							if(rpcDevice->channels.at(*l)->parameterSets.find(currentFrameValues.parameterSetType) == rpcDevice->channels.at(*l)->parameterSets.end()) continue;
							if(!rpcDevice->channels.at(*l)->parameterSets.at(currentFrameValues.parameterSetType)->getParameter((*k)->id)) continue;
							currentFrameValues.values[(*k)->id].channels.push_back(*l);
							setValues = true;
						}
					}
					if(setValues) currentFrameValues.values[(*k)->id].value = data;
				}
			}
			if(!currentFrameValues.values.empty()) frameValues.push_back(currentFrameValues);
		} while(++i != range.second && i != rpcDevice->framesByMessageType.end());
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::handleDominoEvent(std::shared_ptr<BaseLib::RPC::Parameter> parameter, std::string& frameID, uint32_t channel)
{
	try
	{
		if(!parameter || !parameter->hasDominoEvents) return;
		for(std::vector<std::shared_ptr<BaseLib::RPC::PhysicalParameterEvent>>::iterator j = parameter->physicalParameter->eventFrames.begin(); j != parameter->physicalParameter->eventFrames.end(); ++j)
		{
			if((*j)->frame != frameID) continue;
			if(!(*j)->dominoEvent) continue;
			if(!_variablesToReset.empty())
			{
				_variablesToResetMutex.lock();
				for(std::vector<std::shared_ptr<VariableToReset>>::iterator k = _variablesToReset.begin(); k != _variablesToReset.end(); ++k)
				{
					if((*k)->channel == channel && (*k)->key == parameter->id)
					{
						BaseLib::Output::printDebug("Debug: Deleting element " + parameter->id + " from _variablesToReset. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frameID);
						_variablesToReset.erase(k);
						break; //The key should only be once in the vector, so breaking is ok and we can't continue as the iterator is invalidated.
					}
				}
				_variablesToResetMutex.unlock();
			}
			std::shared_ptr<BaseLib::RPC::Parameter> delayParameter = rpcDevice->channels.at(channel)->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::values)->getParameter((*j)->dominoEventDelayID);
			if(!delayParameter) continue;
			int64_t delay = delayParameter->convertFromPacket(valuesCentral[channel][(*j)->dominoEventDelayID].data)->integerValue;
			if(delay < 0) continue; //0 is allowed. When 0 the parameter will be reset immediately
			int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			std::shared_ptr<VariableToReset> variable(new VariableToReset);
			variable->channel = channel;
			BaseLib::HelperFunctions::memcpyBigEndian(variable->data, (*j)->dominoEventValue);
			variable->resetTime = time + (delay * 1000);
			variable->key = parameter->id;
			variable->isDominoEvent = true;
			_variablesToResetMutex.lock();
			_variablesToReset.push_back(variable);
			_variablesToResetMutex.unlock();
			BaseLib::Output::printDebug("Debug: " + parameter->id + " will be reset in " + std::to_string((variable->resetTime - time) / 1000) + "s.", 5);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void BidCoSPeer::setRSSIDevice(uint8_t rssi)
{
	try
	{
		if(!_centralFeatures || _disposing || rssi == 0) return;
		uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if(valuesCentral.find(0) != valuesCentral.end() && valuesCentral.at(0).find("RSSI_DEVICE") != valuesCentral.at(0).end() && (time - _lastRSSIDevice) > 10)
		{
			_lastRSSIDevice = time;
			BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral.at(0).at("RSSI_DEVICE");
			parameter->data.at(0) = rssi;

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("RSSI_DEVICE")}));
			std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
			rpcValues->push_back(parameter->rpcParameter->convertFromPacket(parameter->data));

			raiseRPCEvent(_peerID, 0, _serialNumber + ":0", valueKeys, rpcValues);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool BidCoSPeer::hasLowbatBit(std::shared_ptr<BaseLib::RPC::DeviceFrame> frame)
{
	try
	{
		//Three things to check to see if position 9.7 is used: channelField, subtypeIndex and parameter indices
		if(frame->channelField == 9 && frame->channelFieldSize >= 0.8) return false;
		else if(frame->subtypeIndex == 9) return false;
		for(std::vector<BaseLib::RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
		{
			if(j->index >= 9 && j->index < 10)
			{
				//fmod is needed for sizes > 1 (e. g. for frame WEATHER_EVENT)
				//9.8 is not working, result is 9.7999999
				if((j->index + std::fmod(j->size, 1)) >= 9.79) return false;
			}
		}
		return true;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void BidCoSPeer::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(!packet) return;
		if(!_centralFeatures || _disposing) return;
		if(packet->senderAddress() != _address && (!hasTeam() || packet->senderAddress() != _team.address || packet->destinationAddress() == getCentral()->getAddress())) return;
		if(!rpcDevice) return;
		setLastPacketReceived();
		setRSSIDevice(packet->rssiDevice());
		serviceMessages->endUnreach();
		std::vector<FrameValues> frameValues;
		getValuesFromPacket(packet, frameValues);
		//bool resendPacket = false;
		std::shared_ptr<BidCoSPacket> sentPacket;
		std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> valueKeys;
		std::map<uint32_t, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>>> rpcValues;
		//Loop through all matching frames
		for(std::vector<FrameValues>::iterator a = frameValues.begin(); a != frameValues.end(); ++a)
		{
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame;
			if(!a->frameID.empty()) frame = rpcDevice->framesByID.at(a->frameID);

			std::vector<FrameValues> sentFrameValues;
			bool pushPendingQueues = false;
			if(packet->messageType() == 0x02) //ACK packet: Check if all values were set correctly. If not set value again
			{
				sentPacket = getCentral()->getSentPacket(_address);
				if(sentPacket && sentPacket->messageType() > 0 && !sentPacket->payload()->empty())
				{
					BaseLib::RPC::ParameterSet::Type::Enum sentParameterSetType;
					getValuesFromPacket(sentPacket, sentFrameValues);
				}
			}
			//Check for low battery
			//If values is not empty, packet is valid
			if(rpcDevice->hasBattery && !a->values.empty() && !packet->payload()->empty() && frame && hasLowbatBit(frame))
			{
				if(packet->payload()->at(0) & 0x80) serviceMessages->set("LOWBAT", true);
				else serviceMessages->set("LOWBAT", false);
			}

			for(std::map<std::string, FrameValue>::iterator i = a->values.begin(); i != a->values.end(); ++i)
			{
				for(std::list<uint32_t>::const_iterator j = a->paramsetChannels.begin(); j != a->paramsetChannels.end(); ++j)
				{
					if(std::find(i->second.channels.begin(), i->second.channels.end(), *j) == i->second.channels.end()) continue;
					if(!valueKeys[*j] || !rpcValues[*j])
					{
						valueKeys[*j].reset(new std::vector<std::string>());
						rpcValues[*j].reset(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
					}

					BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
					parameter->data = i->second.value;
					saveParameter(parameter->databaseID, parameter->data);
					if(BaseLib::Obj::ins->debugLevel >= 4) BaseLib::Output::printInfo("Info: " + i->first + " of HomeMatic BidCoS peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(i->second.value) + ".");

					 //Process service messages
					if(parameter->rpcParameter && (parameter->rpcParameter->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !i->second.value.empty())
					{
						if(parameter->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeEnum)
						{
							serviceMessages->set(i->first, i->second.value.at(0), *j);
						}
						else if(parameter->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
						{
							serviceMessages->set(i->first, (bool)i->second.value.at(0));
						}
					}

					valueKeys[*j]->push_back(i->first);
					rpcValues[*j]->push_back(rpcDevice->channels.at(*j)->parameterSets.at(a->parameterSetType)->getParameter(i->first)->convertFromPacket(i->second.value, true));
				}
			}

			if(isTeam() && !valueKeys.empty())
			{
				//Set SENDERADDRESS so that the we can identify the sending peer in our home automation software
				std::shared_ptr<BidCoSPeer> senderPeer(getCentral()->getPeer(packet->destinationAddress()));
				if(senderPeer)
				{
					//Check for low battery
					//If values is not empty, packet is valid
					if(senderPeer->rpcDevice->hasBattery && !a->values.empty() && !packet->payload()->empty() && frame && hasLowbatBit(frame))
					{
						if(packet->payload()->at(0) & 0x80) senderPeer->serviceMessages->set("LOWBAT", true);
						else senderPeer->serviceMessages->set("LOWBAT", false);
					}
					for(std::list<uint32_t>::const_iterator i = a->paramsetChannels.begin(); i != a->paramsetChannels.end(); ++i)
					{
						std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter(rpcDevice->channels.at(*i)->parameterSets.at(a->parameterSetType)->getParameter("SENDERADDRESS"));
						if(rpcParameter)
						{
							BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[*i]["SENDERADDRESS"];
							parameter->data = rpcParameter->convertToPacket(senderPeer->getSerialNumber() + ":" + std::to_string(*i));
							saveParameter(parameter->databaseID, parameter->data);
							valueKeys[*i]->push_back("SENDERADDRESS");
							rpcValues[*i]->push_back(rpcParameter->convertFromPacket(parameter->data, true));
						}
					}
				}
			}

			//We have to do this in a seperate loop, because all parameters need to be set first
			for(std::map<std::string, FrameValue>::iterator i = a->values.begin(); i != a->values.end(); ++i)
			{
				for(std::list<uint32_t>::const_iterator j = a->paramsetChannels.begin(); j != a->paramsetChannels.end(); ++j)
				{
					if(std::find(i->second.channels.begin(), i->second.channels.end(), *j) == i->second.channels.end()) continue;
					handleDominoEvent(rpcDevice->channels.at(*j)->parameterSets.at(a->parameterSetType)->getParameter(i->first), a->frameID, *j);
				}
			}
		}
		if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::wakeUp) && packet->senderAddress() == _address && (packet->controlByte() & 2) && pendingBidCoSQueues && !pendingBidCoSQueues->empty()) //Packet is wake me up packet and not bidirectional
		{
			if(packet->controlByte() & 32) //Bidirectional?
			{
				std::vector<uint8_t> payload;
				payload.push_back(0x00);
				std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(packet->messageCounter(), 0x81, 0x02, getCentral()->getAddress(), _address, payload));
				getCentral()->sendPacket(_physicalInterface, ok);
				getCentral()->enqueuePendingQueues(_address);
			}
			else
			{
				std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(_physicalInterface, BidCoSQueueType::DEFAULT));
				queue->noSending = true;
				std::vector<uint8_t> payload;
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(packet->messageCounter(), 0xA1, 0x12, getCentral()->getAddress(), _address, payload));
				queue->push(configPacket);
				queue->push(getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));

				getCentral()->enqueuePackets(_address, queue, true);
			}
		}
		else if((packet->controlByte() & 0x20) && packet->destinationAddress() == getCentral()->getAddress())
		{
			getCentral()->sendOK(packet->messageCounter(), packet->senderAddress());
		}

		//if(!rpcValues.empty() && !resendPacket)
		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::const_iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;
				std::string address(_serialNumber + ":" + std::to_string(j->first));
				raiseEvent(_peerID, j->first, j->second, rpcValues.at(j->first));
				raiseRPCEvent(_peerID, j->first, address, j->second, rpcValues.at(j->first));
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getDeviceDescription(int32_t channel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> description(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

		std::string type;
		std::shared_ptr<BaseLib::RPC::DeviceType> rpcDeviceType = rpcDevice->getType(_deviceType, _firmwareVersion);
		if(rpcDeviceType) type = rpcDeviceType->id;
		else if(_deviceType.type() == (uint32_t)DeviceType::HMRCV50) type = "HM-RCV-50";
		else
		{
			if(!rpcDevice->supportedTypes.empty()) type = rpcDevice->supportedTypes.at(0)->id;
		}

		if(channel == -1) //Base device
		{
			description->structValue->insert(BaseLib::RPC::RPCStructElement("FAMILY", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("FAMILY_STRING", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::Obj::family->getName()))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)_peerID))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber))));

			std::shared_ptr<BaseLib::RPC::RPCVariable> variable = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
			std::shared_ptr<BaseLib::RPC::RPCVariable> variable2 = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("CHILDREN", variable));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("CHANNELS", variable2));

			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				if(i->second->hidden) continue;
				variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber + ":" + std::to_string(i->first))));
				variable2->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->first)));
			}

			if(_firmwareVersion != 0)
			{
				std::ostringstream stringStream;
				stringStream << std::setw(2) << std::hex << _firmwareVersion << std::dec;
				std::string firmware = stringStream.str();
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(firmware.substr(0, 1) + "." + firmware.substr(1)))));
			}
			else
			{
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("?")))));
			}

			int32_t newFirmwareVersion = getNewFirmwareVersion();
			if(newFirmwareVersion > _firmwareVersion)
			{
				std::ostringstream stringStream;
				stringStream << std::setw(2) << std::hex << newFirmwareVersion << std::dec;
				std::string firmware = stringStream.str();
				description->structValue->insert(BaseLib::RPC::RPCStructElement("AVAILABLE_FIRMWARE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(firmware.substr(0, 1) + "." + firmware.substr(1)))));
			}

			int32_t uiFlags = (int32_t)rpcDevice->uiFlags;
			if(isTeam()) uiFlags |= BaseLib::RPC::Device::UIFlags::dontdelete;
			description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(uiFlags))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(getCentral()->getSerialNumber()))));

			variable = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("PARAMSETS", variable));
			variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("MASTER")))); //Always MASTER

			description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("")))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("PHYSICAL_ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::to_string(_address)))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::to_string(_address)))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("ROAMING", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(0))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("RX_MODE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)rpcDevice->rxModes))));

			if(!type.empty()) description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(type))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("VERSION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcDevice->version))));
		}
		else
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
			std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
			if(rpcChannel->hidden) return description;

			description->structValue->insert(BaseLib::RPC::RPCStructElement("FAMILY", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("FAMILY_STRING", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::Obj::family->getName()))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)_peerID))));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));

			int32_t aesActive = 0;
			if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && !configCentral.at(channel).at("AES_ACTIVE").data.empty() && configCentral.at(channel).at("AES_ACTIVE").data.at(0) != 0)
			{
				aesActive = 1;
			}
			description->structValue->insert(BaseLib::RPC::RPCStructElement("AES_ACTIVE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(aesActive))));

			int32_t direction = 0;
			std::ostringstream linkSourceRoles;
			std::ostringstream linkTargetRoles;
			if(rpcChannel->linkRoles)
			{
				for(std::vector<std::string>::iterator k = rpcChannel->linkRoles->sourceNames.begin(); k != rpcChannel->linkRoles->sourceNames.end(); ++k)
				{
					//Probably only one direction is supported, but just in case I use the "or"
					if(!k->empty())
					{
						if(direction & 1) linkSourceRoles << " ";
						linkSourceRoles << *k;
						direction |= 1;
					}
				}
				for(std::vector<std::string>::iterator k = rpcChannel->linkRoles->targetNames.begin(); k != rpcChannel->linkRoles->targetNames.end(); ++k)
				{
					//Probably only one direction is supported, but just in case I use the "or"
					if(!k->empty())
					{
						if(direction & 2) linkTargetRoles << " ";
						linkTargetRoles << *k;
						direction |= 2;
					}
				}
			}

			//Overwrite direction when manually set
			if(rpcChannel->direction != BaseLib::RPC::DeviceChannel::Direction::Enum::none) direction = (int32_t)rpcChannel->direction;
			description->structValue->insert(BaseLib::RPC::RPCStructElement("DIRECTION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(direction))));

			int32_t uiFlags = (int32_t)rpcChannel->uiFlags;
			if(isTeam()) uiFlags |= BaseLib::RPC::DeviceChannel::UIFlags::dontdelete;
			description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(uiFlags))));

			int32_t groupedWith = getChannelGroupedWith(channel);
			if(groupedWith > -1)
			{
				description->structValue->insert(BaseLib::RPC::RPCStructElement("GROUP", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber + ":" + std::to_string(groupedWith)))));
			}

			description->structValue->insert(BaseLib::RPC::RPCStructElement("INDEX", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(channel))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("LINK_SOURCE_ROLES", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(linkSourceRoles.str()))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("LINK_TARGET_ROLES", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(linkTargetRoles.str()))));

			std::shared_ptr<BaseLib::RPC::RPCVariable> variable = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
			description->structValue->insert(BaseLib::RPC::RPCStructElement("PARAMSETS", variable));
			for(std::map<BaseLib::RPC::ParameterSet::Type::Enum, std::shared_ptr<BaseLib::RPC::ParameterSet>>::iterator j = rpcChannel->parameterSets.begin(); j != rpcChannel->parameterSets.end(); ++j)
			{
				variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->second->typeString())));
			}
			//if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::link) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::link)->typeString())));
			//if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::master) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::master)->typeString())));
			//if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::values) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::values)->typeString())));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber))));

			if(!type.empty()) description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT_TYPE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(type))));

			if(!teamChannels.empty() && !rpcChannel->teamTag.empty())
			{
				std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
				for(std::vector<std::pair<std::string, uint32_t>>::iterator j = teamChannels.begin(); j != teamChannels.end(); ++j)
				{
					array->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->first + ":" + std::to_string(j->second))));
				}
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TEAM_CHANNELS", array));
			}

			if(!_team.serialNumber.empty() && rpcChannel->hasTeam)
			{
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TEAM", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_team.serialNumber))));

				description->structValue->insert(BaseLib::RPC::RPCStructElement("TEAM_TAG", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcChannel->teamTag))));
			}
			else if(!_serialNumber.empty() && _serialNumber[0] == '*' && !rpcChannel->teamTag.empty())
			{
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TEAM_TAG", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcChannel->teamTag))));
			}

			description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcChannel->type))));

			description->structValue->insert(BaseLib::RPC::RPCStructElement("VERSION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcDevice->version))));
		}
		return description;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> BidCoSPeer::getDeviceDescription()
{
	try
	{
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> descriptions(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
		descriptions->push_back(getDeviceDescription(-1));

		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> description = getDeviceDescription(i->first);
			if(!description->structValue->empty()) descriptions->push_back(description);
		}

		return descriptions;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>>();
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getParamsetId(uint32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BasicPeer> remotePeer;
		if(type == BaseLib::RPC::ParameterSet::Type::link && !remoteSerialNumber.empty())
		{
			remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);
			if(!remotePeer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown remote peer.");
		}

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcDevice->channels[channel]->parameterSets[type]->id));
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::putParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> variables, bool onlyPushing)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty()) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));

		if(type == BaseLib::RPC::ParameterSet::Type::Enum::master)
		{
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> changedParameters;
			//allParameters is necessary to temporarily store all values. It is used to set changedParameters.
			//This is necessary when there are multiple variables per index and not all of them are changed.
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> allParameters;
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::vector<uint8_t> value;
				if(configCentral[channel].find(i->first) == configCentral[channel].end()) continue;
				BaseLib::Systems::RPCConfigurationParameter* parameter = &configCentral[channel][i->first];
				if(!parameter->rpcParameter) continue;
				value = parameter->rpcParameter->convertToPacket(i->second);
				std::vector<uint8_t> shiftedValue = value;
				parameter->rpcParameter->adjustBitPosition(shiftedValue);
				int32_t intIndex = (int32_t)parameter->rpcParameter->physicalParameter->index;
				int32_t list = parameter->rpcParameter->physicalParameter->list;
				if(list == 9999) list = 0;
				if(allParameters[list].find(intIndex) == allParameters[list].end()) allParameters[list][intIndex] = shiftedValue;
				else
				{
					uint32_t index = 0;
					for(std::vector<uint8_t>::iterator j = shiftedValue.begin(); j != shiftedValue.end(); ++j)
					{
						if(index >= allParameters[list][intIndex].size()) allParameters[list][intIndex].push_back(0);
						allParameters[list][intIndex].at(index) |= *j;
						index++;
					}
				}
				parameter->data = value;
				saveParameter(parameter->databaseID, parameter->data);
				BaseLib::Output::printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " and channel " + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(allParameters[list][intIndex]) + ".");
				//Only send to device when parameter is of type config
				if(parameter->rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::config && parameter->rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::configString) continue;
				changedParameters[list][intIndex] = allParameters[list][intIndex];
			}

			if(changedParameters.empty() || changedParameters.begin()->second.empty()) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));

			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(_physicalInterface, BidCoSQueueType::CONFIG));
			queue->noSending = true;
			std::vector<uint8_t> payload;
			std::shared_ptr<HomeMaticCentral> central = getCentral();
			bool firstPacket = true;

			for(std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>>::iterator i = changedParameters.begin(); i != changedParameters.end(); ++i)
			{
				//CONFIG_START
				payload.push_back(channel);
				payload.push_back(0x05);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(i->first);
				uint8_t controlByte = 0xA0;
				//Only send first packet as burst packet
				if(firstPacket && (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst)) controlByte |= 0x10;
				firstPacket = false;
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter, controlByte, 0x01, central->getAddress(), _address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				setMessageCounter(_messageCounter + 1);

				//CONFIG_WRITE_INDEX
				payload.push_back(channel);
				payload.push_back(0x08);
				for(std::map<int32_t, std::vector<uint8_t>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					int32_t index = j->first;
					for(std::vector<uint8_t>::iterator k = j->second.begin(); k != j->second.end(); ++k)
					{
						payload.push_back(index);
						payload.push_back(*k);
						index++;
						if(payload.size() == 16)
						{
							configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x01, central->getAddress(), _address, payload));
							queue->push(configPacket);
							queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
							payload.clear();
							setMessageCounter(_messageCounter + 1);
							payload.push_back(channel);
							payload.push_back(0x08);
						}
					}
				}
				if(payload.size() > 2)
				{
					configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x01, central->getAddress(), _address, payload));
					queue->push(configPacket);
					queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
					payload.clear();
					setMessageCounter(_messageCounter + 1);
				}
				else payload.clear();

				//END_CONFIG
				payload.push_back(channel);
				payload.push_back(0x06);
				configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x01, central->getAddress(), _address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				setMessageCounter(_messageCounter + 1);
			}

			pendingBidCoSQueues->push(queue);
			serviceMessages->setConfigPending(true);
			if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::always) || (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst))
			{
				if(!onlyPushing) getCentral()->enqueuePendingQueues(_address);
			}
			else
			{
				BaseLib::Output::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
			}
			raiseRPCUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), 0);
		}
		else if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
		{
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				setValue(channel, i->first, i->second);
			}
		}
		else if(type == BaseLib::RPC::ParameterSet::Type::Enum::link)
		{
			std::shared_ptr<BasicPeer> remotePeer;
			if(!remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);
			if(!remotePeer) return BaseLib::RPC::RPCVariable::createError(-3, "Not paired to this peer.");
			if(linksCentral[channel].find(remotePeer->address) == linksCentral[channel].end()) BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
			if(linksCentral[channel][_address].find(remotePeer->channel) == linksCentral[channel][_address].end()) BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");

			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> changedParameters;
			//allParameters is necessary to temporarily store all values. It is used to set changedParameters.
			//This is necessary when there are multiple variables per index and not all of them are changed.
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> allParameters;
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::vector<uint8_t> value;
				if(linksCentral[channel][remotePeer->address][remotePeer->channel].find(i->first) == linksCentral[channel][remotePeer->address][remotePeer->channel].end()) continue;
				BaseLib::Systems::RPCConfigurationParameter* parameter = &linksCentral[channel][remotePeer->address][remotePeer->channel][i->first];
				if(!parameter->rpcParameter) continue;
				value = parameter->rpcParameter->convertToPacket(i->second);
				std::vector<uint8_t> shiftedValue = value;
				parameter->rpcParameter->adjustBitPosition(shiftedValue);
				int32_t intIndex = (int32_t)parameter->rpcParameter->physicalParameter->index;
				int32_t list = parameter->rpcParameter->physicalParameter->list;
				if(list == 9999) list = 0;
				if(allParameters[list].find(intIndex) == allParameters[list].end()) allParameters[list][intIndex] = shiftedValue;
				else
				{
					uint32_t index = 0;
					for(std::vector<uint8_t>::iterator j = shiftedValue.begin(); j != shiftedValue.end(); ++j)
					{
						if(index >= allParameters[list][intIndex].size()) allParameters[list][intIndex].push_back(0);
						allParameters[list][intIndex].at(index) |= *j;
						index++;
					}
				}
				parameter->data = value;
				saveParameter(parameter->databaseID, parameter->data);
				BaseLib::Output::printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " and channel " + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(allParameters[list][intIndex]) + ".");
				//Only send to device when parameter is of type config
				if(parameter->rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::config && parameter->rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::configString) continue;
				changedParameters[list][intIndex] = allParameters[list][intIndex];
			}

			if(changedParameters.empty() || changedParameters.begin()->second.empty()) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));

			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(_physicalInterface, BidCoSQueueType::CONFIG));
			queue->noSending = true;
			std::vector<uint8_t> payload;
			std::shared_ptr<HomeMaticCentral> central = getCentral();
			bool firstPacket = true;

			for(std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>>::iterator i = changedParameters.begin(); i != changedParameters.end(); ++i)
			{
				//CONFIG_START
				payload.push_back(channel);
				payload.push_back(0x05);
				payload.push_back(remotePeer->address >> 16);
				payload.push_back((remotePeer->address >> 8) & 0xFF);
				payload.push_back(remotePeer->address & 0xFF);
				payload.push_back(remotePeer->channel);
				payload.push_back(i->first);
				uint8_t controlByte = 0xA0;
				//Only send first packet as burst packet
				if(firstPacket && (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst)) controlByte |= 0x10;
				firstPacket = false;
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter, controlByte, 0x01, central->getAddress(), _address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				setMessageCounter(_messageCounter + 1);

				//CONFIG_WRITE_INDEX
				payload.push_back(channel);
				payload.push_back(0x08);
				for(std::map<int32_t, std::vector<uint8_t>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					int32_t index = j->first;
					for(std::vector<uint8_t>::iterator k = j->second.begin(); k != j->second.end(); ++k)
					{
						payload.push_back(index);
						payload.push_back(*k);
						index++;
						if(payload.size() == 16)
						{
							configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x01, central->getAddress(), _address, payload));
							queue->push(configPacket);
							queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
							payload.clear();
							setMessageCounter(_messageCounter + 1);
							payload.push_back(channel);
							payload.push_back(0x08);
						}
					}
				}
				if(payload.size() > 2)
				{
					configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x01, central->getAddress(), _address, payload));
					queue->push(configPacket);
					queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
					payload.clear();
					setMessageCounter(_messageCounter + 1);
				}
				else payload.clear();

				//END_CONFIG
				payload.push_back(channel);
				payload.push_back(0x06);
				configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter, 0xA0, 0x01, central->getAddress(), _address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				setMessageCounter(_messageCounter + 1);
			}

			pendingBidCoSQueues->push(queue);
			serviceMessages->setConfigPending(true);
			if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::always) || (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst))
			{
				if(!onlyPushing) getCentral()->enqueuePendingQueues(_address);
			}
			else
			{
				BaseLib::Output::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
			}
			raiseRPCUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), 0);
		}
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<BaseLib::RPC::RPCVariable> variables(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == BaseLib::RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);

		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::internal)) continue;
			std::shared_ptr<BaseLib::RPC::RPCVariable> element;
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
			{
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = valuesCentral[channel][(*i)->id].rpcParameter->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}
			else if(type == BaseLib::RPC::ParameterSet::Type::Enum::master)
			{
				if(configCentral.find(channel) == configCentral.end()) continue;
				if(configCentral[channel].find((*i)->id) == configCentral[channel].end()) continue;
				element = configCentral[channel][(*i)->id].rpcParameter->convertFromPacket(configCentral[channel][(*i)->id].data);
			}
			else if(remotePeer)
			{
				if(linksCentral.find(channel) == linksCentral.end()) continue;
				if(linksCentral[channel][remotePeer->address][remotePeer->channel].find((*i)->id) == linksCentral[channel][remotePeer->address][remotePeer->channel].end()) continue;
				if(remotePeer->channel != remoteChannel) continue;
				element = linksCentral[channel][remotePeer->address][remotePeer->channel][(*i)->id].rpcParameter->convertFromPacket(linksCentral[channel][remotePeer->address][remotePeer->channel][(*i)->id].data);
			}

			if(!element) continue;
			if(element->type == BaseLib::RPC::RPCVariableType::rpcVoid) continue;
			variables->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, element));
		}
		return variables;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(_peers.find(senderChannel) == _peers.end()) return BaseLib::RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverSerialNumber, receiverChannel);
		if(!remotePeer) return BaseLib::RPC::RPCVariable::createError(-2, "Peer not found.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> response(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		response->structValue->insert(BaseLib::RPC::RPCStructElement("DESCRIPTION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(remotePeer->linkDescription))));
		response->structValue->insert(BaseLib::RPC::RPCStructElement("NAME", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(remotePeer->linkName))));
		return response;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::setLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(_peers.find(senderChannel) == _peers.end()) return BaseLib::RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverSerialNumber, receiverChannel);
		if(!remotePeer) return BaseLib::RPC::RPCVariable::createError(-2, "Peer not found.");
		remotePeer->linkDescription = description;
		remotePeer->linkName = name;
		savePeers();
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getLinkPeers(int32_t channel, bool returnID)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		if(channel > -1)
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			//Return if there are no link roles defined
			std::shared_ptr<BaseLib::RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
			if(!linkRoles) return array;
			std::shared_ptr<HomeMaticCentral> central = getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<BidCoSPeer> peer(central->getPeer((*i)->address));
				if(returnID && !peer) continue;
				bool peerKnowsMe = false;
				if(peer && peer->getPeer(channel, _address)) peerKnowsMe = true;

				std::string peerSerial = (*i)->serialNumber;
				if((*i)->serialNumber.empty())
				{
					if(peerKnowsMe)
					{
						(*i)->serialNumber = peer->getSerialNumber();
						peerSerial = (*i)->serialNumber;
					}
					else
					{
						//Peer not paired to central
						std::ostringstream stringstream;
						stringstream << '@' << std::hex << std::setw(6) << std::setfill('0') << (*i)->address;
						peerSerial = stringstream.str();
					}
				}
				if(returnID)
				{
					std::shared_ptr<BaseLib::RPC::RPCVariable> address(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					array->arrayValue->push_back(address);
					address->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)peer->getID())));
					address->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->channel)));
				}
				else array->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(peerSerial + ":" + std::to_string((*i)->channel))));
			}
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				std::shared_ptr<BaseLib::RPC::RPCVariable> linkPeers = getLinkPeers(i->first, returnID);
				array->arrayValue->insert(array->arrayValue->end(), linkPeers->arrayValue->begin(), linkPeers->arrayValue->end());
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getLink(int32_t channel, int32_t flags, bool avoidDuplicates)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::RPCVariable::createError(-2, "Not a central peer.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		std::shared_ptr<BaseLib::RPC::RPCVariable> element;
		bool groupFlag = false;
		if(flags & 0x01) groupFlag = true;
		if(channel > -1 && !groupFlag) //Get link of single channel
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			bool isSender = false;
			//Return if there are no link roles defined
			std::shared_ptr<BaseLib::RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
			if(!linkRoles) return array;
			if(!linkRoles->sourceNames.empty()) isSender = true;
			else if(linkRoles->targetNames.empty()) return array;
			std::shared_ptr<HomeMaticCentral> central = getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<BidCoSPeer> remotePeer(central->getPeer((*i)->address));
				if(!remotePeer)
				{
					BaseLib::Output::printDebug("Debug: Can't return link description for peer with address 0x" + BaseLib::HelperFunctions::getHexString((*i)->address, 6) + ". The peer is not paired to Homegear.");
					continue;
				}
				bool peerKnowsMe = false;
				if(remotePeer && remotePeer->getPeer((*i)->channel, _address, channel)) peerKnowsMe = true;

				//Don't continue if peer is sender and exists in central's peer array to avoid generation of duplicate results when requesting all links (only generate results when we are sender)
				if(!isSender && peerKnowsMe && avoidDuplicates) return array;
				//If we are receiver this point is only reached, when the sender is not paired to this central

				std::string peerSerial = (*i)->serialNumber;
				int32_t brokenFlags = 0;
				if((*i)->serialNumber.empty())
				{
					if(peerKnowsMe ||
					  (*i)->address == _address) //Link to myself with non-existing (virtual) channel (e. g. switches use this)
					{
						(*i)->serialNumber = remotePeer->getSerialNumber();
						peerSerial = (*i)->serialNumber;
					}
					else
					{
						//Peer not paired to central
						std::ostringstream stringstream;
						stringstream << '@' << std::hex << std::setw(6) << std::setfill('0') << (*i)->address;
						peerSerial = stringstream.str();
						if(isSender) brokenFlags = 2; //LINK_FLAG_RECEIVER_BROKEN
						else brokenFlags = 1; //LINK_FLAG_SENDER_BROKEN
					}
				}
				//Relevent for switches
				if(peerSerial == _serialNumber && rpcDevice->channels.find((*i)->channel) == rpcDevice->channels.end())
				{
					if(isSender) brokenFlags = 2 | 4; //LINK_FLAG_RECEIVER_BROKEN | PEER_IS_ME
					else brokenFlags = 1 | 4; //LINK_FLAG_SENDER_BROKEN | PEER_IS_ME
				}
				if(brokenFlags == 0 && remotePeer && remotePeer->serviceMessages->getUnreach()) brokenFlags = 2;
				if(serviceMessages->getUnreach()) brokenFlags |= 1;
				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
				element->structValue->insert(BaseLib::RPC::RPCStructElement("DESCRIPTION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->linkDescription))));
				element->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(brokenFlags))));
				element->structValue->insert(BaseLib::RPC::RPCStructElement("NAME", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->linkName))));
				if(isSender)
				{
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(peerSerial + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)remotePeer->getID()))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_CHANNEL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->channel))));
					if(flags & 4)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, BaseLib::RPC::ParameterSet::Type::Enum::link, _serialNumber, channel);
						else paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> description;
						if(!(brokenFlags & 2) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel);
						else description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)_peerID))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_CHANNEL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(channel))));
					if(flags & 2)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1)) paramset = getParamset(channel, BaseLib::RPC::ParameterSet::Type::Enum::link, peerSerial, (*i)->channel);
						else paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> description;
						if(!(brokenFlags & 1)) description = getDeviceDescription(channel);
						else description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_DESCRIPTION", description));
					}
				}
				else //When sender is broken
				{
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)_peerID))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_CHANNEL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(channel))));
					if(flags & 4)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = getParamset(channel, BaseLib::RPC::ParameterSet::Type::Enum::link, peerSerial, (*i)->channel);
						else paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> description;
						if(!(brokenFlags & 2)) description = getDeviceDescription(channel);
						else description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(peerSerial + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)remotePeer->getID()))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_CHANNEL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((*i)->channel))));
					if(flags & 2)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, BaseLib::RPC::ParameterSet::Type::Enum::link, _serialNumber, channel);
						else paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> description;
						if(!(brokenFlags & 1) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel);
						else description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_DESCRIPTION", description));
					}
				}
				array->arrayValue->push_back(element);
			}
		}
		else
		{
			if(channel > -1 && groupFlag) //Get links for each grouped channel
			{
				if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
				std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
				if(rpcChannel->paired)
				{
					element = getLink(channel, flags & 0xFFFFFFFE, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());

					int32_t groupedWith = getChannelGroupedWith(channel);
					if(groupedWith > -1)
					{
						element = getLink(groupedWith, flags & 0xFFFFFFFE, avoidDuplicates);
						array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
					}
				}
				else
				{
					element = getLink(channel, flags & 0xFFFFFFFE, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
			else //Get links for all channels
			{
				flags &= 7; //Remove flag 8 and 16. Both are not processed, when getting links for all devices.
				for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					element = getLink(i->first, flags, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getParamsetDescription(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set");

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == BaseLib::RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<BaseLib::RPC::RPCVariable> descriptions(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<BaseLib::RPC::RPCVariable> description(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<BaseLib::RPC::RPCVariable> element;
		uint32_t index = 0;
		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::internal)) continue;
			if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeBoolean)
			{
				BaseLib::RPC::LogicalParameterBoolean* parameter = (BaseLib::RPC::LogicalParameterBoolean*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
					element->booleanValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "BOOL";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeString)
			{
				BaseLib::RPC::LogicalParameterString* parameter = (BaseLib::RPC::LogicalParameterString*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "STRING";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeAction)
			{
				BaseLib::RPC::LogicalParameterAction* parameter = (BaseLib::RPC::LogicalParameterAction*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
					element->booleanValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "ACTION";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeInteger)
			{
				BaseLib::RPC::LogicalParameterInteger* parameter = (BaseLib::RPC::LogicalParameterInteger*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
					element->integerValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				if(!parameter->specialValues.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> specialElement(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("VALUE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->second))));
						element->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(BaseLib::RPC::RPCStructElement("SPECIAL", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "INTEGER";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeEnum)
			{
				BaseLib::RPC::LogicalParameterEnum* parameter = (BaseLib::RPC::LogicalParameterEnum*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
					element->integerValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "ENUM";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
				for(std::vector<BaseLib::RPC::ParameterOption>::iterator j = parameter->options.begin(); j != parameter->options.end(); ++j)
				{
					element->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->id)));
				}
				description->structValue->insert(BaseLib::RPC::RPCStructElement("VALUE_LIST", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeFloat)
			{
				BaseLib::RPC::LogicalParameterFloat* parameter = (BaseLib::RPC::LogicalParameterFloat*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcFloat));
					element->floatValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcFloat));
				element->floatValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcFloat));
				element->floatValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				if(!parameter->specialValues.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, double>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> specialElement(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("VALUE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->second))));
						element->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(BaseLib::RPC::RPCStructElement("SPECIAL", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "FLOAT";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}

			index++;
			descriptions->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, description));
			description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		}
		return descriptions;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::getValue(uint32_t channel, std::string valueKey)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		return valuesCentral[channel][valueKey].rpcParameter->convertFromPacket(valuesCentral[channel][valueKey].data);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

bool BidCoSPeer::setHomegearValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value)
{
	try
	{
		if(_deviceType.type() == (uint32_t)DeviceType::HMCCVD && valueKey == "VALVE_STATE" && _peers.find(1) != _peers.end() && _peers[1].size() > 0 && _peers[1].at(0)->hidden)
		{
			if(!_peers[1].at(0)->device)
			{
				_peers[1].at(0)->device = getDevice(_peers[1].at(0)->address);
				if(_peers[1].at(0)->device->getDeviceType() != (uint32_t)DeviceType::HMCCTC) return false;
			}
			if(_peers[1].at(0)->device)
			{
				if(_peers[1].at(0)->device->getDeviceType() != (uint32_t)DeviceType::HMCCTC) return false;
				HM_CC_TC* tc = (HM_CC_TC*)_peers[1].at(0)->device.get();
				tc->setValveState(value->integerValue);
				std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
				if(!rpcParameter) return false;
				BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
				parameter->data = rpcParameter->convertToPacket(value);
				saveParameter(parameter->databaseID, parameter->data);
				BaseLib::Output::printInfo("Info: Setting valve state of HM-CC-VD with id " + std::to_string(_peerID) + " to " + std::to_string(value->integerValue / 2) + "%.");
				return true;
			}
		}
		else if(_deviceType.type() == (uint32_t)DeviceType::HMSECSD)
		{
			if(valueKey == "STATE")
			{
				std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
				if(!rpcParameter) return false;
				BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
				parameter->data = rpcParameter->convertToPacket(value);
				saveParameter(parameter->databaseID, parameter->data);

				std::shared_ptr<HomeMaticCentral> central = getCentral();
				std::shared_ptr<BidCoSPeer> associatedPeer = central->getPeer(_address);
				if(!associatedPeer)
				{
					BaseLib::Output::printError("Error: Could not handle \"STATE\", because the main team peer is not paired to this central.");
					return false;
				}
				if(associatedPeer->getTeamData().empty()) associatedPeer->getTeamData().push_back(0);
				std::vector<uint8_t> payload;
				payload.push_back(0x01);
				payload.push_back(associatedPeer->getTeamData().at(0));
				associatedPeer->getTeamData().at(0)++;
				associatedPeer->saveVariable(11, associatedPeer->getTeamData());
				if(value->booleanValue) payload.push_back(0xC8);
				else payload.push_back(0x01);
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(central->messageCounter()->at(0), 0x94, 0x41, _address, central->getAddress(), payload));
				central->messageCounter()->at(0)++;
				central->sendPacketMultipleTimes(_physicalInterface, packet, _address, 6, 1000, true);
				return true;
			}
			else if(valueKey == "INSTALL_TEST")
			{
				std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
				if(!rpcParameter) return false;
				BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
				parameter->data = rpcParameter->convertToPacket(value);
				saveParameter(parameter->databaseID, parameter->data);

				std::shared_ptr<HomeMaticCentral> central = getCentral();
				std::shared_ptr<BidCoSPeer> associatedPeer = central->getPeer(_address);
				if(!associatedPeer)
				{
					BaseLib::Output::printError("Error: Could not handle \"INSTALL_TEST\", because the main team peer is not paired to this central.");
					return false;
				}
				if(associatedPeer->getTeamData().empty()) associatedPeer->getTeamData().push_back(0);
				std::vector<uint8_t> payload;
				payload.push_back(0x00);
				payload.push_back(associatedPeer->getTeamData().at(0));
				associatedPeer->getTeamData().at(0)++;
				associatedPeer->saveVariable(11, associatedPeer->getTeamData());
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(central->messageCounter()->at(0), 0x94, 0x40, _address, central->getAddress(), payload));
				central->messageCounter()->at(0)++;
				central->sendPacketMultipleTimes(_physicalInterface, packet, _address, 6, 600, true);
				return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return false;
}

void BidCoSPeer::addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters)
{
	try
	{
		if(parameters->integers.size() != 3) return;
		if(parameters->strings.size() != 1) return;
		BaseLib::Output::printMessage("addVariableToResetCallback invoked for parameter " + parameters->strings.at(0) + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ".", 5);
		BaseLib::Output::printInfo("Parameter " + parameters->strings.at(0) + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + " will be reset at " + BaseLib::HelperFunctions::getTimeString(parameters->integers.at(2)) + ".");
		std::shared_ptr<VariableToReset> variable(new VariableToReset);
		variable->channel = parameters->integers.at(0);
		int32_t integerValue = parameters->integers.at(1);
		BaseLib::HelperFunctions::memcpyBigEndian(variable->data, integerValue);
		variable->resetTime = parameters->integers.at(2);
		variable->key = parameters->strings.at(0);
		_variablesToResetMutex.lock();
		_variablesToReset.push_back(variable);
		_variablesToResetMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoSPeer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return BaseLib::RPC::RPCVariable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {valueKey});
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>> { value });
		if(rpcParameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::Enum::store)
		{
			parameter->data = rpcParameter->convertToPacket(value);
			saveParameter(parameter->databaseID, parameter->data);
			raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		}
		else if(rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::command) return BaseLib::RPC::RPCVariable::createError(-6, "Parameter is not settable.");
		//_resendCounter[valueKey] = 0;
		if(!rpcParameter->conversion.empty() && rpcParameter->conversion.at(0)->type == BaseLib::RPC::ParameterConversion::Type::Enum::toggle)
		{
			//Handle toggle parameter
			std::string toggleKey = rpcParameter->conversion.at(0)->stringValue;
			if(toggleKey.empty()) return BaseLib::RPC::RPCVariable::createError(-6, "No toggle parameter specified (parameter attribute value is empty).");
			if(valuesCentral[channel].find(toggleKey) == valuesCentral[channel].end()) return BaseLib::RPC::RPCVariable::createError(-5, "Toggle parameter not found.");
			BaseLib::Systems::RPCConfigurationParameter* toggleParam = &valuesCentral[channel][toggleKey];
			std::shared_ptr<BaseLib::RPC::RPCVariable> toggleValue;
			if(toggleParam->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
			{
				toggleValue = toggleParam->rpcParameter->convertFromPacket(toggleParam->data);
				toggleValue->booleanValue = !toggleValue->booleanValue;
			}
			else if(toggleParam->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeInteger ||
					toggleParam->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeFloat)
			{
				int32_t currentToggleValue = (int32_t)toggleParam->data.at(0);
				std::vector<uint8_t> temp({0});
				if(currentToggleValue != toggleParam->rpcParameter->conversion.at(0)->on) temp.at(0) = toggleParam->rpcParameter->conversion.at(0)->on;
				else temp.at(0) = toggleParam->rpcParameter->conversion.at(0)->off;
				toggleValue = toggleParam->rpcParameter->convertFromPacket(temp);
			}
			else return BaseLib::RPC::RPCVariable::createError(-6, "Toggle parameter has to be of type boolean, float or integer.");
			return setValue(channel, toggleKey, toggleValue);
		}
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return BaseLib::RPC::RPCVariable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return BaseLib::RPC::RPCVariable::createError(-6, "No frame was found for parameter " + valueKey);
		std::shared_ptr<BaseLib::RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		std::vector<uint8_t> data = rpcParameter->convertToPacket(value);
		parameter->data = data;
		saveParameter(parameter->databaseID, data);
		if(BaseLib::Obj::ins->debugLevel > 4) BaseLib::Output::printDebug("Debug: " + valueKey + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to " + BaseLib::HelperFunctions::getHexString(data) + ".");

		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(_physicalInterface, BidCoSQueueType::PEER));
		queue->noSending = true;

		std::vector<uint8_t> payload;
		if(frame->subtype > -1 && frame->subtypeIndex >= 9)
		{
			while((signed)payload.size() - 1 < frame->subtypeIndex - 9) payload.push_back(0);
			payload.at(frame->subtypeIndex - 9) = (uint8_t)frame->subtype;
		}
		if(frame->channelField >= 9)
		{
			while((signed)payload.size() - 1 < frame->channelField - 9) payload.push_back(0);
			payload.at(frame->channelField - 9) = (uint8_t)channel;
		}
		uint8_t controlByte = 0xA0;
		if(getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst) controlByte |= 0x10;
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter, controlByte, (uint8_t)frame->type, getCentral()->getAddress(), _address, payload));

		for(std::vector<BaseLib::RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				std::vector<uint8_t> data;
				BaseLib::HelperFunctions::memcpyBigEndian(data, i->constValue);
				packet->setPosition(i->index, i->size, data);
				continue;
			}
			BaseLib::Systems::RPCConfigurationParameter* additionalParameter = nullptr;
			//We can't just search for param, because it is ambiguous (see for example LEVEL for HM-CC-TC.
			if(i->param.empty() && !i->additionalParameter.empty() && valuesCentral[channel].find(i->additionalParameter) != valuesCentral[channel].end())
			{
				additionalParameter = &valuesCentral[channel][i->additionalParameter];
				int32_t intValue = 0;
				BaseLib::HelperFunctions::memcpyBigEndian(intValue, additionalParameter->data);
				if(!i->omitIfSet || intValue != i->omitIf)
				{
					//Don't set ON_TIME when value is false
					if(i->additionalParameter != "ON_TIME" || (rpcParameter->physicalParameter->valueID == "STATE" && value->booleanValue) || (rpcParameter->physicalParameter->valueID == "LEVEL" && value->floatValue > 0)) packet->setPosition(i->index, i->size, additionalParameter->data);
				}
			}
			//param sometimes is ambiguous (e. g. LEVEL of HM-CC-TC), so don't search and use the given parameter when possible
			else if(i->param == rpcParameter->physicalParameter->valueID || i->param == rpcParameter->physicalParameter->id) packet->setPosition(i->index, i->size, valuesCentral[channel][valueKey].data);
			//Search for all other parameters
			else
			{
				bool paramFound = false;
				for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator j = valuesCentral[channel].begin(); j != valuesCentral[channel].end(); ++j)
				{
					//Only compare id. Till now looking for value_id was not necessary.
					if(i->param == j->second.rpcParameter->physicalParameter->id)
					{
						packet->setPosition(i->index, i->size, j->second.data);
						paramFound = true;
						break;
					}
				}
				if(!paramFound) BaseLib::Output::printError("Error constructing packet. param \"" + i->param + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
			}
			if(i->additionalParameter == "ON_TIME" && additionalParameter)
			{
				if((rpcParameter->physicalParameter->valueID != "STATE" && rpcParameter->physicalParameter->valueID != "LEVEL") || (rpcParameter->physicalParameter->valueID == "STATE" && rpcParameter->logicalParameter->type != BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean) || (rpcParameter->physicalParameter->valueID == "LEVEL" && rpcParameter->logicalParameter->type != BaseLib::RPC::LogicalParameter::Type::Enum::typeFloat))
				{
					BaseLib::Output::printError("Error: Can't set \"ON_TIME\" for " + rpcParameter->physicalParameter->valueID + ". Currently \"ON_TIME\" is only supported for \"STATE\" of type \"boolean\" or \"LEVEL\" of type \"float\". Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					continue;
				}
				if(!_variablesToReset.empty())
				{
					_variablesToResetMutex.lock();
					for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
					{
						if((*i)->channel == channel && (*i)->key == rpcParameter->physicalParameter->valueID)
						{
							BaseLib::Output::printMessage("Debug: Deleting element from _variablesToReset. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id, 5);
							_variablesToReset.erase(i);
							break; //The key should only be once in the vector, so breaking is ok and we can't continue as the iterator is invalidated.
						}
					}
					_variablesToResetMutex.unlock();
				}
				if((rpcParameter->physicalParameter->valueID == "STATE" && !value->booleanValue) || (rpcParameter->physicalParameter->valueID == "LEVEL" && value->floatValue == 0) || additionalParameter->data.empty() || additionalParameter->data.at(0) == 0) continue;
				std::shared_ptr<CallbackFunctionParameter> parameters(new CallbackFunctionParameter());
				parameters->integers.push_back(channel);
				parameters->integers.push_back(0); //false = off
				int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				parameters->integers.push_back(time + std::lround(additionalParameter->rpcParameter->convertFromPacket(additionalParameter->data)->floatValue * 1000));
				parameters->strings.push_back(rpcParameter->physicalParameter->valueID);
				queue->callbackParameter = parameters;
				queue->queueEmptyCallback = delegate<void (std::shared_ptr<CallbackFunctionParameter>)>::from_method<BidCoSPeer, &BidCoSPeer::addVariableToResetCallback>(this);
			}
		}
		if(!rpcParameter->physicalParameter->resetAfterSend.empty())
		{
			for(std::vector<std::string>::iterator j = rpcParameter->physicalParameter->resetAfterSend.begin(); j != rpcParameter->physicalParameter->resetAfterSend.end(); ++j)
			{
				if(valuesCentral.at(channel).find(*j) == valuesCentral.at(channel).end()) continue;
				std::shared_ptr<BaseLib::RPC::RPCVariable> logicalDefaultValue = valuesCentral.at(channel).at(*j).rpcParameter->logicalParameter->getDefaultValue();
				std::vector<uint8_t> defaultValue = valuesCentral.at(channel).at(*j).rpcParameter->convertToPacket(logicalDefaultValue);
				if(defaultValue != valuesCentral.at(channel).at(*j).data)
				{
					BaseLib::Systems::RPCConfigurationParameter* tempParam = &valuesCentral.at(channel).at(*j);
					tempParam->data = defaultValue;
					saveParameter(tempParam->databaseID, tempParam->data);
					BaseLib::Output::printInfo( "Info: Parameter \"" + *j + "\" was reset to " + BaseLib::HelperFunctions::getHexString(defaultValue) + ". Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					valueKeys->push_back(*j);
					values->push_back(logicalDefaultValue);
				}
			}
		}
		setMessageCounter(_messageCounter + 1);
		queue->parameterName = valueKey;
		queue->channel = channel;
		queue->push(packet);
		queue->push(getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		pendingBidCoSQueues->removeQueue(valueKey, channel);
		pendingBidCoSQueues->push(queue);
		if((getRXModes() & BaseLib::RPC::Device::RXModes::Enum::always) || (getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst))
		{
			if(HomeMaticDevice::isDimmer(_deviceType) || HomeMaticDevice::isSwitch(_deviceType)) queue->retries = 12;
			getCentral()->enqueuePendingQueues(_address);
		}
		else BaseLib::Output::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");

		raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _variablesToResetMutex.unlock();
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _variablesToResetMutex.unlock();
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        _variablesToResetMutex.unlock();
    }
    return BaseLib::RPC::RPCVariable::createError(-1, "unknown error");
}
}
