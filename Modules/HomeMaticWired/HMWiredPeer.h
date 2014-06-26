/* Copyright 2013-2014 Sathya Laufer
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

#ifndef HMWIREDPEER_H_
#define HMWIREDPEER_H_

#include "../Base/BaseLib.h"
#include "HMWiredPacket.h"

#include <list>

namespace HMWired
{
class CallbackFunctionParameter;
class HMWiredCentral;
class HMWiredDevice;

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
	BaseLib::RPC::ParameterSet::Type::Enum parameterSetType;
	std::map<std::string, FrameValue> values;
};

class HMWiredPeer : public BaseLib::Systems::Peer
{
public:
	HMWiredPeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	HMWiredPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	virtual ~HMWiredPeer();

	//Features
	virtual bool wireless() { return false; }
	//End features

	//In table variables:
	int32_t getMessageCounter() { return _messageCounter; }
	void setMessageCounter(int32_t value) { _messageCounter = value; saveVariable(5, value); }
	//End

	bool ignorePackets = false;

	virtual std::string handleCLICommand(std::string command);
	void initializeCentralConfig();
	void initializeLinkConfig(int32_t channel, std::shared_ptr<BaseLib::Systems::BasicPeer> peer);
	std::vector<int32_t> setConfigParameter(double index, double size, std::vector<uint8_t>& binaryValue);
	std::vector<int32_t> setMasterConfigParameter(int32_t channelIndex, double index, double step, double size, std::vector<uint8_t>& binaryValue);
	std::vector<int32_t> setMasterConfigParameter(int32_t channelIndex, int32_t addressStart, int32_t addressStep, double indexOffset, double size, std::vector<uint8_t>& binaryValue);
	std::vector<int32_t> setMasterConfigParameter(int32_t channel, std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet, std::shared_ptr<BaseLib::RPC::Parameter> parameter, std::vector<uint8_t>& binaryValue);
	std::vector<uint8_t> getConfigParameter(double index, double size, int32_t mask = -1, bool onlyKnownConfig = false);
	std::vector<uint8_t> getMasterConfigParameter(int32_t channelIndex, double index, double step, double size);
	std::vector<uint8_t> getMasterConfigParameter(int32_t channelIndex, int32_t addressStart, int32_t addressStep, double indexOffset, double size);
	std::vector<uint8_t> getMasterConfigParameter(int32_t channel, std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet, std::shared_ptr<BaseLib::RPC::Parameter> parameter);
	virtual bool load(BaseLib::Systems::LogicalDevice* device);
    void serializePeers(std::vector<uint8_t>& encodedData);
    void unserializePeers(std::shared_ptr<std::vector<char>> serializedData);
    virtual void loadVariables(BaseLib::Systems::LogicalDevice* device = nullptr, std::shared_ptr<BaseLib::Database::DataTable> rows = std::shared_ptr<BaseLib::Database::DataTable>());
    virtual void saveVariables();
	void savePeers();
	bool hasPeers(int32_t channel) { if(_peers.find(channel) == _peers.end() || _peers[channel].empty()) return false; else return true; }
	void addPeer(int32_t channel, std::shared_ptr<BaseLib::Systems::BasicPeer> peer);
	void removePeer(int32_t channel, uint64_t id, int32_t remoteChannel);
	int32_t getFreeEEPROMAddress(int32_t channel, bool isSender);
	int32_t getPhysicalIndexOffset(int32_t channel);
	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion();
    virtual bool firmwareUpdateAvailable();
	void restoreLinks();

	virtual std::shared_ptr<HMWiredPacket> getResponse(std::shared_ptr<HMWiredPacket> packet);
	virtual void reset();
	std::shared_ptr<BaseLib::RPC::ParameterSet> getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type);
	void getValuesFromPacket(std::shared_ptr<HMWiredPacket> packet, std::vector<FrameValues>& frameValue);
	void packetReceived(std::shared_ptr<HMWiredPacket> packet);

	//RPC methods
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getDeviceInfo(std::map<std::string, bool> fields);
	std::shared_ptr<BaseLib::RPC::RPCVariable> getLinkInfo(int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel);
	std::shared_ptr<BaseLib::RPC::RPCVariable> setLinkInfo(int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description);
	std::shared_ptr<BaseLib::RPC::RPCVariable> getLinkPeers(int32_t channel, bool returnID);
	std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetDescription(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	std::shared_ptr<BaseLib::RPC::RPCVariable> getValue(uint32_t channel, std::string valueKey);
	std::shared_ptr<BaseLib::RPC::RPCVariable> putParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> variables, bool putUnchanged = false, bool onlyPushing = false);
	std::shared_ptr<BaseLib::RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value);
	//End RPC methods
protected:
	uint32_t _bitmask[9] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

	//In table variables:
	uint8_t _messageCounter = 0;
	//End

	virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
	virtual std::shared_ptr<BaseLib::Systems::LogicalDevice> getDevice(int32_t address);
};

}

#endif /* HMWIREDPEER_H_ */
