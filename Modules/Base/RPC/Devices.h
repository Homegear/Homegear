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

#ifndef DEVICES_H_
#define DEVICES_H_

#include <vector>
#include <memory>

#include "../Systems/Packet.h"
#include "Device.h"

namespace BaseLib
{
namespace RPC
{

class Devices
{
public:
	Devices();
	virtual ~Devices() {}
	virtual void load(std::string path);
	virtual std::shared_ptr<Device> find(Systems::DeviceFamilies family, std::shared_ptr<Systems::Packet> packet);
	virtual std::shared_ptr<Device> find(Systems::DeviceFamilies family, std::string typeID, std::shared_ptr<Systems::Packet> packet = std::shared_ptr<Systems::Packet>());
	virtual std::shared_ptr<Device> find(Systems::LogicalDeviceType deviceType, uint32_t firmwareVersion, std::shared_ptr<Systems::Packet> packet = std::shared_ptr<Systems::Packet>());
	virtual std::shared_ptr<Device> find(Systems::LogicalDeviceType deviceType, uint32_t firmwareVersion, int32_t countFromSysinfo);
protected:
	std::vector<std::shared_ptr<Device>> _devices;
};

}
}
#endif /* DEVICES_H_ */
