#include "Devices.h"
#include "../GD.h"
#include <dirent.h>

namespace XMLRPC
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
					if(GD::debugLevel == 5) cout << "Loading XML RPC device " << entry->d_name << endl;
					_devices.push_back(deviceDir + "/" + entry->d_name);
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

Device* Devices::find(HMDeviceTypes deviceType)
{
	try
	{
		for(std::vector<Device>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<DeviceType>::iterator j = i->supportedTypes.begin(); j != i->supportedTypes.end(); ++j)
			{
				if(j->matches(deviceType)) return &(*i);
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

Device* Devices::find(BidCoSPacket* packet)
{
	try
	{
		for(std::vector<Device>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(std::vector<DeviceType>::iterator j = i->supportedTypes.begin(); j != i->supportedTypes.end(); ++j)
			{
				if(j->matches(packet)) return &(*i);
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
