#include "ServiceMessages.h"
#include "GD.h"

ServiceMessages::ServiceMessages(std::string peerSerialNumber, std::string serializedObject) : ServiceMessages(peerSerialNumber)
{
	if(serializedObject.empty()) return;
	if(GD::debugLevel >= 5) std::cout << "Unserializing service message: " << serializedObject << std::endl;

	std::istringstream stringstream(serializedObject);
	uint32_t pos = 0;
	unreach = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	stickyUnreach = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	configPending = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	lowbat = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	rssiDevice = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	rssiPeer = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
}

std::string ServiceMessages::serialize()
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(1) << (int32_t)unreach;
	stringstream << std::setw(1) << (int32_t)stickyUnreach;
	stringstream << std::setw(1) << (int32_t)configPending;
	stringstream << std::setw(1) << (int32_t)lowbat;
	stringstream << std::setw(8) << rssiDevice;
	stringstream << std::setw(8) << rssiPeer;
	stringstream << std::dec;
	return stringstream.str();
}
