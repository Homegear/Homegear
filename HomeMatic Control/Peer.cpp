#include "Peer.h"
#include "GD.h"

void Peer::initializeCentralConfig()
{
	if(rpcDevice == nullptr)
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
					valuesCentral[i->first][(*j)->id] = parameter;
				}
			}
		}
		if(i->second->parameterSets.find(RPC::ParameterSet::Type::link) != i->second->parameterSets.end() && i->second->parameterSets[RPC::ParameterSet::Type::link])
		{
			std::shared_ptr<RPC::ParameterSet> linkSet = i->second->parameterSets[RPC::ParameterSet::Type::link];
			for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = linkSet->parameters.begin(); j != linkSet->parameters.end(); ++j)
			{
				if(!(*j)->id.empty())
				{
					parameter = RPCConfigurationParameter();
					parameter.rpcParameter = *j;
					linksCentral[i->first][(*j)->id] = parameter;
				}
			}
		}
	}
}

Peer::Peer(std::string serializedObject, HomeMaticDevice* device)
{
	pendingBidCoSQueues = std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>>(new std::queue<std::shared_ptr<BidCoSQueue>>());
	if(serializedObject.empty()) return;
	if(GD::debugLevel >= 5) std::cout << "Unserializing peer: " << serializedObject << std::endl;

	std::istringstream stringstream(serializedObject);
	std::string entry;
	uint32_t pos = 0;
	address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	int32_t serialNumberSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
	_serialNumber = serializedObject.substr(pos, serialNumberSize); pos += serialNumberSize;
	firmwareVersion = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	remoteChannel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	localChannel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	deviceType = (HMDeviceTypes)std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	//This loads the corresponding xmlrpcDevice unnecessarily for virtual device peers, too. But so what?
	rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion);
	if(!rpcDevice && GD::debugLevel >= 2) std::cout << "Error: Device type not found: 0x" << std::hex << (uint32_t)deviceType << " Firmware version: " << firmwareVersion << std::endl;
	messageCounter = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	homegearFeatures = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	team.address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	serialNumberSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
	if(serialNumberSize > 0) { team.serialNumber = serializedObject.substr(pos, serialNumberSize); pos += serialNumberSize; }
	uint32_t configSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
	for(uint32_t i = 0; i < configSize; i++)
	{
		config[std::stoll(serializedObject.substr(pos, 8), 0, 16)] = std::stoll(serializedObject.substr(pos + 8, 8), 0, 16); pos += 16;
	}
	unserializeConfig(serializedObject, configCentral, RPC::ParameterSet::Type::master, pos);
	unserializeConfig(serializedObject, valuesCentral, RPC::ParameterSet::Type::values, pos);
	unserializeConfig(serializedObject, linksCentral, RPC::ParameterSet::Type::link, pos);
	uint32_t peersSize = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
	for(uint32_t i = 0; i < peersSize; i++)
	{
		uint32_t channel = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
		uint32_t peerCount = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
		for(uint32_t j = 0; j < peerCount; j++)
		{
			BasicPeer basicPeer;
			basicPeer.address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			serialNumberSize = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
			basicPeer.serialNumber = serializedObject.substr(pos, serialNumberSize); pos += serialNumberSize;
			basicPeer.hidden = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
			peers[channel].push_back(basicPeer);
		}
	}
	uint32_t pendingQueuesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	for(uint32_t i = 0; i < pendingQueuesSize; i++)
	{
		uint32_t queueLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		if(queueLength > 6)
		{
			pendingBidCoSQueues->push(std::shared_ptr<BidCoSQueue>(new BidCoSQueue(serializedObject.substr(pos, queueLength), device))); pos += queueLength;
			pendingBidCoSQueues->back()->noSending = true;
		}
	}
	uint32_t serializedServiceMessagesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	serviceMessages.reset(new ServiceMessages(_serialNumber, serializedObject.substr(pos, serializedServiceMessagesSize)));
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
	for(std::unordered_map<int32_t, std::vector<BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
	{
		for(std::vector<BasicPeer>::iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			if(j->hidden)
			{
				device = GD::devices.get(j->address);
				if(device) GD::devices.remove(j->address);
				j->device.reset();
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
	stringstream << std::setw(2) << (int32_t)messageCounter;
	stringstream << std::setw(1) << (int32_t)homegearFeatures;
	stringstream << std::setw(8) << team.address;
	stringstream << std::setw(4) << team.serialNumber.size();
	stringstream << team.serialNumber;
	stringstream << std::setw(8) << config.size();
	for(std::unordered_map<int32_t, int32_t>::const_iterator i = config.begin(); i != config.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second;
	}
	serializeConfig(stringstream, configCentral);
	serializeConfig(stringstream, valuesCentral);
	serializeConfig(stringstream, linksCentral);
	stringstream << std::setw(8) << peers.size();
	for(std::unordered_map<int32_t, std::vector<BasicPeer>>::const_iterator i = peers.begin(); i != peers.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second.size();
		for(std::vector<BasicPeer>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			stringstream << std::setw(8) << j->address;
			stringstream << std::setw(4) << j->serialNumber.size();
			stringstream << j->serialNumber;
			stringstream << std::setw(1) << j->hidden;
		}
	}
	stringstream << std::setw(8) << pendingBidCoSQueues->size();
	while(!pendingBidCoSQueues->empty())
	{
		std::string bidCoSQueue = pendingBidCoSQueues->front()->serialize();
		stringstream << std::setw(8) << bidCoSQueue.size();
		stringstream << bidCoSQueue;
		pendingBidCoSQueues->pop();
	}
	std::string serializedServiceMessages = serviceMessages->serialize();
	stringstream << std::setw(8) << serializedServiceMessages.length();
	stringstream << serializedServiceMessages;
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

void Peer::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{

	try
	{
		if(!rpcDevice) return;
		if(packet->messageType() == 0) return; //Don't handle pairing packets, because equal_range returns all elements with "0" as argument
		std::pair<std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator,std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByMessageType.equal_range((uint32_t)packet->messageType());
		if(range.first == rpcDevice->framesByMessageType.end()) return;
		std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator i = range.first;
		bool sendOK = false;
		std::shared_ptr<std::vector<std::string>> valueKeys;
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values;
		int32_t parameterSetChannel = -1;
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
			for(std::vector<RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				if((int32_t)j->index >= (signed)packet->payload()->size()) continue;
				int64_t value = packet->getPosition(j->index, j->size, j->isSigned);
				if(j->constValue > -1)
				{
					if(value != j->constValue) break; else continue;
				}
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
				{
					if(channel > -1 && (*k)->parentParameterSet->channel != channel) continue;
					if((*k)->physicalParameter->valueID != j->param) continue;
					//channel often is -1
					if(parameterSetChannel < 0) parameterSetChannel = (*k)->parentParameterSet->channel;
					else if(parameterSetChannel != (*k)->parentParameterSet->channel)
					{
						if(GD::debugLevel >= 2) std::cout << "Error: Parameters from device device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << " have multiple channels. That is not allowed." << std::endl;
						continue;
					}
					valuesCentral[parameterSetChannel][(*k)->id].value = value;
					if(GD::debugLevel >= 4) std::cout << "Info: " << (*k)->id << " of device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << " was set to " << value << "." << std::endl;
					if(!valueKeys)
					{
						valueKeys.reset(new std::vector<std::string>());
						values.reset(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
					}
					valueKeys->push_back((*k)->id);
					values->push_back((*k)->convertFromPacket(value));
					sendOK = true;
				}
			}
		} while(++i != range.second && i != rpcDevice->framesByMessageType.end());
		if(sendOK && (packet->controlByte() & 32) && packet->destinationAddress() == GD::devices.getCentral()->address())
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
		if(sendOK && parameterSetChannel > -1)
		{
			if(parameterSetChannel > -1) GD::rpcClient.broadcastEvent(_serialNumber + ":" + std::to_string(parameterSetChannel), valueKeys, values);
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
		for(std::vector<std::shared_ptr<RPC::LinkRole>>::iterator j = rpcChannel->linkRoles.begin(); j != rpcChannel->linkRoles.end(); ++j)
		{
			for(std::vector<std::string>::iterator k = (*j)->sourceNames.begin(); k != (*j)->sourceNames.end(); ++k)
			{
				//Probably only one direction is supported, but just in case I use the "or"
				if(!k->empty())
				{
					if(direction & 1) linkSourceRoles << " ";
					linkSourceRoles << *k;
					direction |= 1;
				}
			}
			for(std::vector<std::string>::iterator k = (*j)->targetNames.begin(); k != (*j)->targetNames.end(); ++k)
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

std::shared_ptr<RPC::RPCVariable> Peer::getParamset(uint32_t channel, RPC::ParameterSet::Type::Enum type)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> variables(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

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
			else if(type == RPC::ParameterSet::Type::Enum::link)
			{
				if(linksCentral.find(channel) == linksCentral.end()) continue;
				if(linksCentral[channel].find((*i)->id) == linksCentral[channel].end()) continue;
				element = linksCentral[channel][(*i)->id].rpcParameter->convertFromPacket(linksCentral[channel][(*i)->id].value);
			}

			if(element->errorStruct) continue;
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

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetDescription(uint32_t channel, RPC::ParameterSet::Type::Enum type)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "unknown channel");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "unknown parameter set");
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

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "DEFAULT";
				element->booleanValue = parameter->defaultValue;
				description->structValue->push_back(element);

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

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "DEFAULT";
				element->stringValue = parameter->defaultValue;
				description->structValue->push_back(element);

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

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->name = "DEFAULT";
				element->booleanValue = parameter->defaultValue;
				description->structValue->push_back(element);

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

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "DEFAULT";
				element->integerValue = parameter->defaultValue;
				description->structValue->push_back(element);

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

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->name = "DEFAULT";
				element->integerValue = parameter->defaultValue;
				description->structValue->push_back(element);

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

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
				element->name = "DEFAULT";
				element->floatValue = parameter->defaultValue;
				description->structValue->push_back(element);

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
		if(deviceType == HMDeviceTypes::HMCCVD && valueKey == "VALVE_STATE" && peers.find(1) != peers.end() && peers[1].size() > 0 && peers[1].at(0).hidden)
		{
			if(!peers[1].at(0).device)
			{
				peers[1].at(0).device = GD::devices.get(peers[1].at(0).address);
				if(peers[1].at(0).device->deviceType() != HMDeviceTypes::HMCCTC) peers[1].at(0).device.reset();
			}
			if(peers[1].at(0).device)
			{
				HM_CC_TC* tc = (HM_CC_TC*)peers[1].at(0).device.get();
				tc->setValveState(value->integerValue);
				if(GD::debugLevel >= 4) std::cout << "Setting valve state of HM-CC-VD with address 0x" << address << " to " << (tc->getNewValueState() * 100 / 256) << "%." << std::endl;
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

std::shared_ptr<RPC::RPCVariable> Peer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(channel == 0 && serviceMessages->set(valueKey, value)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		std::shared_ptr<RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::command) return RPC::RPCVariable::createError(-6, "Parameter is not settable.");
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return RPC::RPCVariable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return RPC::RPCVariable::createError(-6, "frame not found");
		std::shared_ptr<RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		valuesCentral[channel][valueKey].value = rpcParameter->convertToPacket(value);
		if(GD::debugLevel >= 5) std::cout << "Debug: " << valueKey << " of device 0x" << std::hex << address << std::dec << " with serial number " << _serialNumber << " was set to " << valuesCentral[channel][valueKey].value << "." << std::endl;

		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::DEFAULT));
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
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0xA0, (uint8_t)frame->type, GD::devices.getCentral()->address(), address, payload));
		for(std::vector<RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				packet->setPosition(i->index, i->size, i->constValue);
				continue;
			}
			//We can't just search for param, because it is ambiguous (see for example LEVEL for HM-CC-TC.
			if(i->param == rpcParameter->physicalParameter->valueID) packet->setPosition(i->index, i->size, valuesCentral[channel][valueKey].value);
			else if(GD::debugLevel >= 2) std::cout << "Error: Can't generate parameter packet, because the frame parameter does not equal the set parameter. Device: 0x" << std::hex << address << std::dec << " Serial number: " << _serialNumber << " Frame: " << frame->id << std::endl;
		}
		messageCounter++;
		queue->push(packet);
		queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));

		if(!pendingBidCoSQueues) pendingBidCoSQueues.reset(new std::queue<std::shared_ptr<BidCoSQueue>>());
		pendingBidCoSQueues->push(queue);
		if(!(rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp)) GD::devices.getCentral()->enqueuePackets(address, queue, true);
		else if(GD::debugLevel >= 5) std::cout << "Debug: Packet was queued and will be sent with next wake me up packet." << std::endl;
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
    return RPC::RPCVariable::createError(-1, "unknown error");
}
