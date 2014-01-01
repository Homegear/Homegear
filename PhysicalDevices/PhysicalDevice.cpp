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

#include "PhysicalDevice.h"
#include "../GD.h"
#include "../HelperFunctions.h"
#include "Cul.h"
#include "TICC1100.h"
#include "CRC_RS485.h"

namespace PhysicalDevices
{

PhysicalDevice::PhysicalDevice()
{

}

PhysicalDevice::~PhysicalDevice()
{

}

std::shared_ptr<PhysicalDevice> PhysicalDevice::create(std::string deviceType)
{
	try
	{
		if(deviceType == "cul")
		{
			return std::shared_ptr<PhysicalDevice>(new Cul());
		}
		else if(deviceType == "cc1100")
		{
			return std::shared_ptr<PhysicalDevice>(new TICC1100());
		}
		else if(deviceType == "crc_rs485")
		{
			return std::shared_ptr<PhysicalDevice>(new CRCRS485());
		}
		else
		{
			HelperFunctions::printError("Error: Unsupported physical device type: " + deviceType);
		}
	}
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<PhysicalDevice>();
}

void PhysicalDevice::addHomeMaticDevice(HomeMaticDevice* device)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		_homeMaticDevices.push_back(device);
    }
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _homeMaticDevicesMutex.unlock();
}

void PhysicalDevice::removeHomeMaticDevice(HomeMaticDevice* device)
{
	try
	{
		_homeMaticDevicesMutex.lock();
		_homeMaticDevices.remove(device);
    }
    catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _homeMaticDevicesMutex.unlock();
}

void PhysicalDevice::callCallback(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::vector<HomeMaticDevice*> devices;
		_homeMaticDevicesMutex.lock();
		//We need to copy all elements. In packetReceived so much can happen, that _homeMaticDevicesMutex might deadlock
		for(std::list<HomeMaticDevice*>::const_iterator i = _homeMaticDevices.begin(); i != _homeMaticDevices.end(); ++i)
		{
			devices.push_back(*i);
		}
		_homeMaticDevicesMutex.unlock();
		for(std::vector<HomeMaticDevice*>::iterator i = devices.begin(); i != devices.end(); ++i)
		{
			(*i)->packetReceived(packet);
		}
	}
    catch(const std::exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_homeMaticDevicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_homeMaticDevicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

}
