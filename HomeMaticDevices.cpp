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
						device = std::shared_ptr<HomeMaticDevice>(new HM_SD());
						break;
					default:
						break;
					}
					if(device)
					{
						device->unserialize(serializedObject, dutyCycleMessageCounter, col->second->intValue);
						device->loadPeersFromDatabase();
						_devices.push_back(device);
					}
				}
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
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

void HomeMaticDevices::stopDutyCycles()
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		std::thread stop(&HomeMaticDevices::stopDutyCycle, this, (*i));
		stop.detach();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(8000));
}

void HomeMaticDevices::stopDutyCycle(std::shared_ptr<HomeMaticDevice> device)
{
	device->stopDutyCycle();
}

void HomeMaticDevices::save()
{
	try
	{
		std::cout << "Waiting for duty cycles to stop..." << std::endl;
		//The stopping is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
		//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of this program.
		stopDutyCycles();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			(*i)->savePeersToDatabase();
			std::ostringstream command;
			command << "SELECT 1 FROM devices WHERE address=" << std::dec << (*i)->address();
			DataTable result = GD::db.executeCommand(command.str());
			if(result.empty())
			{
				std::ostringstream command2;
				command2 << "INSERT INTO devices VALUES(" << (*i)->address() << "," << (uint32_t)(*i)->deviceType() << ",'" << (*i)->serialize() << "'," << (int32_t)(*i)->messageCounter()->at(1) << "," << (*i)->lastDutyCycleEvent() << ")";
				GD::db.executeCommand(command2.str());
			}
			else
			{
				std::ostringstream command2;
				command2 << "UPDATE devices SET serializedObject='" << (*i)->serialize() << "',dutyCycleMessageCounter=" << (int32_t)(*i)->messageCounter()->at(1) << ",lastDutyCycle=" << (*i)->lastDutyCycleEvent() << " WHERE address=" << std::dec << (*i)->address();
				GD::db.executeCommand(command2.str());
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
}

void HomeMaticDevices::add(HomeMaticDevice* device)
{
	if(device == nullptr) return;
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
}

bool HomeMaticDevices::remove(int32_t address)
{
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
			return true;
		}
	}
	return false;
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(int32_t address)
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->address() == address)
		{
			return (*i);
		}
	}
	return nullptr;
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(std::string serialNumber)
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->serialNumber() == serialNumber)
		{
			return (*i);
		}
	}
	return nullptr;
}
