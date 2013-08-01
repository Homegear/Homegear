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
#include <thread>

class PendingBidCoSQueues;
class CallbackFunctionParameter;
class HomeMaticDevice;
class BidCoSQueue;
class BidCoSMessages;

#include "delegate.hpp"
#include "RPC/Device.h"
#include "RPC/RPCVariable.h"
#include "ServiceMessages.h"

class VariableToReset
{
public:
	uint32_t channel = 0;
	std::string key;
	std::vector<uint8_t> data;
	int64_t resetTime = 0;
	bool isDominoEvent = false;

	VariableToReset() {}
	virtual ~VariableToReset() {}
};

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
	std::vector<uint8_t> data;
};

class Peer
{
    public:
		Peer();
		Peer(std::string serializedObject, HomeMaticDevice* device);
		virtual ~Peer();

		std::shared_ptr<ServiceMessages> serviceMessages;
        int32_t address = 0;
        std::string getSerialNumber() { return _serialNumber; }
        void setSerialNumber(std::string serialNumber) { if(serialNumber.length() > 100) return; _serialNumber = serialNumber; if(!serviceMessages) serviceMessages = std::shared_ptr<ServiceMessages>(new ServiceMessages(serialNumber)); else serviceMessages->setPeerSerialNumber(serialNumber); }
        int32_t countFromSysinfo = 0;
        int32_t firmwareVersion = 0;
        int32_t remoteChannel = 0;
        int32_t localChannel = 0;
        HMDeviceTypes deviceType;
        uint8_t messageCounter = 0;
        std::unordered_map<int32_t, int32_t> config;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> configCentral;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> valuesCentral;
        std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>> linksCentral;
        std::shared_ptr<RPC::Device> rpcDevice;

        BasicPeer team;
        int32_t teamChannel = -1;
        std::vector<std::pair<std::string, uint32_t>> teamChannels;
        bool pairingComplete = false;

        std::shared_ptr<PendingBidCoSQueues> pendingBidCoSQueues;

        void initializeCentralConfig();
        void initializeLinkConfig(int32_t channel, int32_t address, int32_t remoteChannel);
        void applyConfigFunction(int32_t channel, int32_t address, int32_t remoteChannel);
        std::string serialize();
        void serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config);
        void serializeConfig(std::ostringstream& stringstream, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>& config);
        void unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos);
        void unserializeConfig(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos);
        void deleteFromDatabase(int32_t parentAddress);
        void saveToDatabase(int32_t parentAddress);
        void deletePairedVirtualDevices();
        bool hasPeers(int32_t channel) { if(_peers.find(channel) == _peers.end() || _peers[channel].empty()) return false; else return true; }
        void addPeer(int32_t channel, std::shared_ptr<BasicPeer> peer);
        std::shared_ptr<BasicPeer> getPeer(int32_t channel, int32_t address, int32_t remoteChannel = -1);
        std::shared_ptr<BasicPeer> getPeer(int32_t channel, std::string serialNumber, int32_t remoteChannel = -1);
        void removePeer(int32_t channel, int32_t address);
        void addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters);

        void handleDominoEvent(std::shared_ptr<RPC::Parameter> parameter, std::string& frameID, uint32_t channel);
        void getValuesFromPacket(std::shared_ptr<BidCoSPacket> packet, std::string& frameID, uint32_t& parameterSetChannel, RPC::ParameterSet::Type::Enum& parameterSetType, std::map<std::string, std::vector<uint8_t>>& values);
        void packetReceived(std::shared_ptr<BidCoSPacket> packet);
        bool setHomegearValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
        int32_t getChannelGroupedWith(int32_t channel);

        std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> getDeviceDescription();
        std::shared_ptr<RPC::RPCVariable> getDeviceDescription(int32_t channel);
        std::shared_ptr<RPC::RPCVariable> getLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
        std::shared_ptr<RPC::RPCVariable> setLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
        std::shared_ptr<RPC::RPCVariable> getLinkPeers(int32_t channel);
        std::shared_ptr<RPC::RPCVariable> getLink(int32_t channel, int32_t flags, bool avoidDuplicates);
        std::shared_ptr<RPC::RPCVariable> getParamsetDescription(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
        std::shared_ptr<RPC::RPCVariable> getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type);
        std::shared_ptr<RPC::RPCVariable> getParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
        std::shared_ptr<RPC::RPCVariable> getServiceMessages();
        std::shared_ptr<RPC::RPCVariable> getValue(uint32_t channel, std::string valueKey);
        std::shared_ptr<RPC::RPCVariable> putParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> variables, bool putUnchanged = false, bool onlyPushing = false);
        std::shared_ptr<RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
    private:
        bool _stopWorkerThread = true;
        std::thread _workerThread;
        std::string _serialNumber;
        std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>> _peers;
        std::mutex _variablesToResetMutex;
        std::vector<std::shared_ptr<VariableToReset>> _variablesToReset;
        void worker();
};

#endif // PEER_H
