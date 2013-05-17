#ifndef PEER_H
#define PEER_H

#include <string>
#include <unordered_map>

enum HMDeviceTypes { HMUNKNOWN = 0xFFFFFFFF, HMRCV50 = 0x0000, HMCCTC = 0x0039, HMCCVD = 0x003A };

class Peer
{
    public:
		Peer() {}
		virtual ~Peer() {}

        int32_t address = 0;
        std::string serialNumber = "";
        int32_t firmwareVersion = 0;
        int32_t remoteChannel = 0;
        int32_t localChannel = 0;
        HMDeviceTypes deviceType = HMRCV50;
        uint8_t messageCounter = 0;
        std::unordered_map<int32_t, int32_t> config;
};

#endif // PEER_H
