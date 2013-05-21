/*
 * HomeMaticDevices.cpp
 *
 *  Created on: May 20, 2013
 *      Author: sathya
 */

#include "HomeMaticDevices.h"
#include "HM-CC-TC.h"
#include "HM-SD.h"
#include "HM-CC-VD.h"

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
		int32_t deviceType = 0;
		std::string serializedObject;
		uint8_t dutyCycleMessageCounter;
		for(std::vector<DataColumn>::iterator col = row->begin(); col != row->end(); ++col)
		{
			if(col->index == 1)
			{
				deviceType = col->intValue;
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
				case 0x39:
					device = new HM_CC_TC(serializedObject, dutyCycleMessageCounter, col->intValue);
					break;
				case 0x3A:
					device = new HM_CC_VD(serializedObject, dutyCycleMessageCounter, col->intValue);
					break;
				case 0xFFFFFFFE:
					device = new HM_SD(serializedObject, dutyCycleMessageCounter, col->intValue);
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
			//GD::db->executeCommand("INSERT INTO bla VALUES('Hallo')");
        //std::vector<std::vector<DataColumn>> rows = GD::db->executeCommand("SELECT * FROM bla");
        //cout << rows[0][0].textValue << endl;
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
			command2 << "INSERT INTO devices VALUES(" << (*i)->address() << "," << (*i)->deviceType() << ",'" << (*i)->serialize() << "'," << (int32_t)(*i)->messageCounter()->at(1) << "," << (*i)->lastDutyCycleEvent() << ")";
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
	std::ostringstream command;
	command << "INSERT INTO devices VALUES(" << std::dec << device->address() << "," << device->deviceType() << ",'" << device->serialize() << "'," << (int32_t)device->messageCounter()->at(1) << ","  << device->lastDutyCycleEvent() << ")";
	GD::db.executeCommand(command.str());
	_devices.push_back(device);
}

bool HomeMaticDevices::remove(int32_t address)
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->address() == address)
		{
			delete (*i);
			_devices.erase(i);
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
