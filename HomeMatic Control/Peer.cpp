#include "Peer.h"
#include "GD.h"

XMLRPCConfigurationParameter::XMLRPCConfigurationParameter(std::string serializedObject)
{

}

std::string XMLRPCConfigurationParameter::serialize()
{

}

void Peer::initializeCentralConfig()
{
	if(xmlrpcDevice == nullptr)
	{
		if(GD::debugLevel >= 3) cout << "Warning: Tried to initialize peer's central config without xmlrpcDevice being set." << endl;
		return;
	}
	XMLRPCConfigurationParameter parameter;
	for(std::vector<XMLRPC::Parameter>::iterator i = xmlrpcDevice->parameterSet.parameters.begin(); i != xmlrpcDevice->parameterSet.parameters.end(); ++i)
	{
		if(i->physicalParameter.list < 9999)
		{
			parameter = XMLRPCConfigurationParameter();
			parameter.xmlrpcParameter = &(*i);
			configCentral[0][i->physicalParameter.list][i->physicalParameter.index] = parameter;
		}
	}
	for(std::vector<XMLRPC::DeviceChannel>::iterator i = xmlrpcDevice->channels.begin(); i != xmlrpcDevice->channels.end(); ++i)
	{
		XMLRPC::ParameterSet* masterSet = i->getParameterSet(XMLRPC::ParameterSet::Type::master);
		if(masterSet == nullptr) continue;
		for(std::vector<XMLRPC::Parameter>::iterator j = masterSet->parameters.begin(); j != masterSet->parameters.end(); ++j)
		{
			if(j->physicalParameter.list < 9999)
			{
				parameter = XMLRPCConfigurationParameter();
				parameter.xmlrpcParameter = &(*j);
				configCentral[i->index][j->physicalParameter.list][j->physicalParameter.index] = parameter;
			}
		}
	}
}

Peer::Peer(std::string serializedObject)
{
	pendingBidCoSQueues = shared_ptr<std::queue<shared_ptr<BidCoSQueue>>>(new std::queue<shared_ptr<BidCoSQueue>>());
	if(GD::debugLevel == 5) cout << "Unserializing peer: " << serializedObject << endl;

	std::istringstream stringstream(serializedObject);
	std::string entry;
	address = std::stoll(serializedObject.substr(0, 8), 0, 16);
	serialNumber = serializedObject.substr(8, 10);
	firmwareVersion = std::stoll(serializedObject.substr(18, 8), 0, 16);
	remoteChannel = std::stoll(serializedObject.substr(26, 2), 0, 16);
	localChannel = std::stoll(serializedObject.substr(28, 2), 0, 16);
	deviceType = (HMDeviceTypes)std::stoll(serializedObject.substr(30, 8), 0, 16);
	//This loads the corresponding xmlrpcDevice unnecessarily for virtual device peers, too. But so what?
	xmlrpcDevice = GD::xmlrpcDevices.find(deviceType, firmwareVersion);
	if(xmlrpcDevice == nullptr && GD::debugLevel >= 2) cout << "Error: Device type not found: 0x" << std::hex << (uint32_t)deviceType << " Firmware version: " << firmwareVersion << endl;
	messageCounter = std::stoll(serializedObject.substr(38, 2), 0, 16);
	uint32_t configSize = (std::stoll(serializedObject.substr(40, 8)) * 16);
	for(uint32_t i = 48; i < (48 + configSize); i += 16)
		config[std::stoll(serializedObject.substr(i, 8), 0, 16)] = std::stoll(serializedObject.substr(i + 8, 8), 0, 16);
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
	stringstream << std::dec;
	return stringstream.str();
}
