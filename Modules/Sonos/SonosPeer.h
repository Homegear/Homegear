/* Copyright 2013-2015 Sathya Laufer
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

#ifndef SONOSPEER_H_
#define SONOSPEER_H_

#include "../Base/BaseLib.h"

#include <list>

namespace Sonos
{
class SonosCentral;
class SonosDevice;
class SonosPacket;

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

class SonosPeer : public BaseLib::Systems::Peer
{
public:
	SonosPeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	SonosPeer(int32_t id, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	virtual ~SonosPeer();

	//Features
	virtual bool wireless() { return false; }
	//End features

	// {{{ In table variables
	virtual void setIp(std::string value);
	// }}}

	void worker();
	virtual std::string handleCLICommand(std::string command);

	virtual bool load(BaseLib::Systems::LogicalDevice* device);
    virtual void loadVariables(BaseLib::Systems::LogicalDevice* device = nullptr, std::shared_ptr<BaseLib::Database::DataTable> rows = std::shared_ptr<BaseLib::Database::DataTable>());
    virtual void saveVariables();
    virtual void savePeers() {}

	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion() { return 0; }
	virtual std::string getFirmwareVersionString() { return Peer::getFirmwareVersionString(); }
	virtual std::string getFirmwareVersionString(int32_t firmwareVersion) { return _firmwareVersionString; }
    virtual bool firmwareUpdateAvailable() { return false; }

    void packetReceived(std::shared_ptr<SonosPacket> packet);

    std::string printConfig();

    /**
	 * {@inheritDoc}
	 */
    virtual void homegearShuttingDown();

	//RPC methods
	virtual std::shared_ptr<BaseLib::RPC::Variable> getDeviceInfo(int32_t clientID, std::map<std::string, bool> fields);
	virtual std::shared_ptr<BaseLib::RPC::Variable> getParamsetDescription(int32_t clientID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::Variable> getParamset(int32_t clientID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::Variable> putParamset(int32_t clientID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::Variable> variables, bool onlyPushing = false);
	virtual std::shared_ptr<BaseLib::RPC::Variable> setValue(int32_t clientID, uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::Variable> value);
	//End RPC methods
protected:
	std::shared_ptr<BaseLib::RPC::RPCEncoder> _binaryEncoder;
	std::shared_ptr<BaseLib::RPC::RPCDecoder> _binaryDecoder;
	std::shared_ptr<BaseLib::HTTPClient> _httpClient;
	int32_t _lastAvTransportSubscription = 0;

	virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
	virtual std::shared_ptr<BaseLib::Systems::LogicalDevice> getDevice(int32_t address);
	void getValuesFromPacket(std::shared_ptr<SonosPacket> packet, std::vector<FrameValues>& frameValue);

	/**
	 * {@inheritDoc}
	 */
	virtual std::shared_ptr<BaseLib::RPC::Variable> getValueFromDevice(std::shared_ptr<BaseLib::RPC::Parameter>& parameter, int32_t channel, bool asynchronous);

	virtual std::shared_ptr<BaseLib::RPC::ParameterSet> getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type);
};

}

#endif
