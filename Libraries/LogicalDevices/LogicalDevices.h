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

#ifndef HOMEMATICDEVICES_H_
#define HOMEMATICDEVICES_H_

class LogicalDevice;

#include "../HelperFunctions/HelperFunctions.h"
#include "../Systems/General/DeviceFamily.h"

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

class UpdateInfo
{
public:
	UpdateInfo();
	virtual ~UpdateInfo();
	void reset();

	std::mutex updateMutex;
	int32_t devicesToUpdate = -1;
	int32_t currentUpdate = -1;
	uint64_t currentDevice = 0;
	int32_t currentDeviceProgress = -1;

	std::map<uint64_t, std::pair<int32_t, std::string>> results;
};

class LogicalDevices
{
public:
	UpdateInfo updateInfo;

	LogicalDevices();
	virtual ~LogicalDevices();
	void convertDatabase();
	void load();
	void save(bool full, bool crash = false);
	void dispose();
	bool familySelected() { return (bool)_currentFamily; }
	std::string handleCLICommand(std::string& command);
private:
	std::shared_ptr<DeviceFamily> _currentFamily;

	void initializeDatabase();
	void loadDevicesFromDatabase(bool version_0_0_7);
};

#endif /* HOMEMATICDEVICES_H_ */
