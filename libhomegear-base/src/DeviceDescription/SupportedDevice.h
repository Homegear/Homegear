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

#ifndef SUPPORTEDDEVICE_H_
#define SUPPORTEDDEVICE_H_

#include "../Systems/DeviceFamilies.h"
#include "../Systems/DeviceTypes.h"
#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>
#include <memory>
#include <vector>

using namespace rapidxml;

namespace BaseLib
{

class Obj;

namespace DeviceDescription
{

class HomegearDevice;
class SupportedDevice;

typedef std::shared_ptr<SupportedDevice> PSupportedDevice;
typedef std::vector<PSupportedDevice> SupportedDevices;

class SupportedDevice
{
public:
	SupportedDevice(BaseLib::Obj* baseLib, HomegearDevice* device);
	SupportedDevice(BaseLib::Obj* baseLib, xml_node<>* node, HomegearDevice* device);
	virtual ~SupportedDevice() {}

	std::string id;
	std::string description;
	uint32_t typeNumber = 0;
	uint32_t minFirmwareVersion = 0;
	uint32_t maxFirmwareVersion = 0;

	bool matches(Systems::DeviceFamilies family, std::string typeId);
	bool matches(Systems::LogicalDeviceType deviceType, uint32_t firmwareVersion);

	/**
	 * Checks if a firmware version matches this device description.
	 *
	 * @param version The firmware version to check or "-1".
	 * @return Returns true if the firmware version is within minFirmwareVersion and maxFirmwareVersion or if it is -1.
	 */
	bool checkFirmwareVersion(int32_t version);
protected:
	BaseLib::Obj* _bl = nullptr;
	HomegearDevice* _device = nullptr;
};
}
}

#endif
