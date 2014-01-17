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

#include "PhysicalDevices.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace PhysicalDevices
{
PhysicalDevices::PhysicalDevices()
{

}

void PhysicalDevices::reset()
{
	_physicalDevicesMutex.lock();
	_physicalDevices.clear();
	_physicalDevicesMutex.unlock();
}

void PhysicalDevices::load(std::string filename)
{
	try
	{
		reset();
		int32_t index = 0;
		char input[1024];
		FILE *fin;
		int32_t len, ptr;
		bool found = false;

		if (!(fin = fopen(filename.c_str(), "r")))
		{
			HelperFunctions::printError("Unable to open physical device config file: " + filename + ". " + strerror(errno));
			return;
		}

		std::shared_ptr<PhysicalDeviceSettings> settings(new PhysicalDeviceSettings());
		while (fgets(input, 1024, fin))
		{
			if(input[0] == '#') continue;
			len = strlen(input);
			if (len < 2) continue;
			if (input[len-1] == '\n') input[len-1] = '\0';
			ptr = 0;
			if(input[0] == '[')
			{
				while(ptr < len)
				{
					if (input[ptr] == ']')
					{
						input[ptr] = '\0';
						if(!settings->device.empty() && !settings->type.empty() && settings->family != DeviceFamily::none)
						{
							std::shared_ptr<PhysicalDevice> device = PhysicalDevice::create(settings);
							_physicalDevicesMutex.lock();
							if(device) _physicalDevices[settings->family] = device;
							_physicalDevicesMutex.unlock();
						}
						settings.reset(new PhysicalDeviceSettings());
						std::string name(&input[1]);
						HelperFunctions::toLower(name);
						if(name == "homematicbidcos") settings->family = DeviceFamily::HomeMaticBidCoS;
						else if(name == "homematicwired") settings->family = DeviceFamily::HomeMaticWired;
						HelperFunctions::printDebug("Debug: Reading config for physical device family " + DeviceFamilies::getName(settings->family));
						break;
					}
					ptr++;
				}
				continue;
			}
			found = false;
			while(ptr < len)
			{
				if (input[ptr] == '=')
				{
					found = true;
					input[ptr++] = '\0';
					break;
				}
				ptr++;
			}
			if(found)
			{
				std::string name(input);
				HelperFunctions::toLower(name);
				HelperFunctions::trim(name);
				std::string value(&input[ptr]);
				HelperFunctions::trim(value);

				if(name == "devicetype")
				{
					HelperFunctions::toLower(value);
					settings->type = value;
					HelperFunctions::printDebug("Debug: deviceType of family " + DeviceFamilies::getName(settings->family) + " set to " + settings->type);
				}
				else if(name == "device")
				{
					settings->device = value;
					HelperFunctions::printDebug("Debug: device of family " + DeviceFamilies::getName(settings->family) + " set to " + settings->device);
				}
				else if(name == "responsedelay")
				{
					settings->responseDelay = HelperFunctions::getNumber(value);
					if(settings->responseDelay > 10000) settings->responseDelay = 10000;
					HelperFunctions::printDebug("Debug: responseDelay of family " + DeviceFamilies::getName(settings->family) + " set to " + std::to_string(settings->responseDelay));
				}
				else if(name == "gpio1")
				{
					int32_t number = HelperFunctions::getNumber(value);
					if(number > 0)
					{
						settings->gpio[1].number = number;
						HelperFunctions::printDebug("Debug: GPIO1 of family " + DeviceFamilies::getName(settings->family) + " set to " + std::to_string(settings->gpio[1].number));
					}
				}
				else if(name == "gpio2")
				{
					int32_t number = HelperFunctions::getNumber(value);
					if(number > 0)
					{
						settings->gpio[2].number = number;
						HelperFunctions::printDebug("Debug: GPIO2 of family " + DeviceFamilies::getName(settings->family) + " set to " + std::to_string(settings->gpio[2].number));
					}
				}
				else if(name == "gpio3")
				{
					int32_t number = HelperFunctions::getNumber(value);
					if(number > 0)
					{
						settings->gpio[3].number = number;
						HelperFunctions::printDebug("Debug: GPIO3 of family " + DeviceFamilies::getName(settings->family) + " set to " + std::to_string(settings->gpio[3].number));
					}
				}
				else
				{
					HelperFunctions::printWarning("Warning: Unknown physical device setting: " + std::string(input));
				}
			}
		}
		if(!settings->device.empty() && !settings->type.empty() && settings->family != DeviceFamily::none)
		{
			std::shared_ptr<PhysicalDevice> device = PhysicalDevice::create(settings);
			_physicalDevicesMutex.lock();
			if(device) _physicalDevices[settings->family] = device;
			_physicalDevicesMutex.unlock();
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
		_physicalDevicesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_physicalDevicesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_physicalDevicesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<PhysicalDevice> PhysicalDevices::get(DeviceFamily family)
{
	std::shared_ptr<PhysicalDevice> device;
	try
	{
		_physicalDevicesMutex.lock();
		if(_physicalDevices.find(family) != _physicalDevices.end()) device = _physicalDevices[family];
		else device = std::shared_ptr<PhysicalDevice>(new PhysicalDevice());
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
    _physicalDevicesMutex.unlock();
    return device;
}

uint32_t PhysicalDevices::count()
{
	uint32_t size = 0;
	try
	{
		_physicalDevicesMutex.lock();
		size = _physicalDevices.size();
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
    _physicalDevicesMutex.unlock();
    return size;
}

bool PhysicalDevices::isOpen()
{
	try
	{
		if(_physicalDevices.empty()) return false;
		_physicalDevicesMutex.lock();
		for(std::map<DeviceFamily, std::shared_ptr<PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			if(!i->second->isOpen())
			{
				_physicalDevicesMutex.unlock();
				return false;
			}
		}
		_physicalDevicesMutex.unlock();
		return true;
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
    _physicalDevicesMutex.unlock();
	return false;
}

void PhysicalDevices::startListening()
{
	try
	{
		_physicalDevicesMutex.lock();
		for(std::map<DeviceFamily, std::shared_ptr<PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			i->second->startListening();
		}
		_physicalDevicesMutex.unlock();
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
}

void PhysicalDevices::stopListening()
{
	try
	{
		_physicalDevicesMutex.lock();
		for(std::map<DeviceFamily, std::shared_ptr<PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			i->second->stopListening();
		}
		_physicalDevicesMutex.unlock();
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
}

void PhysicalDevices::setup(int32_t userID, int32_t groupID)
{
	try
	{
		_physicalDevicesMutex.lock();
		for(std::map<DeviceFamily, std::shared_ptr<PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			i->second->setup(userID, groupID);
		}
		_physicalDevicesMutex.unlock();
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
}
} /* namespace PhysicalDevices */
