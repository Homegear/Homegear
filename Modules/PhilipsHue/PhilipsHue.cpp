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

#include "PhilipsHue.h"
#include "PhilipsHueDeviceTypes.h"
#include "PhysicalInterfaces/HueBridge.h"
#include "LogicalDevices/PhilipsHueCentral.h"
#include "GD.h"

namespace PhilipsHue
{

PhilipsHue::PhilipsHue(BaseLib::Obj* bl, BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(bl, eventHandler)
{
	GD::bl = _bl;
	GD::family = this;
	GD::out.init(bl);
	GD::out.setPrefix("Module Philips hue: ");
	GD::out.printDebug("Debug: Loading module...");
	_family = BaseLib::Systems::DeviceFamilies::PhilipsHue;
	GD::rpcDevices.init(_bl);
}

PhilipsHue::~PhilipsHue()
{

}

bool PhilipsHue::init()
{
	GD::out.printInfo("Loading XML RPC devices...");
	GD::rpcDevices.load();
	if(GD::rpcDevices.empty()) return false;
	return true;
}

void PhilipsHue::dispose()
{
	if(_disposed) return;
	DeviceFamily::dispose();

	GD::physicalInterface.reset();
	_central.reset();
	GD::rpcDevices.clear();
}

std::shared_ptr<BaseLib::Systems::Central> PhilipsHue::getCentral() { return _central; }

std::shared_ptr<BaseLib::Systems::IPhysicalInterface> PhilipsHue::createPhysicalDevice(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings)
{
	try
	{
		std::shared_ptr<IPhilipsHueInterface> device;
		if(!settings) return device;
		GD::out.printDebug("Debug: Creating physical device. Type defined in physicalinterfaces.conf is: " + settings->type);
		if(settings->type == "huebridge") device.reset(new HueBridge(settings));
		else GD::out.printError("Error: Unsupported physical device type: " + settings->type);
		if(device) GD::physicalInterface = device;
		return device;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<BaseLib::Systems::IPhysicalInterface>();
}

uint32_t PhilipsHue::getUniqueAddress(uint32_t seed)
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

std::string PhilipsHue::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
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

std::shared_ptr<PhilipsHueDevice> PhilipsHue::getDevice(uint32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((uint32_t)(*i)->getAddress() == address)
			{
				std::shared_ptr<PhilipsHueDevice> device(std::dynamic_pointer_cast<PhilipsHueDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
			}
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _devicesMutex.unlock();
	return std::shared_ptr<PhilipsHueDevice>();
}

std::shared_ptr<PhilipsHueDevice> PhilipsHue::getDevice(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<PhilipsHueDevice> device(std::dynamic_pointer_cast<PhilipsHueDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
			}
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _devicesMutex.unlock();
	return std::shared_ptr<PhilipsHueDevice>();
}

void PhilipsHue::load()
{
	try
	{
		_devices.clear();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDevices();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			GD::out.printMessage("Loading device " + std::to_string(deviceID));
			int32_t address = row->second.at(1)->intValue;
			std::string serialNumber = row->second.at(2)->textValue;
			uint32_t deviceType = row->second.at(3)->intValue;
			std::shared_ptr<BaseLib::Systems::LogicalDevice> device;
			switch((DeviceType)deviceType)
			{
			case DeviceType::CENTRAL:
				_central = std::shared_ptr<PhilipsHueCentral>(new PhilipsHueCentral(deviceID, serialNumber, address, this));
				device = _central;
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
			if(!_central) createCentral();
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void PhilipsHue::createCentral()
{
	try
	{
		if(_central) return;
		uint32_t seed = 0xfd0000 + BaseLib::HelperFunctions::getRandomNumber(1, 32767);

		int32_t address = getUniqueAddress(seed);
		std::string serialNumber(getUniqueSerialNumber("VHC", BaseLib::HelperFunctions::getRandomNumber(1, 9999999)));

		_central.reset(new PhilipsHueCentral(0, serialNumber, address, this));
		add(_central);
		GD::out.printMessage("Created central with id " + std::to_string(_central->getID()) + ", address 0x" + BaseLib::HelperFunctions::getHexString(address, 6) + " and serial number " + serialNumber);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string PhilipsHue::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if((command == "unselect" || command == "u") && _currentDevice && !_currentDevice->peerSelected())
		{
			_currentDevice.reset();
			return "Device unselected.\n";
		}
		else if((command.compare(0, 7, "devices") != 0 || BaseLib::HelperFunctions::isShortCLICommand(command)) && _currentDevice)
		{
			return _currentDevice->handleCLICommand(command);
		}
		else if(command == "devices help" || command == "dh" || command == "help" || command == "h")
		{
			stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "devices list (ls)\tList all Philips hue devices" << std::endl;
			stringStream << "devices create (dc)\tCreate a virtual Philips hue device" << std::endl;
			stringStream << "devices remove (dr)\tRemove a virtual Philips hue device" << std::endl;
			stringStream << "devices select (ds)\tSelect a virtual Philips hue device" << std::endl;
			stringStream << "unselect (u)\t\tUnselect this device family" << std::endl;
			return stringStream.str();
		}
		else if(command == "devices list" || command == "dl" || (command == "ls" && !_currentDevice))
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
		else if(command.compare(0, 14, "devices create") == 0 || command.compare(0, 2, "dc") == 0)
		{
			int32_t address = -1;
			uint32_t deviceType = (uint32_t)DeviceType::none;
			std::string serialNumber;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'c') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					address = BaseLib::Math::getNumber(element, true);
					if(address == 0) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 4 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 2 + offset)
				{
					serialNumber = element;
					if(serialNumber.size() > 10) return "Serial number too long.\n";
				}
				else if(index == 3 + offset) deviceType = BaseLib::Math::getNumber(element, true);
				index++;
			}
			if(index < 4 + offset)
			{
				stringStream << "Description: This command creates a new virtual device." << std::endl;
				stringStream << "Usage: devices create ADDRESS SERIALNUMBER DEVICETYPE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tAny unused 3 byte address in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VSW9179403" << std::endl;
				stringStream << "  DEVICETYPE:\tThe type of the device to create. Example: FEFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported Philips hue virtual device id's:" << std::endl;
				stringStream << "  FEFFFFFD:\tCentral device" << std::endl;
				return stringStream.str();
			}

			switch(deviceType)
			{
			case (uint32_t)DeviceType::CENTRAL:
				if(_central) stringStream << "Cannot create more than one Philips hue central device." << std::endl;
				else
				{
					_central.reset(new PhilipsHueCentral(0, serialNumber, address, this));
					add(_central);
					stringStream << "Created Philips hue Central with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				}
				break;
			default:
				return "Unknown device type.\n";
			}
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices remove") == 0 || command.compare(0, 2, "dr") == 0)
		{
			uint64_t id = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'r') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					id = BaseLib::Math::getNumber(element, false);
					if(id == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 1 + offset)
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
				if(_central && id == _central->getID()) _central.reset();
				remove(id);
				stringStream << "Removing device." << std::endl;
			}
			else stringStream << "Device not found." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices select") == 0 || command.compare(0, 2, "ds") == 0)
		{
			uint64_t id = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 's') ? 0 : 1;
			int32_t index = 0;
			bool central = false;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					if(element == "central" || element == "c") central = true;
					else
					{
						id = BaseLib::Math::getNumber(element, false);
						if(id == 0) return "Invalid id.\n";
					}
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command selects a virtual device." << std::endl;
				stringStream << "Usage: devices select DEVICEID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEVICEID:\tThe id of the device to select or \"central\" as a shortcut to select the central device. Example: 131" << std::endl;
				return stringStream.str();
			}

			_currentDevice = central ? _central : get(id);
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
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

std::shared_ptr<BaseLib::RPC::Variable> PhilipsHue::getPairingMethods()
{
	try
	{
		if(!_central) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		std::shared_ptr<BaseLib::RPC::Variable> array(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		array->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("searchDevices"))));
		return array;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}
}
