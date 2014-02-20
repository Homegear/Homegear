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

#ifndef HMWIRED_H_
#define HMWIRED_H_

#include "../../Database/Database.h"
#include "../../HelperFunctions/HelperFunctions.h"
#include "../General/DeviceFamily.h"

namespace HMWired
{
class HMWiredDevice;
class HMWiredCentral;

class HMWired : public DeviceFamily
{
public:
	HMWired();
	virtual ~HMWired();

	virtual std::shared_ptr<PhysicalDevices::PhysicalDevice> createPhysicalDevice(std::shared_ptr<PhysicalDevices::PhysicalDeviceSettings> settings);
	virtual void load(bool version_0_0_7);
	virtual std::shared_ptr<HMWiredDevice> getDevice(uint32_t address);
	virtual std::shared_ptr<HMWiredDevice> getDevice(std::string serialNumber);
	virtual std::shared_ptr<Central> getCentral();
	virtual std::string handleCLICommand(std::string& command);
	virtual bool deviceSelected() { return (bool)_currentDevice; }
	virtual std::string getName() { return "HomeMatic Wired"; }
private:
	std::shared_ptr<LogicalDevice> _currentDevice;
	std::shared_ptr<HMWiredCentral> _central;

	void createCentral();
	void createSpyDevice();
	uint32_t getUniqueAddress(uint32_t seed);
	std::string getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber);
};

} /* namespace HMWired */

#endif /* HMWIRED_H_ */
