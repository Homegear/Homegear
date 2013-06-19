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
#include "ServiceMessages.h"

class BasicPeer
{
public:
	BasicPeer() {}
	BasicPeer(int32_t addr, std::string serial, bool hid) { address = addr; serialNumber = serial; hidden = hid; }
	virtual ~BasicPeer() {}

	int32_t address = 0;
	std::string serialNumber;
	int32_t channel = 0;
	bool hidden = false;
	std::string linkName;
	std::string linkDescription;
	std::shared_ptr<HomeMaticDevice> device;
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
		Peer() { serviceMessages = std::shared_ptr<ServiceMessages>(new ServiceMessages("")); pendingBidCoSQueues = std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>>(new std::queue<std::shared_ptr<BidCoSQueue>>()); }
		Peer(std::string serializedObject, HomeMaticDevice* device);
		virtual ~Peer() {}

		std::shared_ptr<ServiceMessages> serviceMessages;
        int32_t address = 0;
        std::string getSerialNumber() { return _serialNumber; }
        void setSerialNumber(std::string serialNumber) { if(serialNumber.length() > 100) return; _serialNumber = serialNumber; if(!serviceMessages) serviceMessages = std::shared_ptr<ServiceMessages>(new ServiceMessages(serialNumber)); else serviceMessages->setPeerSerialNumber(serialNumber); }
        int32_t firmwareVersion = 0;
        int32_t remoteChannel = 0;
        int32_t localChannel = 0;
        HMDeviceTypes deviceType;
        uint8_t messageCounter = 0;
        std::unordered_map<int32_t, int32_t> config;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> configCentral;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> valuesCentral;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> linksCentral;
        std::shared_ptr<RPC::Device> rpcDevice;
        std::unordered_map<int32_t, std::vector<BasicPeer>> peers;
        BasicPeer team;
        std::vector<std::pair<std::string, uint32_t>> teamChannels;
        bool homegearFeatures = false;

        //Has to be shared_ptr because Peer must be copyable
        std::shared_ptr<std::queue<std::shared_ptr<BidCoSQueue>>> pendingBidCoSQueues;

        void initializeCentralConfig();
        std::string serialize();
        void serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config);
        void unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos);
        void deleteFromDatabase(int32_t parentAddress);
        void saveToDatabase(int32_t parentAddress);
        void deletePairedVirtualDevices();

        void packetReceived(std::shared_ptr<BidCoSPacket> packet);
        bool setHomegearValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);

        std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> getDeviceDescription();
        std::shared_ptr<RPC::RPCVariable> getDeviceDescription(int32_t channel);
        std::shared_ptr<RPC::RPCVariable> getLink(int32_t channel, int32_t flags);
        std::shared_ptr<RPC::RPCVariable> getParamsetDescription(uint32_t channel, RPC::ParameterSet::Type::Enum type);
        std::shared_ptr<RPC::RPCVariable> getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type);
        std::shared_ptr<RPC::RPCVariable> getParamset(uint32_t channel, RPC::ParameterSet::Type::Enum type);
        std::shared_ptr<RPC::RPCVariable> getServiceMessages();
        std::shared_ptr<RPC::RPCVariable> getValue(uint32_t channel, std::string valueKey);
        std::shared_ptr<RPC::RPCVariable> putParamset(uint32_t channel, RPC::ParameterSet::Type::Enum type, std::shared_ptr<RPC::RPCVariable> variables);
        std::shared_ptr<RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
    private:
        std::string _serialNumber;
};

#endif // PEER_H
