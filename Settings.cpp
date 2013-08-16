#include "Settings.h"
#include "HelperFunctions.h"
#include "GD.h"

Settings::Settings()
{

}

void Settings::reset()
{
	_rpcInterface = "0.0.0.0";
	_rfDeviceType = "cul";
	_rfDevice = "/dev/ttyACM0";
	_databasePath = GD::executablePath + "db.sql";
	_logfilePath = "/var/log/homegear/";
}

void Settings::load(std::string filename)
{
	try
	{
		reset();
		char input[1024];
		FILE *fin;
		int32_t len, ptr;
		bool found = false;

		if (!(fin = fopen(filename.c_str(), "r")))
		{
			std::cerr << "Unable to open config file: " + filename << std::endl;
			return;
		}

		while (fgets(input, 1024, fin))
		{
			if(input[0] == '#') continue;
			len = strlen(input);
			if (len < 2) continue;
			if (input[len-1] == '\n') input[len-1] = '\0';
			ptr = 0;
			found = false;
			while(ptr < len)
			{
				if (input[ptr] == '=')
				{
					found = true;
					input[ptr++] = '\0';
					break;
				}
				ptr++;
			}
			if(found)
			{
				std::string name(input);
				HelperFunctions::toLower(name);
				HelperFunctions::trim(name);
				std::string value(&input[ptr]);
				HelperFunctions::trim(value);
				if(name == "rpcport")
				{
					_rpcPort = HelperFunctions::getNumber(value);
					if(_rpcPort < 1) _rpcPort = 2001;
					if(GD::debugLevel >= 5) std::cout << "Debug: rpcPort set to " << std::to_string(_rpcPort) << std::endl;
				}
				else if(name == "rpcinterface")
				{
					_rpcInterface = value;
					if(_rpcInterface.empty()) _rpcInterface = "0.0.0.0";
					if(GD::debugLevel >= 5) std::cout << "Debug: rpcInterface set to " << _rpcInterface << std::endl;
				}
				else if(name == "debuglevel")
				{
					_debugLevel = HelperFunctions::getNumber(value);
					if(_debugLevel < 0) _debugLevel = 3;
					GD::debugLevel = _debugLevel;
					if(GD::debugLevel >= 5) std::cout << "Debug: debugLevel set to " << std::to_string(_debugLevel) << std::endl;
				}
				else if(name == "databasepath")
				{
					_databasePath = value;
					if(_databasePath.empty()) _databasePath = GD::executablePath + "db.sql";
					if(GD::debugLevel >= 5) std::cout << "Debug: databasePath set to " << _databasePath << std::endl;
				}
				else if(name == "rfdevicetype")
				{
					HelperFunctions::toLower(value);
					_rfDeviceType = value;
					if(_rfDeviceType.empty()) _rfDeviceType = "cul";
					if(GD::debugLevel >= 5) std::cout << "Debug: rfDeviceType set to " << _rfDeviceType << std::endl;
				}
				else if(name == "rfdevice")
				{
					_rfDevice = value;
					if(_rfDevice.empty()) _rfDevice = "/dev/ttyACM0";
					if(GD::debugLevel >= 5) std::cout << "Debug: rfDevice set to " << _rfDevice << std::endl;
				}
				else if(name == "logfilepath")
				{
					_logfilePath = value;
					if(_logfilePath.empty()) _logfilePath = "/var/log/homegear/";
					if(_logfilePath[_logfilePath.size() - 1] != '/') _logfilePath.push_back('/');
					if(GD::debugLevel >= 5) std::cout << "Debug: logfilePath set to " << _logfilePath << std::endl;
				}
				else if(GD::debugLevel >= 3)
				{
					std::cout << "Warning: Setting not found: " << std::string(input) << std::endl;
				}
			}
		}

		fclose(fin);
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
