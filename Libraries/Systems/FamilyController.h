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

#ifndef FAMILYCONTROLLER_H_
#define FAMILYCONTROLLER_H_

#include "../../Modules/Base/BaseLib.h"

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include <dlfcn.h>

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

	ModuleLoader(const ModuleLoader&);
	ModuleLoader& operator=(const ModuleLoader&);
};

class FamilyController : public BaseLib::Systems::DeviceFamily::IFamilyEventSink
{
public:
	//Family event handling

	//Database
		//General
		virtual void onCreateSavepointSynchronous(std::string name);
		virtual void onReleaseSavepointSynchronous(std::string name);
		virtual void onCreateSavepointAsynchronous(std::string name);
		virtual void onReleaseSavepointAsynchronous(std::string name);

		//Metadata
		virtual void onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID = "");

		//Device
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetDevices(uint32_t family);
		virtual void onDeleteDevice(uint64_t deviceID);
		virtual uint64_t onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family);
		virtual void onSaveDeviceVariable(BaseLib::Database::DataRow& data);
		virtual void onDeletePeers(int32_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeers(uint64_t deviceID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetDeviceVariables(uint64_t deviceID);

		//Peer
		virtual void onDeletePeer(uint64_t id);
		virtual uint64_t onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber);
		virtual void onSavePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual void onSavePeerVariable(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeerParameters(uint64_t peerID);
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetPeerVariables(uint64_t peerID);
		virtual void onDeletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual bool onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID);

		//Service messages
		virtual std::shared_ptr<BaseLib::Database::DataTable> onGetServiceMessages(uint64_t peerID);
		virtual void onSaveServiceMessage(uint64_t peerID, BaseLib::Database::DataRow& data);
		virtual void onDeleteServiceMessage(uint64_t databaseID);
	//End database

	virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values);
	virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
	virtual void onRPCNewDevices(std::shared_ptr<BaseLib::RPC::Variable> deviceDescriptions);
	virtual void onRPCDeleteDevices(std::shared_ptr<BaseLib::RPC::Variable> deviceAddresses, std::shared_ptr<BaseLib::RPC::Variable> deviceInfo);
	virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values);
	virtual int32_t onIsAddonClient(int32_t clientID);
	//End Family event handling

	FamilyController();
	virtual ~FamilyController();
	void init();
	void dispose();

	static void create();
	static void destroy();
	void loadModules();
	void load();
	void save(bool full);
	bool familySelected() { return (bool)_currentFamily; }
	std::string handleCLICommand(std::string& command);
	bool familyAvailable(BaseLib::Systems::DeviceFamilies family);

	/*
     * Executed before Homegear starts shutting down.
     */
	void homegearShuttingDown();

	std::shared_ptr<BaseLib::RPC::Variable> listFamilies();
private:
	bool _disposed = false;
	std::shared_ptr<BaseLib::RPC::Variable> _rpcCache;

	std::set<BaseLib::Systems::DeviceFamilies> _familiesWithoutPhysicalInterface;
	std::map<std::string, std::unique_ptr<ModuleLoader>> moduleLoaders;
	BaseLib::Systems::DeviceFamily* _currentFamily = nullptr;

	FamilyController(const FamilyController&);
	FamilyController& operator=(const FamilyController&);
};

#endif
