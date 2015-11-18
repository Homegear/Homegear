/* Copyright 2013-2015 Sathya Laufer
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

#ifndef MISCELLANEOUS_H_
#define MISCELLANEOUS_H_

#include "homegear-base/BaseLib.h"

using namespace BaseLib;

namespace Misc
{
class MiscDevice;
class MiscCentral;

class Miscellaneous : public BaseLib::Systems::DeviceFamily
{
public:
	Miscellaneous(BaseLib::Obj* bl, BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler);
	virtual ~Miscellaneous();
	virtual bool init();
	virtual void dispose();

	virtual void load();
	virtual std::shared_ptr<MiscDevice> getDevice(uint32_t address);
	virtual std::shared_ptr<MiscDevice> getDevice(std::string serialNumber);
	virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
	virtual std::string handleCLICommand(std::string& command);
	virtual bool skipFamilyCLI() { return true; }
	virtual bool hasPhysicalInterface() { return false; }
	virtual PVariable getPairingMethods();
private:
	std::shared_ptr<MiscCentral> _central;

	void createCentral();
	void createSpyDevice();
	uint32_t getUniqueAddress(uint32_t seed);
	std::string getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber);
};

}

#endif
