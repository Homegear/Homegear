/* Copyright 2013 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

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
#include <list>

class PendingBidCoSQueues;
class CallbackFunctionParameter;
class HomeMaticDevice;
class BidCoSQueue;
class BidCoSMessages;

#include "../delegate.hpp"
#include "../RPC/Device.h"
#include "../RPC/RPCVariable.h"
#include "../ServiceMessages.h"

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

class FrameValue
{
public:
	std::list<uint32_t> channels;
	std::vector<uint8_t> value;
};

class FrameValues
{
public:
	std::string frameID;
	std::list<uint32_t> paramsetChannels;
	RPC::ParameterSet::Type::Enum parameterSetType;
	std::map<std::string, FrameValue> values;
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
	std::vector<uint8_t> data;
};

class RPCConfigurationParameter
{
public:
	RPCConfigurationParameter() {}
	virtual ~RPCConfigurationParameter() {}

	uint32_t databaseID = 0;
	std::shared_ptr<RPC::Parameter> rpcParameter;
	std::vector<uint8_t> data;
	std::vector<uint8_t> partialData;
};

class Peer
{
    public:
		Peer(int32_t parentAddress, bool centralFeatures);
		Peer(int32_t id, int32_t address, std::string serialNumber, int32_t parentAddress, bool centralFeatures);
		virtual ~Peer();
		void dispose();

		//In table peers:
		int32_t getParentAddress() { return _parentAddress; }
		int32_t getAddress() { return _address; }
		void setAddress(int32_t value) { _address = value; if(_peerID > 0) save(true, false, false); }
		std::string getSerialNumber() { return _serialNumber; }
		void setSerialNumber(std::string serialNumber) { if(serialNumber.length() > 20) return; _serialNumber = serialNumber; if(_peerID > 0) save(true, false, false); }
		//End

		//In table variables:
		int32_t getFirmwareVersion() { return _firmwareVersion; }
		void setFirmwareVersion(int32_t value) { _firmwareVersion = value; saveVariable(0, value); }
		int32_t getRemoteChannel() { return _remoteChannel; }
		void setRemoteChannel(int32_t value) { _remoteChannel = value; saveVariable(1, value); }
		int32_t getLocalChannel() { return _localChannel; }
		void setLocalChannel(int32_t value) { _localChannel = value; saveVariable(2, value); }
		LogicalDeviceType getDeviceType() { return _deviceType; }
		void setDeviceType(LogicalDeviceType value) { _deviceType = value; saveVariable(3, (uint32_t)value.id()); }
		int32_t getCountFromSysinfo() { return _countFromSysinfo; }
		void setCountFromSysinfo(int32_t value) { _countFromSysinfo = value; saveVariable(4, value); }
		int32_t getMessageCounter() { return _messageCounter; }
		void setMessageCounter(int32_t value) { _messageCounter = value; saveVariable(5, value); }
		int32_t getPairingComplete() { return _pairingComplete; }
		void setPairingComplete(int32_t value) { _pairingComplete = value; saveVariable(6, value); }
		int32_t getTeamChannel() { return _teamChannel; }
		void setTeamChannel(int32_t value) { _teamChannel = value; saveVariable(7, value); }
		int32_t getTeamRemoteAddress() { return _team.address; }
		void setTeamRemoteAddress(int32_t value) { _team.address = value; saveVariable(8, value); }
		int32_t getTeamRemoteChannel() { return _team.channel; }
		void setTeamRemoteChannel(int32_t value) { _team.channel = value; saveVariable(9, value); }
		std::string getTeamRemoteSerialNumber() { return _team.serialNumber; }
		void setTeamRemoteSerialNumber(std::string value) { _team.serialNumber = value; saveVariable(10, value); }
		std::vector<uint8_t>& getTeamData() { return _team.data; }
		//End

		//Needed, so the peer gets not saved in central's worker thread while being deleted
		bool deleting = false;
		std::shared_ptr<ServiceMessages> serviceMessages;
        void setCentralFeatures(bool value) { _centralFeatures = value; }

        std::unordered_map<int32_t, int32_t> config;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> configCentral;
        std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>> valuesCentral;
        std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>> linksCentral;
        std::shared_ptr<RPC::Device> rpcDevice;

        std::vector<std::pair<std::string, uint32_t>> teamChannels;

        std::shared_ptr<PendingBidCoSQueues> pendingBidCoSQueues;

        void worker();
        std::string handleCLICommand(std::string command);
        void stopThreads();
        void initializeCentralConfig();
        void initializeLinkConfig(int32_t channel, int32_t address, int32_t remoteChannel, bool useConfigFunction);
        void applyConfigFunction(int32_t channel, int32_t address, int32_t remoteChannel);
        bool load(HomeMaticDevice* device);
        void save(bool savePeer, bool saveVariables, bool saveCentralConfig);
        void unserialize_0_0_6(std::string& serializedObject, HomeMaticDevice* device);
        void serializePeers(std::vector<uint8_t>& encodedData);
        void unserializePeers(std::shared_ptr<std::vector<char>> serializedData);
        void serializeNonCentralConfig(std::vector<uint8_t>& encodedData);
        void unserializeNonCentralConfig(std::shared_ptr<std::vector<char>> serializedData);
        void serializeVariablesToReset(std::vector<uint8_t>& encodedData);
        void unserializeVariablesToReset(std::shared_ptr<std::vector<char>> serializedData);
        void loadConfig();
        void saveConfig();
        void loadVariables(HomeMaticDevice* device = nullptr);
        void saveVariables();
        void saveVariable(uint32_t index, int32_t intValue);
        void saveVariable(uint32_t index, std::string& stringValue);
        void saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue);
        void savePeers();
        void saveNonCentralConfig();
        void saveVariablesToReset();
        void saveServiceMessages();
        void savePendingQueues();
        void saveParameter(uint32_t parameterID, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t channel, std::string& parameterName, std::vector<uint8_t>& value, int32_t remoteAddress = 0, uint32_t remoteChannel = 0);
        void saveParameter(uint32_t parameterID, std::vector<uint8_t>& value);
        void unserializeConfig_0_0_6(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos);
        void unserializeConfig_0_0_6(std::string& serializedObject, std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>& config, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t& pos);
        void deleteFromDatabase();
        void deletePairedVirtualDevice(int32_t address);
        void deletePairedVirtualDevices();
        bool hasTeam() { return !_team.serialNumber.empty(); }
        bool isTeam() { return _serialNumber.front() == '*'; }
        bool hasPeers(int32_t channel) { if(_peers.find(channel) == _peers.end() || _peers[channel].empty()) return false; else return true; }
        void addPeer(int32_t channel, std::shared_ptr<BasicPeer> peer);
        std::shared_ptr<BasicPeer> getPeer(int32_t channel, int32_t address, int32_t remoteChannel = -1);
        std::shared_ptr<HomeMaticDevice> getHiddenPeerDevice();
        std::shared_ptr<BasicPeer> getHiddenPeer(int32_t channel);
        std::shared_ptr<BasicPeer> getPeer(int32_t channel, std::string serialNumber, int32_t remoteChannel = -1);
        void removePeer(int32_t channel, int32_t address, int32_t remoteChannel);
        void addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters);
        void setRSSI(uint8_t rssi);

        void handleDominoEvent(std::shared_ptr<RPC::Parameter> parameter, std::string& frameID, uint32_t channel);
        void getValuesFromPacket(std::shared_ptr<BidCoSPacket> packet, std::vector<FrameValues>& frameValue);
        bool hasLowbatBit(std::shared_ptr<RPC::DeviceFrame> frame);
        void packetReceived(std::shared_ptr<BidCoSPacket> packet);
        bool setHomegearValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
        int32_t getChannelGroupedWith(int32_t channel);
        std::string printConfig();
        void setLastPacketReceived();
        uint32_t getLastPacketReceived() { return _lastPacketReceived; }

        std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> getDeviceDescription();
        std::shared_ptr<RPC::RPCVariable> getDeviceDescription(int32_t channel);
        std::shared_ptr<RPC::RPCVariable> getLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
        std::shared_ptr<RPC::RPCVariable> setLinkInfo(int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
        std::shared_ptr<RPC::RPCVariable> getLinkPeers(int32_t channel);
        std::shared_ptr<RPC::RPCVariable> getLink(int32_t channel, int32_t flags, bool avoidDuplicates);
        std::shared_ptr<RPC::RPCVariable> getParamsetDescription(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
        std::shared_ptr<RPC::RPCVariable> getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
        std::shared_ptr<RPC::RPCVariable> getParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
        std::shared_ptr<RPC::RPCVariable> getServiceMessages();
        std::shared_ptr<RPC::RPCVariable> getValue(uint32_t channel, std::string valueKey);
        std::shared_ptr<RPC::RPCVariable> putParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> variables, bool putUnchanged = false, bool onlyPushing = false);
        std::shared_ptr<RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
    private:
        uint32_t _peerID = 0;
        std::map<uint32_t, uint32_t> _variableDatabaseIDs;
        bool _disposing = false;
        bool _centralFeatures = false;
        uint32_t _lastPacketReceived = 0;
        uint32_t _lastRSSI = 0;
        std::mutex _databaseMutex;
        std::mutex _variablesToResetMutex;
        std::vector<std::shared_ptr<VariableToReset>> _variablesToReset;
        //std::map<std::string, uint32_t> _resendCounter;

        //In table peers:
        int32_t _parentAddress = 0;
        int32_t _address = 0;
        std::string _serialNumber;
        //End

        //In table variables:
        int32_t _firmwareVersion = 0;
		int32_t _remoteChannel = 0;
		int32_t _localChannel = 0;
		LogicalDeviceType _deviceType;
		int32_t _countFromSysinfo = 0;
		uint8_t _messageCounter = 0;
		bool _pairingComplete = false;
		int32_t _teamChannel = -1;
		BasicPeer _team;
		std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>> _peers;
		//End
};

#endif // PEER_H
