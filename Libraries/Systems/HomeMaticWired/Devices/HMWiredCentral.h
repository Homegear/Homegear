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

#include "../../../HelperFunctions/HelperFunctions.h"
#include "../../../LogicalDevices/Central.h"
#include "../HMWiredDevice.h"
#include "../../../Types/RPCVariable.h"

#include <memory>
#include <mutex>
#include <string>

namespace HMWired
{

class HMWiredCentral : public HMWiredDevice, public Central
{
public:
	HMWiredCentral();
	HMWiredCentral(uint32_t deviceType, std::string serialNumber, int32_t address);
	virtual ~HMWiredCentral();
	void init();
	bool packetReceived(std::shared_ptr<Packet> packet);
	std::string handleCLICommand(std::string command);
	uint64_t getPeerIDFromSerial(std::string serialNumber) { std::shared_ptr<HMWiredPeer> peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }

	virtual bool knowsDevice(std::string serialNumber);
	virtual bool knowsDevice(uint64_t id);

	virtual std::shared_ptr<RPC::RPCVariable> addLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<RPC::RPCVariable> deleteDevice(std::string serialNumber, int32_t flags);
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceDescription(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceDescription(uint64_t id, int32_t channel);
	virtual std::shared_ptr<RPC::RPCVariable> getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getLinkPeers(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<RPC::RPCVariable> getLinks(std::string serialNumber, int32_t channel, int32_t flags);
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetDescription(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getPeerID(int32_t address);
	virtual std::shared_ptr<RPC::RPCVariable> getPeerID(std::string serialNumber);
	virtual std::shared_ptr<RPC::RPCVariable> getServiceMessages();
	virtual std::shared_ptr<RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<RPC::RPCVariable> getValue(uint64_t id, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<RPC::RPCVariable> listDevices();
	virtual std::shared_ptr<RPC::RPCVariable> listDevices(std::shared_ptr<std::map<uint64_t, int32_t>> knownDevices);
	virtual std::shared_ptr<RPC::RPCVariable> putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset);
	virtual std::shared_ptr<RPC::RPCVariable> removeLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<RPC::RPCVariable> searchDevices();
	virtual std::shared_ptr<RPC::RPCVariable> setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<RPC::RPCVariable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
	virtual std::shared_ptr<RPC::RPCVariable> setValue(uint64_t id, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
protected:
	std::shared_ptr<HMWiredPeer> createPeer(int32_t address, int32_t firmwareVersion, LogicalDeviceType deviceType, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);
};

} /* namespace HMWired */

#endif /* HMWIREDCENTRAL_H_ */
