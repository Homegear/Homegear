#ifndef PEER_H
#define PEER_H

#include <iomanip>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "XMLRPC/Device.h"
#include "HMDeviceTypes.h"

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
        HMDeviceTypes deviceType;
        uint8_t messageCounter = 0;
        std::unordered_map<int32_t, int32_t> config;
        XMLRPC::Device* xmlrpcDevice = nullptr;

        std::string serialize();
};

#endif // PEER_H
