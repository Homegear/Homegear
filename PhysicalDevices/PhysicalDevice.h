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

#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "../Exception.h"
#include "../HomeMaticDevice.h"

namespace PhysicalDevices
{

class PhysicalDevice
{
public:
	PhysicalDevice();

	virtual void init(std::string physicalDevice) {}

	virtual ~PhysicalDevice();

	static std::shared_ptr<PhysicalDevice> create(std::string deviceType);


	virtual void startListening() {}
	virtual void stopListening() {}
	virtual void addHomeMaticDevice(HomeMaticDevice*);
	virtual void removeHomeMaticDevice(HomeMaticDevice*);
	virtual void sendPacket(std::shared_ptr<BidCoSPacket> packet) {}
	virtual bool isOpen() { return false; }
protected:
	std::mutex _homeMaticDevicesMutex;
    std::list<HomeMaticDevice*> _homeMaticDevices;
    std::string _rfDevice;
	std::thread _listenThread;
	std::thread _callbackThread;
	bool _stopCallbackThread;
	std::string _lockfile;
	std::mutex _sendMutex;
	bool _stopped = false;

	virtual void callCallback(std::shared_ptr<BidCoSPacket> packet);
};

}
#endif /* PHYSICALDEVICE_H_ */
