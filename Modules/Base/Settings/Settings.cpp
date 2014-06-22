/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "Settings.h"
#include "../BaseLib.h"

namespace BaseLib
{

Settings::Settings()
{
}

void Settings::init(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void Settings::reset()
{
	_certPath = "/etc/homegear/homegear.crt";
	_keyPath = "/etc/homegear/homegear.key";
	_dhParamPath = "/etc/homegear/dh2048.pem";
	_debugLevel = 3;
	_databasePath = _bl->executablePath + "db.sql";
	_databaseSynchronous = false;
	_databaseMemoryJournal = true;
	_logfilePath = "/var/log/homegear/";
	_prioritizeThreads = true;
	_workerThreadWindow = 3000;
	_rpcServerThreadPriority = 0;
	_deviceDescriptionPath = "/etc/homegear/devices/";
	_clientSettingsPath = "/etc/homegear/rpcclients.conf";
	_serverSettingsPath = "/etc/homegear/rpcservers.conf";
	_physicalInterfaceSettingsPath = "/etc/homegear/physicalinterfaces.conf";
	_modulePath = "/var/lib/homegear/modules/";
	_scriptPath = "/var/lib/homegear/scripts/";
	_firmwarePath = "/var/lib/homegear/firmware/";
	_tunnelClients.clear();
	_gpioPath = "/sys/class/gpio/";
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
			_bl->out.printError("Unable to open config file: " + filename + ". " + strerror(errno));
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
				if(name == "certpath")
				{
					_certPath = value;
					if(_certPath.empty()) _certPath = "/etc/homegear/homegear.crt";
					_bl->out.printDebug("Debug: certPath set to " + _certPath);
				}
				else if(name == "keypath")
				{
					_keyPath = value;
					if(_keyPath.empty()) _keyPath = "/etc/homegear/homegear.key";
					_bl->out.printDebug("Debug: keyPath set to " + _keyPath);
				}
				else if(name == "loaddhparamsfromfile")
				{
					if(HelperFunctions::toLower(value) == "true") _loadDHParamsFromFile = true;
					else if(HelperFunctions::toLower(value) == "false") _loadDHParamsFromFile = false;
					_bl->out.printDebug("Debug: loadDHParamsFromFile set to " + std::to_string(_loadDHParamsFromFile));
				}
				else if(name == "dhparampath")
				{
					_dhParamPath = value;
					if(_dhParamPath.empty()) _dhParamPath = "/etc/homegear/dh2048.pem";
					_bl->out.printDebug("Debug: dhParamPath set to " + _dhParamPath);
				}
				else if(name == "debuglevel")
				{
					_debugLevel = HelperFunctions::getNumber(value);
					if(_debugLevel < 0) _debugLevel = 3;
					_bl->debugLevel = _debugLevel;
					_bl->out.printDebug("Debug: debugLevel set to " + std::to_string(_debugLevel));
				}
				else if(name == "databasepath")
				{
					_databasePath = value;
					if(_databasePath.empty()) _databasePath = _bl->executablePath + "db.sql";
					_bl->out.printDebug("Debug: databasePath set to " + _databasePath);
				}
				else if(name == "databasesynchronous")
				{
					if(HelperFunctions::toLower(value) == "true") _databaseSynchronous = true;
					_bl->out.printDebug("Debug: databaseSynchronous set to " + std::to_string(_databaseSynchronous));
				}
				else if(name == "databasememoryjournal")
				{
					if(HelperFunctions::toLower(value) == "false") _databaseMemoryJournal = false;
					_bl->out.printDebug("Debug: databaseMemoryJournal set to " + std::to_string(_databaseMemoryJournal));
				}
				else if(name == "logfilepath")
				{
					_logfilePath = value;
					if(_logfilePath.empty()) _logfilePath = "/var/log/homegear/";
					if(_logfilePath.back() != '/') _logfilePath.push_back('/');
					_bl->out.printDebug("Debug: logfilePath set to " + _logfilePath);
				}
				else if(name == "prioritizethreads")
				{
					if(HelperFunctions::toLower(value) == "false") _prioritizeThreads = false;
					_bl->out.printDebug("Debug: prioritizeThreads set to " + std::to_string(_prioritizeThreads));
				}
				else if(name == "workerthreadwindow")
				{
					_workerThreadWindow = HelperFunctions::getNumber(value);
					if(_workerThreadWindow > 3600000) _workerThreadWindow = 3600000;
					_bl->out.printDebug("Debug: workerThreadWindow set to " + std::to_string(_workerThreadWindow));
				}
				else if(name == "rpcserverthreadpriority")
				{
					_rpcServerThreadPriority = HelperFunctions::getNumber(value);
					if(_rpcServerThreadPriority > 99) _rpcServerThreadPriority = 99;
					if(_rpcServerThreadPriority < 0) _rpcServerThreadPriority = 0;
					_bl->out.printDebug("Debug: rpcServerThreadPriority set to " + std::to_string(_rpcServerThreadPriority));
				}
				else if(name == "devicedescriptionpath")
				{
					_deviceDescriptionPath = value;
					if(_deviceDescriptionPath.empty()) _deviceDescriptionPath = "/etc/homegear/devices";
					if(_deviceDescriptionPath.back() != '/') _deviceDescriptionPath.push_back('/');
					_bl->out.printDebug("Debug: deviceDescriptionPath set to " + _deviceDescriptionPath);
				}
				else if(name == "serversettingspath")
				{
					_serverSettingsPath = value;
					if(_serverSettingsPath.empty()) _serverSettingsPath = "/etc/homegear/rpcservers.conf";
					_bl->out.printDebug("Debug: serverSettingsPath set to " + _serverSettingsPath);
				}
				else if(name == "clientsettingspath")
				{
					_clientSettingsPath = value;
					if(_clientSettingsPath.empty()) _clientSettingsPath = "/etc/homegear/rpcclients.conf";
					_bl->out.printDebug("Debug: clientSettingsPath set to " + _clientSettingsPath);
				}
				else if(name == "physicalinterfacesettingspath")
				{
					_physicalInterfaceSettingsPath = value;
					if(_physicalInterfaceSettingsPath.empty()) _physicalInterfaceSettingsPath = "/etc/homegear/physicalinterfaces.conf";
					_bl->out.printDebug("Debug: physicalDeviceSettingsPath set to " + _physicalInterfaceSettingsPath);
				}
				else if(name == "modulepath")
				{
					_modulePath = value;
					if(_modulePath.empty()) _modulePath = "/var/lib/homegear/modules/";
					if(_modulePath.back() != '/') _modulePath.push_back('/');
					_bl->out.printDebug("Debug: libraryPath set to " + _modulePath);
				}
				else if(name == "scriptpath")
				{
					_scriptPath = value;
					if(_scriptPath.empty()) _scriptPath = "/var/lib/homegear/scripts/";
					if(_scriptPath.back() != '/') _scriptPath.push_back('/');
					_bl->out.printDebug("Debug: scriptPath set to " + _scriptPath);
				}
				else if(name == "firmwarepath")
				{
					_firmwarePath = value;
					if(_firmwarePath.empty()) _firmwarePath = "/var/lib/homegear/firmware/";
					if(_firmwarePath.back() != '/') _firmwarePath.push_back('/');
					_bl->out.printDebug("Debug: firmwarePath set to " + _firmwarePath);
				}
				else if(name == "redirecttosshtunnel")
				{
					if(!value.empty()) _tunnelClients[HelperFunctions::toLower(value)] = true;
				}
				else if(name == "gpiopath")
				{
					_gpioPath = value;
					if(_gpioPath.empty()) _gpioPath = "/sys/class/gpio/";
					if(_gpioPath.back() != '/') _gpioPath.push_back('/');
					_bl->out.printDebug("Debug: gpioPath set to " + _gpioPath);
				}
				else
				{
					_bl->out.printWarning("Warning: Setting not found: " + std::string(input));
				}
			}
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
