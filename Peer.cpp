class CallbackFunctionParameter;

#include "Peer.h"
#include "GD.h"

void Peer::initializeCentralConfig()
{
	if(!rpcDevice)
	{
		if(GD::debugLevel >= 3) std::cout << "Warning: Tried to initialize peer's central config without xmlrpcDevice being set." << std::endl;
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
				if(!(*j)->id.empty())
				{
					parameter = RPCConfigurationParameter();
					parameter.rpcParameter = *j;
					parameter.value = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
					configCentral[i->first][(*j)->id] = parameter;
				}
			}
		}
		if(i->second->parameterSets.find(RPC::ParameterSet::Type::values) != i->second->parameterSets.end() && i->second->parameterSets[RPC::ParameterSet::Type::values])
		{
			std::shared_ptr<RPC::ParameterSet> valueSet = i->second->parameterSets[RPC::ParameterSet::Type::values];
			for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = valueSet->parameters.begin(); j != valueSet->parameters.end(); ++j)
			{
				if(!(*j)->id.empty())
				{
					parameter = RPCConfigurationParameter();
					parameter.rpcParameter = *j;
					parameter.value = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
					valuesCentral[i->first][(*j)->id] = parameter;
				}
			}
		}
	}
}

void Peer::initializeLinkConfig(int32_t channel, int32_t address)
{
	if(rpcDevice->channels[channel]->parameterSets.find(RPC::ParameterSet::Type::link) == rpcDevice->channels[channel]->parameterSets.end()) return;
	std::shared_ptr<RPC::ParameterSet> linkSet = rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::link];
	RPCConfigurationParameter parameter;
	for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = linkSet->parameters.begin(); j != linkSet->parameters.end(); ++j)
	{
		if(!(*j)->id.empty())
		{
			parameter = RPCConfigurationParameter();
			parameter.rpcParameter = *j;
			parameter.value = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
			linksCentral[channel][address][(*j)->id] = parameter;
		}
	}
}

Peer::~Peer()
{
	try
	{
		_stopWorkerThread = true;
		if(_workerThread.joinable()) _workerThread.join();
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

Peer::Peer()
{
	serviceMessages = std::shared_ptr<ServiceMessages>(new ServiceMessages(""));
	pendingBidCoSQueues = std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>>(new std::queue<std::shared_ptr<BidCoSQueue>>());
}

Peer::Peer(std::string serializedObject, HomeMaticDevice* device) : Peer()
{
	try
	{
		pendingBidCoSQueues = std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>>(new std::queue<std::shared_ptr<BidCoSQueue>>());
		if(serializedObject.empty()) return;
		if(GD::debugLevel >= 5) std::cout << "Unserializing peer: " << serializedObject << std::endl;

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
		if(!rpcDevice && GD::debugLevel >= 2) std::cout << "Error: Device type not found: 0x" << std::hex << (uint32_t)deviceType << " Firmware version: " << firmwareVersion << std::endl;
		messageCounter = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		pairingComplete = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
		team.address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		stringSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
		if(stringSize > 0) { team.serialNumber = serializedObject.substr(pos, stringSize); pos += stringSize; }
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
			}
		}
		unserializeConfig(serializedObject, configCentral, RPC::ParameterSet::Type::master, pos);
		unserializeConfig(serializedObject, valuesCentral, RPC::ParameterSet::Type::values, pos);
		unserializeConfig(serializedObject, linksCentral, RPC::ParameterSet::Type::link, pos);
		uint32_t serializedServiceMessagesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		serviceMessages.reset(new ServiceMessages(_serialNumber, serializedObject.substr(pos, serializedServiceMessagesSize))); pos += serializedServiceMessagesSize;
		uint32_t pendingQueuesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < pendingQueuesSize; i++)
		{
			uint32_t queueLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			if(queueLength > 6)
			{
				std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(serializedObject.substr(pos, queueLength), device)); pos += queueLength;
				queue->noSending = true;
				if(queue->getQueueType() == BidCoSQueueType::UNPAIRING || queue->getQueueType() == BidCoSQueueType::CONFIG) queue->serviceMessages = serviceMessages;
				bool hasCallbackFunction = std::stol(serializedObject.substr(pos, 1)); pos += 1;
				if(hasCallbackFunction)
				{
					std::shared_ptr<CallbackFunctionParameter> parameters(new CallbackFunctionParameter());
					parameters->integers.push_back(std::stoll(serializedObject.substr(pos, 4), 0, 16)); pos += 4;
					uint32_t keyLength = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
					parameters->strings.push_back(serializedObject.substr(pos, keyLength)); pos += keyLength;
					parameters->integers.push_back(std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
					parameters->integers.push_back(std::stoll(serializedObject.substr(pos, 8), 0, 16) * 1000); pos += 8;
					queue->callbackParameter = parameters;
					queue->queueEmptyCallback = delegate<void (std::shared_ptr<CallbackFunctionParameter>)>::from_method<Peer, &Peer::addVariableToResetCallback>(this);
				}
				pendingBidCoSQueues->push(queue);
			}
		}
		uint32_t variablesToResetSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < variablesToResetSize; i++)
		{
			std::shared_ptr<VariableToReset> variable(new VariableToReset());
			variable->channel = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
			uint32_t keyLength = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
			variable->key = serializedObject.substr(pos, keyLength); pos += keyLength;
			variable->value = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			variable->resetTime = std::stoll(serializedObject.substr(pos, 8), 0, 16) * 1000; pos += 8;
			variable->isDominoEvent = std::stol(serializedObject.substr(pos, 1)); pos += 1;
			_variablesToReset.push_back(variable);
		}
		if(!_variablesToReset.empty())
		{
			_stopWorkerThread = false;
			_workerThread = std::thread(&Peer::worker, this);
		}
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void Peer::worker()
{
	std::chrono::milliseconds sleepingTime(3000);
	std::vector<uint32_t> positionsToDelete;
	std::vector<std::shared_ptr<VariableToReset>> variablesToReset;
	int32_t index;
	int64_t time;
	while(!_stopWorkerThread)
	{
		try
		{
			std::this_thread::sleep_for(sleepingTime);
			time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			if(!_variablesToReset.empty())
			{
				index = 0;
				_variablesToResetMutex.lock();
				for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
				{
					if((*i)->resetTime + 5000 <= time)
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
						valuesCentral.at((*i)->channel).at((*i)->key).value = (*i)->value;
						std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {(*i)->key});
						std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>> { valuesCentral.at((*i)->channel).at((*i)->key).rpcParameter->convertFromPacket((*i)->value) });
						if(GD::debugLevel >= 4) std::cout << "Info: Domino event: " << (*i)->key << " of device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << " was set to " << (*i)->value << "." << std::endl;
						GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string((*i)->channel), valueKeys, rpcValues);
					}
					else
					{
						std::shared_ptr<RPC::RPCVariable> rpcValue = valuesCentral.at((*i)->channel).at((*i)->key).rpcParameter->convertFromPacket((*i)->value);
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
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
			_variablesToResetMutex.unlock();
		}
		catch(const Exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
			_variablesToResetMutex.unlock();
		}
		catch(...)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
			_variablesToResetMutex.unlock();
		}
	}
}

void Peer::addPeer(int32_t channel, std::shared_ptr<BasicPeer> peer)
{
	//Allow unknown channel for myself. Needed e. g. for switches.
	if(peer->address != address && rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
	_peers[channel].push_back(peer);
	initializeLinkConfig(channel, peer->address);
}

std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, std::string serialNumber)
{
	for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
	{
		if((*i)->serialNumber.empty())
		{
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
			if(central && central->getPeers()->find((*i)->address) != central->getPeers()->end())
			{
				(*i)->serialNumber = central->getPeers()->at((*i)->address)->getSerialNumber();
			}
		}
		if((*i)->serialNumber == serialNumber) return *i;
	}
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, int32_t address)
{
	for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
	{
		if((*i)->address == address) return *i;
	}
	return std::shared_ptr<BasicPeer>();
}

void Peer::removePeer(int32_t channel, int32_t address)
{
	for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
	{
		if((*i)->address == address)
		{
			_peers[channel].erase(i);
			linksCentral[channel].erase(linksCentral[channel].find(address));
			return;
		}
	}
}

void Peer::deleteFromDatabase(int32_t parentAddress)
{
	DataColumnVector data;
	data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_serialNumber)));
	GD::db.executeCommand("DELETE FROM metadata WHERE objectID=?", data);

	std::ostringstream command;
	command << "DELETE FROM peers WHERE parent=" << std::dec << parentAddress << " AND " << " address=" << address;
	GD::db.executeCommand(command.str());
}

void Peer::saveToDatabase(int32_t parentAddress)
{
	deleteFromDatabase(parentAddress);
	std::ostringstream command;
	command << "INSERT INTO peers VALUES(" << parentAddress << "," << address << ",'" <<  serialize() << "')";
	GD::db.executeCommand(command.str());
}

void Peer::deletePairedVirtualDevices()
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
				if(device) GD::devices.remove((*j)->address);
				(*j)->device.reset();
			}
		}
	}
}

std::string Peer::serialize()
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
	stringstream << std::setw(8) << team.address;
	stringstream << std::setw(4) << team.serialNumber.size();
	stringstream << team.serialNumber;
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
		}
	}
	serializeConfig(stringstream, configCentral);
	serializeConfig(stringstream, valuesCentral);
	serializeConfig(stringstream, linksCentral);
	std::string serializedServiceMessages = serviceMessages->serialize();
	stringstream << std::setw(8) << serializedServiceMessages.length();
	stringstream << serializedServiceMessages;
	stringstream << std::setw(8) << pendingBidCoSQueues->size();
	while(!pendingBidCoSQueues->empty())
	{
		std::shared_ptr<BidCoSQueue> queue = pendingBidCoSQueues->front();
		std::string bidCoSQueue = queue->serialize();
		stringstream << std::setw(8) << bidCoSQueue.size();
		stringstream << bidCoSQueue;
		bool hasCallbackFunction = (queue->callbackParameter && queue->callbackParameter->integers.size() == 3 && queue->callbackParameter->strings.size() == 1);
		stringstream << std::setw(1) << (int32_t)hasCallbackFunction;
		if(hasCallbackFunction)
		{
			stringstream << std::setw(4) << queue->callbackParameter->integers.at(0);
			stringstream << std::setw(4) << queue->callbackParameter->strings.at(0).size();
			stringstream << queue->callbackParameter->strings.at(0);
			stringstream << std::setw(8) << queue->callbackParameter->integers.at(1);
			stringstream << std::setw(8) << (queue->callbackParameter->integers.at(2) / 1000);
		}
		pendingBidCoSQueues->pop();
	}
	stringstream << std::setw(8) << _variablesToReset.size();
	for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
	{
		stringstream << std::setw(4) << (*i)->channel;
		stringstream << std::setw(4) << (*i)->key.size();
		stringstream << (*i)->key;
		stringstream << std::setw(8) << (*i)->value;
		stringstream << std::setw(8) << ((*i)->resetTime / 1000);
		stringstream << std::setw(1) << (int32_t)(*i)->isDominoEvent;
	}
	stringstream << std::dec;
	return stringstream.str();
}

void Peer::serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config)
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
				if(GD::debugLevel >= 1) std::cout << "Critical: Parameter has no corresponding RPC parameter. Writing dummy data. Device: " << address << " Channel: " << i->first << std::endl;
				stringstream << std::setw(8) << 0;
				stringstream << std::setw(8) << 0;
				stringstream << std::setw(1) << 0;
				continue;
			}
			if(j->second.rpcParameter->id.size() == 0 && GD::debugLevel >= 2)
			{
				std::cout << "Error: Parameter has no id." << std::endl;
			}
			stringstream << std::setw(8) << j->second.rpcParameter->id.size();
			stringstream << j->second.rpcParameter->id;
			//Four bytes should be enough. Probably even two bytes would be sufficient.
			stringstream << std::setw(8) << j->second.value;
			stringstream << std::setw(1) << (int32_t)j->second.changed;
		}
	}
}

void Peer::serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>& config)
{
	stringstream << std::setw(8) << config.size();
	for(std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>::const_iterator i = config.begin(); i != config.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second.size();
		for(std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			stringstream << std::setw(8) << j->first;
			stringstream << std::setw(8) << j->second.size();
			for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
			{
				if(!k->second.rpcParameter)
				{
					if(GD::debugLevel >= 1) std::cout << "Critical: Parameter has no corresponding RPC parameter. Writing dummy data. Device: " << address << " Channel: " << i->first << std::endl;
					stringstream << std::setw(8) << 0;
					stringstream << std::setw(8) << 0;
					stringstream << std::setw(1) << 0;
					continue;
				}
				if(k->second.rpcParameter->id.size() == 0 && GD::debugLevel >= 2)
				{
					std::cout << "Error: Parameter has no id." << std::endl;
				}
				stringstream << std::setw(8) << k->second.rpcParameter->id.size();
				stringstream << k->second.rpcParameter->id;
				//Four bytes should be enough. Probably even two bytes would be sufficient.
				stringstream << std::setw(8) << k->second.value;
				stringstream << std::setw(1) << (int32_t)k->second.changed;
			}
		}
	}
}

void Peer::unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos)
{
	uint32_t configSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
	for(uint32_t i = 0; i < configSize; i++)
	{
		uint32_t channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		uint32_t parameterCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t k = 0; k < parameterCount; k++)
		{
			uint32_t idLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			if(idLength == 0 && GD::debugLevel >= 1) std::cout << "Critical: Added central config parameter without id. Device: 0x" << std::hex << address << std::dec << " Channel: " << channel;
			std::string id = serializedObject.substr(pos, idLength); pos += idLength;
			RPCConfigurationParameter* parameter = &config[channel][id];
			parameter->value = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			parameter->changed = (bool)std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
			if(rpcDevice == nullptr)
			{
				if(GD::debugLevel >= 1) std::cout << "Critical: No xml rpc device found for peer 0x" << std::hex << address << "." << std::dec;
				continue;
			}
			parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(id);
		}
	}
}

void Peer::unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos)
{
	uint32_t configSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
	for(uint32_t i = 0; i < configSize; i++)
	{
		uint32_t channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		uint32_t peerCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t j = 0; j < peerCount; j++)
		{
			int32_t address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			uint32_t parameterCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			for(uint32_t k = 0; k < parameterCount; k++)
			{
				uint32_t idLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				if(idLength == 0 && GD::debugLevel >= 1) std::cout << "Critical: Added central config parameter without id. Device: 0x" << std::hex << address << std::dec << " Channel: " << channel;
				std::string id = serializedObject.substr(pos, idLength); pos += idLength;
				RPCConfigurationParameter* parameter = &config[channel][address][id];
				parameter->value = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				parameter->changed = (bool)std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
				if(rpcDevice == nullptr)
				{
					if(GD::debugLevel >= 1) std::cout << "Critical: No xml rpc device found for peer 0x" << std::hex << address << "." << std::dec;
					continue;
				}
				parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(id);
				//Clean up - delete if the peer doesn't exist anymore
				if(!getPeer(channel, address)) config[channel].erase(address);
			}
		}
	}
}

void Peer::getValuesFromPacket(std::shared_ptr<BidCoSPacket> packet, std::string& frameID, uint32_t& parameterSetChannel, RPC::ParameterSet::Type::Enum& parameterSetType, std::map<std::string, int64_t>& values)
{
	try
	{
		if(!rpcDevice) return;
		if(packet->messageType() == 0) return; //Don't handle pairing packets, because equal_range returns all elements with "0" as argument
		std::pair<std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator,std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByMessageType.equal_range((uint32_t)packet->messageType());
		if(range.first == rpcDevice->framesByMessageType.end()) return;
		std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator i = range.first;
		parameterSetChannel = -1;
		do
		{
			std::shared_ptr<RPC::DeviceFrame> frame(i->second);
			if(!frame) continue;
			if(frame->direction == RPC::DeviceFrame::Direction::Enum::fromDevice && packet->senderAddress() != address) continue;
			if(frame->direction == RPC::DeviceFrame::Direction::Enum::toDevice && packet->destinationAddress() != address) continue;
			if(packet->payload()->empty()) continue;
			if(frame->subtype > -1 && frame->subtypeIndex >= 9 && (signed)packet->payload()->size() > (frame->subtypeIndex - 9) && packet->payload()->at(frame->subtypeIndex - 9) != (unsigned)frame->subtype) continue;
			int32_t channelIndex = frame->channelField;
			int32_t channel = -1;
			if(channelIndex >= 9 && (signed)packet->payload()->size() > (channelIndex - 9)) channel = packet->payload()->at(channelIndex - 9);
			frameID = frame->id;

			for(std::vector<RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				int64_t value = 0;
				if(j->size > 0 && j->index > 0)
				{
					if(((int32_t)j->index) - 9 >= (signed)packet->payload()->size()) continue;
					value = packet->getPosition(j->index, j->size, j->isSigned);

					if(j->constValue > -1)
					{
						if(value != j->constValue) break; else continue;
					}
				}
				else if(j->constValue > -1)
				{
					value = j->constValue;
				}
				else continue;
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
				{
					parameterSetChannel = (channel < 0) ? 0 : channel;
					parameterSetType = (*k)->parentParameterSet->type;
					if(rpcDevice->channels.find(parameterSetChannel) == rpcDevice->channels.end()) continue;
					if(rpcDevice->channels.at(parameterSetChannel)->parameterSets.at(parameterSetType).get() != (*k)->parentParameterSet) continue;
					if((*k)->physicalParameter->valueID != j->param) continue;
					values[(*k)->id] = value;
				}
			}
		} while(++i != range.second && i != rpcDevice->framesByMessageType.end());
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
						if(GD::debugLevel >= 5) std::cerr << "Debug: Deleting element " << parameter->id << " from _variablesToReset. Device: 0x" << std::hex << address << std::dec << " Serial number: " << _serialNumber << " Frame: " << frameID << std::endl;
						_variablesToReset.erase(k);
						break; //The key should only be once in the vector, so breaking is ok and we can't continue as the iterator is invalidated.
					}
				}
				_variablesToResetMutex.unlock();
			}
			std::shared_ptr<RPC::Parameter> delayParameter = rpcDevice->channels.at(channel)->parameterSets.at(RPC::ParameterSet::Type::Enum::values)->getParameter((*j)->dominoEventDelayID);
			if(!delayParameter) continue;
			int64_t delay = delayParameter->convertFromPacket(valuesCentral[channel][(*j)->dominoEventDelayID].value)->integerValue;
			if(delay < 0) continue; //0 is allowed. When 0 the parameter will be reset immediately
			int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			std::shared_ptr<VariableToReset> variable(new VariableToReset);
			variable->channel = channel;
			variable->value = (*j)->dominoEventValue;
			variable->resetTime = time + (delay * 1000);
			variable->key = parameter->id;
			variable->isDominoEvent = true;
			_variablesToResetMutex.lock();
			_variablesToReset.push_back(variable);
			_variablesToResetMutex.unlock();
			if(_stopWorkerThread)
			{
				_stopWorkerThread = false;
				_workerThread = std::thread(&Peer::worker, this);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void Peer::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		uint32_t parameterSetChannel = 0;
		std::map<std::string, int64_t> values;
		RPC::ParameterSet::Type::Enum parameterSetType;
		std::string frameID;
		getValuesFromPacket(packet, frameID, parameterSetChannel, parameterSetType, values);
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());

		std::map<std::string, int64_t> sentValues;
		std::shared_ptr<BidCoSPacket> sentPacket;
		std::string sentFrameID;
		if(packet->messageType() == 0x02) //ACK packet: Check if all values were set correctly. If not set value again
		{
			sentPacket = GD::devices.getCentral()->getSentPacket(address);
			if(sentPacket && sentPacket->messageType() > 0 && !sentPacket->payload()->empty())
			{
				uint32_t sentParameterSetChannel = 0;
				RPC::ParameterSet::Type::Enum sentParameterSetType;
				getValuesFromPacket(sentPacket, sentFrameID, sentParameterSetChannel, sentParameterSetType, sentValues);
			}
		}

		bool resendPacket = false;
		for(std::map<std::string, int64_t>::iterator i = values.begin(); i != values.end(); ++i)
		{
			valuesCentral[parameterSetChannel][i->first].value = i->second;
			if(sentValues.find(i->first) != sentValues.end())
			{
				if(sentValues.at(i->first) != i->second) resendPacket = true;
			}
			else
			{
				if(GD::debugLevel >= 4) std::cout << "Info: " << i->first << " of device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << " was set to " << i->second << "." << std::endl;
				valueKeys->push_back(i->first);
				rpcValues->push_back(rpcDevice->channels.at(parameterSetChannel)->parameterSets.at(parameterSetType)->getParameter(i->first)->convertFromPacket(i->second));
			}
		}

		//We have to do this in a seperate loop, because all parameters need to be set first
		for(std::map<std::string, int64_t>::iterator i = values.begin(); i != values.end(); ++i)
		{
			handleDominoEvent(rpcDevice->channels.at(parameterSetChannel)->parameterSets.at(parameterSetType)->getParameter(i->first), frameID, parameterSetChannel);
		}

		if(resendPacket && sentPacket)
		{
			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			queue->noSending = true;
			serviceMessages->configPending = true;
			queue->serviceMessages = serviceMessages;
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
			*packet = *sentPacket;
			packet->setMessageCounter(messageCounter);
			messageCounter++;
			if(GD::debugLevel >= 4) std::cout << "Info: Resending values for device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << ", because they were not set correctly." << std::endl;
			queue->push(sentPacket);
			queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			if(!(rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp)) GD::devices.getCentral()->enqueuePackets(address, queue, true);
			else
			{
				if(!pendingBidCoSQueues) pendingBidCoSQueues.reset(new std::queue<std::shared_ptr<BidCoSQueue>>());
				pendingBidCoSQueues->push(queue);
				if(GD::debugLevel >= 5) std::cout << "Debug: Packet was queued and will be sent with next wake me up packet." << std::endl;
			}
		}

		if(!values.empty() && (packet->controlByte() & 32) && packet->destinationAddress() == GD::devices.getCentral()->address())
		{
			GD::devices.getCentral()->sendOK(packet->messageCounter(), packet->senderAddress());
		}
		if((rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp) && packet->senderAddress() == address && !(packet->controlByte() & 32) && (packet->controlByte() & 2) && pendingBidCoSQueues && !pendingBidCoSQueues->empty()) //Packet is wake me up packet and not bidirectional
		{
			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::DEFAULT));
			queue->noSending = true;

			std::vector<uint8_t> payload;
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(packet->messageCounter(), 0xA1, 0x12, GD::devices.getCentral()->address(), address, payload));
			queue->push(configPacket);
			queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));

			GD::devices.getCentral()->enqueuePackets(address, queue, true);
		}
		if(!rpcValues->empty())
		{
			GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string(parameterSetChannel), valueKeys, rpcValues);
		}
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

std::shared_ptr<RPC::RPCVariable> Peer::getDeviceDescription(int32_t channel)
{
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
		if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && configCentral.at(channel).at("AES_ACTIVE").value != 0)
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

std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> Peer::getDeviceDescription()
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

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->channels[channel]->parameterSets[type]->id));
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::putParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> variables, bool onlyPushing)
{
	try
	{
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty() || variables->structValue->front()->name.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

		if(type == RPC::ParameterSet::Type::Enum::master)
		{
			std::map<int32_t, std::map<int32_t, uint8_t>> changedParameters;
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if((*i)->name.empty()) continue;
				int64_t value = 0;
				if(configCentral[channel].find((*i)->name) == configCentral[channel].end()) continue;
				RPCConfigurationParameter* parameter = &configCentral[channel][(*i)->name];
				value = parameter->rpcParameter->convertToPacket(*i);
				parameter->value = value;
				if(GD::debugLevel >= 4) std::cout << "Info: Parameter " << (*i)->name << " set to 0x" << std::hex << value << std::dec << "." << std::endl;
				int32_t intIndex = (int32_t)parameter->rpcParameter->physicalParameter->index;
				int32_t list = parameter->rpcParameter->physicalParameter->list;
				if(list == 9999) list = 0;
				value = parameter->rpcParameter->getBytes(value);
				if(changedParameters[list].find(intIndex) == changedParameters[list].end()) changedParameters[list][intIndex] = value;
				else changedParameters[list][intIndex] |= value;
			}

			if(changedParameters.empty() || changedParameters.begin()->second.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			queue->noSending = true;
			serviceMessages->configPending = true;
			queue->serviceMessages = serviceMessages;
			std::vector<uint8_t> payload;
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();

			for(std::map<int32_t, std::map<int32_t, uint8_t>>::iterator i = changedParameters.begin(); i != changedParameters.end(); ++i)
			{
				//CONFIG_START
				payload.push_back(channel);
				payload.push_back(0x05);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(0);
				payload.push_back(i->first);
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				messageCounter++;

				//CONFIG_WRITE_INDEX
				payload.push_back(channel);
				payload.push_back(0x08);
				for(std::map<int32_t, uint8_t>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					payload.push_back(j->first);
					payload.push_back(j->second);
					if(payload.size() == 16)
					{
						configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
						queue->push(configPacket);
						queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						messageCounter++;
						payload.push_back(i->first);
						payload.push_back(0x08);
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

			if(!pendingBidCoSQueues) pendingBidCoSQueues.reset(new std::queue<std::shared_ptr<BidCoSQueue>>());
			pendingBidCoSQueues->push(queue);
			if(!onlyPushing && !(rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp)) GD::devices.getCentral()->enqueuePackets(address, queue, true);
			else if(GD::debugLevel >= 5) std::cout << "Debug: Packet was queued and will be sent with next wake me up packet." << std::endl;
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
			if(!remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber);
			if(!remotePeer) return RPC::RPCVariable::createError(-3, "Not paired to this peer.");
			if(remotePeer->channel != remoteChannel) RPC::RPCVariable::createError(-3, "Unknown remote channel.");
			if(linksCentral[channel].find(remotePeer->address) == linksCentral[channel].end()) RPC::RPCVariable::createError(-3, "Unknown parameter set.");

			std::map<int32_t, std::map<int32_t, uint8_t>> changedParameters;
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if((*i)->name.empty()) continue;
				int64_t value = 0;
				if(linksCentral[channel][remotePeer->address].find((*i)->name) == linksCentral[channel][remotePeer->address].end()) continue;
				RPCConfigurationParameter* parameter = &linksCentral[channel][remotePeer->address][(*i)->name];
				value = parameter->rpcParameter->convertToPacket(*i);
				parameter->value = value;
				if(GD::debugLevel >= 4) std::cout << "Info: Parameter " << (*i)->name << " set to 0x" << std::hex << value << std::dec << "." << std::endl;
				int32_t intIndex = (int32_t)parameter->rpcParameter->physicalParameter->index;
				int32_t list = parameter->rpcParameter->physicalParameter->list;
				if(list == 9999) list = 0;
				value = parameter->rpcParameter->getBytes(value);
				if(changedParameters[list].find(intIndex) == changedParameters[list].end()) changedParameters[list][intIndex] = value;
				else changedParameters[list][intIndex] |= value;
			}

			if(changedParameters.empty() || changedParameters.begin()->second.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

			std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			queue->noSending = true;
			serviceMessages->configPending = true;
			queue->serviceMessages = serviceMessages;
			std::vector<uint8_t> payload;
			std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();

			for(std::map<int32_t, std::map<int32_t, uint8_t>>::iterator i = changedParameters.begin(); i != changedParameters.end(); ++i)
			{
				//CONFIG_START
				payload.push_back(channel);
				payload.push_back(0x05);
				payload.push_back(remotePeer->address >> 16);
				payload.push_back((remotePeer->address >> 8) & 0xFF);
				payload.push_back(remotePeer->address & 0xFF);
				payload.push_back(remotePeer->channel);
				payload.push_back(i->first);
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
				queue->push(configPacket);
				queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				messageCounter++;

				//CONFIG_WRITE_INDEX
				payload.push_back(channel);
				payload.push_back(0x08);
				for(std::map<int32_t, uint8_t>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					payload.push_back(j->first);
					payload.push_back(j->second);
					if(payload.size() == 16)
					{
						configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(messageCounter, 0xA0, 0x01, central->address(), address, payload));
						queue->push(configPacket);
						queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						messageCounter++;
						payload.push_back(i->first);
						payload.push_back(0x08);
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

			if(!pendingBidCoSQueues) pendingBidCoSQueues.reset(new std::queue<std::shared_ptr<BidCoSQueue>>());
			pendingBidCoSQueues->push(queue);
			if(!(rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp)) GD::devices.getCentral()->enqueuePackets(address, queue, true);
			else if(GD::debugLevel >= 5) std::cout << "Debug: Packet was queued and will be sent with next wake me up packet." << std::endl;
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> variables(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber);
		if(remotePeer && remotePeer->channel != remoteChannel)  return RPC::RPCVariable::createError(-2, "Unknown remote channel");

		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service)) continue;
			std::shared_ptr<RPC::RPCVariable> element;
			if(type == RPC::ParameterSet::Type::Enum::values)
			{
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = valuesCentral[channel][(*i)->id].rpcParameter->convertFromPacket(valuesCentral[channel][(*i)->id].value);
			}
			else if(type == RPC::ParameterSet::Type::Enum::master)
			{
				if(configCentral.find(channel) == configCentral.end()) continue;
				if(configCentral[channel].find((*i)->id) == configCentral[channel].end()) continue;
				element = configCentral[channel][(*i)->id].rpcParameter->convertFromPacket(configCentral[channel][(*i)->id].value);
			}
			else if(remotePeer)
			{
				if(linksCentral.find(channel) == linksCentral.end()) continue;
				if(linksCentral[channel][remotePeer->address].find((*i)->id) == linksCentral[channel][remotePeer->address].end()) continue;
				if(remotePeer->channel != remoteChannel) continue;
				element = linksCentral[channel][remotePeer->address][(*i)->id].rpcParameter->convertFromPacket(linksCentral[channel][remotePeer->address][(*i)->id].value);
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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel)
{
	try
	{
		if(_peers.find(senderChannel) == _peers.end()) return RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverSerialNumber);
		if(!remotePeer) return RPC::RPCVariable::createError(-2, "No peer found for sender channel and receiver serial number.");
		if(remotePeer->channel != receiverChannel)  RPC::RPCVariable::createError(-2, "No peer found for receiver channel.");
		std::shared_ptr<RPC::RPCVariable> response(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		response->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("DESCRIPTION", remotePeer->linkDescription)));
		response->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("NAME", remotePeer->linkName)));
		return response;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
	catch(...)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
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
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
	catch(...)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
	}
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLinkPeers(int32_t channel)
{
	try
	{
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
				std::shared_ptr<Peer> peer;
				if(central->getPeers()->find((*i)->address) != central->getPeers()->end()) peer = central->getPeers()->at((*i)->address);
				bool peerKnowsMe = false;
				if(peer && peer->getPeer((*i)->channel, address)) peerKnowsMe = true;

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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLink(int32_t channel, int32_t flags, bool avoidDuplicates)
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> element;
		if(channel > -1)
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
				std::shared_ptr<Peer> peer;
				if(central->getPeers()->find((*i)->address) != central->getPeers()->end()) peer = central->getPeers()->at((*i)->address);
				bool peerKnowsMe = false;
				if(peer && peer->getPeer((*i)->channel, address)) peerKnowsMe = true;

				//Don't continue if peer is sender and exists in central's peer array to avoid generation of duplicate results when requesting all links (only generate results when we are sender)
				if(!isSender && peerKnowsMe && avoidDuplicates) return array;
				//If we are receiver this point is only reached, when the sender is not paired to this central

				std::string peerSerial = (*i)->serialNumber;
				int32_t brokenFlags = 0;
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
						if(isSender) brokenFlags = 2 | 4; //LINK_FLAG_RECEIVER_BROKEN | PEER_IS_ME
						else brokenFlags = 1 | 4; //LINK_FLAG_SENDER_BROKEN | PEER_IS_ME
					}*/
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
				if(brokenFlags == 0 && central->getPeers()->find((*i)->address) != central->getPeers()->end() && central->getPeers()->at((*i)->address)->serviceMessages->unreach) brokenFlags = 2;
				if(serviceMessages->unreach) brokenFlags |= 1;
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
						if(!(brokenFlags & 2) && peer) paramset = peer->getParamset((*i)->channel, RPC::ParameterSet::Type::Enum::link, _serialNumber, channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						paramset->name = "RECEIVER_PARAMSET";
						element->structValue->push_back(paramset);
					}
					if(flags & 16)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 2) && peer) description = peer->getDeviceDescription((*i)->channel);
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
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				element = getLink(i->first, flags, avoidDuplicates);
				array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetDescription(uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(channel < -1) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "unknown channel");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "unknown parameter set");

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber);
		if(remotePeer && remotePeer->channel != remoteChannel)  return RPC::RPCVariable::createError(-2, "Unknown remote channel");

		std::shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> descriptions(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> element;
		uint32_t index = 0;
		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service)) continue;
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
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getServiceMessages()
{
	if(!serviceMessages) return RPC::RPCVariable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get();
}

std::shared_ptr<RPC::RPCVariable> Peer::getValue(uint32_t channel, std::string valueKey)
{
	if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
	if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
	return valuesCentral[channel][valueKey].rpcParameter->convertFromPacket(valuesCentral[channel][valueKey].value);
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
				if(_peers[1].at(0)->device->deviceType() != HMDeviceTypes::HMCCTC) _peers[1].at(0)->device.reset();
			}
			if(_peers[1].at(0)->device)
			{
				HM_CC_TC* tc = (HM_CC_TC*)_peers[1].at(0)->device.get();
				tc->setValveState(value->integerValue);
				if(GD::debugLevel >= 4) std::cout << "Setting valve state of HM-CC-VD with address 0x" << std::hex << address << std::dec << " to " << value->integerValue << "%." << std::endl;
				return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
	return false;
}

void Peer::addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters)
{
	try
	{
		if(parameters->integers.size() != 3) return;
		if(parameters->strings.size() != 1) return;
		if(GD::debugLevel >= 5) std::cerr << "Debug: addVariableToResetCallback invoked for parameter " << parameters->strings.at(0) << " of device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << "." << std::endl;
		std::shared_ptr<VariableToReset> variable(new VariableToReset);
		variable->channel = parameters->integers.at(0);
		variable->value = parameters->integers.at(1);
		variable->resetTime = parameters->integers.at(2);
		variable->key = parameters->strings.at(0);
		_variablesToResetMutex.lock();
		_variablesToReset.push_back(variable);
		_variablesToResetMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

std::shared_ptr<RPC::RPCVariable> Peer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(valueKey.empty()) return RPC::RPCVariable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		std::shared_ptr<RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(rpcParameter->physicalParameter->interface == RPC::PhysicalParameter::Interface::Enum::store)
		{
			valuesCentral[channel][valueKey].value = rpcParameter->convertToPacket(value);
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		}
		else if(rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::command) return RPC::RPCVariable::createError(-6, "Parameter is not settable.");
		if(!rpcParameter->conversion.empty() && rpcParameter->conversion.at(0)->type == RPC::ParameterConversion::Type::Enum::toggle)
		{
			std::string toggleKey = rpcParameter->conversion.at(0)->stringValue;
			if(toggleKey.empty()) return RPC::RPCVariable::createError(-6, "No toggle parameter specified (parameter attribute value is empty).");
			if(valuesCentral[channel].find(toggleKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Toggle parameter not found.");
			if(valuesCentral[channel][toggleKey].rpcParameter->logicalParameter->type != RPC::LogicalParameter::Type::Enum::typeBoolean) return RPC::RPCVariable::createError(-6, "Toggle parameter has to be of type boolean.");
			std::shared_ptr<RPC::RPCVariable> toggleValue = valuesCentral[channel][toggleKey].rpcParameter->convertFromPacket(valuesCentral[channel][toggleKey].value);
			toggleValue->booleanValue = !toggleValue->booleanValue;
			return setValue(channel, toggleKey, toggleValue);
		}
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return RPC::RPCVariable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return RPC::RPCVariable::createError(-6, "frame not found");
		std::shared_ptr<RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		valuesCentral[channel][valueKey].value = rpcParameter->convertToPacket(value);
		if(GD::debugLevel >= 5) std::cout << "Debug: " << valueKey << " of device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << " was set to " << valuesCentral[channel][valueKey].value << "." << std::endl;

		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		queue->noSending = true;
		serviceMessages->configPending = true;
		queue->serviceMessages = serviceMessages;

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
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0xA0, (uint8_t)frame->type, GD::devices.getCentral()->address(), address, payload));
		for(std::vector<RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				packet->setPosition(i->index, i->size, i->constValue);
				continue;
			}
			//We can't just search for param, because it is ambiguous (see for example LEVEL for HM-CC-TC.
			if(i->param.empty() && !i->additionalParameter.empty() && valuesCentral[channel].find(i->additionalParameter) != valuesCentral[channel].end())
			{
				int64_t intValue = valuesCentral[channel][i->additionalParameter].value;
				if(!i->omitIfSet || intValue != i->omitIf)
				{
					//Don't set ON_TIME when value is false
					if(i->additionalParameter != "ON_TIME" || value->booleanValue) packet->setPosition(i->index, i->size, intValue);
				}
			}
			else if(i->param == rpcParameter->physicalParameter->valueID) packet->setPosition(i->index, i->size, valuesCentral[channel][valueKey].value);
			else if(GD::debugLevel >= 2) std::cerr << "Error: Can't generate parameter packet, because the frame parameter does not equal the set parameter. Device: 0x" << std::hex << address << std::dec << " Serial number: " << _serialNumber << " Frame: " << frame->id << std::endl;
			if(i->additionalParameter == "ON_TIME")
			{
				if(rpcParameter->physicalParameter->valueID != "STATE" || rpcParameter->logicalParameter->type != RPC::LogicalParameter::Type::Enum::typeBoolean)
				{
					if(GD::debugLevel >= 2) std::cerr << "Error: Can't set \"ON_TIME\" for " << rpcParameter->physicalParameter->valueID << ". Currently \"ON_TIME\" is only supported for \"STATE\" of type \"boolean\". Device: 0x" << std::hex << address << std::dec << " Serial number: " << _serialNumber << " Frame: " << frame->id << std::endl;
					continue;
				}
				if(!_variablesToReset.empty())
				{
					_variablesToResetMutex.lock();
					for(std::vector<std::shared_ptr<VariableToReset>>::iterator i = _variablesToReset.begin(); i != _variablesToReset.end(); ++i)
					{
						if((*i)->channel == channel && (*i)->key == rpcParameter->physicalParameter->valueID)
						{
							if(GD::debugLevel >= 5) std::cerr << "Debug: Deleting element from _variablesToReset. Device: 0x" << std::hex << address << std::dec << " Serial number: " << _serialNumber << " Frame: " << frame->id << std::endl;
							_variablesToReset.erase(i);
							break; //The key should only be once in the vector, so breaking is ok and we can't continue as the iterator is invalidated.
						}
					}
					_variablesToResetMutex.unlock();
				}
				if(!value->booleanValue || valuesCentral[channel][i->additionalParameter].value == 0) continue;
				std::shared_ptr<CallbackFunctionParameter> parameters(new CallbackFunctionParameter());
				parameters->integers.push_back(channel);
				parameters->integers.push_back(0); //false = off
				int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				parameters->integers.push_back(time + std::roundl(i->convertFromPacket(valuesCentral[channel][i->additionalParameter].value)->floatValue * 1000));
				parameters->strings.push_back(rpcParameter->physicalParameter->valueID);
				queue->callbackParameter = parameters;
				queue->queueEmptyCallback = delegate<void (std::shared_ptr<CallbackFunctionParameter>)>::from_method<Peer, &Peer::addVariableToResetCallback>(this);
				if(_stopWorkerThread)
				{
					_stopWorkerThread = false;
					_workerThread = std::thread(&Peer::worker, this);
				}
			}
		}
		if(!rpcParameter->physicalParameter->resetAfterSend.empty())
		{
			for(std::vector<std::string>::iterator j = rpcParameter->physicalParameter->resetAfterSend.begin(); j != rpcParameter->physicalParameter->resetAfterSend.end(); ++j)
			{
				if(valuesCentral.at(channel).find(*j) == valuesCentral.at(channel).end()) continue;
				std::shared_ptr<RPC::RPCVariable> logicalDefaultValue = valuesCentral.at(channel).at(*j).rpcParameter->logicalParameter->getDefaultValue();
				int32_t defaultValue = valuesCentral.at(channel).at(*j).rpcParameter->convertToPacket(logicalDefaultValue);
				if(defaultValue != valuesCentral.at(channel).at(*j).value)
				{
					valuesCentral.at(channel).at(*j).value = defaultValue;
					if(GD::debugLevel >= 4) std::cout << "INFO: Parameter \"" << *j << "\" was reset to " << valuesCentral.at(channel).at(*j).value << ". Device: 0x" << std::hex << address << std::dec << " Serial number: " << _serialNumber << " Frame: " << frame->id << std::endl;
					std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {*j});
					std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values(new std::vector<std::shared_ptr<RPC::RPCVariable>> { logicalDefaultValue });
					GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string(channel), valueKeys, values);
				}
			}
		}
		messageCounter++;
		queue->push(packet);
		queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		if(!(rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp))
		{
			if(valueKey == "STATE") queue->burst = true;
			GD::devices.getCentral()->enqueuePackets(address, queue, true);
		}
		else
		{
			if(!pendingBidCoSQueues) pendingBidCoSQueues.reset(new std::queue<std::shared_ptr<BidCoSQueue>>());
			pendingBidCoSQueues->push(queue);
			if(GD::debugLevel >= 5) std::cout << "Debug: Packet was queued and will be sent with next wake me up packet." << std::endl;
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _variablesToResetMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _variablesToResetMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _variablesToResetMutex.unlock();
    }
    return RPC::RPCVariable::createError(-1, "unknown error");
}
