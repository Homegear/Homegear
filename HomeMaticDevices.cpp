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
