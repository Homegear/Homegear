#include "Peer.h"

Peer::Peer(std::string serializedObject)
{
	std::istringstream stringstream(serializedObject);
	std::string entry;
	int32_t i = 0;
	while(std::getline(stringstream, entry, '.'))
	{
		switch(i)
		{
		case 0:
			address = std::stoll(entry, 0, 16);
			break;
		case 1:
			serialNumber = entry;
			break;
		case 2:
			firmwareVersion = std::stoll(entry, 0, 16);
			break;
		case 3:
			remoteChannel = std::stoll(entry, 0, 16);
			break;
		case 4:
			localChannel = std::stoll(entry, 0, 16);
			break;
		case 5:
			deviceType = (HMDeviceTypes)std::stoll(entry, 0, 16);
			break;
		case 6:
			messageCounter = std::stoll(entry, 0, 16);
			break;
		case 7:
			std::istringstream stringstream2(entry);
			std::string entry2;
			int32_t key;
			int32_t j = 0;
			while(std::getline(stringstream2, entry2, ','))
			{
				if(j % 2 == 0) key = std::stoll(entry2, 0, 16);
				else config[key] = std::stoll(entry2, 0, 16);
				j++;
			}
			break;
		}
		i++;
	}
}

std::string Peer::serialize()
{
	std::ostringstream stringstream;
	stringstream << std::hex;
	stringstream << address << ".";
	stringstream << serialNumber << ".";
	stringstream << firmwareVersion << ".";
	stringstream << remoteChannel << ".";
	stringstream << localChannel << ".";
	stringstream << (int32_t)deviceType << ".";
	stringstream << (int32_t)messageCounter << ".";
	for(std::unordered_map<int32_t, int32_t>::const_iterator i = config.begin(); i != config.end();)
	{
		stringstream << i->first << ",";
		stringstream << i->second;
		if(i != config.end()) stringstream << ",";
		++i;
	}
	stringstream << std::dec;
	return stringstream.str();
}
