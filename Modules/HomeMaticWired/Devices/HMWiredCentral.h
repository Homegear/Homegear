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

#ifndef HMWIREDCENTRAL_H_
#define HMWIREDCENTRAL_H_

class HMWiredPacket;

#include "../HMWiredDevice.h"

#include <memory>
#include <mutex>
#include <string>

namespace HMWired
{

class HMWiredCentral : public HMWiredDevice, public BaseLib::Systems::Central
{
public:
	HMWiredCentral(IDeviceEventSink* eventHandler);
	HMWiredCentral(uint32_t deviceType, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
	virtual ~HMWiredCentral();
	void init();
	virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet);
	std::string handleCLICommand(std::string command);
	uint64_t getPeerIDFromSerial(std::string serialNumber) { std::shared_ptr<HMWiredPeer> peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }
	void updateFirmwares(std::vector<uint64_t> ids);
	void updateFirmware(uint64_t id);
	void handleAnnounce(std::shared_ptr<HMWiredPacket> packet);
	bool peerInit(std::shared_ptr<HMWiredPeer> peer);

	virtual bool knowsDevice(std::string serialNumber);
	virtual bool knowsDevice(uint64_t id);

	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> addLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> addLink(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> deleteDevice(std::string serialNumber, int32_t flags);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> deleteDevice(uint64_t peerID, int32_t flags);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getDeviceDescription(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getDeviceDescription(uint64_t id, int32_t channel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getLinkPeers(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getLinkPeers(uint64_t peerID, int32_t channel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getLinks(std::string serialNumber, int32_t channel, int32_t flags);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getLinks(uint64_t peerID, int32_t channel, int32_t flags);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetDescription(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetDescription(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamset(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamset(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetId(std::string serialNumber, uint32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getParamsetId(uint64_t peerID, uint32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getPeerID(int32_t address);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getPeerID(std::string serialNumber);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getServiceMessages(bool returnID);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> getValue(uint64_t id, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> listDevices();
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> listDevices(std::shared_ptr<std::map<uint64_t, int32_t>> knownDevices);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> putParamset(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> putParamset(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> removeLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> removeLink(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> searchDevices();
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> setValue(uint64_t id, uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value);
	virtual std::shared_ptr<BaseLib::RPC::RPCVariable> updateFirmware(std::vector<uint64_t> ids, bool manual);
protected:
	std::shared_ptr<HMWiredPeer> createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);
private:
	std::mutex _peerInitMutex;

	//Updates:
	bool _updateMode = false;
	std::mutex _updateMutex;
	std::thread _updateFirmwareThread;
	//End
};

} /* namespace HMWired */

#endif /* HMWIREDCENTRAL_H_ */
