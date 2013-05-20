/*
 * HomeMaticDevices.cpp
 *
 *  Created on: May 20, 2013
 *      Author: sathya
 */

#include "HomeMaticDevices.h"
#include "HM-CC-TC.h"
#include "HM-SD.h"

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
	GD::db.executeCommand("CREATE TABLE IF NOT EXISTS devices (address INTEGER, deviceTYPE INTEGER, serializedObject TEXT, lastDutyCycle INTEGER)");
	std::vector<std::vector<DataColumn>> rows = GD::db.executeCommand("SELECT * FROM devices");
	for(std::vector<std::vector<DataColumn> >::iterator row = rows.begin(); row != rows.end(); ++row)
	{
		HomeMaticDevice* device = nullptr;
		int32_t deviceType = 0;
		for(std::vector<DataColumn>::iterator col = row->begin(); col != row->end(); ++col)
		{
			if(col->index == 1)
			{
				deviceType = col->intValue;
			}
			if(col->index == 2)
			{
				switch(deviceType)
				{
				case 0x39:
					device = new HM_CC_TC(col->textValue);
					break;
				case 0xFFFFFFFE:
					device = new HM_SD(col->textValue);
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

void HomeMaticDevices::save()
{
	for(std::vector<HomeMaticDevice*>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		try
		{
			std::ostringstream command;
			command << "DELETE FROM devices WHERE address='" << std::dec << (*i)->address() << "'";
			GD::db.executeCommand(command.str());
			std::ostringstream command2;
			command2 << "INSERT INTO devices VALUES(" << (*i)->address() << "," << (*i)->deviceType() << ",'" << (*i)->serialize() << "'," << (*i)->lastDutyCycleEvent() << ")";
			GD::db.executeCommand(command2.str());
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
		}
	}
}

void HomeMaticDevices::add(HomeMaticDevice* device)
{
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
			command << "DELETE FROM devices WHERE address='" << std::dec << address << "'";
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
