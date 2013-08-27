class CallbackFunctionParameter;

#include "Peer.h"
#include "GD.h"
#include "HelperFunctions.h"

void Peer::initializeCentralConfig()
{
	try
	{
		if(!rpcDevice)
		{
			HelperFunctions::printWarning("Warning: Tried to initialize peer's central config without xmlrpcDevice being set.");
			return;
		}
		RPCConfigurationParameter parameter;
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			if(i->second->parameterSets.find(RPC::ParameterSet::Type::master) != i->second->parameterSets.end() && i->second->parameterSets[RPC::ParameterSet::Type::master])
			{
				std::shared_ptr<RPC::ParameterSet> masterSet = i->second->parameterSets[RPC::ParameterSet::Type::master];
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = masterSet->parameters.begin(); j != masterSet->parameters.end(); ++j)
				{
					if(!(*j)->id.empty() && configCentral[i->first].find((*j)->id) == configCentral[i->first].end())
					{
						parameter = RPCConfigurationParameter();
						parameter.rpcParameter = *j;
						parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
						configCentral[i->first][(*j)->id] = parameter;
					}
				}
			}
			if(i->second->parameterSets.find(RPC::ParameterSet::Type::values) != i->second->parameterSets.end() && i->second->parameterSets[RPC::ParameterSet::Type::values])
			{
				std::shared_ptr<RPC::ParameterSet> valueSet = i->second->parameterSets[RPC::ParameterSet::Type::values];
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = valueSet->parameters.begin(); j != valueSet->parameters.end(); ++j)
				{
					if(!(*j)->id.empty() && valuesCentral[i->first].find((*j)->id) == valuesCentral[i->first].end())
					{
						parameter = RPCConfigurationParameter();
						parameter.rpcParameter = *j;
						parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
						valuesCentral[i->first][(*j)->id] = parameter;
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::initializeLinkConfig(int32_t channel, int32_t peerAddress, int32_t remoteChannel, bool useConfigFunction)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		if(rpcDevice->channels[channel]->parameterSets.find(RPC::ParameterSet::Type::link) == rpcDevice->channels[channel]->parameterSets.end()) return;
		std::shared_ptr<RPC::ParameterSet> linkSet = rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::link];
		//This line creates an empty link config. This is essential as the link config must exist, even if it is empty.
		std::unordered_map<std::string, RPCConfigurationParameter>* linkConfig = &linksCentral[channel][peerAddress][remoteChannel];
		RPCConfigurationParameter parameter;
		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = linkSet->parameters.begin(); j != linkSet->parameters.end(); ++j)
		{
			if(!(*j)->id.empty() && linkConfig->find((*j)->id) == linkConfig->end())
			{
				parameter = RPCConfigurationParameter();
				parameter.rpcParameter = *j;
				parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
				linkConfig->insert(std::pair<std::string, RPCConfigurationParameter>((*j)->id, parameter));
			}
		}
		if(useConfigFunction) applyConfigFunction(channel, peerAddress, remoteChannel);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::applyConfigFunction(int32_t channel, int32_t peerAddress, int32_t remoteChannel)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		if(rpcDevice->channels[channel]->parameterSets.find(RPC::ParameterSet::Type::link) == rpcDevice->channels[channel]->parameterSets.end()) return;
		std::shared_ptr<RPC::ParameterSet> linkSet = rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::link];
		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central) return; //Shouldn't happen
		std::shared_ptr<Peer> remotePeer(central->getPeer(peerAddress));
		if(!remotePeer || !remotePeer->rpcDevice) return; //Shouldn't happen
		if(remotePeer->rpcDevice->channels.find(remoteChannel) == remotePeer->rpcDevice->channels.end()) return;
		std::shared_ptr<RPC::DeviceChannel> remoteRPCChannel = remotePeer->rpcDevice->channels.at(remoteChannel);
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
		HelperFunctions::printInfo("Info: Device 0x" + HelperFunctions::getHexString(address) + ": Applying channel function " + function + ".");
		for(RPC::DefaultValue::iterator j = linkSet->defaultValues.at(function).begin(); j != linkSet->defaultValues.at(function).end(); ++j)
		{
			RPCConfigurationParameter* parameter = &linksCentral[channel][peerAddress][remoteChannel][j->first];
			if(!parameter->rpcParameter) continue;
			parameter->data = parameter->rpcParameter->convertToPacket(j->second);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::~Peer()
{
	try
	{
		if(serviceMessages) serviceMessages->dispose();
		dispose();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
	}
	catch(const std::exception& ex)
    {
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::Peer(bool centralFeatures)
{
	try
	{
		_centralFeatures = centralFeatures;
		_lastPacketReceived = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		rpcDevice.reset();
		team.address = 0;
		if(centralFeatures)
		{
			serviceMessages.reset(new ServiceMessages(this));
			pendingBidCoSQueues.reset(new PendingBidCoSQueues());
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::Peer(std::string serializedObject, HomeMaticDevice* device, bool centralFeatures) : Peer(centralFeatures)
{
	try
	{
		if(serializedObject.empty()) return;
		HelperFunctions::printDebug("Unserializing peer: " + serializedObject, 5);

		std::istringstream stringstream(serializedObject);
		std::string entry;
		uint32_t pos = 0;
		address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		int32_t stringSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
		_serialNumber = serializedObject.substr(pos, stringSize); pos += stringSize;
		firmwareVersion = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		remoteChannel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		localChannel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		deviceType = (HMDeviceTypes)std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		countFromSysinfo = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
		//This loads the corresponding xmlrpcDevice unnecessarily for virtual device peers, too. But so what?
		rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, countFromSysinfo);
		if(!rpcDevice) HelperFunctions::printError("Error: Device type not found: 0x" + HelperFunctions::getHexString((uint32_t)deviceType) + " Firmware version: " + std::to_string(firmwareVersion));
		messageCounter = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		pairingComplete = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
		teamChannel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		team.address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		team.channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		stringSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
		if(stringSize > 0) { team.serialNumber = serializedObject.substr(pos, stringSize); pos += stringSize; }
		uint32_t dataSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < dataSize; i++)
		{
			team.data.push_back(std::stol(serializedObject.substr(pos, 2), 0, 16)); pos += 2;
		}
		uint32_t configSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
		for(uint32_t i = 0; i < configSize; i++)
		{
			config[std::stoll(serializedObject.substr(pos, 8), 0, 16)] = std::stoll(serializedObject.substr(pos + 8, 8), 0, 16); pos += 16;
		}
		uint32_t peersSize = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
		for(uint32_t i = 0; i < peersSize; i++)
		{
			uint32_t channel = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
			uint32_t peerCount = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
			for(uint32_t j = 0; j < peerCount; j++)
			{
				std::shared_ptr<BasicPeer> basicPeer(new BasicPeer());
				basicPeer->address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				basicPeer->channel = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
				stringSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
				basicPeer->serialNumber = serializedObject.substr(pos, stringSize); pos += stringSize;
				basicPeer->hidden = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
				_peers[channel].push_back(basicPeer);
				stringSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				basicPeer->linkName = serializedObject.substr(pos, stringSize); pos += stringSize;
				stringSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				basicPeer->linkDescription = serializedObject.substr(pos, stringSize); pos += stringSize;
				dataSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				for(uint32_t k = 0; k < dataSize; k++)
				{
					basicPeer->data.push_back(std::stol(serializedObject.substr(pos, 2), 0, 16)); pos += 2;
				}
			}
		}
		unserializeConfig(serializedObject, configCentral, RPC::ParameterSet::Type::master, pos);
		unserializeConfig(serializedObject, valuesCentral, RPC::ParameterSet::Type::values, pos);
		unserializeConfig(serializedObject, linksCentral, RPC::ParameterSet::Type::link, pos);
		uint32_t serializedServiceMessagesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		if(_centralFeatures) serviceMessages.reset(new ServiceMessages(this, serializedObject.substr(pos, serializedServiceMessagesSize))); pos += serializedServiceMessagesSize;
		uint32_t pendingQueuesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		if(_centralFeatures) pendingBidCoSQueues.reset(new PendingBidCoSQueues(serializedObject.substr(pos, pendingQueuesSize), this, device)); pos += pendingQueuesSize;
		uint32_t variablesToResetSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < variablesToResetSize; i++)
		{
			std::shared_ptr<VariableToReset> variable(new VariableToReset());
			variable->channel = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
			uint32_t keyLength = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
			variable->key = serializedObject.substr(pos, keyLength); pos += keyLength;
			dataSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			for(uint32_t j = 0; j < dataSize; j++)
			{
				variable->data.push_back(std::stol(serializedObject.substr(pos, 2), 0, 16)); pos += 2;
			}
			variable->resetTime = std::stoll(serializedObject.substr(pos, 8), 0, 16) * 1000; pos += 8;
			variable->isDominoEvent = std::stol(serializedObject.substr(pos, 1)); pos += 1;
			try
			{
				_variablesToResetMutex.lock();
				_variablesToReset.push_back(variable);
			}
			catch(const std::exception& ex)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(Exception& ex)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_variablesToResetMutex.unlock();
		}
		initializeCentralConfig();

		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				initializeLinkConfig(i->first, (*j)->address, (*j)->channel, false);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::worker()
{
	if(!_centralFeatures || _disposing) return;
	std::vector<uint32_t> positionsToDelete;
	std::vector<std::shared_ptr<VariableToReset>> variablesToReset;
	int32_t wakeUpIndex = 0;
	int32_t index;
	int64_t time;
	try
	{
		time = HelperFunctions::getTime();
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
					valuesCentral.at((*i)->channel).at((*i)->key).data = (*i)->data;
					std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {(*i)->key});
					std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>> { valuesCentral.at((*i)->channel).at((*i)->key).rpcParameter->convertFromPacket((*i)->data) });
					HelperFunctions::printInfo("Info: Domino event: " + (*i)->key + " of device 0x" + HelperFunctions::getHexString(address) + " with serial number " + _serialNumber + ":" + std::to_string((*i)->channel) + " was reset.");
					GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string((*i)->channel), valueKeys, rpcValues);
				}
				else
				{
					std::shared_ptr<RPC::RPCVariable> rpcValue = valuesCentral.at((*i)->channel).at((*i)->key).rpcParameter->convertFromPacket((*i)->data);
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
		serviceMessages->checkUnreach();
		if(serviceMessages->getConfigPending() && (!pendingBidCoSQueues || pendingBidCoSQueues->empty()))
		{
			serviceMessages->setConfigPending(false);
			saveToDatabase(GD::devices.getCentral()->address());
		}
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_variablesToResetMutex.unlock();
	}
	catch(Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_variablesToResetMutex.unlock();
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_variablesToResetMutex.unlock();
	}
}

std::string Peer::handleCLICommand(std::string command)
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

			if(team.serialNumber.empty()) stringStream << "This peer doesn't support teams." << std::endl;
			else stringStream << "Team address: 0x" << std::hex << team.address << std::dec << " Team serial number: " << team.serialNumber << std::endl;
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
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

void Peer::addPeer(int32_t channel, std::shared_ptr<BasicPeer> peer)
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
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}



std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, std::string serialNumber, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->serialNumber.empty())
			{
				std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
				if(central)
				{
					std::shared_ptr<Peer> peer(central->getPeer((*i)->address));
					if(peer) (*i)->serialNumber = peer->getSerialNumber();
				}
			}
			if((*i)->serialNumber == serialNumber && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<HomeMaticDevice> Peer::getHiddenPeerDevice()
{
	try
	{
		//This is pretty dirty, but as all hidden peers should be the same device, this should always work
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator j = _peers.begin(); j != _peers.end(); ++j)
		{
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = j->second.begin(); i != j->second.end(); ++i)
			{
				if((*i)->hidden) return GD::devices.get((*i)->address);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<HomeMaticDevice>();
}

std::shared_ptr<BasicPeer> Peer::getHiddenPeer(int32_t channel)
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, int32_t address, int32_t remoteChannel)
{
	try
	{
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == address && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

void Peer::removePeer(int32_t channel, int32_t address, int32_t remoteChannel)
{
	try
	{
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == address && (*i)->channel == remoteChannel)
			{
				_peers[channel].erase(i);
				if(linksCentral[channel].find(address) != linksCentral[channel].end()) linksCentral[channel].erase(linksCentral[channel].find(address));
				return;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::deleteFromDatabase(int32_t parentAddress)
{
	try
	{
		_databaseMutex.lock();
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_serialNumber)));
		GD::db.executeCommand("DELETE FROM metadata WHERE objectID=?", data);

		std::ostringstream command;
		command << "DELETE FROM peers WHERE parent=" << std::dec << parentAddress << " AND " << " address=" << address;
		GD::db.executeCommand(command.str());
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveToDatabase(int32_t parentAddress)
{
	try
	{
		if(deleting) return;
		_databaseMutex.lock();
		std::ostringstream command;
		command << "SELECT 1 FROM peers WHERE parent=" << std::dec << parentAddress << " AND address=" << address;
		DataTable result = GD::db.executeCommand(command.str());
		if(result.empty())
		{
			std::ostringstream command2;
			command2 << "INSERT INTO peers VALUES(" << parentAddress << "," << address << ",'" <<  serialize() << "')";
			GD::db.executeCommand(command2.str());
		}
		else
		{
			std::ostringstream command2;
			command2 << "UPDATE peers SET serializedObject='" <<  serialize() << "' WHERE parent=" << std::dec << parentAddress << " AND address=" << address;
			GD::db.executeCommand(command2.str());
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::deletePairedVirtualDevice(int32_t address)
{
	try
	{
		std::shared_ptr<HomeMaticDevice> device(GD::devices.get(address));
		if(device)
		{
			GD::devices.remove(address);
			device->reset();
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::deletePairedVirtualDevices()
{
	try
	{
		std::shared_ptr<HomeMaticDevice> device;
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!*j) continue;
				if((*j)->hidden)
				{
					device = GD::devices.get((*j)->address);
					if(device)
					{
						GD::devices.remove((*j)->address);
						device->reset();
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string Peer::serialize()
{
	try
	{
		std::ostringstream stringstream;
		stringstream << std::hex << std::uppercase << std::setfill('0');
		stringstream << std::setw(8) << address;
		stringstream << std::setw(4) << _serialNumber.size();
		stringstream << _serialNumber;
		stringstream << std::setw(8) << firmwareVersion;
		stringstream << std::setw(2) << remoteChannel;
		stringstream << std::setw(2) << localChannel;
		stringstream << std::setw(8) << (int32_t)deviceType;
		stringstream << std::setw(4) << countFromSysinfo;
		stringstream << std::setw(2) << (int32_t)messageCounter;
		stringstream << std::setw(1) << (int32_t)pairingComplete;
		stringstream << std::setw(8) << teamChannel;
		stringstream << std::setw(8) << team.address;
		stringstream << std::setw(8) << team.channel;
		stringstream << std::setw(4) << team.serialNumber.size();
		stringstream << team.serialNumber;
		stringstream << std::setw(8) << team.data.size();
		for(std::vector<uint8_t>::iterator i = team.data.begin(); i != team.data.end(); ++i)
		{
			stringstream << std::setw(2) << (int32_t)(*i);
		}
		stringstream << std::setw(8) << config.size();
		for(std::unordered_map<int32_t, int32_t>::const_iterator i = config.begin(); i != config.end(); ++i)
		{
			stringstream << std::setw(8) << i->first;
			stringstream << std::setw(8) << i->second;
		}
		stringstream << std::setw(8) << _peers.size();
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			stringstream << std::setw(8) << i->first;
			stringstream << std::setw(8) << i->second.size();
			for(std::vector<std::shared_ptr<BasicPeer>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!*j) continue;
				stringstream << std::setw(8) << (*j)->address;
				stringstream << std::setw(4) << (*j)->channel;
				stringstream << std::setw(4) << (*j)->serialNumber.size();
				stringstream << (*j)->serialNumber;
				stringstream << std::setw(1) << (*j)->hidden;
				stringstream << std::setw(8) << (*j)->linkName.size();
				stringstream << (*j)->linkName;
				stringstream << std::setw(8) << (*j)->linkDescription.size();
				stringstream << (*j)->linkDescription;
				stringstream << std::setw(8) << (*j)->data.size();
				for(std::vector<uint8_t>::iterator k = (*j)->data.begin(); k != (*j)->data.end(); ++k)
				{
					stringstream << std::setw(2) << (int32_t)(*k);
				}
			}
		}
		serializeConfig(stringstream, configCentral);
		serializeConfig(stringstream, valuesCentral);
		serializeConfig(stringstream, linksCentral);
		if(_centralFeatures && serviceMessages)
		{
			std::string serializedServiceMessages = serviceMessages->serialize();
			stringstream << std::setw(8) << serializedServiceMessages.length();
			stringstream << serializedServiceMessages;
		}
		else stringstream << std::setw(8) << 0;
		if(_centralFeatures && pendingBidCoSQueues)
		{
			std::string serializedPendingQueues = pendingBidCoSQueues->serialize();
			stringstream << std::setw(8) << serializedPendingQueues.length();
			stringstream << serializedPendingQueues;
		}
		else stringstream << std::setw(8) << 0;
		try
		{
			_variablesToResetMutex.lock();
			stringstream << std::setw(8) << _variablesToReset.size();
			for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
			{
				stringstream << std::setw(4) << (*i)->channel;
				stringstream << std::setw(4) << (*i)->key.size();
				stringstream << (*i)->key;
				stringstream << std::setw(8) << (*i)->data.size();
				for(std::vector<uint8_t>::iterator j = (*i)->data.begin(); j != (*i)->data.end(); ++j)
				{
					stringstream << std::setw(2) << (int32_t)(*j);
				}
				stringstream << std::setw(8) << ((*i)->resetTime / 1000);
				stringstream << std::setw(1) << (int32_t)(*i)->isDominoEvent;
			}
		}
		catch(const std::exception& ex)
		{
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(Exception& ex)
		{
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		_variablesToResetMutex.unlock();
		stringstream << std::dec;
		return stringstream.str();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return "";
}

void Peer::serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config)
{
	try
	{
		stringstream << std::setw(8) << config.size();
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator i = config.begin(); i != config.end(); ++i)
		{
			stringstream << std::setw(8) << i->first;
			stringstream << std::setw(8) << i->second.size();
			for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!j->second.rpcParameter)
				{
					HelperFunctions::printCritical("Critical: Parameter " + j->first + " has no corresponding RPC parameter. Writing dummy data. Device: 0x" + HelperFunctions::getHexString(address) + " Channel: " + std::to_string(i->first));
					stringstream << std::setw(8) << 0;
					stringstream << std::setw(8) << 0;
					continue;
				}
				if(j->second.rpcParameter->id.size() == 0)
				{
					HelperFunctions::printError("Error: Parameter has no id.");
				}
				stringstream << std::setw(8) << j->second.rpcParameter->id.size();
				stringstream << j->second.rpcParameter->id;
				stringstream << std::setw(8) << j->second.data.size();
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringstream << std::setw(2) << (int32_t)(*k);
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>& config)
{
	try
	{
		stringstream << std::setw(8) << config.size();
		for(std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>::const_iterator i = config.begin(); i != config.end(); ++i)
		{
			stringstream << std::setw(8) << i->first;
			stringstream << std::setw(8) << i->second.size();
			for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringstream << std::setw(8) << j->first;
				stringstream << std::setw(8) << j->second.size();
				for(std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					stringstream << std::setw(8) << k->first;
					stringstream << std::setw(8) << k->second.size();
					for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator l = k->second.begin(); l != k->second.end(); ++l)
					{
						if(!l->second.rpcParameter)
						{
							HelperFunctions::printCritical("Critical: Parameter " + l->first + " has no corresponding RPC parameter. Writing dummy data. Device: 0x" + HelperFunctions::getHexString(address) + " Channel: " + std::to_string(i->first));
							stringstream << std::setw(8) << 0;
							stringstream << std::setw(8) << 0;
							continue;
						}
						if(l->second.rpcParameter->id.size() == 0)
						{
							HelperFunctions::printError("Error: Parameter has no id.");
						}
						stringstream << std::setw(8) << l->second.rpcParameter->id.size();
						stringstream << l->second.rpcParameter->id;
						stringstream << std::setw(8) << l->second.data.size();
						for(std::vector<uint8_t>::const_iterator m = l->second.data.begin(); m != l->second.data.end(); ++m)
						{
							stringstream << std::setw(2) << (int32_t)(*m);
						}
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos)
{
	try
	{
		uint32_t configSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < configSize; i++)
		{
			uint32_t channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			uint32_t parameterCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) continue;
			if(rpcDevice->channels[channel]->parameterSets.find(parameterSetType) == rpcDevice->channels[channel]->parameterSets.end()) continue;
			for(uint32_t k = 0; k < parameterCount; k++)
			{
				uint32_t idLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				std::string id;
				if(idLength == 0) HelperFunctions::printCritical("Critical: Added central config parameter without id. Device: 0x" + HelperFunctions::getHexString(address) + " Channel: " + std::to_string(channel));
				if(idLength > 0) { id = serializedObject.substr(pos, idLength); pos += idLength; }
				else id = "Parameter" + std::to_string(k);
				RPCConfigurationParameter* parameter = &config[channel][id];
				uint32_t dataSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				for(uint32_t l = 0; l < dataSize; l++)
				{
					parameter->data.push_back(std::stol(serializedObject.substr(pos, 2), 0, 16)); pos += 2;
				}
				if(!rpcDevice)
				{
					HelperFunctions::printError("Critical: No xml rpc device found for peer 0x" + HelperFunctions::getHexString(address) + ".");
					continue;
				}
				parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(id);
				if(!parameter->rpcParameter)
				{
					HelperFunctions::printError("Error: Deleting parameter " + id + ", because no corresponding RPC parameter was found. Device: 0x" + HelperFunctions::getHexString(address) + " Channel: " + std::to_string(channel));
					config[channel].erase(id);
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos)
{
	try
	{
		uint32_t configSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < configSize; i++)
		{
			uint32_t channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) continue;
			if(rpcDevice->channels[channel]->parameterSets.find(parameterSetType) == rpcDevice->channels[channel]->parameterSets.end()) continue;
			uint32_t peerCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			for(uint32_t j = 0; j < peerCount; j++)
			{
				int32_t address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				uint32_t channelCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				for(uint32_t k = 0; k < channelCount; k++)
				{
					int32_t remoteChannel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
					uint32_t parameterCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
					for(uint32_t l = 0; l < parameterCount; l++)
					{
						uint32_t idLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
						std::string id;
						if(idLength == 0) HelperFunctions::printError("Critical: Added central config parameter without id. Device: 0x" + HelperFunctions::getHexString(address) + " Channel: " + std::to_string(channel));
						if(idLength > 0) { id = serializedObject.substr(pos, idLength); pos += idLength; }
						else id = "Parameter" + std::to_string(l);
						RPCConfigurationParameter* parameter = &config[channel][address][remoteChannel][id];
						uint32_t dataSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
						for(uint32_t m = 0; m < dataSize; m++)
						{
							parameter->data.push_back(std::stol(serializedObject.substr(pos, 2), 0, 16)); pos += 2;
						}
						if(!rpcDevice)
						{
							HelperFunctions::printError("Critical: No xml rpc device found for peer 0x" + HelperFunctions::getHexString(address) + ".");
							continue;
						}
						parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(id);
						if(!parameter->rpcParameter)
						{
							HelperFunctions::printError("Error: Deleting parameter " + id + ", because no corresponding RPC parameter was found. Device: 0x" + HelperFunctions::getHexString(address) + " Channel: " + std::to_string(channel));
							config[channel][address][remoteChannel].erase(id);
						}
						//Clean up - delete if the peer doesn't exist anymore
						if(!getPeer(channel, address, remoteChannel)) config[channel][address].erase(remoteChannel);
						if(config[channel][address].empty()) config[channel].erase(address);
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string Peer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
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
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
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
		for(std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>::const_iterator i = linksCentral.begin(); i != linksCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t" << "Address: " << std::hex << "0x" << j->first << std::endl;
				stringStream << "\t\t{" << std::endl;
				for(std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					stringStream << "\t\t\t" << "Remote channel: " << std::dec << k->first << std::endl;
					stringStream << "\t\t\t{" << std::endl;
					for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator l = k->second.begin(); l != k->second.end(); ++l)
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

int32_t Peer::getChannelGroupedWith(int32_t channel)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return -1;
		if(rpcDevice->channels.at(channel)->paired)
		{
			uint32_t firstGroupChannel = 0;
			//Find first channel of group
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return -1;
}

void Peer::getValuesFromPacket(std::shared_ptr<BidCoSPacket> packet, std::string& frameID, std::list<uint32_t>& paramsetChannels, RPC::ParameterSet::Type::Enum& parameterSetType, std::map<std::string, std::vector<uint8_t>>& values)
{
	try
	{
		if(!rpcDevice) return;
		if(packet->messageType() == 0) return; //Don't handle pairing packets, because equal_range returns all elements with "0" as argument
		std::pair<std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator,std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByMessageType.equal_range((uint32_t)packet->messageType());
		if(range.first == rpcDevice->framesByMessageType.end()) return;
		std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator i = range.first;
		paramsetChannels.clear();
		do
		{
			std::shared_ptr<RPC::DeviceFrame> frame(i->second);
			if(!frame) continue;
			if(frame->direction == RPC::DeviceFrame::Direction::Enum::fromDevice && packet->senderAddress() != address && (!hasTeam() || packet->senderAddress() != team.address)) continue;
			if(frame->direction == RPC::DeviceFrame::Direction::Enum::toDevice && packet->destinationAddress() != address) continue;
			if(packet->payload()->empty()) continue;
			if(frame->subtype > -1 && frame->subtypeIndex >= 9 && (signed)packet->payload()->size() > (frame->subtypeIndex - 9) && packet->payload()->at(frame->subtypeIndex - 9) != (unsigned)frame->subtype) continue;
			int32_t channelIndex = frame->channelField;
			int32_t channel = -1;
			if(channelIndex >= 9 && (signed)packet->payload()->size() > (channelIndex - 9)) channel = packet->payload()->at(channelIndex - 9);
			if(channel > -1 && frame->channelFieldSize < 1.0) channel &= (0xFF >> (8 - std::lround(frame->channelFieldSize * 10) % 10));
			if(frame->fixedChannel > -1) channel = frame->fixedChannel;
			frameID = frame->id;

			for(std::vector<RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				std::vector<uint8_t> data;
				if(j->size > 0 && j->index > 0)
				{
					if(((int32_t)j->index) - 9 >= (signed)packet->payload()->size()) continue;
					data = packet->getPosition(j->index, j->size, -1);

					if(j->constValue > -1)
					{
						int32_t intValue = 0;
						HelperFunctions::memcpyBigEndian(intValue, data);
						if(intValue != j->constValue) break; else continue;
					}
				}
				else if(j->constValue > -1)
				{
					HelperFunctions::memcpyBigEndian(data, j->constValue);
				}
				else continue;
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
				{
					if((*k)->physicalParameter->valueID != j->param) continue;
					parameterSetType = (*k)->parentParameterSet->type;
					bool setValues = false;
					if(paramsetChannels.empty()) //Fill paramsetChannels
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
							if(rpcDevice->channels.at(l)->parameterSets.at(parameterSetType).get() != (*k)->parentParameterSet) continue;
							paramsetChannels.push_back(l);
							setValues = true;
						}
					}
					else //Use paramsetChannels
					{
						for(std::list<uint32_t>::const_iterator l = paramsetChannels.begin(); l != paramsetChannels.end(); ++l)
						{
							if(rpcDevice->channels.at(*l)->parameterSets.at(parameterSetType).get() != (*k)->parentParameterSet) continue; //Shouldn't happen as all parameters should be in the same parameter set
							setValues = true;
							break;
						}
					}
					if(setValues) values[(*k)->id] = data;
				}
			}
		} while(++i != range.second && i != rpcDevice->framesByMessageType.end());
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::handleDominoEvent(std::shared_ptr<RPC::Parameter> parameter, std::string& frameID, uint32_t channel)
{
	try
	{
		if(!parameter->hasDominoEvents) return;
		for(std::vector<std::shared_ptr<RPC::PhysicalParameterEvent>>::iterator j = parameter->physicalParameter->eventFrames.begin(); j != parameter->physicalParameter->eventFrames.end(); ++j)
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
						HelperFunctions::printDebug("Debug: Deleting element " + parameter->id + " from _variablesToReset. Device: 0x" + HelperFunctions::getHexString(address) + " Serial number: " + _serialNumber + " Frame: " + frameID);
						_variablesToReset.erase(k);
						break; //The key should only be once in the vector, so breaking is ok and we can't continue as the iterator is invalidated.
					}
				}
				_variablesToResetMutex.unlock();
			}
			std::shared_ptr<RPC::Parameter> delayParameter = rpcDevice->channels.at(channel)->parameterSets.at(RPC::ParameterSet::Type::Enum::values)->getParameter((*j)->dominoEventDelayID);
			if(!delayParameter) continue;
			int64_t delay = delayParameter->convertFromPacket(valuesCentral[channel][(*j)->dominoEventDelayID].data)->integerValue;
			if(delay < 0) continue; //0 is allowed. When 0 the parameter will be reset immediately
			int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			std::shared_ptr<VariableToReset> variable(new VariableToReset);
			variable->channel = channel;
			HelperFunctions::memcpyBigEndian(variable->data, (*j)->dominoEventValue);
			variable->resetTime = time + (delay * 1000);
			variable->key = parameter->id;
			variable->isDominoEvent = true;
			_variablesToResetMutex.lock();
			_variablesToReset.push_back(variable);
			_variablesToResetMutex.unlock();
			HelperFunctions::printDebug("Debug: " + parameter->id + " will be reset in " + std::to_string((variable->resetTime - time) / 1000) + "s.", 5);
			/*if(!_workerThread.joinable())
			{
				_stopThreads = false;
				_workerThread = std::thread(&Peer::worker, this);
			}*/
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::setLastPacketReceived()
{
	_lastPacketReceived = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Peer::setRSSI(uint8_t rssi)
{
	try
	{
		if(_centralFeatures || _disposing) return;
		uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if(valuesCentral.find(0) != valuesCentral.end() && valuesCentral.at(0).find("RSSI_DEVICE") != valuesCentral.at(0).end() && (time - _lastRSSI) > 10)
		{
			_lastRSSI = time;
			RPCConfigurationParameter* parameter = &valuesCentral.at(0).at("RSSI_DEVICE");
			parameter->data.at(0) = rssi;

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({"RSSI_DEVICE"}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(parameter->rpcParameter->convertFromPacket(parameter->data));

			GD::rpcClient.broadcastEvent(_serialNumber + ":0", valueKeys, rpcValues);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(!_centralFeatures || _disposing) return;
		if(packet->senderAddress() != address && (!hasTeam() || packet->senderAddress() != team.address)) return;
		if(!rpcDevice) return;
		setLastPacketReceived();
		setRSSI(packet->rssi());
		serviceMessages->endUnreach();
		std::list<uint32_t> paramsetChannels;
		std::map<std::string, std::vector<uint8_t>> values;
		RPC::ParameterSet::Type::Enum parameterSetType;
		std::string frameID;
		getValuesFromPacket(packet, frameID, paramsetChannels, parameterSetType, values);
		std::shared_ptr<RPC::DeviceFrame> frame;
		if(!frameID.empty()) frame = rpcDevice->framesByID.at(frameID);
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());

		std::map<std::string, std::vector<uint8_t>> sentValues;
		std::shared_ptr<BidCoSPacket> sentPacket;
		std::string sentFrameID;
		bool pushPendingQueues = false;
		if(packet->messageType() == 0x02) //ACK packet: Check if all values were set correctly. If not set value again
		{
			sentPacket = GD::devices.getCentral()->getSentPacket(address);
			if(sentPacket && sentPacket->messageType() > 0 && !sentPacket->payload()->empty())
			{
				std::list<uint32_t> sentParamsetChannels;
				RPC::ParameterSet::Type::Enum sentParameterSetType;
				getValuesFromPacket(sentPacket, sentFrameID, sentParamsetChannels, sentParameterSetType, sentValues);
			}
		}

		//Check for low battery
		//If values is not empty, packet is valid
		if(rpcDevice->hasBattery && !values.empty() && !packet->payload()->empty() && frame)
		{
			bool hasLowbatBit = true;
			//Three things to check to see if position 9.7 is used: channelField, subtypeIndex and parameter indices
			if(frame->channelField == 9 && frame->channelFieldSize >= 0.8) hasLowbatBit = false;
			else if(frame->subtypeIndex == 9) hasLowbatBit = false;
			for(std::vector<RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				if(j->index >= 9 && j->index < 10)
				{
					//fmod is needed for sizes > 1 (e. g. for frame WEATHER_EVENT)
					if((j->index + std::fmod(j->size, 1)) >= 9.8) hasLowbatBit = false;
				}
			}

			if(hasLowbatBit)
			{
				if(packet->payload()->at(0) & 0x80) serviceMessages->set("LOWBAT", true);
				else serviceMessages->set("LOWBAT", false);
			}
		}

		bool resendPacket = false;
		for(std::map<std::string, std::vector<uint8_t>>::iterator i = values.begin(); i != values.end(); ++i)
		{
			for(std::list<uint32_t>::const_iterator j = paramsetChannels.begin(); j != paramsetChannels.end(); ++j)
			{
				RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
				parameter->data = i->second;
				 //Process error as service message
				if(i->first.size() >= 5 && i->first.substr(0, 5) == "ERROR" && !i->second.empty() && parameter->rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeEnum)
				{
					int32_t errorValue = (int32_t)i->second.at(0);
					RPC::LogicalParameterEnum* logicalParameter = (RPC::LogicalParameterEnum*)parameter->rpcParameter->logicalParameter.get();
					serviceMessages->set(i->first, i->second.at(0), *j);
				}

				if(sentValues.find(i->first) != sentValues.end())
				{
					if(sentValues.at(i->first) != i->second) resendPacket = true;
				}
				else
				{
					HelperFunctions::printInfo("Info: " + i->first + " of device 0x" + HelperFunctions::getHexString(address) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + HelperFunctions::getHexString(i->second) + ".");
					valueKeys->push_back(i->first);
					rpcValues->push_back(rpcDevice->channels.at(*j)->parameterSets.at(parameterSetType)->getParameter(i->first)->convertFromPacket(i->second, true));
				}
			}
		}

		if(isTeam() && !valueKeys->empty())
		{
			//Set SENDERADDRESS so that the we can identify the sending peer in our home automation software
			std::shared_ptr<Peer> senderPeer(GD::devices.getCentral()->getPeer(packet->destinationAddress()));
			if(senderPeer)
			{
				for(std::list<uint32_t>::const_iterator i = paramsetChannels.begin(); i != paramsetChannels.end(); ++i)
				{
					std::shared_ptr<RPC::Parameter> rpcParameter(rpcDevice->channels.at(*i)->parameterSets.at(parameterSetType)->getParameter("SENDERADDRESS"));
					if(rpcParameter)
					{
						RPCConfigurationParameter* parameter = &valuesCentral[*i]["SENDERADDRESS"];
						parameter->data = rpcParameter->convertToPacket(senderPeer->getSerialNumber() + ":" + std::to_string(*i));
						valueKeys->push_back("SENDERADDRESS");
						rpcValues->push_back(rpcParameter->convertFromPacket(parameter->data, true));
					}
				}
			}
		}

		//We have to do this in a seperate loop, because all parameters need to be set first
		for(std::map<std::string, std::vector<uint8_t>>::iterator i = values.begin(); i != values.end(); ++i)
		{
			for(std::list<uint32_t>::const_iterator j = paramsetChannels.begin(); j != paramsetChannels.end(); ++j)
			{
				handleDominoEvent(rpcDevice->channels.at(*j)->parameterSets.at(parameterSetType)->getParameter(i->first), frameID, *j);
			}
		}

		if(resendPacket && sentPacket)
		{
			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::PEER));
			queue->noSending = true;
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
			*packet = *sentPacket;
			packet->setMessageCounter(messageCounter);
			messageCounter++;
			HelperFunctions::printInfo("Info: Resending values for device 0x" + HelperFunctions::getHexString(address) + " with serial number " + _serialNumber + ", because they were not set correctly.");
			queue->push(sentPacket);
			queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			if((rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)) GD::devices.getCentral()->enqueuePackets(address, queue, true);
			else
			{
				pendingBidCoSQueues->push(queue);
				HelperFunctions::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
			}
		}

		if((rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp) && packet->senderAddress() == address && (packet->controlByte() & 2) && pendingBidCoSQueues && !pendingBidCoSQueues->empty()) //Packet is wake me up packet and not bidirectional
		{
			if(packet->controlByte() & 32) //Bidirectional?
			{
				std::vector<uint8_t> payload;
				payload.push_back(0x00);
				std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(packet->messageCounter(), 0x81, 0x02, GD::devices.getCentral()->address(), address, payload));
				GD::devices.getCentral()->sendPacket(ok);
				GD::devices.getCentral()->enqueuePendingQueues(address);
			}
			else
			{
				std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::DEFAULT));
				queue->noSending = true;
				std::vector<uint8_t> payload;
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(packet->messageCounter(), 0xA1, 0x12, GD::devices.getCentral()->address(), address, payload));
				queue->push(configPacket);
				queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));

				GD::devices.getCentral()->enqueuePackets(address, queue, true);
			}
		}
		else if(!values.empty() && (packet->controlByte() & 32) && packet->destinationAddress() == GD::devices.getCentral()->address())
		{
			GD::devices.getCentral()->sendOK(packet->messageCounter(), packet->senderAddress());
		}
		//else if(((rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)) && pendingBidCoSQueues && !pendingBidCoSQueues->empty())
		//{
			//GD::devices.getCentral()->enqueuePendingQueues(address);
		//}
		if(!rpcValues->empty())
		{
			for(std::list<uint32_t>::const_iterator j = paramsetChannels.begin(); j != paramsetChannels.end(); ++j)
			{
				GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string(*j), valueKeys, rpcValues);
			}
			saveToDatabase(GD::devices.getCentral()->address());
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<RPC::RPCVariable> Peer::getDeviceDescription(int32_t channel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::string type;
		std::shared_ptr<RPC::DeviceType> rpcDeviceType = rpcDevice->getType(deviceType, firmwareVersion);
		if(rpcDeviceType) type = rpcDeviceType->id;
		else
		{
			type = HMDeviceType::getString(deviceType);
			if(type.empty() && !rpcDevice->supportedTypes.empty()) type = rpcDevice->supportedTypes.at(0)->id;
		}

		if(channel == -1) //Base device
		{
			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ADDRESS", _serialNumber)));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray)));
			RPC::RPCVariable* variable = description->structValue->back().get();
			variable->name = "CHILDREN";
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				if(i->second->hidden) continue;
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(i->first))));
			}

			if(firmwareVersion != 0)
			{
				std::ostringstream stringStream;
				stringStream << std::setw(2) << std::hex << (int32_t)firmwareVersion << std::dec;
				std::string firmware = stringStream.str();
				description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FIRMWARE", firmware.substr(0, 1) + "." + firmware.substr(1))));
			}
			else
			{
				description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FIRMWARE", std::string("?"))));
			}

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FLAGS", (int32_t)rpcDevice->uiFlags)));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("INTERFACE", GD::devices.getCentral()->serialNumber())));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray)));
			variable = description->structValue->back().get();
			variable->name = "PARAMSETS";
			variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("MASTER")))); //Always MASTER

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("PARENT", std::string(""))));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("RF_ADDRESS", std::to_string(address))));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ROAMING", 0)));

			if(!type.empty()) description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TYPE", type)));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("VERSION", rpcDevice->version)));
		}
		else
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
			if(rpcChannel->hidden) return description;

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ADDRESS", _serialNumber + ":" + std::to_string(channel))));

			int32_t aesActive = 0;
			if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && !configCentral.at(channel).at("AES_ACTIVE").data.empty() && configCentral.at(channel).at("AES_ACTIVE").data.at(0) != 0)
			{
				aesActive = 1;
			}
			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("AES_ACTIVE", aesActive)));

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
			if(rpcChannel->direction != RPC::DeviceChannel::Direction::Enum::none) direction = (int32_t)rpcChannel->direction;
			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("DIRECTION", direction)));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FLAGS", (int32_t)rpcChannel->uiFlags)));

			int32_t groupedWith = getChannelGroupedWith(channel);
			if(groupedWith > -1)
			{
				description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("GROUP", _serialNumber + ":" + std::to_string(groupedWith))));
			}

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("INDEX", channel)));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("LINK_SOURCE_ROLES", linkSourceRoles.str())));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("LINK_TARGET_ROLES", linkTargetRoles.str())));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray)));
			RPC::RPCVariable* variable = description->structValue->back().get();
			variable->name = "PARAMSETS";
			for(std::map<RPC::ParameterSet::Type::Enum, std::shared_ptr<RPC::ParameterSet>>::iterator j = rpcChannel->parameterSets.begin(); j != rpcChannel->parameterSets.end(); ++j)
			{
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second->typeString())));
			}
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::master) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::master)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::values) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::values)->typeString())));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("PARENT", _serialNumber)));

			if(!type.empty()) description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("PARENT_TYPE", type)));

			if(!teamChannels.empty() && !rpcChannel->teamTag.empty())
			{
				std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				array->name = "TEAM_CHANNELS";
				for(std::vector<std::pair<std::string, uint32_t>>::iterator j = teamChannels.begin(); j != teamChannels.end(); ++j)
				{
					array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first + ":" + std::to_string(j->second))));
				}
				description->structValue->push_back(array);
			}

			if(!team.serialNumber.empty() && rpcChannel->hasTeam)
			{
				description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TEAM", team.serialNumber)));

				description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TEAM_TAG", rpcChannel->teamTag)));
			}
			else if(!_serialNumber.empty() && _serialNumber[0] == '*' && !rpcChannel->teamTag.empty())
			{
				description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TEAM_TAG", rpcChannel->teamTag)));
			}

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TYPE", rpcChannel->type)));

			description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("VERSION", rpcDevice->version)));
		}
		return description;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> Peer::getDeviceDescription()
{
	try
	{
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> descriptions(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
		descriptions->push_back(getDeviceDescription(-1));

		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			std::shared_ptr<RPC::RPCVariable> description = getDeviceDescription(i->first);
			if(!description->structValue->empty()) descriptions->push_back(description);
		}

		return descriptions;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>>();
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty())
		{
			remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);
			if(!remotePeer) return RPC::RPCVariable::createError(-2, "Unknown remote peer.");
		}

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->channels[channel]->parameterSets[type]->id));
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::putParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> variables, bool putUnchanged, bool onlyPushing)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty() || variables->structValue->front()->name.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

		if(type == RPC::ParameterSet::Type::Enum::master)
		{
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> changedParameters;
			//allParameters is necessary to temporarily store all values. It is used to set changedParameters.
			//This is necessary when there are multiple variables per index and not all of them are changed.
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> allParameters;
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if((*i)->name.empty()) continue;
				std::vector<uint8_t> value;
				if(configCentral[channel].find((*i)->name) == configCentral[channel].end()) continue;
				RPCConfigurationParameter* parameter = &configCentral[channel][(*i)->name];
				if(!parameter->rpcParameter) continue;
				value = parameter->rpcParameter->convertToPacket(*i);
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
				//Continue when value is unchanged except parameter is of the same index as a changed one.
				if(parameter->data == value && !putUnchanged) continue;
				parameter->data = value;
				HelperFunctions::printInfo("Info: Parameter " + (*i)->name + " of device 0x" + HelperFunctions::getHexString(address) + " was set to 0x" + HelperFunctions::getHexString(allParameters[list][intIndex]) + ".");
				//Only send to device when parameter is of type config
				if(parameter->rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::config && parameter->rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::configString) continue;
				changedParameters[list][intIndex] = allParameters[list][intIndex];
			}

			if(changedParameters.empty() || changedParameters.begin()->second.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			queue->noSending = true;
			serviceMessages->setConfigPending(true);
			std::vector<uint8_t> payload;
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
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
				if(firstPacket && (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)) controlByte |= 0x10;
				firstPacket = false;
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(messageCounter, controlByte, 0x01, central->address(), address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				messageCounter++;

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
							configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
							queue->push(configPacket);
							queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
							payload.clear();
							messageCounter++;
							payload.push_back(channel);
							payload.push_back(0x08);
						}
					}
				}
				if(payload.size() > 2)
				{
					configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
					queue->push(configPacket);
					queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
					payload.clear();
					messageCounter++;
				}
				else payload.clear();

				//END_CONFIG
				payload.push_back(channel);
				payload.push_back(0x06);
				configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				messageCounter++;
			}

			pendingBidCoSQueues->push(queue);
			if(!onlyPushing && ((rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst))) GD::devices.getCentral()->enqueuePendingQueues(address);
			else HelperFunctions::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
		}
		else if(type == RPC::ParameterSet::Type::Enum::values)
		{
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if((*i)->name.empty()) continue;
				setValue(channel, (*i)->name, (*i));
			}
		}
		else if(type == RPC::ParameterSet::Type::Enum::link)
		{
			std::shared_ptr<BasicPeer> remotePeer;
			if(!remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);
			if(!remotePeer) return RPC::RPCVariable::createError(-3, "Not paired to this peer.");
			if(linksCentral[channel].find(remotePeer->address) == linksCentral[channel].end()) RPC::RPCVariable::createError(-3, "Unknown parameter set.");
			if(linksCentral[channel][address].find(remotePeer->channel) == linksCentral[channel][address].end()) RPC::RPCVariable::createError(-3, "Unknown parameter set.");

			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> changedParameters;
			//allParameters is necessary to temporarily store all values. It is used to set changedParameters.
			//This is necessary when there are multiple variables per index and not all of them are changed.
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> allParameters;
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if((*i)->name.empty()) continue;
				std::vector<uint8_t> value;
				if(linksCentral[channel][remotePeer->address][remotePeer->channel].find((*i)->name) == linksCentral[channel][remotePeer->address][remotePeer->channel].end()) continue;
				RPCConfigurationParameter* parameter = &linksCentral[channel][remotePeer->address][remotePeer->channel][(*i)->name];
				if(!parameter->rpcParameter) continue;
				value = parameter->rpcParameter->convertToPacket(*i);
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
				//Continue when value is unchanged except parameter is of the same index as a changed one.
				if(parameter->data == value && !putUnchanged) continue;
				parameter->data = value;
				HelperFunctions::printInfo("Info: Parameter " + (*i)->name + " of device 0x" + HelperFunctions::getHexString(address) + " was set to 0x" + HelperFunctions::getHexString(allParameters[list][intIndex]) + ".");
				//Only send to device when parameter is of type config
				if(parameter->rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::config && parameter->rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::configString) continue;
				changedParameters[list][intIndex] = allParameters[list][intIndex];
			}

			if(changedParameters.empty() || changedParameters.begin()->second.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			queue->noSending = true;
			if(serviceMessages) serviceMessages->setConfigPending(true);
			std::vector<uint8_t> payload;
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
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
				if(firstPacket && (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)) controlByte |= 0x10;
				firstPacket = false;
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(messageCounter, controlByte, 0x01, central->address(), address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				messageCounter++;

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
							configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
							queue->push(configPacket);
							queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
							payload.clear();
							messageCounter++;
							payload.push_back(channel);
							payload.push_back(0x08);
						}
					}
				}
				if(payload.size() > 2)
				{
					configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
					queue->push(configPacket);
					queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
					payload.clear();
					messageCounter++;
				}
				else payload.clear();

				//END_CONFIG
				payload.push_back(channel);
				payload.push_back(0x06);
				configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				messageCounter++;
			}

			pendingBidCoSQueues->push(queue);
			if(!onlyPushing && ((rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst))) GD::devices.getCentral()->enqueuePendingQueues(address);
			else HelperFunctions::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> variables(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);

		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::internal)) continue;
			std::shared_ptr<RPC::RPCVariable> element;
			if(type == RPC::ParameterSet::Type::Enum::values)
			{
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = valuesCentral[channel][(*i)->id].rpcParameter->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}
			else if(type == RPC::ParameterSet::Type::Enum::master)
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
			else element = RPC::RPCVariable::createError(-3, "Unknown parameter set.");

			if(!element || element->errorStruct) continue;
			if(element->type == RPC::RPCVariableType::rpcVoid) continue;
			element->name = (*i)->id;
			variables->structValue->push_back(element);
		}
		return variables;
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(_peers.find(senderChannel) == _peers.end()) return RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverSerialNumber, receiverChannel);
		if(!remotePeer) return RPC::RPCVariable::createError(-2, "No peer found for sender channel and receiver serial number.");
		if(remotePeer->channel != receiverChannel)  RPC::RPCVariable::createError(-2, "No peer found for receiver channel.");
		std::shared_ptr<RPC::RPCVariable> response(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		response->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("DESCRIPTION", remotePeer->linkDescription)));
		response->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("NAME", remotePeer->linkName)));
		return response;
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::setLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(_peers.find(senderChannel) == _peers.end()) return RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverSerialNumber);
		if(!remotePeer) return RPC::RPCVariable::createError(-2, "No peer found for sender channel and receiver serial number.");
		if(remotePeer->channel != receiverChannel)  RPC::RPCVariable::createError(-2, "No peer found for receiver channel.");
		remotePeer->linkDescription = description;
		remotePeer->linkName = name;
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLinkPeers(int32_t channel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		if(channel > -1)
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			//Return if there are no link roles defined
			std::shared_ptr<RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
			if(!linkRoles) return array;
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<Peer> peer(central->getPeer((*i)->address));
				bool peerKnowsMe = false;
				if(peer && peer->getPeer(channel, address)) peerKnowsMe = true;

				std::string peerSerial = (*i)->serialNumber;
				if((*i)->serialNumber.empty())
				{
					if(peerKnowsMe)
					{
						(*i)->serialNumber = peer->getSerialNumber();
						peerSerial = (*i)->serialNumber;
					}
					/*else if((*i)->address == address) //Link to myself with non-existing (virtual) channel (e. g. switches use this)
					{
						(*i)->serialNumber = _serialNumber;
						peerSerial = (*i)->serialNumber;
					}*/
					else
					{
						//Peer not paired to central
						std::ostringstream stringstream;
						stringstream << '@' << std::hex << std::setw(6) << std::setfill('0') << (*i)->address;
						peerSerial = stringstream.str();
					}
				}
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peerSerial + ":" + std::to_string((*i)->channel))));
			}
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				std::shared_ptr<RPC::RPCVariable> linkPeers = getLinkPeers(i->first);
				array->arrayValue->insert(array->arrayValue->end(), linkPeers->arrayValue->begin(), linkPeers->arrayValue->end());
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLink(int32_t channel, int32_t flags, bool avoidDuplicates)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> element;
		bool groupFlag = false;
		if(flags & 0x01) groupFlag = true;
		if(channel > -1 && !groupFlag) //Get link of single channel
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			bool isSender = false;
			//Return if there are no link roles defined
			std::shared_ptr<RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
			if(!linkRoles) return array;
			if(!linkRoles->sourceNames.empty()) isSender = true;
			else if(linkRoles->targetNames.empty()) return array;
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<Peer> remotePeer(central->getPeer((*i)->address));
				bool peerKnowsMe = false;
				if(remotePeer && remotePeer->getPeer((*i)->channel, address, channel)) peerKnowsMe = true;

				//Don't continue if peer is sender and exists in central's peer array to avoid generation of duplicate results when requesting all links (only generate results when we are sender)
				if(!isSender && peerKnowsMe && avoidDuplicates) return array;
				//If we are receiver this point is only reached, when the sender is not paired to this central

				std::string peerSerial = (*i)->serialNumber;
				int32_t brokenFlags = 0;
				if((*i)->serialNumber.empty())
				{
					if(peerKnowsMe ||
					  (*i)->address == address) //Link to myself with non-existing (virtual) channel (e. g. switches use this)
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
				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
				element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("DESCRIPTION", (*i)->linkDescription)));
				element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FLAGS", brokenFlags)));
				element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("NAME", (*i)->linkName)));
				if(isSender)
				{
					element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("RECEIVER", peerSerial + ":" + std::to_string((*i)->channel))));
					if(flags & 4)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, RPC::ParameterSet::Type::Enum::link, _serialNumber, channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						paramset->name = "RECEIVER_PARAMSET";
						element->structValue->push_back(paramset);
					}
					if(flags & 16)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 2) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel);
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						description->name = "RECEIVER_DESCRIPTION";
						element->structValue->push_back(description);
					}
					element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("SENDER", _serialNumber + ":" + std::to_string(channel))));
					if(flags & 2)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1)) paramset = getParamset(channel, RPC::ParameterSet::Type::Enum::link, peerSerial, (*i)->channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						paramset->name = "SENDER_PARAMSET";
						element->structValue->push_back(paramset);
					}
					if(flags & 8)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 1)) description = getDeviceDescription(channel);
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						description->name = "SENDER_DESCRIPTION";
						element->structValue->push_back(description);
					}
				}
				else //When sender is broken
				{
					element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("RECEIVER", _serialNumber + ":" + std::to_string(channel))));
					if(flags & 4)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						//Sender is broken, so just insert empty paramset
						paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						paramset->name = "RECEIVER_PARAMSET";
						element->structValue->push_back(paramset);
					}
					element->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("SENDER", peerSerial + ":" + std::to_string((*i)->channel))));
					if(flags & 2)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						//Sender is broken, so just insert empty paramset
						paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						paramset->name = "SENDER_PARAMSET";
						element->structValue->push_back(paramset);
					}
				}
				array->arrayValue->push_back(element);
			}
		}
		else
		{
			if(channel > -1 && groupFlag) //Get links for each grouped channel
			{
				if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
				std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
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
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					element = getLink(i->first, flags, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetDescription(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set");

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);

		std::shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> descriptions(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> element;
		uint32_t index = 0;
		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::internal)) continue;
			description->name = (*i)->id;
			if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeBoolean)
			{
				RPC::LogicalParameterBoolean* parameter = (RPC::LogicalParameterBoolean*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "CONTROL";
					element->stringValue = (*i)->control;
					description->structValue->push_back(element);
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
					element->name = "DEFAULT";
					element->booleanValue = parameter->defaultValue;
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "FLAGS";
				element->integerValue = (*i)->uiFlags;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "ID";
				element->stringValue = (*i)->id;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "MAX";
				element->booleanValue = parameter->max;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "MIN";
				element->booleanValue = parameter->min;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "OPERATIONS";
				element->integerValue = (*i)->operations;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "TAB_ORDER";
				element->integerValue = index;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "TYPE";
				element->stringValue = "BOOL";
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "UNIT";
				element->stringValue = parameter->unit;
				description->structValue->push_back(element);
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeString)
			{
				RPC::LogicalParameterString* parameter = (RPC::LogicalParameterString*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "CONTROL";
					element->stringValue = (*i)->control;
					description->structValue->push_back(element);
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "DEFAULT";
					element->stringValue = parameter->defaultValue;
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "FLAGS";
				element->integerValue = (*i)->uiFlags;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "ID";
				element->stringValue = (*i)->id;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "MAX";
				element->stringValue = parameter->max;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "MIN";
				element->stringValue = parameter->min;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "OPERATIONS";
				element->integerValue = (*i)->operations;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "TAB_ORDER";
				element->integerValue = index;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "TYPE";
				element->stringValue = "STRING";
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "UNIT";
				element->stringValue = parameter->unit;
				description->structValue->push_back(element);
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeAction)
			{
				RPC::LogicalParameterAction* parameter = (RPC::LogicalParameterAction*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "CONTROL";
					element->stringValue = (*i)->control;
					description->structValue->push_back(element);
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
					element->name = "DEFAULT";
					element->booleanValue = parameter->defaultValue;
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "FLAGS";
				element->integerValue = (*i)->uiFlags;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "ID";
				element->stringValue = (*i)->id;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "MAX";
				element->booleanValue = parameter->max;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "MIN";
				element->booleanValue = parameter->min;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "OPERATIONS";
				element->integerValue = (*i)->operations;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "TAB_ORDER";
				element->integerValue = index;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "TYPE";
				element->stringValue = "ACTION";
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "UNIT";
				element->stringValue = parameter->unit;
				description->structValue->push_back(element);
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeInteger)
			{
				RPC::LogicalParameterInteger* parameter = (RPC::LogicalParameterInteger*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "CONTROL";
					element->stringValue = (*i)->control;
					description->structValue->push_back(element);
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
					element->name = "DEFAULT";
					element->integerValue = parameter->defaultValue;
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "FLAGS";
				element->integerValue = (*i)->uiFlags;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "ID";
				element->stringValue = (*i)->id;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "MAX";
				element->integerValue = parameter->max;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "MIN";
				element->integerValue = parameter->min;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "OPERATIONS";
				element->integerValue = (*i)->operations;
				description->structValue->push_back(element);

				if(!parameter->specialValues.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					element->name = "SPECIAL";
					for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ID", j->first)));
						specialElement->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("VALUE", j->second)));
						element->structValue->push_back(specialElement);
					}
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "TAB_ORDER";
				element->integerValue = index;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "TYPE";
				element->stringValue = "INTEGER";
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "UNIT";
				element->stringValue = parameter->unit;
				description->structValue->push_back(element);
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeEnum)
			{
				RPC::LogicalParameterEnum* parameter = (RPC::LogicalParameterEnum*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "CONTROL";
					element->stringValue = (*i)->control;
					description->structValue->push_back(element);
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
					element->name = "DEFAULT";
					element->integerValue = parameter->defaultValue;
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "FLAGS";
				element->integerValue = (*i)->uiFlags;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "ID";
				element->stringValue = (*i)->id;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "MAX";
				element->integerValue = parameter->max;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "MIN";
				element->integerValue = parameter->min;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "OPERATIONS";
				element->integerValue = (*i)->operations;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "TAB_ORDER";
				element->integerValue = index;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "TYPE";
				element->stringValue = "ENUM";
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "UNIT";
				element->stringValue = parameter->unit;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				element->name = "VALUE_LIST";
				for(std::vector<RPC::ParameterOption>::iterator j = parameter->options.begin(); j != parameter->options.end(); ++j)
				{
					element->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->id)));
				}
				description->structValue->push_back(element);
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeFloat)
			{
				RPC::LogicalParameterFloat* parameter = (RPC::LogicalParameterFloat*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->name = "CONTROL";
					element->stringValue = (*i)->control;
					description->structValue->push_back(element);
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
					element->name = "DEFAULT";
					element->floatValue = parameter->defaultValue;
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "FLAGS";
				element->integerValue = (*i)->uiFlags;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "ID";
				element->stringValue = (*i)->id;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
				element->name = "MAX";
				element->floatValue = parameter->max;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
				element->name = "MIN";
				element->floatValue = parameter->min;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "OPERATIONS";
				element->integerValue = (*i)->operations;
				description->structValue->push_back(element);

				if(!parameter->specialValues.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					element->name = "SPECIAL";
					for(std::unordered_map<std::string, double>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ID", j->first)));
						specialElement->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("VALUE", j->second)));
						element->structValue->push_back(specialElement);
					}
					description->structValue->push_back(element);
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "TAB_ORDER";
				element->integerValue = index;
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "TYPE";
				element->stringValue = "FLOAT";
				description->structValue->push_back(element);

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->name = "UNIT";
				element->stringValue = parameter->unit;
				description->structValue->push_back(element);
			}

			index++;
			descriptions->structValue->push_back(description);
			description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		}
		return descriptions;
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getServiceMessages()
{
	if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
	if(!serviceMessages) return RPC::RPCVariable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get();
}

std::shared_ptr<RPC::RPCVariable> Peer::getValue(uint32_t channel, std::string valueKey)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		return valuesCentral[channel][valueKey].rpcParameter->convertFromPacket(valuesCentral[channel][valueKey].data);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

bool Peer::setHomegearValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(deviceType == HMDeviceTypes::HMCCVD && valueKey == "VALVE_STATE" && _peers.find(1) != _peers.end() && _peers[1].size() > 0 && _peers[1].at(0)->hidden)
		{
			if(!_peers[1].at(0)->device)
			{
				_peers[1].at(0)->device = GD::devices.get(_peers[1].at(0)->address);
				if(_peers[1].at(0)->device->deviceType() != HMDeviceTypes::HMCCTC) return false;
			}
			if(_peers[1].at(0)->device)
			{
				if(_peers[1].at(0)->device->deviceType() != HMDeviceTypes::HMCCTC) return false;
				HM_CC_TC* tc = (HM_CC_TC*)_peers[1].at(0)->device.get();
				tc->setValveState(value->integerValue);
				std::shared_ptr<RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
				if(!rpcParameter) return false;
				valuesCentral[channel][valueKey].data = rpcParameter->convertToPacket(value);
				HelperFunctions::printInfo("Info: Setting valve state of HM-CC-VD with address 0x" + HelperFunctions::getHexString(address) + " to " + std::to_string(value->integerValue) + "%.");
				return true;
			}
		}
		else if(deviceType == HMDeviceTypes::HMSECSD)
		{
			if(valueKey == "STATE")
			{
				std::shared_ptr<RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
				if(!rpcParameter) return false;
				valuesCentral[channel][valueKey].data = rpcParameter->convertToPacket(value);

				std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
				std::shared_ptr<Peer> associatedPeer = central->getPeer(address);
				if(!associatedPeer)
				{
					HelperFunctions::printError("Error: Could not handle \"STATE\", because the main team peer is not paired to this central.");
					return false;
				}
				if(associatedPeer->team.data.empty()) associatedPeer->team.data.push_back(0);
				std::vector<uint8_t> payload;
				payload.push_back(0x01);
				payload.push_back(associatedPeer->team.data.at(0));
				associatedPeer->team.data.at(0)++;
				if(value->booleanValue) payload.push_back(0xC8);
				else payload.push_back(0x01);
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(central->messageCounter()->at(0), 0x94, 0x41, address, central->address(), payload));
				central->messageCounter()->at(0)++;
				central->sendPacketMultipleTimes(packet, address, 6, 1000, true);
				return true;
			}
			else if(valueKey == "INSTALL_TEST")
			{
				std::shared_ptr<RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
				if(!rpcParameter) return false;
				valuesCentral[channel][valueKey].data = rpcParameter->convertToPacket(value);

				std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
				std::shared_ptr<Peer> associatedPeer = central->getPeer(address);
				if(!associatedPeer)
				{
					HelperFunctions::printError("Error: Could not handle \"INSTALL_TEST\", because the main team peer is not paired to this central.");
					return false;
				}
				if(associatedPeer->team.data.empty()) associatedPeer->team.data.push_back(0);
				std::vector<uint8_t> payload;
				payload.push_back(0x00);
				payload.push_back(associatedPeer->team.data.at(0));
				associatedPeer->team.data.at(0)++;
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(central->messageCounter()->at(0), 0x94, 0x40, address, central->address(), payload));
				central->messageCounter()->at(0)++;
				central->sendPacketMultipleTimes(packet, address, 6, 600, true);
				return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return false;
}

void Peer::addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters)
{
	try
	{
		if(parameters->integers.size() != 3) return;
		if(parameters->strings.size() != 1) return;
		HelperFunctions::printMessage("addVariableToResetCallback invoked for parameter " + parameters->strings.at(0) + " of device 0x" + HelperFunctions::getHexString(address) + " with serial number " + _serialNumber + ".", 5);
		std::shared_ptr<VariableToReset> variable(new VariableToReset);
		variable->channel = parameters->integers.at(0);
		int32_t integerValue = parameters->integers.at(1);
		HelperFunctions::memcpyBigEndian(variable->data, integerValue);
		variable->resetTime = parameters->integers.at(2);
		variable->key = parameters->strings.at(0);
		_variablesToResetMutex.lock();
		_variablesToReset.push_back(variable);
		_variablesToResetMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<RPC::RPCVariable> Peer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return RPC::RPCVariable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		std::shared_ptr<RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {valueKey});
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values(new std::vector<std::shared_ptr<RPC::RPCVariable>> { value });
		if(rpcParameter->physicalParameter->interface == RPC::PhysicalParameter::Interface::Enum::store)
		{
			valuesCentral[channel][valueKey].data = rpcParameter->convertToPacket(value);
			GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		}
		else if(rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::command) return RPC::RPCVariable::createError(-6, "Parameter is not settable.");
		if(!rpcParameter->conversion.empty() && rpcParameter->conversion.at(0)->type == RPC::ParameterConversion::Type::Enum::toggle)
		{
			//Handle toggle parameter
			std::string toggleKey = rpcParameter->conversion.at(0)->stringValue;
			if(toggleKey.empty()) return RPC::RPCVariable::createError(-6, "No toggle parameter specified (parameter attribute value is empty).");
			if(valuesCentral[channel].find(toggleKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Toggle parameter not found.");
			RPCConfigurationParameter* toggleParam = &valuesCentral[channel][toggleKey];
			std::shared_ptr<RPC::RPCVariable> toggleValue;
			if(toggleParam->rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeBoolean)
			{
				toggleValue = toggleParam->rpcParameter->convertFromPacket(toggleParam->data);
				toggleValue->booleanValue = !toggleValue->booleanValue;
			}
			else if(toggleParam->rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeInteger ||
					toggleParam->rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeFloat)
			{
				int32_t currentToggleValue = (int32_t)toggleParam->data.at(0);
				if(currentToggleValue != toggleParam->rpcParameter->conversion.at(0)->on) toggleParam->data.at(0) = toggleParam->rpcParameter->conversion.at(0)->on;
				else toggleParam->data.at(0) = toggleParam->rpcParameter->conversion.at(0)->off;
				toggleValue = toggleParam->rpcParameter->convertFromPacket(toggleParam->data);
			}
			else return RPC::RPCVariable::createError(-6, "Toggle parameter has to be of type boolean, float or integer.");
			return setValue(channel, toggleKey, toggleValue);
		}
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return RPC::RPCVariable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return RPC::RPCVariable::createError(-6, "frame not found");
		std::shared_ptr<RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		valuesCentral[channel][valueKey].data = rpcParameter->convertToPacket(value);
		HelperFunctions::printDebug("Debug: " + valueKey + " of device 0x" + HelperFunctions::getHexString(address) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to " + HelperFunctions::getHexString(valuesCentral[channel][valueKey].data) + ".");

		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::PEER));
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
		if(rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst) controlByte |= 0x10;
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, controlByte, (uint8_t)frame->type, GD::devices.getCentral()->address(), address, payload));
		for(std::vector<RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				std::vector<uint8_t> data;
				HelperFunctions::memcpyBigEndian(data, i->constValue);
				packet->setPosition(i->index, i->size, data);
				continue;
			}
			//We can't just search for param, because it is ambiguous (see for example LEVEL for HM-CC-TC.
			if(i->param.empty() && !i->additionalParameter.empty() && valuesCentral[channel].find(i->additionalParameter) != valuesCentral[channel].end())
			{
				int32_t intValue = 0;
				HelperFunctions::memcpyBigEndian(intValue, valuesCentral[channel][i->additionalParameter].data);
				if(!i->omitIfSet || intValue != i->omitIf)
				{
					//Don't set ON_TIME when value is false
					if(i->additionalParameter != "ON_TIME" || value->booleanValue) packet->setPosition(i->index, i->size, valuesCentral[channel][i->additionalParameter].data);
				}
			}
			//param sometimes is ambiguous (e. g. LEVEL of HM-CC-TC), so don't search and use the given parameter when possible
			else if(i->param == rpcParameter->physicalParameter->valueID || i->param == rpcParameter->physicalParameter->id) packet->setPosition(i->index, i->size, valuesCentral[channel][valueKey].data);
			//Search for all other parameters
			else
			{
				bool paramFound = false;
				for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator j = valuesCentral[channel].begin(); j != valuesCentral[channel].end(); ++j)
				{
					//Only compare id. Till now looking for value_id was not necessary.
					if(i->param == j->second.rpcParameter->physicalParameter->id)
					{
						packet->setPosition(i->index, i->size, j->second.data);
						paramFound = true;
						break;
					}
				}
				if(!paramFound) HelperFunctions::printError("Error constructing packet. param \"" + i->param + "\" not found. Device: 0x" + HelperFunctions::getHexString(address) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
			}
			if(i->additionalParameter == "ON_TIME")
			{
				if(rpcParameter->physicalParameter->valueID != "STATE" || rpcParameter->logicalParameter->type != RPC::LogicalParameter::Type::Enum::typeBoolean)
				{
					HelperFunctions::printError("Error: Can't set \"ON_TIME\" for " + rpcParameter->physicalParameter->valueID + ". Currently \"ON_TIME\" is only supported for \"STATE\" of type \"boolean\". Device: 0x" + HelperFunctions::getHexString(address) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					continue;
				}
				if(!_variablesToReset.empty())
				{
					_variablesToResetMutex.lock();
					for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
					{
						if((*i)->channel == channel && (*i)->key == rpcParameter->physicalParameter->valueID)
						{
							HelperFunctions::printMessage("Debug: Deleting element from _variablesToReset. Device: 0x" + HelperFunctions::getHexString(address) + " Serial number: " + _serialNumber + " Frame: " + frame->id, 5);
							_variablesToReset.erase(i);
							break; //The key should only be once in the vector, so breaking is ok and we can't continue as the iterator is invalidated.
						}
					}
					_variablesToResetMutex.unlock();
				}
				if(!value->booleanValue || valuesCentral[channel][i->additionalParameter].data.empty() || valuesCentral[channel][i->additionalParameter].data.at(0) == 0) continue;
				std::shared_ptr<CallbackFunctionParameter> parameters(new CallbackFunctionParameter());
				parameters->integers.push_back(channel);
				parameters->integers.push_back(0); //false = off
				int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				parameters->integers.push_back(time + std::lround(i->convertFromPacket(valuesCentral[channel][i->additionalParameter].data)->floatValue * 1000));
				parameters->strings.push_back(rpcParameter->physicalParameter->valueID);
				queue->callbackParameter = parameters;
				queue->queueEmptyCallback = delegate<void (std::shared_ptr<CallbackFunctionParameter>)>::from_method<Peer, &Peer::addVariableToResetCallback>(this);
				/*if(!_workerThread.joinable())
				{
					_stopThreads = false;
					_workerThread = std::thread(&Peer::worker, this);
				}*/
			}
		}
		if(!rpcParameter->physicalParameter->resetAfterSend.empty())
		{
			for(std::vector<std::string>::iterator j = rpcParameter->physicalParameter->resetAfterSend.begin(); j != rpcParameter->physicalParameter->resetAfterSend.end(); ++j)
			{
				if(valuesCentral.at(channel).find(*j) == valuesCentral.at(channel).end()) continue;
				std::shared_ptr<RPC::RPCVariable> logicalDefaultValue = valuesCentral.at(channel).at(*j).rpcParameter->logicalParameter->getDefaultValue();
				std::vector<uint8_t> defaultValue = valuesCentral.at(channel).at(*j).rpcParameter->convertToPacket(logicalDefaultValue);
				if(defaultValue != valuesCentral.at(channel).at(*j).data)
				{
					valuesCentral.at(channel).at(*j).data = defaultValue;
					HelperFunctions::printInfo( "Info: Parameter \"" + *j + "\" was reset to " + HelperFunctions::getHexString(defaultValue) + ". Device: 0x" + HelperFunctions::getHexString(address) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					valueKeys->push_back(*j);
					values->push_back(logicalDefaultValue);
				}
			}
		}
		messageCounter++;
		queue->push(packet);
		queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		pendingBidCoSQueues->push(queue);
		if((rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst))
		{
			if(valueKey == "STATE") queue->retries = 12;
			GD::devices.getCentral()->enqueuePendingQueues(address);
		}
		else HelperFunctions::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");

		GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string(channel), valueKeys, values);

		saveToDatabase(GD::devices.getCentral()->address());
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _variablesToResetMutex.unlock();
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _variablesToResetMutex.unlock();
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        _variablesToResetMutex.unlock();
    }
    return RPC::RPCVariable::createError(-1, "unknown error");
}
