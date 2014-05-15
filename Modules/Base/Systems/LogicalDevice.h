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

#ifndef LOGICALDEVICE_H_
#define LOGICALDEVICE_H_

#include "../HelperFunctions/HelperFunctions.h"
#include "../Systems/DeviceFamilies.h"
#include "../Systems/Packet.h"
#include "../RPC/RPCVariable.h"
#include "../IEvents.h"
#include "Peer.h"

#include <memory>

namespace BaseLib
{
namespace Systems
{
class LogicalDevice : public Peer::IPeerEventSink, public IEvents
{
public:
	//Event handling
	class IDeviceEventSink : public IEventSinkBase
	{
	public:
		virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values) = 0;
		virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint) = 0;
		virtual void onRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions) = 0;
		virtual void onRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo) = 0;
		virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values) = 0;
	};

	virtual void raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	virtual void raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void raiseRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions);
	virtual void raiseRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo);
	virtual void raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	//End event handling

	//Peer event handling
	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	//End Peer event handling

	LogicalDevice(IDeviceEventSink* eventHandler);
	LogicalDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
	virtual ~LogicalDevice();

	virtual bool packetReceived(std::shared_ptr<Packet> packet) { return false; }
	virtual DeviceFamilies deviceFamily() = 0;

	virtual int32_t getAddress() { return _address; }
	virtual int32_t getID() { return _deviceID; }
    virtual std::string getSerialNumber() { return _serialNumber; }
    virtual uint32_t getDeviceType() { return _deviceType; }
	virtual std::string handleCLICommand(std::string command) { return ""; }
	virtual bool peerSelected() { return false; }
	virtual void dispose(bool wait = true) {}
	virtual void deletePeersFromDatabase() {}
	virtual void load() {}
	virtual void loadPeers(bool version_0_0_7) {}
	virtual void save(bool saveDevice) {}
	virtual void savePeers(bool full) {}
protected:
	int32_t _deviceID = 0;
	int32_t _address = 0;
    std::string _serialNumber;
    uint32_t _deviceType = 0;
};

}
}
#endif /* LOGICALDEVICE_H_ */
