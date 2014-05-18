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

#ifndef DEVICEFAMILY_H_
#define DEVICEFAMILY_H_

#include "../Database/Database.h"
#include "Central.h"
#include "LogicalDevice.h"
#include "PhysicalDeviceSettings.h"
#include "PhysicalDevice.h"
#include "DeviceFamilies.h"
#include "../RPC/RPCVariable.h"

#include <iostream>
#include <string>
#include <memory>

namespace BaseLib
{

class Obj;

namespace Systems
{
class DeviceFamily : public LogicalDevice::IDeviceEventSink, public IEvents
{
public:
	//Event handling
	class IFamilyEventSink : public IEventSinkBase
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

	//Device event handling
	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions);
	virtual void onRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	//End Device event handling

	DeviceFamily(Obj* baseLib, IFamilyEventSink* eventHandler);
	virtual ~DeviceFamily();

	virtual DeviceFamilies getFamily() { return _family; }
	virtual std::shared_ptr<RPC::RPCVariable> listBidcosInterfaces() { return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid)); }
	virtual std::shared_ptr<Systems::PhysicalDevice> createPhysicalDevice(std::shared_ptr<Systems::PhysicalDeviceSettings> settings) { return std::shared_ptr<Systems::PhysicalDevice>(); }
	virtual void load() = 0;
	virtual void save(bool full);
	virtual void add(std::shared_ptr<LogicalDevice> device);
	virtual void remove(uint64_t id);
	virtual void dispose();
	virtual std::shared_ptr<LogicalDevice> get(int32_t address);
	virtual std::shared_ptr<LogicalDevice> get(uint64_t id);
	virtual std::shared_ptr<LogicalDevice> get(std::string serialNumber);
	virtual std::vector<std::shared_ptr<LogicalDevice>> getDevices();
	virtual std::shared_ptr<Central> getCentral() = 0;
	virtual std::string getName() = 0;
	virtual std::string handleCLICommand(std::string& command) = 0;
	virtual bool deviceSelected() { return (bool)_currentDevice; }
protected:
	DeviceFamilies _family = DeviceFamilies::none;
	std::mutex _devicesMutex;
	std::thread _removeThread;
	std::vector<std::shared_ptr<LogicalDevice>> _devices;
	std::shared_ptr<BaseLib::Systems::LogicalDevice> _currentDevice;

	void removeThread(uint64_t id);
};

}
}
#endif
