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

} /* namespace XMLRPC */
