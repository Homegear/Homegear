#include "Peer.h"
#include "GD.h"

void Peer::initializeCentralConfig()
{
	if(rpcDevice == nullptr)
	{
		if(GD::debugLevel >= 3) cout << "Warning: Tried to initialize peer's central config without xmlrpcDevice being set." << endl;
		return;
	}
	RPCConfigurationParameter parameter;
	for(std::vector<shared_ptr<RPC::Parameter>>::iterator i = rpcDevice->parameterSet->parameters.begin(); i != rpcDevice->parameterSet->parameters.end(); ++i)
	{
		if((*i)->physicalParameter->list < 9999)
		{
			parameter = RPCConfigurationParameter();
			parameter.rpcParameter = *i;
			if(!(*i)->id.empty()) configCentral[0][(*i)->id] = parameter;
		}
	}
	for(std::map<uint32_t, shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
	{
		if(i->second->parameterSets.find(RPC::ParameterSet::Type::master) == i->second->parameterSets.end() || !i->second->parameterSets[RPC::ParameterSet::Type::master]) continue;
		shared_ptr<RPC::ParameterSet> masterSet = i->second->parameterSets[RPC::ParameterSet::Type::master];
		for(std::vector<shared_ptr<RPC::Parameter>>::iterator j = masterSet->parameters.begin(); j != masterSet->parameters.end(); ++j)
		{
			if((*j)->physicalParameter->list < 9999)
			{
				parameter = RPCConfigurationParameter();
				parameter.rpcParameter = *j;
				if(!(*j)->id.empty()) configCentral[i->first][(*j)->id] = parameter;
			}
		}
	}
}

Peer::Peer(std::string serializedObject, HomeMaticDevice* device)
{
	pendingBidCoSQueues = shared_ptr<std::queue<shared_ptr<BidCoSQueue>>>(new std::queue<shared_ptr<BidCoSQueue>>());
	if(GD::debugLevel >= 5) cout << "Unserializing peer: " << serializedObject << endl;

	std::istringstream stringstream(serializedObject);
	std::string entry;
	uint32_t pos = 0;
	address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	serialNumber = serializedObject.substr(pos, 10); pos += 10;
	firmwareVersion = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	remoteChannel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	localChannel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	deviceType = (HMDeviceTypes)std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	//This loads the corresponding xmlrpcDevice unnecessarily for virtual device peers, too. But so what?
	rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion);
	if(rpcDevice == nullptr && GD::debugLevel >= 2) cout << "Error: Device type not found: 0x" << std::hex << (uint32_t)deviceType << " Firmware version: " << firmwareVersion << endl;
	messageCounter = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	uint32_t configSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
	for(uint32_t i = 0; i < configSize; i++)
	{
		config[std::stoll(serializedObject.substr(pos, 8), 0, 16)] = std::stoll(serializedObject.substr(pos + 8, 8), 0, 16); pos += 16;
	}
	uint32_t configCentralSize = std::stoll(serializedObject.substr(pos, 8)); pos += 8;
	for(uint32_t i = 0; i < configCentralSize; i++)
	{
		uint32_t channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		uint32_t parameterCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t k = 0; k < parameterCount; k++)
		{
			uint32_t idLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			if(idLength == 0 && GD::debugLevel >= 1) cout << "Critical: Added central config parameter without id. Device: " << address << " Channel: " << channel;
			std::string id = serializedObject.substr(pos, idLength); pos += idLength;
			RPCConfigurationParameter* parameter = &configCentral[channel][id];
			parameter->value = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			parameter->changed = (bool)std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
			if(rpcDevice == nullptr)
			{
				if(GD::debugLevel >= 1) cout << "Critical: No xml rpc device found for peer 0x" << std::hex << address << "." << std::dec;
				continue;
			}
			parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::Enum::master]->getParameter(id);
		}
	}
	uint32_t peersSize = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
	for(uint32_t i = 0; i < peersSize; i++)
	{
		uint32_t channel = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
		uint32_t peerCount = (std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
		for(uint32_t j = 0; j < peerCount; j++)
		{
			peers[channel].push_back(std::stoll(serializedObject.substr(pos, 8), 0, 16)); pos += 8;
		}
	}
	uint32_t pendingQueuesSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	for(uint32_t i = 0; i < pendingQueuesSize; i++)
	{
		uint32_t queueLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		if(queueLength > 0)
		{
			pendingBidCoSQueues->push(shared_ptr<BidCoSQueue>(new BidCoSQueue(serializedObject.substr(pos, queueLength), device))); pos += queueLength;
			pendingBidCoSQueues->back()->noSending = true;
		}
	}
}

std::string Peer::serialize()
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << address;
	if(serialNumber.size() != 10) stringstream << "0000000000"; else stringstream << serialNumber;
	stringstream << std::setw(8) << firmwareVersion;
	stringstream << std::setw(2) << remoteChannel;
	stringstream << std::setw(2) << localChannel;
	stringstream << std::setw(8) << (int32_t)deviceType;
	stringstream << std::setw(2) << (int32_t)messageCounter;
	stringstream << std::setw(8) << config.size();
	for(std::unordered_map<int32_t, int32_t>::const_iterator i = config.begin(); i != config.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second;
	}
	stringstream << std::setw(8) << configCentral.size();
	for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second.size();
		for(std::unordered_map<std::string, RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			if(!j->second.rpcParameter)
			{
				if(GD::debugLevel >= 1) cout << "Critical: Parameter has no corresponding RPC parameter. Writing dummy data. Device: " << address << " Channel: " << i->first << endl;
				stringstream << std::setw(8) << 0;
				stringstream << std::setw(8) << 0;
				stringstream << std::setw(1) << 0;
				continue;
			}
			if(j->second.rpcParameter->id.size() == 0 && GD::debugLevel >= 2)
			{
				cout << "Error: Parameter has no id." << endl;
			}
			stringstream << std::setw(8) << j->second.rpcParameter->id.size();
			stringstream << j->second.rpcParameter->id;
			//Four byte should be enough. Probably even two byte would be sufficient.
			stringstream << std::setw(8) << j->second.value;
			stringstream << std::setw(1) << (int32_t)j->second.changed;
		}
	}
	stringstream << std::setw(8) << peers.size();
	for(std::unordered_map<int32_t, std::vector<PairedPeer>>::const_iterator i = peers.begin(); i != peers.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second.size();
		for(std::vector<PairedPeer>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			stringstream << std::setw(8) << j->address;
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
	stringstream << std::dec;
	return stringstream.str();
}

std::vector<std::shared_ptr<RPC::RPCVariable>> Peer::getDeviceDescription()
{
	std::vector<std::shared_ptr<RPC::RPCVariable>> descriptions;
	std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ADDRESS", serialNumber)));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray)));
	RPC::RPCVariable* variable = description->structValue->back().get();
	variable->name = "CHILDREN";
	for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
	{
		variable->arrayValue->push_back(shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(serialNumber + ":" + std::to_string(i->second->index))));
	}

	if(firmwareVersion != 0)
	{
		std::ostringstream stringStream;
		stringStream << std::setw(2) << std::hex << (int32_t)firmwareVersion << std::dec;
		std::string firmware = stringStream.str();
		if(firmware.size() == 2) description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FIRMWARE", firmware.substr(0, 1) + "." + firmware.substr(1))));
	}

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FLAGS", (int32_t)rpcDevice->uiFlags)));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("INTERFACE", GD::devices.getCentral()->serialNumber())));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray)));
	variable = description->structValue->back().get();
	variable->name = "PARAMSETS";
	variable->arrayValue->push_back(shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("MASTER")));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("PARENT", "")));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("RF_ADDRESS", std::to_string(address))));

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ROAMING", 0)));

	shared_ptr<RPC::DeviceType> type = rpcDevice->getType(deviceType, firmwareVersion);
	if(type)
	{
		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TYPE", type->id)));
	}

	description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("VERSION", rpcDevice->version)));

	descriptions.push_back(description);

	for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
	{
		description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("ADDRESS", serialNumber + ":" + std::to_string(i->first))));

		int32_t aesActive = 0;
		if(configCentral.find(i->first) != configCentral.end() && configCentral.at(i->first).find("AES_ACTIVE") != configCentral.at(i->first).end() && configCentral.at(i->first).at("AES_ACTIVE").value != 0)
		{
			aesActive = 1;
		}
		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("AES_ACTIVE", aesActive)));

		int32_t direction = 0;
		ostringstream linkSourceRoles;
		ostringstream linkTargetRoles;
		for(std::vector<std::shared_ptr<RPC::LinkRole>>::iterator j = i->second->linkRoles.begin(); j != i->second->linkRoles.end(); ++j)
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
		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("DIRECTION", direction)));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("FLAGS", (int32_t)i->second->uiFlags)));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("INDEX", i->first)));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("LINK_SOURCE_ROLES", linkSourceRoles.str())));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("LINK_TARGET_ROLES", linkTargetRoles.str())));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray)));
		variable = description->structValue->back().get();
		variable->name = "PARAMSETS";
		for(std::map<RPC::ParameterSet::Type::Enum, std::shared_ptr<RPC::ParameterSet>>::iterator j = i->second->parameterSets.begin(); j != i->second->parameterSets.end(); ++j)
		{
			variable->arrayValue->push_back(shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second->typeString())));
		}

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("PARENT", serialNumber)));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("PARENT_TYPE", type->id)));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("TYPE", i->second->type)));

		description->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("VERSION", rpcDevice->version)));

		descriptions.push_back(description);
	}

	return descriptions;
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetDescription(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type)
{
	if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable);
	if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable);
	shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
	std::shared_ptr<RPC::RPCVariable> descriptions(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
	std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
	std::shared_ptr<RPC::RPCVariable> element;
	uint32_t index = 0;
	for(vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
	{
		description->name = (*i)->id;
		if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeBoolean)
		{
			shared_ptr<RPC::LogicalParameterBoolean> parameter(dynamic_cast<RPC::LogicalParameterBoolean*>((*i)->logicalParameter.get()));

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
		else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeInteger)
		{
			shared_ptr<RPC::LogicalParameterInteger> parameter(dynamic_cast<RPC::LogicalParameterInteger*>((*i)->logicalParameter.get()));

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
			element->stringValue = "INTEGER";
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
