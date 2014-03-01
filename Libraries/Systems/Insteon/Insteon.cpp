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

#include "Insteon.h"
#include "PhysicalDevices/Insteon_Hub_X10.h"
#include "InsteonDeviceTypes.h"
//#include "Devices/InsteonCentral.h"
#include "Devices/Insteon-SD.h"
#include "../../GD/GD.h"

namespace Insteon
{

Insteon::Insteon()
{
	_family = DeviceFamilies::Insteon;
}

Insteon::~Insteon()
{

}

std::shared_ptr<Central> Insteon::getCentral() { return std::shared_ptr<Central>(); /*return _central;*/ }

std::shared_ptr<PhysicalDevices::PhysicalDevice> Insteon::createPhysicalDevice(std::shared_ptr<PhysicalDevices::PhysicalDeviceSettings> settings)
{
	try
	{
		if(!settings) return std::shared_ptr<PhysicalDevices::PhysicalDevice>();
		if(settings->type == "insteonhubx10") return std::shared_ptr<PhysicalDevices::PhysicalDevice>(new PhysicalDevices::InsteonHubX10(settings));
		else Output::printError("Error: Unsupported physical device type for family Insteon: " + settings->type);
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
	return std::shared_ptr<PhysicalDevices::PhysicalDevice>();
}

uint32_t Insteon::getUniqueAddress(uint32_t seed)
{
	uint32_t prefix = seed;
	seed = HelperFunctions::getRandomNumber(0, 65535);
	uint32_t i = 0;
	while(getDevice(prefix + seed) && i++ < 10000)
	{
		seed += 131;
		if(seed > 65535) seed -= 40000;
	}
	return prefix + seed;
}

std::string Insteon::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
{
	if(seedPrefix.size() != 3) throw Exception("seedPrefix must have a size of 3.");
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

std::shared_ptr<InsteonDevice> Insteon::getDevice(uint32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				std::shared_ptr<InsteonDevice> device(std::dynamic_pointer_cast<InsteonDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
			}
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
    _devicesMutex.unlock();
	return std::shared_ptr<InsteonDevice>();
}

std::shared_ptr<InsteonDevice> Insteon::getDevice(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<InsteonDevice> device(std::dynamic_pointer_cast<InsteonDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
			}
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
    _devicesMutex.unlock();
	return std::shared_ptr<InsteonDevice>();
}

void Insteon::load(bool version_0_0_7)
{
	try
	{
		_devices.clear();
		DataTable rows = GD::db.executeCommand("SELECT * FROM devices WHERE deviceFamily=" + std::to_string((uint32_t)DeviceFamilies::HomeMaticWired));
		bool spyDeviceExists = false;
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			Output::printMessage("Loading Insteon device " + std::to_string(deviceID));
			int32_t address = row->second.at(1)->intValue;
			std::string serialNumber = row->second.at(2)->textValue;
			uint32_t deviceType = row->second.at(3)->intValue;

			std::shared_ptr<LogicalDevice> device;
			switch((DeviceType)deviceType)
			{
			/*case DeviceType::HMWIREDCENTRAL:
				_central = std::shared_ptr<InsteonDevice>(new InsteonDevice(deviceID, serialNumber, address));
				device = _central;
				break;*/
			case DeviceType::INSTEONSD:
				spyDeviceExists = true;
				device = std::shared_ptr<LogicalDevice>(new Insteon_SD(deviceID, serialNumber, address));
				break;
			default:
				break;
			}

			if(device)
			{
				device->load();
				device->loadPeers(version_0_0_7);
				_devicesMutex.lock();
				_devices.push_back(device);
				_devicesMutex.unlock();
			}
		}
		if(GD::physicalDevices.get(DeviceFamilies::Insteon)->isOpen())
		{
			//if(!_central) createCentral();
			if(!spyDeviceExists) createSpyDevice();
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
}

void Insteon::createSpyDevice()
{
	try
	{
		uint32_t seed = 0xfe0000 + HelperFunctions::getRandomNumber(1, 65535);

		//ToDo: Use HMWiredCentral to get unique address
		int32_t address = getUniqueAddress(seed);
		std::string serialNumber(getUniqueSerialNumber("VIS", HelperFunctions::getRandomNumber(1, 9999999)));

		add(std::shared_ptr<LogicalDevice>(new Insteon_SD(0, serialNumber, address)));
		Output::printMessage("Created Insteon spy device with address 0x" + HelperFunctions::getHexString(address, 8) + " and serial number " + serialNumber);
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

void Insteon::createCentral()
{
	try
	{
		/*if(_central) return;

		std::string serialNumber(getUniqueSerialNumber("VWC", HelperFunctions::getRandomNumber(1, 9999999)));

		_central.reset(new HMWiredCentral(0, serialNumber, 1));
		add(_central);
		Output::printMessage("Created HomeMatic Wired central with address 0x" + HelperFunctions::getHexString(1, 8) + " and serial number " + serialNumber);*/
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

std::string Insteon::handleCLICommand(std::string& command)
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
			stringStream << "unselect\t\tUnselect this device family" << std::endl;
			stringStream << "devices list\t\tList all HomeMatic Wired devices" << std::endl;
			stringStream << "devices create\t\tCreate a virtual HomeMatic Wired device" << std::endl;
			stringStream << "devices remove\t\tRemove a virtual HomeMatic Wired device" << std::endl;
			stringStream << "devices select\t\tSelect a virtual HomeMatic Wired device" << std::endl;
			return stringStream.str();
		}
		else if(command == "devices list")
		{
			_devicesMutex.lock();
			std::vector<std::shared_ptr<LogicalDevice>> devices;
			for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
			{
				stringStream << "ID: " << std::setw(6) << std::setfill(' ') << (*i)->getID() << "\tAddress: 0x" << std::hex << std::setw(8) << std::setfill('0') << (*i)->getAddress() << "\tSerial number: " << (*i)->getSerialNumber() << "\tDevice type: " << (*i)->getDeviceType() << std::endl << std::dec;
			}
			_devicesMutex.unlock();
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
					address = HelperFunctions::getNumber(element, true);
					if(address == 0) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 4 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 3)
				{
					serialNumber = element;
					if(serialNumber.size() > 10) return "Serial number too long.\n";
				}
				else if(index == 4) deviceType = HelperFunctions::getNumber(element, true);
				index++;
			}
			if(index < 5)
			{
				stringStream << "Description: This command creates a new virtual device." << std::endl;
				stringStream << "Usage: devices create ADDRESS SERIALNUMBER DEVICETYPE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tAny unused 4 byte address in hexadecimal format. Example: 101A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VSW9179403" << std::endl;
				stringStream << "  DEVICETYPE:\tThe type of the device to create. Example: FEFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported HomeMatic Wired virtual device id's:" << std::endl;
				stringStream << "  FEFFFFFD:\tCentral device" << std::endl;
				stringStream << "  FEFFFFFE:\tSpy device" << std::endl;
				return stringStream.str();
			}

			switch(deviceType)
			{
			/*case (uint32_t)DeviceType::HMWIREDCENTRAL:
				if(_central) stringStream << "Cannot create more than one HomeMatic Wired central device." << std::endl;
				else
				{
					add(std::shared_ptr<LogicalDevice>(new HMWiredCentral(0, serialNumber, address)));
					stringStream << "Created HomeMatic Wired Central with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				}
				break;*/
			case (uint32_t)DeviceType::INSTEONSD:
				add(std::shared_ptr<LogicalDevice>(new Insteon_SD(0, serialNumber, address)));
				stringStream << "Created HomeMatic Wired Spy Device with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
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
					id = HelperFunctions::getNumber(element, true);
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
				//if(_central && address == _central->getAddress()) _central.reset();
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
						id = HelperFunctions::getNumber(element, true);
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

			_currentDevice = get(id); // = central ? _central : get(address);
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
    return "Error executing command. See log file for more details.\n";
}

} /* namespace HMWired */
