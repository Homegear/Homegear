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

#include "../../Database/Database.h"
#include "../../LogicalDevices/Central.h"
#include "../../LogicalDevices/LogicalDevice.h"
#include "../../PhysicalDevices/PhysicalDeviceSettings.h"
#include "../../PhysicalDevices/PhysicalDevice.h"
#include "DeviceFamilies.h"
#include "../../Types/RPCVariable.h"

#include <iostream>
#include <string>
#include <memory>

class DeviceFamily
{
public:
	DeviceFamily() {}
	virtual ~DeviceFamily();

	virtual std::shared_ptr<RPC::RPCVariable> listBidcosInterfaces() { return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid)); }
	virtual std::shared_ptr<PhysicalDevices::PhysicalDevice> createPhysicalDevice(std::shared_ptr<PhysicalDevices::PhysicalDeviceSettings> settings) { return std::shared_ptr<PhysicalDevices::PhysicalDevice>(); }
	virtual void load(bool version_0_0_7) {}
	virtual void save(bool full);
	virtual void add(std::shared_ptr<LogicalDevice> device);
	virtual void remove(int32_t address);
	virtual void dispose();
	virtual std::shared_ptr<LogicalDevice> get(int32_t address);
	virtual std::shared_ptr<LogicalDevice> get(std::string serialNumber);
	virtual std::vector<std::shared_ptr<LogicalDevice>> getDevices();
	virtual std::shared_ptr<Central> getCentral() { return std::shared_ptr<Central>(); }
	virtual std::string handleCLICommand(std::string& command) { return ""; }
	virtual bool deviceSelected() { return false; }
protected:
	std::mutex _devicesMutex;
	std::thread _removeThread;
	std::vector<std::shared_ptr<LogicalDevice>> _devices;

	void removeThread(int32_t address);
};

#endif
