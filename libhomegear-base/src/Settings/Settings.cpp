/* Copyright 2013-2015 Sathya Laufer
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
	_enableUPnP = true;
	_uPnPIpAddress = "";
	_ssdpIpAddress = "";
	_ssdpPort = 1900;
	_devLog = false;
	_databasePath = _bl->executablePath + "db.sql";
	_databaseSynchronous = true;
	_databaseMemoryJournal = false;
	_databaseWALJournal = true;
	_databaseMaxBackups = 10;
	_logfilePath = "/var/log/homegear/";
	_prioritizeThreads = true;
	_workerThreadWindow = 3000;
	_cliServerMaxConnections = 50;
	_rpcServerMaxConnections = 50;
	_rpcServerThreadPriority = 0;
	_rpcServerThreadPolicy = SCHED_OTHER;
	_rpcClientMaxServers = 50;
	_rpcClientThreadPriority = 0;
	_rpcClientThreadPolicy = SCHED_OTHER;
	_workerThreadPriority = 0;
	_workerThreadPolicy = SCHED_OTHER;
	_packetQueueThreadPriority = 45;
	_packetQueueThreadPolicy = SCHED_FIFO;
	_packetReceivedThreadPriority = 0;
	_packetReceivedThreadPolicy = SCHED_OTHER;
	_eventThreadMax = 20;
	_eventTriggerThreadPriority = 0;
	_eventTriggerThreadPolicy = SCHED_OTHER;
	_scriptThreadMax = 10;
	_deviceDescriptionPath = "/etc/homegear/devices/";
	_clientSettingsPath = "/etc/homegear/rpcclients.conf";
	_serverSettingsPath = "/etc/homegear/rpcservers.conf";
	_mqttSettingsPath = "/etc/homegear/mqtt.conf";
	_physicalInterfaceSettingsPath = "/etc/homegear/physicalinterfaces.conf";
	_modulePath = "/usr/share/homegear/modules/";
	_scriptPath = "/var/lib/homegear/scripts/";
	_firmwarePath = "/usr/share/homegear/firmware/";
	_tempPath = "/var/lib/homegear/tmp/";
	_tunnelClients.clear();
	_clientAddressesToReplace.clear();
	_gpioPath = "/sys/class/gpio/";
}

bool Settings::changed()
{
	if(_bl->io.getFileLastModifiedTime(_path) != _lastModified ||
		_bl->io.getFileLastModifiedTime(_clientSettingsPath) != _clientSettingsLastModified ||
		_bl->io.getFileLastModifiedTime(_serverSettingsPath) != _serverSettingsLastModified)
	{
		return true;
	}
	return false;
}

void Settings::load(std::string filename)
{
	try
	{
		reset();
		_path = filename;
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
					_debugLevel = Math::getNumber(value);
					if(_debugLevel < 0) _debugLevel = 3;
					_bl->debugLevel = _debugLevel;
					_bl->out.printDebug("Debug: debugLevel set to " + std::to_string(_debugLevel));
				}
				else if(name == "enableupnp")
				{
					if(HelperFunctions::toLower(value) == "false") _enableUPnP = false;
					_bl->out.printDebug("Debug: enableUPnP set to " + std::to_string(_enableUPnP));
				}
				else if(name == "upnpipaddress")
				{
					_uPnPIpAddress = value;
					_bl->out.printDebug("Debug: uPnPIpAddress set to " + _uPnPIpAddress);
				}
				else if(name == "ssdpipaddress")
				{
					_ssdpIpAddress = value;
					_bl->out.printDebug("Debug: ssdpIpAddress set to " + _ssdpIpAddress);
				}
				else if(name == "ssdpport")
				{
					_ssdpPort = Math::getNumber(value);
					if(_ssdpPort < 1 || _ssdpPort > 65535) _ssdpPort = 1900;
					_bl->out.printDebug("Debug: ssdpPort set to " + std::to_string(_ssdpPort));
				}
				else if(name == "devlog")
				{
					if(HelperFunctions::toLower(value) == "true") _devLog = true;
					_bl->out.printDebug("Debug: devLog set to " + std::to_string(_devLog));
				}
				else if(name == "databasepath")
				{
					_databasePath = value;
					if(_databasePath.empty()) _databasePath = _bl->executablePath + "db.sql";
					_bl->out.printDebug("Debug: databasePath set to " + _databasePath);
				}
				else if(name == "databasesynchronous")
				{
					if(HelperFunctions::toLower(value) == "false") _databaseSynchronous = false;
					_bl->out.printDebug("Debug: databaseSynchronous set to " + std::to_string(_databaseSynchronous));
				}
				else if(name == "databasememoryjournal")
				{
					if(HelperFunctions::toLower(value) == "true") _databaseMemoryJournal = true;
					_bl->out.printDebug("Debug: databaseMemoryJournal set to " + std::to_string(_databaseMemoryJournal));
				}
				else if(name == "databasewaljournal")
				{
					if(HelperFunctions::toLower(value) == "false") _databaseWALJournal = false;
					_bl->out.printDebug("Debug: databaseWALJournal set to " + std::to_string(_databaseWALJournal));
				}
				else if(name == "databasemaxbackups")
				{
					_databaseMaxBackups = Math::getNumber(value);
					if(_databaseMaxBackups > 10000) _databaseMaxBackups = 10000;
					_bl->out.printDebug("Debug: databaseMaxBackups set to " + std::to_string(_databaseMaxBackups));
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
					_workerThreadWindow = Math::getNumber(value);
					if(_workerThreadWindow > 3600000) _workerThreadWindow = 3600000;
					_bl->out.printDebug("Debug: workerThreadWindow set to " + std::to_string(_workerThreadWindow));
				}
				else if(name == "cliservermaxconnections")
				{
					_cliServerMaxConnections = Math::getNumber(value);
					_bl->out.printDebug("Debug: cliServerMaxConnections set to " + std::to_string(_cliServerMaxConnections));
				}
				else if(name == "rpcservermaxconnections")
				{
					_rpcServerMaxConnections = Math::getNumber(value);
					_bl->out.printDebug("Debug: rpcServerMaxConnections set to " + std::to_string(_rpcServerMaxConnections));
				}
				else if(name == "rpcserverthreadpriority")
				{
					_rpcServerThreadPriority = Math::getNumber(value);
					if(_rpcServerThreadPriority > 99) _rpcServerThreadPriority = 99;
					if(_rpcServerThreadPriority < 0) _rpcServerThreadPriority = 0;
					_bl->out.printDebug("Debug: rpcServerThreadPriority set to " + std::to_string(_rpcServerThreadPriority));
				}
				else if(name == "rpcserverthreadpolicy")
				{
					_rpcServerThreadPolicy = Threads::getThreadPolicyFromString(value);
					_rpcServerThreadPriority = Threads::parseThreadPriority(_rpcServerThreadPriority, _rpcServerThreadPolicy);
					_bl->out.printDebug("Debug: rpcServerThreadPolicy set to " + std::to_string(_rpcServerThreadPolicy));
				}
				else if(name == "rpcclientmaxservers")
				{
					_rpcClientMaxServers = Math::getNumber(value);
					_bl->out.printDebug("Debug: rpcClientMaxThreads set to " + std::to_string(_rpcClientMaxServers));
				}
				else if(name == "rpcclientthreadpriority")
				{
					_rpcClientThreadPriority = Math::getNumber(value);
					if(_rpcClientThreadPriority > 99) _rpcClientThreadPriority = 99;
					if(_rpcClientThreadPriority < 0) _rpcClientThreadPriority = 0;
					_bl->out.printDebug("Debug: rpcClientThreadPriority set to " + std::to_string(_rpcClientThreadPriority));
				}
				else if(name == "rpcclientthreadpolicy")
				{
					_rpcClientThreadPolicy = Threads::getThreadPolicyFromString(value);
					_rpcClientThreadPriority = Threads::parseThreadPriority(_rpcClientThreadPriority, _rpcClientThreadPolicy);
					_bl->out.printDebug("Debug: rpcClientThreadPolicy set to " + std::to_string(_rpcClientThreadPolicy));
				}
				else if(name == "workerthreadpriority")
				{
					_workerThreadPriority = Math::getNumber(value);
					if(_workerThreadPriority > 99) _workerThreadPriority = 99;
					if(_workerThreadPriority < 0) _workerThreadPriority = 0;
					_bl->out.printDebug("Debug: workerThreadPriority set to " + std::to_string(_workerThreadPriority));
				}
				else if(name == "workerthreadpolicy")
				{
					_workerThreadPolicy = Threads::getThreadPolicyFromString(value);
					_workerThreadPriority = Threads::parseThreadPriority(_workerThreadPriority, _workerThreadPolicy);
					_bl->out.printDebug("Debug: workerThreadPolicy set to " + std::to_string(_workerThreadPolicy));
				}
				else if(name == "packetqueuethreadpriority")
				{
					_packetQueueThreadPriority = Math::getNumber(value);
					if(_packetQueueThreadPriority > 99) _packetQueueThreadPriority = 99;
					if(_packetQueueThreadPriority < 0) _packetQueueThreadPriority = 0;
					_bl->out.printDebug("Debug: physicalInterfaceThreadPriority set to " + std::to_string(_packetQueueThreadPriority));
				}
				else if(name == "packetqueuethreadpolicy")
				{
					_packetQueueThreadPolicy = Threads::getThreadPolicyFromString(value);
					_packetQueueThreadPriority = Threads::parseThreadPriority(_packetQueueThreadPriority, _packetQueueThreadPolicy);
					_bl->out.printDebug("Debug: physicalInterfaceThreadPolicy set to " + std::to_string(_packetQueueThreadPolicy));
				}
				else if(name == "packetreceivedthreadpriority")
				{
					_packetReceivedThreadPriority = Math::getNumber(value);
					if(_packetReceivedThreadPriority > 99) _packetReceivedThreadPriority = 99;
					if(_packetReceivedThreadPriority < 0) _packetReceivedThreadPriority = 0;
					_bl->out.printDebug("Debug: packetReceivedThreadPriority set to " + std::to_string(_packetReceivedThreadPriority));
				}
				else if(name == "packetreceivedthreadpolicy")
				{
					_packetReceivedThreadPolicy = Threads::getThreadPolicyFromString(value);
					_packetReceivedThreadPriority = Threads::parseThreadPriority(_packetReceivedThreadPriority, _packetReceivedThreadPolicy);
					_bl->out.printDebug("Debug: packetReceivedThreadPolicy set to " + std::to_string(_packetReceivedThreadPolicy));
				}
				else if(name == "eventmaxthreads")
				{
					_eventThreadMax = Math::getNumber(value);
					_bl->out.printDebug("Debug: eventMaxThreads set to " + std::to_string(_eventThreadMax));
				}
				else if(name == "eventtriggerthreadpriority")
				{
					_eventTriggerThreadPriority = Math::getNumber(value);
					if(_eventTriggerThreadPriority > 99) _eventTriggerThreadPriority = 99;
					if(_eventTriggerThreadPriority < 0) _eventTriggerThreadPriority = 0;
					_bl->out.printDebug("Debug: eventTriggerThreadPriority set to " + std::to_string(_eventTriggerThreadPriority));
				}
				else if(name == "eventtriggerthreadpolicy")
				{
					_eventTriggerThreadPolicy = Threads::getThreadPolicyFromString(value);
					_eventTriggerThreadPriority = Threads::parseThreadPriority(_eventTriggerThreadPriority, _eventTriggerThreadPolicy);
					_bl->out.printDebug("Debug: eventTriggerThreadPolicy set to " + std::to_string(_eventTriggerThreadPolicy));
				}
				else if(name == "scriptmaxthreads")
				{
					_scriptThreadMax = Math::getNumber(value);
					_bl->out.printDebug("Debug: scriptMaxThreads set to " + std::to_string(_scriptThreadMax));
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
				else if(name == "mqttsettingspath")
				{
					_mqttSettingsPath = value;
					if(_mqttSettingsPath.empty()) _mqttSettingsPath = "/etc/homegear/mqtt.conf";
					_bl->out.printDebug("Debug: mqttSettingsPath set to " + _mqttSettingsPath);
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
					if(_modulePath.empty()) _modulePath = "/usr/share/homegear/modules/";
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
					if(_firmwarePath.empty()) _firmwarePath = "/usr/share/homegear/firmware/";
					if(_firmwarePath.back() != '/') _firmwarePath.push_back('/');
					_bl->out.printDebug("Debug: firmwarePath set to " + _firmwarePath);
				}
				else if(name == "temppath")
				{
					_tempPath = value;
					if(_tempPath.empty()) _tempPath = "/var/lib/homegear/tmp/";
					if(_tempPath.back() != '/') _tempPath.push_back('/');
					_bl->out.printDebug("Debug: tempPath set to " + _tempPath);
				}
				else if(name == "redirecttosshtunnel")
				{
					if(!value.empty()) _tunnelClients[HelperFunctions::toLower(value)] = true;
				}
				else if(name == "replaceclientserveraddress")
				{
					if(!value.empty())
					{
						std::pair<std::string, std::string> addresses = _bl->hf.split(HelperFunctions::toLower(value), ' ');
						if(!value.empty()) _clientAddressesToReplace[addresses.first] = addresses.second;
						_bl->out.printDebug("Debug: Added replaceClientServerAddress " + addresses.first + " " + addresses.second);
					}
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
		_lastModified = _bl->io.getFileLastModifiedTime(filename);
		_clientSettingsLastModified = _bl->io.getFileLastModifiedTime(_clientSettingsPath);
		_serverSettingsLastModified = _bl->io.getFileLastModifiedTime(_serverSettingsPath);
		_mqttSettingsLastModified = _bl->io.getFileLastModifiedTime(_mqttSettingsPath);
	}
	catch(const std::exception& ex)
    {
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
