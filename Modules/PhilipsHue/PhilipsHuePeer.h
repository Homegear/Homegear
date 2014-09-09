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

#ifndef PHILIPSHUEPEER_H_
#define PHILIPSHUEPEER_H_

#include "../Base/BaseLib.h"
#include "PhilipsHuePacket.h"

#include <list>

using namespace BaseLib::Systems;

namespace PhilipsHue
{
class PhilipsHueCentral;
class PhilipsHueDevice;

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

class PhilipsHuePeer : public BaseLib::Systems::Peer
{
public:
	PhilipsHuePeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	PhilipsHuePeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler);
	virtual ~PhilipsHuePeer();

	//Features
	virtual bool wireless() { return true; }
	//End features

	virtual std::string handleCLICommand(std::string command);

	virtual bool load(BaseLib::Systems::LogicalDevice* device);
	virtual void save(bool savePeer, bool saveVariables, bool saveCentralConfig);
    virtual void loadVariables(BaseLib::Systems::LogicalDevice* device = nullptr, std::shared_ptr<BaseLib::Database::DataTable> rows = std::shared_ptr<BaseLib::Database::DataTable>());
    virtual void saveVariables();
	virtual void savePeers() {};
	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion() { return 0; }
	virtual std::string getFirmwareVersionString(int32_t firmwareVersion);
    virtual bool firmwareUpdateAvailable() { return false; }

	void packetReceived(std::shared_ptr<PhilipsHuePacket> packet);

	//RPC methods
	/**
	 * {@inheritDoc}
	 */
    virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getDeviceInfo(std::map<std::string, bool> fields);

    /**
	 * {@inheritDoc}
	 */
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetDescription(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);

	/**
	 * {@inheritDoc}
	 */
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);

	/**
	 * {@inheritDoc}
	 */
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> putParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> variables, bool onlyPushing = false);

	/**
	 * {@inheritDoc}
	 */
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value);
	//End RPC methods
protected:
	std::shared_ptr<BaseLib::RPC::RPCEncoder> _binaryEncoder;
	std::shared_ptr<BaseLib::RPC::RPCDecoder> _binaryDecoder;

	BaseLib::Math::Triangle _rgbGamut;
	double _gamma = 2.2;
	BaseLib::Math::Matrix3x3 _rgbXyzConversionMatrix;
	BaseLib::Math::Matrix3x3 _xyzRgbConversionMatrix;

	virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
	virtual std::shared_ptr<BaseLib::Systems::LogicalDevice> getDevice(int32_t address);
	void getValuesFromPacket(std::shared_ptr<PhilipsHuePacket> packet, std::vector<FrameValues>& frameValue);

	virtual std::shared_ptr<BaseLib::RPC::ParameterSet> getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type);

	void initializeConversionMatrix();
	void getXY(const std::string& rgb, BaseLib::Math::Point2D& xy, uint8_t& brightness);
	void getRGB(const BaseLib::Math::Point2D& xy, const uint8_t& brightness, std::string& rgb);
	double getHueFactor(const double& hue);
	double getHueFactor(const int32_t& hue);

	std::shared_ptr<BaseLib::RPC::RPCVariable> setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value, bool noSending);
};

}

#endif
