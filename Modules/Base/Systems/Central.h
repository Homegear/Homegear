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

#include "../RPC/RPCVariable.h"
#include "../RPC/Device.h"

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

	virtual bool knowsDevice(std::string serialNumber) { return false; }
	virtual bool knowsDevice(uint64_t id) { return false; }

	virtual std::shared_ptr<RPC::RPCVariable> addDevice(std::string serialNumber) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> addLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> addLink(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> deleteDevice(std::string serialNumber, int32_t flags) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> deleteDevice(uint64_t peerID, int32_t flags) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceDescription(std::string serialNumber, int32_t channel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceDescription(uint64_t id, int32_t channel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceInfo(uint64_t id, std::map<std::string, bool> fields) = 0;
	virtual std::shared_ptr<RPC::RPCVariable> getPeerID(int32_t address) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getPeerID(std::string serialNumber) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getInstallMode() { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setLinkInfo(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getLinkPeers(std::string serialNumber, int32_t channel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getLinkPeers(uint64_t peerID, int32_t channel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getLinks(std::string serialNumber, int32_t channel, int32_t flags);
	virtual std::shared_ptr<RPC::RPCVariable> getLinks(uint64_t peerID, int32_t channel, int32_t flags);
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetDescription(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetId(uint64_t peerID, uint32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getParamset(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getServiceMessages(bool returnID) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> getValue(uint64_t id, uint32_t channel, std::string valueKey) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> listDevices(bool channels, std::map<std::string, bool> fields) = 0;
	virtual std::shared_ptr<RPC::RPCVariable> listDevices(bool channels, std::map<std::string, bool> fields, std::shared_ptr<std::map<uint64_t, int32_t>> knownDevices) = 0;
	virtual std::shared_ptr<RPC::RPCVariable> listTeams() { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setTeam(std::string serialNumber, int32_t channel, std::string teamSerialNumber, int32_t teamChannel, bool force = false, bool burst = true) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setTeam(uint64_t peerID, int32_t channel, uint64_t teamID, int32_t teamChannel, bool force = false, bool burst = true) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> putParamset(uint64_t peerID, int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> removeLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> removeLink(uint64_t senderID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> searchDevices() { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setInstallMode(bool on, uint32_t duration = 60, bool debugOutput = true) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setValue(uint64_t id, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setName(uint64_t id, std::string name) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> updateFirmware(std::vector<uint64_t> ids, bool manual) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
	virtual std::shared_ptr<RPC::RPCVariable> setInterface(uint64_t peerID, std::string interfaceID) { return RPC::RPCVariable::createError(-32601, "Method not implemented for this central."); }
protected:
	LogicalDevice* _me = nullptr;
private:
	BaseLib::Obj* _baseLib = nullptr;
};

}
}
#endif /* CENTRAL_H_ */
