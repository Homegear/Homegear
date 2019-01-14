/* Copyright 2013-2019 Homegear GmbH
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

#include "SharedObjectFamilyModules.h"

namespace Homegear
{

class FamilyController
{
public:
	FamilyController();

	virtual ~FamilyController();

	void disposeDeviceFamilies();

	void dispose();

	bool lifetick();

	/**
	 * Returns a vector of type ModuleInfo with information about all loaded and not loaded modules.
	 * @return Returns a vector of type ModuleInfo.
	 */
	std::vector<std::shared_ptr<FamilyModuleInfo>> getModuleInfo();

	/**
	 * Loads a shared object family module. The module needs to be in Homegear's module path.
	 * @param filename The filename of the module (e. g. mod_miscellanous.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Modules successfully loaded, 1: Module already loaded, -1: System error, -2: Module does not exists, -3: Family with same ID already exists, -4: Family initialization failed
	 */
	int32_t loadModule(const std::string& filename);

	/**
	 * Unloads a previously loaded shared object family module.
	 * @param filename The filename of the module (e. g. mod_miscellanous.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Modules successfully loaded, 1: Module not loaded, -1: System error, -2: Module does not exists
	 */
	int32_t unloadModule(const std::string& filename);

	/**
	 * Unloads and loads a shared object family module again. The module needs to be in Homegear's module path.
	 *
	 * @param filename The filename of the module (e. g. mod_miscellanous.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Modules successfully loaded, -1: System error, -2: Module does not exists, -4: Family initialization failed
	 */
	int32_t reloadModule(const std::string& filename);

	void init();

	void loadModules();

	void load();

	void save(bool full);

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

	void onEvent(std::string source, uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values);

	void onRPCEvent(std::string source, uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<BaseLib::PVariable>> values);

	// {{{ Physical interfaces
	uint32_t physicalInterfaceCount(int32_t family);

	void physicalInterfaceStopListening();

	void physicalInterfaceStartListening();

	bool physicalInterfaceIsOpen();

	void physicalInterfaceSetup(int32_t userId, int32_t groupId, bool setPermissions);

	BaseLib::PVariable listInterfaces(int32_t familyId);
	// }}}

	BaseLib::PVariable listFamilies(int32_t familyId);

private:
	SharedObjectFamilyModules _sharedObjectFamilyModules;

	FamilyController(const FamilyController&);

	FamilyController& operator=(const FamilyController&);
};

}

#endif
