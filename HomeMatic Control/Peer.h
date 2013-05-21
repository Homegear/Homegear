#ifndef PEER_H
#define PEER_H

#include <iomanip>
#include <string>
#include <sstream>
#include <unordered_map>

enum HMDeviceTypes { HMUNKNOWN = 0xFFFFFFFF, HMSD = 0xFFFFFFFE, HMRCV50 = 0x0000, HMCCTC = 0x0039, HMCCVD = 0x003A };

class Peer
{
    public:
		Peer() {}
		Peer(std::string serializedObject);
		virtual ~Peer() {}

        int32_t address = 0;
        std::string serialNumber = "";
        int32_t firmwareVersion = 0;
        int32_t remoteChannel = 0;
        int32_t localChannel = 0;
        HMDeviceTypes deviceType = HMUNKNOWN;
        uint8_t messageCounter = 0;
        std::unordered_map<int32_t, int32_t> config;

        std::string serialize();
};

#endif // PEER_H
