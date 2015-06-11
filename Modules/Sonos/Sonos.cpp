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

#include "Sonos.h"
#include "DeviceTypes.h"
#include "PhysicalInterfaces/EventServer.h"
#include "LogicalDevices/SonosCentral.h"
#include "GD.h"

namespace Sonos
{

Sonos::Sonos(BaseLib::Obj* bl, BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(bl, eventHandler)
{
	GD::bl = bl;
	GD::family = this;
	GD::out.init(bl);
	GD::out.setPrefix("Module Sonos: ");
	GD::out.printDebug("Debug: Loading module...");
	_family = BaseLib::Systems::DeviceFamilies::Sonos;
	GD::rpcDevices.init(_bl);
}

Sonos::~Sonos()
{

}

bool Sonos::init()
{
	GD::out.printInfo("Loading XML RPC devices...");
	GD::rpcDevices.load();
	if(GD::rpcDevices.empty()) return false;
	return true;
}

void Sonos::dispose()
{
	if(_disposed) return;
	DeviceFamily::dispose();

	_central.reset();
	GD::rpcDevices.clear();
}

std::shared_ptr<BaseLib::Systems::IPhysicalInterface> Sonos::createPhysicalDevice(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings)
{
	try
	{
		std::shared_ptr<ISonosInterface> device;
		if(!settings) return device;
		GD::out.printDebug("Debug: Creating physical device. Type defined in physicalinterfaces.conf is: " + settings->type);
		if(settings->type == "eventserver") device.reset(new EventServer(settings));
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

std::shared_ptr<BaseLib::Systems::Central> Sonos::getCentral() { return _central; }

uint32_t Sonos::getUniqueAddress(uint32_t seed)
{
	uint32_t prefix = seed;
	seed = BaseLib::HelperFunctions::getRandomNumber(1, 999999);
	uint32_t i = 0;
	while(getDevice(prefix + seed) && i++ < 10000)
	{
		seed += 131;
		if(seed > 999999) seed -= 1000000;
	}
	return prefix + seed;
}

std::string Sonos::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
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

std::shared_ptr<SonosDevice> Sonos::getDevice(uint32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((uint32_t)(*i)->getAddress() == address)
			{
				std::shared_ptr<SonosDevice> device(std::dynamic_pointer_cast<SonosDevice>(*i));
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
	return std::shared_ptr<SonosDevice>();
}

std::shared_ptr<SonosDevice> Sonos::getDevice(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<SonosDevice> device(std::dynamic_pointer_cast<SonosDevice>(*i));
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
	return std::shared_ptr<SonosDevice>();
}

void Sonos::load()
{
	try
	{
		_devices.clear();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDevices();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			GD::out.printMessage("Loading Sonos device " + std::to_string(deviceID));
			std::string serialNumber = row->second.at(2)->textValue;
			uint32_t deviceType = row->second.at(3)->intValue;

			std::shared_ptr<BaseLib::Systems::LogicalDevice> device;
			switch((DeviceType)deviceType)
			{
			case DeviceType::CENTRAL:
				_central = std::shared_ptr<SonosCentral>(new SonosCentral(deviceID, serialNumber, this));
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
		if(!_central) createCentral();
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

void Sonos::createCentral()
{
	try
	{
		if(_central) return;

		std::string serialNumber(getUniqueSerialNumber("VSC", BaseLib::HelperFunctions::getRandomNumber(1, 9999999)));

		_central.reset(new SonosCentral(0, serialNumber, this));
		add(_central);
		GD::out.printMessage("Created Sonos central with id " + std::to_string(_central->getID()) + " and serial number " + serialNumber);
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

std::string Sonos::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if((command == "unselect" || command == "u") && _currentDevice && !_currentDevice->peerSelected())
		{
			_currentDevice.reset();
			return "Device unselected.\n";
		}
		else
		{
			if(!_central) return "Error: No central exists.\n";
			if(!_currentDevice) _currentDevice = _central;
			return _currentDevice->handleCLICommand(command);
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
    return "Error executing command. See log file for more details.\n";
}

std::shared_ptr<BaseLib::RPC::Variable> Sonos::getPairingMethods()
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
