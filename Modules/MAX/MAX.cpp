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

#include "MAX.h"
#include "PhysicalInterfaces/CUL.h"
#include "MAXDeviceTypes.h"
#include "LogicalDevices/MAXSpyDevice.h"
#include "GD.h"

namespace MAX
{

MAX::MAX(BaseLib::Obj* baseLib, IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(baseLib, eventHandler)
{
	BaseLib::Output::setPrefix("Module MAX: ");
	BaseLib::Output::printDebug("Debug: Loading module...");
	_family = BaseLib::Systems::DeviceFamilies::MAX;
}

MAX::~MAX()
{

}

std::shared_ptr<BaseLib::Systems::IPhysicalInterface> MAX::createPhysicalDevice(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings)
{
	try
	{
		if(!settings) return std::shared_ptr<BaseLib::Systems::IPhysicalInterface>();
		BaseLib::Output::printDebug("Debug: Creating physical device. Type defined in physicalinterfaces.conf is: " + settings->type);
		GD::physicalInterface = std::shared_ptr<BaseLib::Systems::IPhysicalInterface>();
		if(!settings) return GD::physicalInterface;
		if(settings->type == "cul") GD::physicalInterface.reset(new CUL(settings));
		else BaseLib::Output::printError("Error: Unsupported physical device type: " + settings->type);
		return GD::physicalInterface;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<BaseLib::Systems::IPhysicalInterface>();
}

uint32_t MAX::getUniqueAddress(uint32_t seed)
{
	uint32_t prefix = seed;
	seed = BaseLib::HelperFunctions::getRandomNumber(1, 16834);
	uint32_t i = 0;
	while(getDevice(prefix + seed) && i++ < 10000)
	{
		seed += 131;
		if(seed > 32767) seed -= 10000;
	}
	return prefix + seed;
}

std::string MAX::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
{
	if(seedPrefix.size() != 3) throw BaseLib::Exception("seedPrefix must have a size of 3.");
	uint32_t i = 0;
	std::ostringstream stringstream;
	stringstream << seedPrefix << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
	std::string temp2 = stringstream.str();
	while((getDevice(temp2)) && i++ < 100000)
	{
		stringstream.str(std::string());
		stringstream.clear();
		seedNumber += 73;
		if(seedNumber > 9999999) seedNumber -= 10000000;
		std::ostringstream stringstream;
		stringstream << seedPrefix << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
		temp2 = stringstream.str();
	}
	return temp2;
}

std::shared_ptr<MAXDevice> MAX::getDevice(uint32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				std::shared_ptr<MAXDevice> device(std::dynamic_pointer_cast<MAXDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
			}
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _devicesMutex.unlock();
	return std::shared_ptr<MAXDevice>();
}

std::shared_ptr<MAXDevice> MAX::getDevice(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<MAXDevice> device(std::dynamic_pointer_cast<MAXDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
			}
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _devicesMutex.unlock();
	return std::shared_ptr<MAXDevice>();
}

void MAX::load()
{
	try
	{
		_devices.clear();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDevices();
		bool spyDeviceExists = false;
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			BaseLib::Output::printMessage("Loading device " + std::to_string(deviceID));
			int32_t address = row->second.at(1)->intValue;
			std::string serialNumber = row->second.at(2)->textValue;
			uint32_t deviceType = row->second.at(3)->intValue;

			std::shared_ptr<BaseLib::Systems::LogicalDevice> device;
			switch((DeviceType)deviceType)
			{
			case DeviceType::MAXSD:
				spyDeviceExists = true;
				device = std::shared_ptr<BaseLib::Systems::LogicalDevice>(new MAXSpyDevice(deviceID, serialNumber, address, this));
				break;
			default:
				break;
			}

			if(device)
			{
				device->load();
				device->loadPeers();
				_devicesMutex.lock();
				_devices.push_back(device);
				_devicesMutex.unlock();
			}
		}
		if(GD::physicalInterface)
		{
			if(!spyDeviceExists) createSpyDevice();
		}
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void MAX::createSpyDevice()
{
	try
	{
		uint32_t seed = 0xfe0000 + BaseLib::HelperFunctions::getRandomNumber(1, 32767);

		int32_t address = getUniqueAddress(seed);
		std::string serialNumber(getUniqueSerialNumber("VMS", BaseLib::HelperFunctions::getRandomNumber(1, 9999999)));

		std::shared_ptr<BaseLib::Systems::LogicalDevice> device(new MAXSpyDevice(0, serialNumber, address, this));
		add(device);
		BaseLib::Output::printMessage("Created spy device with id " + std::to_string(device->getID()) + ", address 0x" + BaseLib::HelperFunctions::getHexString(address, 8) + " and serial number " + serialNumber);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string MAX::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command == "unselect" && _currentDevice && !_currentDevice->peerSelected())
		{
			_currentDevice.reset();
			return "Device unselected.\n";
		}
		else if(command.compare(0, 7, "devices") != 0 && _currentDevice)
		{
			if(!_currentDevice) return "No device selected.\n";
			return _currentDevice->handleCLICommand(command);
		}
		else if(command == "devices help" || command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "devices list\t\tList all MAX devices" << std::endl;
			stringStream << "devices create\t\tCreate a virtual MAX device" << std::endl;
			stringStream << "devices remove\t\tRemove a virtual MAX device" << std::endl;
			stringStream << "devices select\t\tSelect a virtual MAX device" << std::endl;
			stringStream << "unselect\t\tUnselect this device family" << std::endl;
			return stringStream.str();
		}
		else if(command == "devices list")
		{
			std::string bar(" │ ");
			const int32_t idWidth = 8;
			const int32_t addressWidth = 7;
			const int32_t serialWidth = 13;
			const int32_t typeWidth = 8;
			stringStream << std::setfill(' ')
				<< std::setw(idWidth) << "ID" << bar
				<< std::setw(addressWidth) << "Address" << bar
				<< std::setw(serialWidth) << "Serial Number" << bar
				<< std::setw(typeWidth) << "Type"
				<< std::endl;
			stringStream << "─────────┼─────────┼───────────────┼─────────" << std::endl;

			_devicesMutex.lock();
			for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
			{
				stringStream
					<< std::setw(idWidth) << std::setfill(' ') << (*i)->getID() << bar
					<< std::setw(addressWidth) << BaseLib::HelperFunctions::getHexString((*i)->getAddress(), 6) << bar
					<< std::setw(serialWidth) << (*i)->getSerialNumber() << bar
					<< std::setw(typeWidth) << BaseLib::HelperFunctions::getHexString((*i)->getDeviceType()) << std::endl;
			}
			_devicesMutex.unlock();
			stringStream << "─────────┴─────────┴───────────────┴─────────" << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices create") == 0)
		{
			int32_t address;
			uint32_t deviceType;
			std::string serialNumber;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					address = BaseLib::HelperFunctions::getNumber(element, true);
					if(address == 0) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 4 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 3)
				{
					serialNumber = element;
					if(serialNumber.size() > 10) return "Serial number too long.\n";
				}
				else if(index == 4) deviceType = BaseLib::HelperFunctions::getNumber(element, true);
				index++;
			}
			if(index < 5)
			{
				stringStream << "Description: This command creates a new virtual device." << std::endl;
				stringStream << "Usage: devices create ADDRESS SERIALNUMBER DEVICETYPE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tAny unused 3 byte address in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VSW9179403" << std::endl;
				stringStream << "  DEVICETYPE:\tThe type of the device to create. Example: FEFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported MAX virtual device id's:" << std::endl;
				stringStream << "  FEFFFFFD:\tCentral device" << std::endl;
				stringStream << "  FEFFFFFE:\tSpy device" << std::endl;
				return stringStream.str();
			}

			switch(deviceType)
			{
			case (uint32_t)DeviceType::MAXSD:
				add(std::shared_ptr<BaseLib::Systems::LogicalDevice>(new MAXSpyDevice(0, serialNumber, address, this)));
				stringStream << "Created MAX Spy Device with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			default:
				return "Unknown device type.\n";
			}
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices remove") == 0)
		{
			uint64_t id;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					id = BaseLib::HelperFunctions::getNumber(element, false);
					if(id == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a virtual device." << std::endl;
				stringStream << "Usage: devices remove DEVICEID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEVICEID:\tThe id of the device to delete. Example: 131" << std::endl;
				return stringStream.str();
			}

			if(_currentDevice && _currentDevice->getID() == id) _currentDevice.reset();
			if(get(id))
			{
				//if(_central && id == _central->getID()) _central.reset();
				remove(id);
				stringStream << "Removing device." << std::endl;
			}
			else stringStream << "Device not found." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices select") == 0)
		{
			uint64_t id = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			bool central = false;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					if(element == "central") central = true;
					else
					{
						id = BaseLib::HelperFunctions::getNumber(element, false);
						if(id == 0) return "Invalid id.\n";
					}
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a virtual device." << std::endl;
				stringStream << "Usage: devices select DEVICEID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEVICEID:\tThe id of the device to select or \"central\" as a shortcut to select the central device. Example: 131" << std::endl;
				return stringStream.str();
			}

			//_currentDevice = central ? _central : get(id);
			_currentDevice = get(id);
			if(!_currentDevice) stringStream << "Device not found." << std::endl;
			else
			{
				stringStream << "Device selected." << std::endl;
				stringStream << "For information about the device's commands type: \"help\"" << std::endl;
			}

			return stringStream.str();
		}
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

}
