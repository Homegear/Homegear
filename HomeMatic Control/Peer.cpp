#include "Peer.h"
#include "GD.h"

void Peer::initializeCentralConfig()
{
	if(xmlrpcDevice == nullptr)
	{
		if(GD::debugLevel >= 3) cout << "Warning: Tried to initialize peer's central config without xmlrpcDevice being set." << endl;
		return;
	}
	XMLRPCConfigurationParameter parameter;
	for(std::vector<shared_ptr<RPC::Parameter>>::iterator i = xmlrpcDevice->parameterSet.parameters.begin(); i != xmlrpcDevice->parameterSet.parameters.end(); ++i)
	{
		if((*i)->physicalParameter.list < 9999)
		{
			parameter = XMLRPCConfigurationParameter();
			parameter.xmlrpcParameter = *i;
			configCentral[0][(uint32_t)RPC::ParameterSet::Type::Enum::master][(*i)->physicalParameter.list][(*i)->physicalParameter.index] = parameter;
		}
	}
	for(std::vector<shared_ptr<RPC::DeviceChannel>>::iterator i = xmlrpcDevice->channels.begin(); i != xmlrpcDevice->channels.end(); ++i)
	{
		RPC::ParameterSet* masterSet = (*i)->getParameterSet(RPC::ParameterSet::Type::master);
		if(masterSet == nullptr) continue;
		for(std::vector<shared_ptr<RPC::Parameter>>::iterator j = masterSet->parameters.begin(); j != masterSet->parameters.end(); ++j)
		{
			if((*j)->physicalParameter.list < 9999)
			{
				parameter = XMLRPCConfigurationParameter();
				parameter.xmlrpcParameter = *j;
				configCentral[(*i)->index][(uint32_t)RPC::ParameterSet::Type::Enum::master][(*j)->physicalParameter.list][(*j)->physicalParameter.index] = parameter;
			}
		}
	}
}

Peer::Peer(std::string serializedObject, HomeMaticDevice* device)
{
	pendingBidCoSQueues = shared_ptr<std::queue<shared_ptr<BidCoSQueue>>>(new std::queue<shared_ptr<BidCoSQueue>>());
	if(GD::debugLevel == 5) cout << "Unserializing peer: " << serializedObject << endl;

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
	xmlrpcDevice = GD::xmlrpcDevices.find(deviceType, firmwareVersion);
	if(xmlrpcDevice == nullptr && GD::debugLevel >= 2) cout << "Error: Device type not found: 0x" << std::hex << (uint32_t)deviceType << " Firmware version: " << firmwareVersion << endl;
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
		uint32_t typeCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t j = 0; j < typeCount; j++)
		{
			uint32_t parameterSetType = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			uint32_t listCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			for(uint32_t j = 0; j < listCount; j++)
			{
				uint32_t list = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				uint32_t parameterCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				for(uint32_t k = 0; k < parameterCount; k++)
				{
					double index = std::stod(serializedObject.substr(pos, 8)); pos += 8;
					XMLRPCConfigurationParameter* parameter = &configCentral[channel][parameterSetType][list][index];
					parameter->value = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
					parameter->changed = (bool)std::stoi(serializedObject.substr(pos, 1), 0, 16); pos += 1;
					uint32_t idLength = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
					std::string id = serializedObject.substr(pos, idLength); pos += idLength;
					if(xmlrpcDevice == nullptr)
					{
						if(GD::debugLevel >= 3) cout << "Warning: No xml rpc device found for peer 0x" << std::hex << address << "." << std::dec;
						continue;
					}
					parameter->xmlrpcParameter = xmlrpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(id);
				}
			}
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
	for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(8) << i->second.size();
		for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>>::const_iterator l = i->second.begin(); l != i->second.end(); ++l)
		{
			stringstream << std::setw(8) << l->first;
			stringstream << std::setw(8) << l->second.size();
			for(std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>::const_iterator j = l->second.begin(); j != l->second.end(); ++j)
			{
				stringstream << std::setw(8) << j->first;
				stringstream << std::setw(8) << j->second.size();
				for(std::unordered_map<double, XMLRPCConfigurationParameter>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					stringstream << std::setw(8) << k->first;
					//Four byte should be enough. Probably even two byte would be sufficient.
					stringstream << std::setw(8) << k->second.value;
					stringstream << std::setw(1) << (int32_t)k->second.changed;
					if(k->second.xmlrpcParameter == nullptr || k->second.xmlrpcParameter->id.size() == 0)
					{
						stringstream << std::setw(8) << 0;
						continue;
					}
					stringstream << std::setw(8) << k->second.xmlrpcParameter->id.size();
					stringstream << k->second.xmlrpcParameter->id;
				}
			}
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
