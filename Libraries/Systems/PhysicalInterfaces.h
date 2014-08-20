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

#ifndef PHYSICALINTERFACES_H_
#define PHYSICALINTERFACES_H_

#include "../../Modules/Base/BaseLib.h"

#include <memory>
#include <mutex>
#include <iostream>
#include <string>
#include <map>
#include <cstring>
#include <vector>

class PhysicalInterfaces
{
public:
	PhysicalInterfaces();
	virtual ~PhysicalInterfaces() { dispose(); }
	virtual void dispose();
	void load(std::string filename);

	uint32_t count();
	uint32_t count(BaseLib::Systems::DeviceFamilies family);
	std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>> get(BaseLib::Systems::DeviceFamilies family);
	void clear(BaseLib::Systems::DeviceFamilies family);
	void stopListening();
	void startListening();
	bool isOpen();
	void setup(int32_t userID, int32_t groupID);
	std::shared_ptr<BaseLib::RPC::RPCVariable> listInterfaces(int32_t familyID);
private:
	bool _disposing = false;
	std::mutex _physicalInterfacesMutex;
	std::map<BaseLib::Systems::DeviceFamilies, std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>> _physicalInterfaces;

	void reset();
};

#endif /* PHYSICALDEVICES_H_ */
