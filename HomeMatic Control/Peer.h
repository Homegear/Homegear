#ifndef PEER_H
#define PEER_H

#include <iomanip>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <queue>
#include <mutex>

class HomeMaticDevice;
class BidCoSQueue;
class BidCoSMessages;

#include "RPC/Device.h"
#include "RPC/RPCVariable.h"

class BasicPeer
{
public:
	BasicPeer() {}
	BasicPeer(int32_t addr) { address = addr; }
	virtual ~BasicPeer() {}

	int32_t address;
	std::string serialNumber;
};

class RPCConfigurationParameter
{
public:
	RPCConfigurationParameter() {}
	virtual ~RPCConfigurationParameter() {}

	std::shared_ptr<RPC::Parameter> rpcParameter;
	bool changed = false;
	int64_t value = 0;
};

class Peer
{
    public:
		Peer() { pendingBidCoSQueues = std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>>(new std::queue<std::shared_ptr<BidCoSQueue>>()); }
		Peer(std::string serializedObject, HomeMaticDevice* device);
		virtual ~Peer() {}

        int32_t address = 0;
        std::string serialNumber = "";
        int32_t firmwareVersion = 0;
        int32_t remoteChannel = 0;
        int32_t localChannel = 0;
        HMDeviceTypes deviceType;
        uint8_t messageCounter = 0;
        std::unordered_map<int32_t, int32_t> config;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> configCentral;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> valuesCentral;
        std::shared_ptr<RPC::Device> rpcDevice;
        std::unordered_map<int32_t, std::vector<BasicPeer>> peers;
        BasicPeer team;
        std::vector<std::pair<std::string, uint32_t>> teamChannels;

        //Has to be shared_ptr because Peer must be copyable
        std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>> pendingBidCoSQueues;

        void initializeCentralConfig();
        std::string serialize();
        void serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config);
        void unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos);

        void packetReceived(std::shared_ptr<BidCoSPacket> packet);

        std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> getDeviceDescription();
        std::shared_ptr<RPC::RPCVariable> getParamsetDescription(uint32_t channel, RPC::ParameterSet::Type::Enum type);
        std::shared_ptr<RPC::RPCVariable> getValue(uint32_t channel, std::string valueKey);
        std::shared_ptr<RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
    private:
};

#endif // PEER_H
