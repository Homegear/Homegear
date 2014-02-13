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

#include "Devices.h"
#include "../GD/GD.h"

namespace RPC
{

Devices::Devices()
{
}

void Devices::load()
{
	try
	{
		DIR* directory;
		struct dirent* entry;
		int32_t i = -1;
		std::string deviceDir(GD::configPath + "Device types");
		if((directory = opendir(deviceDir.c_str())) != 0)
		{
			while((entry = readdir(directory)) != 0)
			{
				if(entry->d_type == 8)
				{
					try
					{
						if(i == -1) i = 0;
						Output::printDebug("Loading XML RPC device " + deviceDir + "/" + entry->d_name);
						std::shared_ptr<Device> device(new Device(deviceDir + "/" + entry->d_name));
						if(device && device->loaded()) _devices.push_back(device);
						i++;
					}
					catch(const std::exception& ex)
					{
						Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
					}
					catch(Exception& ex)
					{
						Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
					}
					catch(...)
					{
						Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
					}
				}
			}
		}
		else throw(Exception("Could not open directory \"" + GD::configPath + "Device types\"."));
		if(i == -1) throw(Exception("No xml files found in \"" + GD::configPath + "Device types\"."));
		if(i == 0) throw(Exception("Could not open any xml files in \"" + GD::configPath + "Device types\"."));
	}
    catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	exit(3);
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Device> Devices::find(LogicalDeviceType deviceType, uint32_t firmwareVersion, std::shared_ptr<Packet> packet)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		int32_t countFromSysinfo = 0;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(deviceType, firmwareVersion))
				{
					countFromSysinfo = (*i)->getCountFromSysinfo(packet);
					if((*i)->getCountFromSysinfo() != countFromSysinfo)
					{
						//Only set partial match when device's countFromSysinfo is not set
						//Otherwise if (*i)->countFromSysinfo is larger than countFromSysinfo,
						//the channel count for the new device will be too large, becuase
						//setCountFromSysinfo cannot reduce the amount of initialized channels.
						if((*i)->getCountFromSysinfo() == -1) partialMatch = *i;
					}
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

std::shared_ptr<Device> Devices::find(LogicalDeviceType deviceType, uint32_t firmwareVersion, int32_t countFromSysinfo)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(deviceType, firmwareVersion))
				{
					if((*i)->countFromSysinfoIndex > -1 && countFromSysinfo < 0) continue; //Ignore device, because countFromSysinfo is mandatory
					if((*i)->countFromSysinfoIndex > -1 && (*i)->getCountFromSysinfo() != countFromSysinfo)
					{
						if((*i)->getCountFromSysinfo() == -1) partialMatch = *i;
					}
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
     Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
     Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
     Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

std::shared_ptr<Device> Devices::find(DeviceFamilies family, std::string typeID, std::shared_ptr<Packet> packet)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		int32_t countFromSysinfo = 0;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(family, typeID))
				{
					countFromSysinfo = (*i)->getCountFromSysinfo(packet);
					if((*i)->getCountFromSysinfo() != countFromSysinfo)
					{
						if((*i)->getCountFromSysinfo() == -1) partialMatch = *i;
					}
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

std::shared_ptr<Device> Devices::find(DeviceFamilies family, std::shared_ptr<Packet> packet)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		int32_t countFromSysinfo = 0;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(family, packet))
				{
					countFromSysinfo = (*i)->getCountFromSysinfo(packet);
					if((*i)->getCountFromSysinfo() != countFromSysinfo)
					{
						if((*i)->getCountFromSysinfo() == -1) partialMatch = *i;
					}
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

} /* namespace XMLRPC */
