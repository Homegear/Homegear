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

#include "BidCoS.h"
#include "PhysicalInterfaces/IBidCoSInterface.h"
#include "PhysicalInterfaces/COC.h"
#include "PhysicalInterfaces/CUL.h"
#include "PhysicalInterfaces/TICC1100.h"
#include "PhysicalInterfaces/HM-CFG-LAN.h"
#include "PhysicalInterfaces/RTLSDR-LAN.h"
#include "Devices/HomeMaticCentral.h"
#include "BidCoSDeviceTypes.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-SD.h"
#include "../Base/BaseLib.h"
#include "GD.h"

namespace BidCoS
{
BidCoS::BidCoS(BaseLib::Obj* baseLib, IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(baseLib, eventHandler)
{
	BaseLib::Output::setPrefix("Module HomeMatic BidCoS: ");
	BaseLib::Output::printDebug("Debug: Loading module...");
	_family = BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS;
}

BidCoS::~BidCoS()
{

}

std::shared_ptr<BaseLib::Systems::Central> BidCoS::getCentral() { return _central; }

std::shared_ptr<BaseLib::RPC::RPCVariable> BidCoS::listBidcosInterfaces()
{
	try
	{
		if(!_central || !GD::defaultPhysicalInterface) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		std::shared_ptr<BaseLib::RPC::RPCVariable> interface(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		array->arrayValue->push_back(interface);
		interface->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_central->getSerialNumber()))));
		interface->structValue->insert(BaseLib::RPC::RPCStructElement("DESCRIPTION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("Homegear default BidCoS interface")))));
		interface->structValue->insert(BaseLib::RPC::RPCStructElement("CONNECTED", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(GD::defaultPhysicalInterface->isOpen()))));
		interface->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(true))));
		return array;
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::Systems::IPhysicalInterface> BidCoS::createPhysicalDevice(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings)
{
	try
	{
		BaseLib::Output::printDebug("Debug: Creating physical device. Type defined in physicalinterfaces.conf is: " + settings->type);
		std::shared_ptr<IBidCoSInterface> device;
		if(!settings) return device;
		if(settings->type == "cul") device.reset(new CUL(settings));
		else if(settings->type == "coc") device.reset(new COC(settings));
		else if(settings->type == "cc1100") device.reset(new TICC1100(settings));
		else if(settings->type == "hmcfglan") device.reset(new HM_CFG_LAN(settings));
		else if(settings->type == "rtlsdrlan") device.reset(new RTLSDR_LAN(settings));
		else BaseLib::Output::printError("Error: Unsupported physical device type for family HomeMatic BidCoS: " + settings->type);
		if(device)
		{
			GD::physicalInterfaces[settings->id] = device;
			if(settings->isDefault || !GD::defaultPhysicalInterface) GD::defaultPhysicalInterface = device;
		}
		return device;
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

int32_t BidCoS::getUniqueAddress(uint8_t firstByte)
{
	int32_t prefix = firstByte << 16;
	int32_t seed = BaseLib::HelperFunctions::getRandomNumber(1, 9999);
	uint32_t i = 0;
	while(getDevice(prefix + seed) && i++ < 10000)
	{
		seed += 13;
		if(seed > 9999) seed -= 10000;
	}
	return prefix + seed;
}

std::string BidCoS::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
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

void BidCoS::createCentral()
{
	try
	{
		if(_central) return;

		int32_t address = getUniqueAddress(0xfd);
		std::string serialNumber(getUniqueSerialNumber("VBC", BaseLib::HelperFunctions::getRandomNumber(1, 9999999)));

		_central.reset(new HomeMaticCentral(0, serialNumber, address, this));
		add(_central);
		BaseLib::Output::printMessage("Created HomeMatic BidCoS central with id " + std::to_string(_central->getID()) + ", address 0x" + BaseLib::HelperFunctions::getHexString(address, 6) + " and serial number " + serialNumber);
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

void BidCoS::createSpyDevice()
{
	try
	{
		if(!_central) return;

		int32_t seed = 0xfe0000 + BaseLib::HelperFunctions::getRandomNumber(1, 500);

		int32_t address = _central->getUniqueAddress(seed);
		std::string serialNumber(getUniqueSerialNumber("VBS", BaseLib::HelperFunctions::getRandomNumber(1, 9999999)));

		std::shared_ptr<BaseLib::Systems::LogicalDevice> device(new HM_SD(0, serialNumber, address, this));
		add(device);
		BaseLib::Output::printMessage("Created HomeMatic BidCoS spy device with id " + std::to_string(device->getID()) + ", address 0x" + BaseLib::HelperFunctions::getHexString(address, 6) + " and serial number " + serialNumber);
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

std::shared_ptr<HomeMaticDevice> BidCoS::getDevice(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				std::shared_ptr<HomeMaticDevice> device(std::dynamic_pointer_cast<HomeMaticDevice>(*i));
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
	return std::shared_ptr<HomeMaticDevice>();
}

std::shared_ptr<HomeMaticDevice> BidCoS::getDevice(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<BaseLib::Systems::LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<HomeMaticDevice> device(std::dynamic_pointer_cast<HomeMaticDevice>(*i));
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
	return std::shared_ptr<HomeMaticDevice>();
}

void BidCoS::load()
{
	try
	{
		_devices.clear();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDevices();
		bool spyDeviceExists = false;
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			BaseLib::Output::printMessage("Loading HomeMatic BidCoS device " + std::to_string(deviceID));
			int32_t address = row->second.at(1)->intValue;
			std::string serialNumber = row->second.at(2)->textValue;
			uint32_t deviceType = row->second.at(3)->intValue;

			std::shared_ptr<BaseLib::Systems::LogicalDevice> device;
			switch((DeviceType)deviceType)
			{
			case DeviceType::HMCCTC:
				device = std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_CC_TC(deviceID, serialNumber, address, this));
				break;
			case DeviceType::HMLCSW1FM:
				device = std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_LC_SWX_FM(deviceID, serialNumber, address, this));
				break;
			case DeviceType::HMCCVD:
				device = std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_CC_VD(deviceID, serialNumber, address, this));
				break;
			case DeviceType::HMCENTRAL:
				_central = std::shared_ptr<HomeMaticCentral>(new HomeMaticCentral(deviceID, serialNumber, address, this));
				device = _central;
				break;
			case DeviceType::HMSD:
				spyDeviceExists = true;
				device = std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_SD(deviceID, serialNumber, address, this));
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
		if(!GD::physicalInterfaces.empty())
		{
			if(!_central) createCentral();
			if(!spyDeviceExists) createSpyDevice();
			if(_central) _central->addPeersToVirtualDevices();
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

std::string BidCoS::handleCLICommand(std::string& command)
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
			stringStream << "devices list\t\tList all HomeMatic BidCoS devices" << std::endl;
			stringStream << "devices create\t\tCreate a virtual HomeMatic BidCoS device" << std::endl;
			stringStream << "devices remove\t\tRemove a virtual HomeMatic BidCoS device" << std::endl;
			stringStream << "devices select\t\tSelect a virtual HomeMatic BidCoS device" << std::endl;
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
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VTC9179403" << std::endl;
				stringStream << "  DEVICETYPE:\tThe type of the device to create. Example: FFFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported HomeMatic BidCoS virtual device id's:" << std::endl;
				stringStream << "  FFFFFFFD:\tCentral device" << std::endl;
				stringStream << "  FFFFFFFE:\tSpy device" << std::endl;
				stringStream << "  39:\t\tHM-CC-TC" << std::endl;
				stringStream << "  3A:\t\tHM-CC-VD" << std::endl;
				return stringStream.str();
			}

			switch(deviceType)
			{
			case (uint32_t)DeviceType::HMCCTC:
				add(std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_CC_TC(0, serialNumber, address, this)));
				stringStream << "Created HM_CC_TC with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)DeviceType::HMCCVD:
				add(std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_CC_VD(0, serialNumber, address, this)));
				stringStream << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)DeviceType::HMCENTRAL:
				if(_central) stringStream << "Cannot create more than one HomeMatic BidCoS central device." << std::endl;
				else
				{
					_central.reset(new HomeMaticCentral(0, serialNumber, address, this));
					add(_central);
					stringStream << "Created HomeMatic BidCoS Central with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				}
				break;
			case (uint32_t)DeviceType::HMSD:
				add(std::shared_ptr<BaseLib::Systems::LogicalDevice>(new HM_SD(0, serialNumber, address, this)));
				stringStream << "Created HomeMatic BidCoS Spy Device with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
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
				if(_central && id == _central->getID()) _central.reset();
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
} /* namespace BidCoS */
