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

#ifndef PHYSICALINTERFACESETTINGS_H_
#define PHYSICALINTERFACESETTINGS_H_

#include "../Systems/DeviceFamilies.h"

#include <iostream>
#include <string>
#include <map>

namespace BaseLib
{
namespace Systems
{

class GPIOSetting
{
public:
	GPIOSetting() {}
	virtual ~GPIOSetting() {}

	int32_t number = -1;
	std::string path;
};

class PhysicalInterfaceSettings
{
public:
	PhysicalInterfaceSettings() {}
	virtual ~PhysicalInterfaceSettings() {}
	std::string id;
	bool isDefault = false;
	DeviceFamilies family = DeviceFamilies::none;
	std::string device;
	std::string type;
	uint32_t responseDelay = 95;
	std::map<uint32_t, GPIOSetting> gpio;
	int32_t oscillatorFrequency = -1;
	int32_t txPowerSetting = -1;
	int32_t interruptPin = -1;
	uint32_t stackPosition = 0;
	std::string host;
	std::string port;
	std::string portKeepAlive;
	std::string oldRFKey;
	std::string rfKey;
	std::string lanKey;
	uint32_t currentRFKeyIndex = 0;
	bool ssl = false;
	std::string caFile;
	bool verifyCertificate = true;
	bool oneWay = false;
	bool fastSending = false;
	uint32_t timeout = 10;
	uint32_t waitForBus = 100;
	int32_t enableRXValue = -1;
	int32_t enableTXValue = -1;
	int32_t listenThreadPriority = -1;
	int32_t listenThreadPolicy = SCHED_OTHER;
	std::string ttsProgram;
	std::string dataPath;
};

}
}
#endif
