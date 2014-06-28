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

#ifndef MAXPEER_H_
#define MAXPEER_H_

#include "../Base/BaseLib.h"
#include "MAXPacket.h"

#include <list>

namespace MAX
{
class MAXCentral;
class MAXDevice;

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

class MAXPeer : public BaseLib::Systems::Peer
{
public:
	bool ignorePackets = false;

	MAXPeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	MAXPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	virtual ~MAXPeer();

	//Features
	virtual bool wireless() { return true; }
	//End features

	//In table variables:
	int32_t getMessageCounter() { return _messageCounter; }
	void setMessageCounter(int32_t value) { _messageCounter = value; saveVariable(5, value); }
	//End

	virtual std::string handleCLICommand(std::string command);
	void initializeCentralConfig();

	virtual bool load(BaseLib::Systems::LogicalDevice* device);
    void serializePeers(std::vector<uint8_t>& encodedData);
    void unserializePeers(std::shared_ptr<std::vector<char>> serializedData);
    virtual void loadVariables(BaseLib::Systems::LogicalDevice* device = nullptr, std::shared_ptr<BaseLib::Database::DataTable> rows = std::shared_ptr<BaseLib::Database::DataTable>());
    virtual void saveVariables();
	virtual void savePeers();
	bool hasPeers(int32_t channel) { if(_peers.find(channel) == _peers.end() || _peers[channel].empty()) return false; else return true; }
	void addPeer(int32_t channel, std::shared_ptr<BaseLib::Systems::BasicPeer> peer);
	void removePeer(int32_t channel, uint64_t id, int32_t remoteChannel);
	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion() { return 0; }
    virtual bool firmwareUpdateAvailable() { return false; }

	void getValuesFromPacket(std::shared_ptr<MAXPacket> packet, std::vector<FrameValues>& frameValue);
	void packetReceived(std::shared_ptr<MAXPacket> packet);

	//RPC methods
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetDescription(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel) { return BaseLib::RPC::RPCVariable::createError(-32601, "Method not implemented for this peer."); }
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel) { return BaseLib::RPC::RPCVariable::createError(-32601, "Method not implemented for this peer."); }
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> putParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> variables, bool onlyPushing = false) { return BaseLib::RPC::RPCVariable::createError(-32601, "Method not implemented for this peer."); }
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value) { return BaseLib::RPC::RPCVariable::createError(-32601, "Method not implemented for this peer."); }
	//End RPC methods
protected:
	//In table variables:
	uint8_t _messageCounter = 0;
	//End

	virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
	virtual std::shared_ptr<BaseLib::Systems::LogicalDevice> getDevice(int32_t address);

	virtual std::shared_ptr<BaseLib::RPC::ParameterSet> getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type) { return std::shared_ptr<BaseLib::RPC::ParameterSet>(); }
};

}

#endif
