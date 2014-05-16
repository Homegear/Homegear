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

#ifndef FAMILYCONTROLLER_H_
#define FAMILYCONTROLLER_H_

#include "../../Modules/Base/BaseLib.h"

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

class ModuleLoader
{
public:
	ModuleLoader(std::string name, std::string path);
	virtual ~ModuleLoader();

	std::unique_ptr<BaseLib::Systems::DeviceFamily> createModule(BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler);
private:
	std::string _name;
	void* _handle = nullptr;
	std::unique_ptr<BaseLib::Systems::SystemFactory> _factory;
};

class FamilyController : public BaseLib::Systems::DeviceFamily::IFamilyEventSink
{
public:
	//Family event handling
	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onRPCNewDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions);
	virtual void onRPCDeleteDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceAddresses, std::shared_ptr<BaseLib::RPC::RPCVariable> deviceInfo);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values);
	//End Family event handling

	FamilyController();
	virtual ~FamilyController();
	void convertDatabase();
	void loadModules();
	void load();
	void save(bool full, bool crash = false);
	void dispose();
	bool familySelected() { return (bool)_currentFamily; }
	std::string handleCLICommand(std::string& command);
	bool familyAvailable(BaseLib::Systems::DeviceFamilies family);
private:
	std::map<std::string, std::unique_ptr<ModuleLoader>> moduleLoaders;
	BaseLib::Systems::DeviceFamily* _currentFamily;

	void initializeDatabase();
	void loadDevicesFromDatabase(bool version_0_0_7);
};

#endif
