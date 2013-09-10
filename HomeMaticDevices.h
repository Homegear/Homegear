/* Copyright 2013 Sathya Laufer
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

class HomeMaticDevice;
class HomeMaticCentral;

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include "Devices/HomeMaticCentral.h"

class HomeMaticDevices {
public:
	HomeMaticDevices();
	virtual ~HomeMaticDevices();
	void add(HomeMaticDevice* device);
	void remove(int32_t address);
	std::shared_ptr<HomeMaticDevice> get(int32_t address);
	std::shared_ptr<HomeMaticDevice> get(std::string serialNumber);
	std::shared_ptr<HomeMaticCentral> getCentral();
	std::vector<std::shared_ptr<HomeMaticDevice>> getDevices();
	void convertDatabase();
	void load();
	void save(bool full, bool crash = false);
	void dispose();
	bool deviceSelected() { return (bool)_currentDevice; }
	std::string handleCLICommand(std::string& command);

private:
	std::shared_ptr<HomeMaticDevice> _currentDevice;
	std::mutex _devicesMutex;
	std::vector<std::shared_ptr<HomeMaticDevice>> _devices;
	std::shared_ptr<HomeMaticCentral> _central;
	std::thread _removeThread;

	void initializeDatabase();
	void loadDevicesFromDatabase_0_0_6();
	void loadDevicesFromDatabase();
	int32_t getUniqueAddress(uint8_t firstByte);
	std::string getUniqueSerialNumber(std::string seedPrefix);
	void createCentral();
	void createSpyDevice();
	void removeThread(int32_t address);
};

#endif /* HOMEMATICDEVICES_H_ */
