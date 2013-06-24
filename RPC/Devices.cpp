#include "Devices.h"
#include "../GD.h"
#include <dirent.h>

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
		std::string deviceDir(GD::executablePath + "/Device types");
		if((directory = opendir(deviceDir.c_str())) != 0)
		{
			while((entry = readdir(directory)) != 0)
			{
				if(entry->d_type == 8)
				{
					try
					{
						if(i == -1) i = 0;
						if(GD::debugLevel >= 5) std::cout << "Loading XML RPC device " << deviceDir << "/" << entry->d_name << std::endl;
						std::shared_ptr<Device> device(new Device(deviceDir + "/" + entry->d_name));
						if(device && device->loaded()) _devices.push_back(device);
						i++;
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
			}
		}
		else throw(Exception("Could not open directory \"Device types\"."));
		if(i == -1) throw(Exception("No xml files found in \"Device types\"."));
		if(i == 0) throw(Exception("Could not open any xml files in \"Device types\"."));
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    	exit(3);
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
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
    return nullptr;
}

std::shared_ptr<Device> Devices::find(std::string typeID, int32_t countFromSysinfo)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(typeID))
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
    return nullptr;
}

std::shared_ptr<Device> Devices::find(std::shared_ptr<BidCoSPacket> packet, int32_t countFromSysinfo)
{
	try
	{
		std::shared_ptr<Device> partialMatch;
		for(std::vector<std::shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<std::shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(packet))
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
    return nullptr;
}

} /* namespace XMLRPC */
