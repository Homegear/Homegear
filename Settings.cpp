#include "Settings.h"
#include "HelperFunctions.h"
#include "GD.h"

Settings::Settings()
{

}

void Settings::reset()
{
	_rpcInterface = "0.0.0.0";
	_rpcPort = 2001;
	_rpcSSLPort = 2002;
	_certPath = "/etc/homegear/homegear.crt";
	_keyPath = "/etc/homegear/homegear.key";
	_verifyCertificate = true;
	_debugLevel = 3;
	_databasePath = GD::executablePath + "db.sql";
	_databaseSynchronous = false;
	_databaseMemoryJournal = true;
	_mySQLServer = "localhost";
	_mySQLUser = "homegear";
	_mySQLPassword = "homegear";
	_rfDeviceType = "cul";
	_rfDevice = "/dev/ttyACM0";
	_logfilePath = "/var/log/homegear/";
	_prioritizeThreads = true;
	_workerThreadWindow = 3000;
	_bidCoSResponseDelay = 90;
	_rpcServerThreadPriority = 0;
	_tunnelClients.clear();
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
			HelperFunctions::printError("Unable to open config file: " + filename);
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
					HelperFunctions::printDebug("Debug: rpcPort set to " + std::to_string(_rpcPort));
				}
				else if(name == "rpcsslport")
				{
					_rpcSSLPort = HelperFunctions::getNumber(value);
					if(_rpcSSLPort < 1) _rpcSSLPort = 2002;
					HelperFunctions::printDebug("Debug: rpcSSLPort set to " + std::to_string(_rpcSSLPort));
				}
				else if(name == "certpath")
				{
					_certPath = value;
					if(_certPath.empty()) _certPath = "/etc/homegear/homegear.crt";
					HelperFunctions::printDebug("Debug: certPath set to " + _certPath);
				}
				else if(name == "keypath")
				{
					_keyPath = value;
					if(_keyPath.empty()) _keyPath = "/etc/homegear/homegear.key";
					HelperFunctions::printDebug("Debug: keyPath set to " + _keyPath);
				}
				else if(name == "verifycertificate")
				{
					if(HelperFunctions::toLower(value) == "true") _verifyCertificate = true;
					HelperFunctions::printDebug("Debug: verifyCertificate set to " + std::to_string(_verifyCertificate));
				}
				else if(name == "rpcinterface")
				{
					_rpcInterface = value;
					if(_rpcInterface.empty()) _rpcInterface = "0.0.0.0";
					HelperFunctions::printDebug("Debug: rpcInterface set to " + _rpcInterface);
				}
				else if(name == "debuglevel")
				{
					_debugLevel = HelperFunctions::getNumber(value);
					if(_debugLevel < 0) _debugLevel = 3;
					GD::debugLevel = _debugLevel;
					HelperFunctions::printDebug("Debug: debugLevel set to " + std::to_string(_debugLevel));
				}
				else if(name == "databasepath")
				{
					_databasePath = value;
					if(_databasePath.empty()) _databasePath = GD::executablePath + "db.sql";
					HelperFunctions::printDebug("Debug: databasePath set to " + _databasePath);
				}
				else if(name == "databasesynchronous")
				{
					if(HelperFunctions::toLower(value) == "true") _databaseSynchronous = true;
					HelperFunctions::printDebug("Debug: databaseSynchronous set to " + std::to_string(_databaseSynchronous));
				}
				else if(name == "databasememoryjournal")
				{
					if(HelperFunctions::toLower(value) == "false") _databaseMemoryJournal = false;
					HelperFunctions::printDebug("Debug: databaseMemoryJournal set to " + std::to_string(_databaseMemoryJournal));
				}
				else if(name == "mysqlserver")
				{
					_mySQLServer = value;
					if(_mySQLServer.empty()) _mySQLServer = "localhost";
					HelperFunctions::printDebug("Debug: mySQLServer set to " + _mySQLServer);
				}
				else if(name == "mysqldatabase")
				{
					_mySQLDatabase = value;
					if(_mySQLDatabase.empty()) _mySQLDatabase = "homegear";
					HelperFunctions::printDebug("Debug: mySQLDatabase set to " + _mySQLDatabase);
				}
				else if(name == "mysqluser")
				{
					_mySQLUser = value;
					if(_mySQLUser.empty()) _mySQLUser = "homegear";
					HelperFunctions::printDebug("Debug: mySQLUser set to " + _mySQLUser);
				}
				else if(name == "mysqlpassword")
				{
					_mySQLPassword = value;
					if(_mySQLPassword.empty()) _mySQLPassword = "homegear";
					else HelperFunctions::printDebug("Debug: mySQLPassword was set.");
				}
				else if(name == "rfdevicetype")
				{
					HelperFunctions::toLower(value);
					_rfDeviceType = value;
					if(_rfDeviceType.empty()) _rfDeviceType = "cul";
					HelperFunctions::printDebug("Debug: rfDeviceType set to " + _rfDeviceType);
				}
				else if(name == "rfdevice")
				{
					_rfDevice = value;
					if(_rfDevice.empty()) _rfDevice = "/dev/ttyACM0";
					HelperFunctions::printDebug("Debug: rfDevice set to " + _rfDevice);
				}
				else if(name == "logfilepath")
				{
					_logfilePath = value;
					if(_logfilePath.empty()) _logfilePath = "/var/log/homegear/";
					if(_logfilePath[_logfilePath.size() - 1] != '/') _logfilePath.push_back('/');
					HelperFunctions::printDebug("Debug: logfilePath set to " + _logfilePath);
				}
				else if(name == "prioritizethreads")
				{
					if(HelperFunctions::toLower(value) == "false") _prioritizeThreads = false;
					HelperFunctions::printDebug("Debug: prioritizeThreads set to " + std::to_string(_prioritizeThreads));
				}
				else if(name == "workerthreadwindow")
				{
					_workerThreadWindow = HelperFunctions::getNumber(value);
					if(_workerThreadWindow > 3600000) _workerThreadWindow = 3600000;
					HelperFunctions::printDebug("Debug: workerThreadWindow set to " + std::to_string(_workerThreadWindow));
				}
				else if(name == "bidcosresponsedelay")
				{
					_bidCoSResponseDelay = HelperFunctions::getNumber(value);
					if(_bidCoSResponseDelay > 10000) _bidCoSResponseDelay = 10000;
					HelperFunctions::printDebug("Debug: bidCoSResponseDelay set to " + std::to_string(_bidCoSResponseDelay));
				}
				else if(name == "rpcserverthreadpriority")
				{
					_rpcServerThreadPriority = HelperFunctions::getNumber(value);
					if(_rpcServerThreadPriority > 99) _rpcServerThreadPriority = 99;
					if(_rpcServerThreadPriority < 0) _rpcServerThreadPriority = 0;
					HelperFunctions::printDebug("Debug: rpcServerThreadPriority set to " + std::to_string(_rpcServerThreadPriority));
				}
				else if(name == "redirecttosshtunnel")
				{
					if(!value.empty()) _tunnelClients[HelperFunctions::toLower(value)] = true;
				}
				else
				{
					HelperFunctions::printWarning("Warning: Setting not found: " + std::string(input));
				}
			}
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
