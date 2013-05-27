/*
 * HomeMaticDevices.cpp
 *
 *  Created on: May 20, 2013
 *      Author: sathya
 */

#include "HomeMaticDevices.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HM-SD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HomeMaticCentral.h"

HomeMaticDevices::HomeMaticDevices()
{
}

HomeMaticDevices::~HomeMaticDevices()
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		delete (*i);
	}
}

void HomeMaticDevices::load()
{
	GD::db.executeCommand("CREATE TABLE IF NOT EXISTS devices (address INTEGER, deviceTYPE INTEGER, serializedObject TEXT, dutyCycleMessageCounter INTEGER, lastDutyCycle INTEGER)");
	std::vector<std::vector<DataColumn>> rows = GD::db.executeCommand("SELECT * FROM devices");
	for(std::vector<std::vector<DataColumn> >::iterator row = rows.begin(); row != rows.end(); ++row)
	{
		HomeMaticDevice* device = nullptr;
		HMDeviceTypes deviceType;
		std::string serializedObject;
		uint8_t dutyCycleMessageCounter = 0;
		for(std::vector<DataColumn>::iterator col = row->begin(); col != row->end(); ++col)
		{
			if(col->index == 1)
			{
				deviceType = (HMDeviceTypes)col->intValue;
			}
			else if(col->index == 2)
			{
				serializedObject = col->textValue;
			}
			else if(col->index == 3)
			{
				dutyCycleMessageCounter = col->intValue;
			}
			else if(col->index == 4)
			{
				switch(deviceType)
				{
				case HMDeviceTypes::HMCCTC:
					device = new HM_CC_TC(serializedObject, dutyCycleMessageCounter, col->intValue);
					break;
				case HMDeviceTypes::HMCCVD:
					device = new HM_CC_VD(serializedObject, dutyCycleMessageCounter, col->intValue);
					break;
				case HMDeviceTypes::HMCENTRAL:
					device = new HomeMaticCentral(serializedObject);
					_central = device;
					break;
				case HMDeviceTypes::HMSD:
					device = new HM_SD(serializedObject, dutyCycleMessageCounter, col->intValue);
					break;
				default:
					break;
				}
				if(device != nullptr)
				{
					_devices.push_back(device);
					device = nullptr;
				}
			}
		}
	}
}

void HomeMaticDevices::stopDutyCycles()
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		std::thread stop(&HomeMaticDevices::stopDutyCycle, this, (*i));
		stop.detach();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(8000));
}

void HomeMaticDevices::stopDutyCycle(HomeMaticDevice* device)
{
	device->stopDutyCycle();
}

void HomeMaticDevices::save()
{
	try
	{
		cout << "Waiting for duty cycles to stop..." << endl;
		//The stopping is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
		//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of this program.
		stopDutyCycles();
		for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			std::ostringstream command;
			command << "DELETE FROM devices WHERE address=" << std::dec << (*i)->address();
			GD::db.executeCommand(command.str());
			std::ostringstream command2;
			command2 << "INSERT INTO devices VALUES(" << (*i)->address() << "," << (uint32_t)(*i)->deviceType() << ",'" << (*i)->serialize() << "'," << (int32_t)(*i)->messageCounter()->at(1) << "," << (*i)->lastDutyCycleEvent() << ")";
			GD::db.executeCommand(command2.str());
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevices::add(HomeMaticDevice* device)
{
	if(device == nullptr) return;
	std::ostringstream command;
	command << "INSERT INTO devices VALUES(" << std::dec << device->address() << "," << (uint32_t)device->deviceType() << ",'" << device->serialize() << "'," << (int32_t)device->messageCounter()->at(1) << ","  << device->lastDutyCycleEvent() << ")";
	GD::db.executeCommand(command.str());
	_devices.push_back(device);
	if(device->deviceType() == HMDeviceTypes::HMCENTRAL) _central = device;
}

bool HomeMaticDevices::remove(int32_t address)
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->address() == address)
		{
			if(GD::debugLevel == 5) cout << "Deleting device object..." << endl;
			delete (*i);
			if(GD::debugLevel == 5) cout << "Removing device pointer from device array..." << endl;
			_devices.erase(i);

			if(GD::debugLevel == 5) cout << "Deleting database entry..." << endl;
			std::ostringstream command;
			command << "DELETE FROM devices WHERE address=" << std::dec << address;
			GD::db.executeCommand(command.str());
			return true;
		}
	}
	return false;
}

HomeMaticDevice* HomeMaticDevices::get(int32_t address)
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->address() == address)
		{
			return (*i);
		}
	}
	return nullptr;
}

HomeMaticDevice* HomeMaticDevices::get(std::string serialNumber)
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->serialNumber() == serialNumber)
		{
			return (*i);
		}
	}
	return nullptr;
}
