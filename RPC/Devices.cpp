#include "Devices.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC
{

Devices::Devices()
{
}

void Devices::load()
{
	try
	{
		DIR* directory;
		struct dirent* entry;
		int32_t i = -1;
		std::string deviceDir(GD::configPath + "Device types");
		if((directory = opendir(deviceDir.c_str())) != 0)
		{
			while((entry = readdir(directory)) != 0)
			{
				if(entry->d_type == 8)
				{
					try
					{
						if(i == -1) i = 0;
						HelperFunctions::printDebug("Loading XML RPC device " + deviceDir + "/" + entry->d_name);
						std::shared_ptr<Device> device(new Device(deviceDir + "/" + entry->d_name));
						if(device && device->loaded()) _devices.push_back(device);
						i++;
					}
					catch(const std::exception& ex)
					{
						HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
					}
					catch(const Exception& ex)
					{
						HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
					}
					catch(...)
					{
						HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
					}
				}
			}
		}
		else throw(Exception("Could not open directory \"" + GD::configPath + "Device types\"."));
		if(i == -1) throw(Exception("No xml files found in \"" + GD::configPath + "Device types\"."));
		if(i == 0) throw(Exception("Could not open any xml files in \"" + GD::configPath + "Device types\"."));
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    	exit(3);
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Device> Devices::find(HMDeviceTypes deviceType, uint32_t firmwareVersion, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		int32_t countFromSysinfo = 0;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(deviceType, firmwareVersion))
				{
					countFromSysinfo = (*i)->getCountFromSysinfo(packet);
					if((*i)->getCountFromSysinfo() != countFromSysinfo) partialMatch = *i;
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

std::shared_ptr<Device> Devices::find(HMDeviceTypes deviceType, uint32_t firmwareVersion, int32_t countFromSysinfo)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(deviceType, firmwareVersion))
				{
					if((*i)->countFromSysinfoIndex > -1 && (*i)->getCountFromSysinfo() != countFromSysinfo) partialMatch = *i;
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
     HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
     HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
     HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

std::shared_ptr<Device> Devices::find(std::string typeID, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		int32_t countFromSysinfo = 0;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(typeID))
				{
					countFromSysinfo = (*i)->getCountFromSysinfo(packet);
					if((*i)->getCountFromSysinfo() != countFromSysinfo) partialMatch = *i;
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

std::shared_ptr<Device> Devices::find(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		int32_t countFromSysinfo = 0;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(packet))
				{
					countFromSysinfo = (*i)->getCountFromSysinfo(packet);
					if((*i)->getCountFromSysinfo() != countFromSysinfo) partialMatch = *i;
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<Device> newDevice(new Device());
			*newDevice = *partialMatch;
			newDevice->setCountFromSysinfo(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

} /* namespace XMLRPC */
