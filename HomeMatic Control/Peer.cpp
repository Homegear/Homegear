#include "Peer.h"
#include "GD.h"

Peer::Peer(std::string serializedObject)
{
	if(GD::debugLevel == 5) cout << "Unserializing peer: " << serializedObject << endl;

	std::istringstream stringstream(serializedObject);
	std::string entry;
	address = std::stoll(serializedObject.substr(0, 8), 0, 16);
	serialNumber = serializedObject.substr(8, 10);
	firmwareVersion = std::stoll(serializedObject.substr(18, 8), 0, 16);
	remoteChannel = std::stoll(serializedObject.substr(26, 2), 0, 16);
	localChannel = std::stoll(serializedObject.substr(28, 2), 0, 16);
	deviceType = (HMDeviceTypes)std::stoll(serializedObject.substr(30, 8), 0, 16);
	xmlrpcDevice = GD::xmlrpcDevices.find(deviceType);
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
