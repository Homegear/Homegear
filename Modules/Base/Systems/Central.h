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

#ifndef CENTRAL_H_
#define CENTRAL_H_

#include "../RPC/Variable.h"
#include "../RPC/Device.h"

#include <set>

namespace BaseLib
{

namespace Systems
{

class LogicalDevice;

class Central
{
public:
	Central(BaseLib::Obj* baseLib, LogicalDevice* me);
	virtual ~Central();

	virtual LogicalDevice* logicalDevice() { return _me; }
	virtual int32_t physicalAddress();
	virtual uint64_t getPeerIDFromSerial(std::string serialNumber) { return 0; }

	virtual bool knowsDevice(std::string serialNumber) = 0;
	virtual bool knowsDevice(uint64_t id) = 0;

	virtual std::shared_ptr<RPC::Variable> addDevice(std::string serialNumber) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> addLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> addLink(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> deleteDevice(std::string serialNumber, int32_t flags) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> deleteDevice(uint64_t peerID, int32_t flags) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> getAllValues(uint64_t peerID, bool returnWriteOnly);
	virtual std::shared_ptr<RPC::Variable> getDeviceDescription(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<RPC::Variable> getDeviceDescription(uint64_t id, int32_t channel);
	virtual std::shared_ptr<RPC::Variable> getDeviceInfo(uint64_t id, std::map<std::string, bool> fields) = 0;
	virtual std::shared_ptr<RPC::Variable> getPeerID(int32_t address);
	virtual std::shared_ptr<RPC::Variable> getPeerID(std::string serialNumber);
	virtual std::shared_ptr<RPC::Variable> getInstallMode() { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<RPC::Variable> getLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel);
	virtual std::shared_ptr<RPC::Variable> getLinkPeers(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<RPC::Variable> getLinkPeers(uint64_t peerID, int32_t channel);
	virtual std::shared_ptr<RPC::Variable> getLinks(std::string serialNumber, int32_t channel, int32_t flags);
	virtual std::shared_ptr<RPC::Variable> getLinks(uint64_t peerID, int32_t channel, int32_t flags);
	virtual std::shared_ptr<RPC::Variable> getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::Variable> getParamsetDescription(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::Variable> getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::Variable> getParamsetId(uint64_t peerID, uint32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::Variable> getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::Variable> getParamset(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::Variable> getServiceMessages(bool returnID);
	virtual std::shared_ptr<RPC::Variable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<RPC::Variable> getValue(uint64_t id, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<RPC::Variable> listDevices(bool channels, std::map<std::string, bool> fields);
	virtual std::shared_ptr<RPC::Variable> listDevices(bool channels, std::map<std::string, bool> fields, std::shared_ptr<std::set<uint64_t>> knownDevices);
	virtual std::shared_ptr<RPC::Variable> listTeams() { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::Variable> paramset) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> putParamset(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<RPC::Variable> paramset) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> reportValueUsage(std::string serialNumber);
	virtual std::shared_ptr<RPC::Variable> removeLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> removeLink(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> rssiInfo();
	virtual std::shared_ptr<RPC::Variable> searchDevices() { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }

	/**
     * RPC function to change the ID of the peer.
     *
     * @param oldPeerID The current ID of the peer.
     * @param newPeerID The new ID of the peer.
     * @return Returns "RPC void" on success or RPC error "-100" when the new peer ID is invalid and error "-101" when the new peer ID is already in use.
     */
    virtual std::shared_ptr<BaseLib::RPC::Variable> setId(uint64_t oldPeerID, uint64_t newPeerID);
	virtual std::shared_ptr<RPC::Variable> setInstallMode(bool on, uint32_t duration = 60, bool debugOutput = true) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> setInterface(uint64_t peerID, std::string interfaceID) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<RPC::Variable> setLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<RPC::Variable> setName(uint64_t id, std::string name);
	virtual std::shared_ptr<RPC::Variable> setTeam(std::string serialNumber, int32_t channel, std::string teamSerialNumber, int32_t teamChannel, bool force = false, bool burst = true) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> setTeam(uint64_t peerID, int32_t channel, uint64_t teamID, int32_t teamChannel, bool force = false, bool burst = true) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::Variable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::Variable> value);
	virtual std::shared_ptr<RPC::Variable> setValue(uint64_t id, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::Variable> value);
	virtual std::shared_ptr<RPC::Variable> updateFirmware(std::vector<uint64_t> ids, bool manual) { return RPC::Variable::createError(-32601, "Method not implemented for this central."); }
protected:
	LogicalDevice* _me = nullptr;
private:
	BaseLib::Obj* _baseLib = nullptr;
};

}
}
#endif /* CENTRAL_H_ */
