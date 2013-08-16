/*
 * HomeMaticDevices.cpp
 *
 *  Created on: May 20, 2013
 *      Author: sathya
 */

#include "HomeMaticDevices.h"
#include "GD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HM-SD.h"
#include "HelperFunctions.h"

HomeMaticDevices::HomeMaticDevices()
{
}

HomeMaticDevices::~HomeMaticDevices()
{
}

void HomeMaticDevices::load()
{
	try
	{
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS peers (parent INTEGER, address INTEGER, serializedObject TEXT)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS devices (address INTEGER, deviceTYPE INTEGER, serializedObject TEXT, dutyCycleMessageCounter INTEGER, lastDutyCycle INTEGER)");
		DataTable rows = GD::db.executeCommand("SELECT * FROM devices");
		bool spyDeviceExists = false;
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			HMDeviceTypes deviceType = HMDeviceTypes::HMUNKNOWN;
			std::string serializedObject;
			uint8_t dutyCycleMessageCounter = 0;
			for(std::map<uint32_t, std::shared_ptr<DataColumn>>::iterator col = row->second.begin(); col != row->second.end(); ++col)
			{
				if(col->second->index == 1)
				{
					deviceType = (HMDeviceTypes)col->second->intValue;
				}
				else if(col->second->index == 2)
				{
					serializedObject = col->second->textValue;
				}
				else if(col->second->index == 3)
				{
					dutyCycleMessageCounter = col->second->intValue;
				}
				else if(col->second->index == 4)
				{
					std::shared_ptr<HomeMaticDevice> device;
					switch(deviceType)
					{
					case HMDeviceTypes::HMCCTC:
						device = std::shared_ptr<HomeMaticDevice>(new HM_CC_TC());
						break;
					case HMDeviceTypes::HMLCSW1FM:
						device = std::shared_ptr<HomeMaticDevice>(new HM_LC_SWX_FM());
						break;
					case HMDeviceTypes::HMCCVD:
						device = std::shared_ptr<HomeMaticDevice>(new HM_CC_VD());
						break;
					case HMDeviceTypes::HMCENTRAL:
						_central = std::shared_ptr<HomeMaticCentral>(new HomeMaticCentral());
						device = _central;
						break;
					case HMDeviceTypes::HMSD:
						spyDeviceExists = true;
						device = std::shared_ptr<HomeMaticDevice>(new HM_SD());
						break;
					default:
						break;
					}
					if(device)
					{
						device->unserialize(serializedObject, dutyCycleMessageCounter, col->second->intValue);
						device->loadPeersFromDatabase();
						_devicesMutex.lock();
						_devices.push_back(device);
						_devicesMutex.unlock();
					}
				}
			}
		}
		if(!_central) createCentral();
		if(!spyDeviceExists) createSpyDevice();
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_devicesMutex.unlock();
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_devicesMutex.unlock();
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void HomeMaticDevices::createSpyDevice()
{
	try
	{
		if(!_central) return;

		int32_t seed = 0xfe0000 + HelperFunctions::getRandomNumber(1, 500);

		int32_t address = _central->getUniqueAddress(seed);
		std::string serialNumber(_central->getUniqueSerialNumber("VSD", HelperFunctions::getRandomNumber(1, 9999999)));

		add(new HM_SD(serialNumber, address));
		std::cout << "Created HM_SD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}


void HomeMaticDevices::createCentral()
{
	try
	{
		if(_central) return;

		int32_t address = getUniqueAddress(0xfd);
		std::string serialNumber(getUniqueSerialNumber("VCL"));

		add(new HomeMaticCentral(serialNumber, address));
		std::cout << "Created HMCENTRAL with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

std::shared_ptr<HomeMaticCentral> HomeMaticDevices::getCentral()
{
	try
	{
		_devicesMutex.lock();
		if(!_central)
		{
			_devicesMutex.unlock();
			return std::shared_ptr<HomeMaticCentral>();
		}

		std::shared_ptr<HomeMaticCentral> central(_central);
		_devicesMutex.unlock();
		return central;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    _devicesMutex.unlock();
    return std::shared_ptr<HomeMaticCentral>();
}

int32_t HomeMaticDevices::getUniqueAddress(uint8_t firstByte)
{
	int32_t prefix = firstByte << 16;
	int32_t seed = HelperFunctions::getRandomNumber(1, 9999);
	uint32_t i = 0;
	while(get(prefix + seed) && i++ < 10000)
	{
		seed += 13;
		if(seed > 9999) seed -= 10000;
	}
	return prefix + seed;
}

std::string HomeMaticDevices::getUniqueSerialNumber(std::string seedPrefix)
{
	if(seedPrefix.size() != 3) throw Exception("seedPrefix must have a size of 3.");
	uint32_t i = 0;
	int32_t seedNumber = HelperFunctions::getRandomNumber(1, 9999999);
	std::ostringstream stringstream;
	stringstream << seedPrefix << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
	std::string temp2 = stringstream.str();
	while(get(temp2) && i++ < 100000)
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

void HomeMaticDevices::stopThreads()
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			std::thread stop(&HomeMaticDevices::stopThreadsThread, this, (*i));
			stop.detach();
		}
		_devicesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
	std::this_thread::sleep_for(std::chrono::milliseconds(8000));
}

void HomeMaticDevices::stopThreadsThread(std::shared_ptr<HomeMaticDevice> device)
{
	device->stopThreads();
}

void HomeMaticDevices::save()
{
	try
	{
		std::cout << "Waiting for duty cycles and worker threads to stop..." << std::endl;
		//The stopping is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
		//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of the program.
		stopThreads();
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			(*i)->savePeersToDatabase();
			(*i)->saveToDatabase();
		}
		_devicesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void HomeMaticDevices::add(HomeMaticDevice* device)
{
	try
	{
		if(!device) return;
		_devicesMutex.lock();
		std::ostringstream command;
		command << "INSERT INTO devices VALUES(" << std::dec << device->address() << "," << (uint32_t)device->deviceType() << ",'" << device->serialize() << "'," << (int32_t)device->messageCounter()->at(1) << ","  << device->lastDutyCycleEvent() << ")";
		GD::db.executeCommand(command.str());
		if(device->deviceType() == HMDeviceTypes::HMCENTRAL)
		{
			_central = std::shared_ptr<HomeMaticCentral>((HomeMaticCentral*)device);
			_devices.push_back(_central);
		}
		else
		{
			_devices.push_back(std::shared_ptr<HomeMaticDevice>(device));
		}
		_devicesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_devicesMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

bool HomeMaticDevices::remove(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->address() == address)
			{
				if(GD::debugLevel >= 5) std::cout << "Deleting peers from database..." << std::endl;
				(*i)->deletePeersFromDatabase();
				if(GD::debugLevel >= 5) std::cout << "Removing device pointer from device array..." << std::endl;
				_devices.erase(i);
				if(GD::debugLevel >= 5) std::cout << "Deleting database entry..." << std::endl;
				std::ostringstream command;
				command << "DELETE FROM devices WHERE address=" << std::dec << address;
				GD::db.executeCommand(command.str());
				_devicesMutex.unlock();
				return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    _devicesMutex.unlock();
	return false;
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->address() == address)
			{
				_devicesMutex.unlock();
				return (*i);
			}
		}
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    _devicesMutex.unlock();
	return std::shared_ptr<HomeMaticDevice>();
}

std::vector<std::shared_ptr<HomeMaticDevice>> HomeMaticDevices::getDevices()
{
	try
	{
		_devicesMutex.lock();
		std::vector<std::shared_ptr<HomeMaticDevice>> devices;
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			devices.push_back(*i);
		}
		_devicesMutex.unlock();
		return devices;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    _devicesMutex.unlock();
	return std::vector<std::shared_ptr<HomeMaticDevice>>();
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->serialNumber() == serialNumber)
			{
				_devicesMutex.unlock();
				return (*i);
			}
		}
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    _devicesMutex.unlock();
	return std::shared_ptr<HomeMaticDevice>();
}

std::string HomeMaticDevices::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command.compare(0, 7, "devices") != 0 && (_currentDevice || command != "help"))
		{
			if(!_currentDevice) return "No device selected.\n";
			return _currentDevice->handleCLICommand(command);
		}
		else if(command == "devices help" || command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "devices list\t\tList all devices" << std::endl;
			stringStream << "devices create\t\tCreate a virtual device" << std::endl;
			stringStream << "devices remove\t\tRemove a virtual device" << std::endl;
			stringStream << "devices select\t\tSelect a virtual device" << std::endl;
			return stringStream.str();
		}
		else if(command == "devices list")
		{
			_devicesMutex.lock();
			std::vector<std::shared_ptr<HomeMaticDevice>> devices;
			for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
			{
				stringStream << "Address: 0x" << std::hex << (*i)->address() << "\tSerial number: " << (*i)->serialNumber() << "\tDevice type: " << (uint32_t)(*i)->deviceType() << std::endl << std::dec;
			}
			_devicesMutex.unlock();
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices create") == 0)
		{
			int32_t address;
			int32_t deviceType;
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
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
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
				stringStream << "  ADDRESS:\tAny unused 3 byte address in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VTC9179403" << std::endl;
				stringStream << "  DEVICETYPE:\tThe type of the device to create. Example: FFFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported virtual device types:" << std::endl;
				stringStream << "  FFFFFFFD:\tCentral device" << std::endl;
				stringStream << "  FFFFFFFE:\tSpy device" << std::endl;
				stringStream << "  39:\t\tHM-CC-TC" << std::endl;
				stringStream << "  3A:\t\tHM-CC-VD" << std::endl;
				return stringStream.str();
			}

			switch(deviceType)
			{
			case (uint32_t)HMDeviceTypes::HMCCTC:
				GD::devices.add(new HM_CC_TC(serialNumber, address));
				stringStream << "Created HM_CC_TC with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)HMDeviceTypes::HMCCVD:
				GD::devices.add(new HM_CC_VD(serialNumber, address));
				stringStream << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)HMDeviceTypes::HMCENTRAL:
				GD::devices.add(new HomeMaticCentral(serialNumber, address));
				stringStream << "Created HMCENTRAL with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)HMDeviceTypes::HMSD:
				GD::devices.add(new HM_SD(serialNumber, address));
				stringStream << "Created HM_SD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			default:
				return "Unknown device type.\n";
			}
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices remove") == 0)
		{
			int32_t address;

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
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a virtual device." << std::endl;
				stringStream << "Usage: devices remove ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the device to delete in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(_currentDevice && _currentDevice->address() == address) _currentDevice.reset();
			if(GD::devices.remove(address)) stringStream << "Device removed." << std::endl;
			else stringStream << "Device not found." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices select") == 0)
		{
			int32_t address = 0;

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
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a virtual device." << std::endl;
				stringStream << "Usage: devices select ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the device to select in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			_currentDevice = GD::devices.get(address);
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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return "Error executing command. See log file for more details.\n";
}
