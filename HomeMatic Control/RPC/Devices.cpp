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
		std::string deviceDir(GD::executablePath + "/Device types");
		if((directory = opendir(deviceDir.c_str())) != 0)
		{
			while((entry = readdir(directory)) != 0)
			{
				if(entry->d_type == 8)
				{
					if(GD::debugLevel >= 5) cout << "Loading XML RPC device " << entry->d_name << endl;
					_devices.push_back(shared_ptr<Device>(new Device(deviceDir + "/" + entry->d_name)));
				}
			}
		}
		else
		{
			throw(new Exception("Could not open directory \"Device types\"."));
		}
	}
    catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
}

shared_ptr<Device> Devices::find(HMDeviceTypes deviceType, uint32_t firmwareVersion)
{
	try
	{
		for(std::vector<shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(deviceType, firmwareVersion)) return *i;
			}
		}
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Unknown exception." << std::endl;
    }
    return nullptr;
}

shared_ptr<Device> Devices::find(shared_ptr<BidCoSPacket> packet)
{
	try
	{
		for(std::vector<shared_ptr<Device>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<shared_ptr<DeviceType>>::iterator j = (*i)->supportedTypes.begin(); j != (*i)->supportedTypes.end(); ++j)
			{
				if((*j)->matches(packet)) return *i;
			}
		}
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << '\n';
    }
    return nullptr;
}

} /* namespace XMLRPC */
