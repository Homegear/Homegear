/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef FAMILYCONTROLLER_H_
#define FAMILYCONTROLLER_H_

#include <homegear-base/BaseLib.h>

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
	void dispose();
	int32_t getFamilyId();
	std::string getFamilyName();
	std::string getVersion();

	std::unique_ptr<BaseLib::Systems::DeviceFamily> createModule(BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler);
private:
	bool _disposing = false;
	std::string _name;
	int32_t _familyId = -1;
	std::string _familyName;
	std::string _version;
	void* _handle = nullptr;
	std::unique_ptr<BaseLib::Systems::SystemFactory> _factory;

	ModuleLoader(const ModuleLoader&);
	ModuleLoader& operator=(const ModuleLoader&);
};

class FamilyController : public BaseLib::Systems::DeviceFamily::IFamilyEventSink
{
public:
	struct ModuleInfo
	{
		std::string filename;
		std::string baselibVersion;
		int32_t familyId;
		std::string familyName;
		bool loaded;
	};

	// {{{ Family event handling
		//Hooks
		virtual void onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, BaseLib::PEventHandler>& eventHandlers);
		virtual void onRemoveWebserverEventHandler(std::map<int32_t, BaseLib::PEventHandler>& eventHandlers);

		virtual void onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<BaseLib::PVariable>> values);
		virtual void onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint);
		virtual void onRPCNewDevices(BaseLib::PVariable deviceDescriptions);
		virtual void onRPCDeleteDevices(BaseLib::PVariable deviceAddresses, BaseLib::PVariable deviceInfo);
		virtual void onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values);
		virtual void onRunScript(BaseLib::ScriptEngine::PScriptInfo& scriptInfo, bool wait);
		virtual int32_t onIsAddonClient(int32_t clientID);
		virtual int32_t onCheckLicense(int32_t moduleId, int32_t familyId, int32_t deviceId, const std::string& licenseKey);

		//Device description
		virtual void onDecryptDeviceDescription(int32_t moduleId, const std::vector<char>& input, std::vector<char>& output);
	// }}}

	FamilyController();
	virtual ~FamilyController();
	void disposeDeviceFamilies();
	void dispose();
	bool lifetick();

	/**
	 * Returns a vector of type ModuleInfo with information about all loaded and not loaded modules.
	 * @return Returns a vector of type ModuleInfo.
	 */
	std::vector<std::shared_ptr<ModuleInfo>> getModuleInfo();

	/**
	 * Loads a family module. The module needs to be in Homegear's module path.
	 * @param filename The filename of the module (e. g. mod_miscellanous.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Modules successfully loaded, 1: Module already loaded, -1: System error, -2: Module does not exists, -4: Family initialization failed
	 */
	int32_t loadModule(std::string filename);

	/**
	 * Unloads a previously loaded family module.
	 * @param filename The filename of the module (e. g. mod_miscellanous.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Modules successfully loaded, 1: Module not loaded, -1: System error, -2: Module does not exists
	 */
	int32_t unloadModule(std::string filename);

	/**
	 * Unloads and loads a family module again. The module needs to be in Homegear's module path.
	 *
	 * @param filename The filename of the module (e. g. mod_miscellanous.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Modules successfully loaded, -1: System error, -2: Module does not exists, -4: Family initialization failed
	 */
	int32_t reloadModule(std::string filename);

	void loadModules();
	void load();
	void save(bool full);
	bool familySelected() { return (bool)_currentFamily; }
	std::string handleCliCommand(std::string& command);
	bool familyAvailable(int32_t family);

	/*
	 * Returns the family map.
	 */
	std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> getFamilies();

	/*
	 * Returns the device family specified by familyId.
	 */
	std::shared_ptr<BaseLib::Systems::DeviceFamily> getFamily(int32_t familyId);

	/*
	 * Checks if the peer with the provided id exists.
	 */
	bool peerExists(uint64_t peerId);

	/*
     * Executed when Homegear is fully started.
     */
	void homegearStarted();

	/*
     * Executed before Homegear starts shutting down.
     */
	void homegearShuttingDown();

	// {{{ Physical interfaces
	uint32_t physicalInterfaceCount(int32_t family);
	void physicalInterfaceClear(int32_t family);
	void physicalInterfaceStopListening();
	void physicalInterfaceStartListening();
	bool physicalInterfaceIsOpen();
	void physicalInterfaceSetup(int32_t userID, int32_t groupID, bool setPermissions);
	BaseLib::PVariable listInterfaces(int32_t familyID);
	// }}}

	BaseLib::PVariable listFamilies();
private:
	bool _disposed = false;
	BaseLib::PVariable _rpcCache;

	std::set<int32_t> _familiesWithoutPhysicalInterface;
	std::mutex _moduleLoadersMutex;
	std::map<std::string, std::unique_ptr<ModuleLoader>> _moduleLoaders;
	std::map<int32_t, std::string> _moduleFilenames;

	std::mutex _familiesMutex;
	std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> _families;
	std::shared_ptr<BaseLib::Systems::DeviceFamily> _currentFamily;

	FamilyController(const FamilyController&);
	FamilyController& operator=(const FamilyController&);
};

#endif
