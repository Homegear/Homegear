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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <iostream>
#include <string>
#include <map>

namespace BaseLib
{
class Obj;

class Settings
{
public:
	Settings();
	virtual ~Settings() {}
	void init(BaseLib::Obj* baseLib);
	void load(std::string filename);
	bool changed();

	std::string certPath() { return _certPath; }
	std::string keyPath() { return _keyPath;  }
	bool loadDHParamsFromFile() { return _loadDHParamsFromFile; }
	std::string dhParamPath() { return _dhParamPath;  }
	int32_t debugLevel() { return _debugLevel; }
	bool enableUPnP() { return _enableUPnP; }
	std::string uPnPIpAddress() { return _uPnPIpAddress; }
	std::string ssdpIpAddress() { return _ssdpIpAddress; }
	int32_t ssdpPort() { return _ssdpPort; }
	bool devLog() { return _devLog; }
	std::string databasePath() { return _databasePath; }
	bool databaseSynchronous() { return _databaseSynchronous; }
	bool databaseMemoryJournal() { return _databaseMemoryJournal; }
	bool databaseWALJournal() { return _databaseWALJournal; }
	uint32_t databaseMaxBackups() { return _databaseMaxBackups; }
	std::string logfilePath() { return _logfilePath; }
	bool prioritizeThreads() { return _prioritizeThreads; }
	void setPrioritizeThreads(bool value) { _prioritizeThreads = value; }
	uint32_t workerThreadWindow() { return _workerThreadWindow; }
	uint32_t cliServerMaxConnections() { return _cliServerMaxConnections; }
	uint32_t rpcServerMaxConnections() { return _rpcServerMaxConnections; }
	int32_t rpcServerThreadPriority() { return _rpcServerThreadPriority; }
	int32_t rpcServerThreadPolicy() { return _rpcServerThreadPolicy; }
	uint32_t rpcClientMaxServers() { return _rpcClientMaxServers; }
	int32_t rpcClientThreadPriority() { return _rpcClientThreadPriority; }
	int32_t rpcClientThreadPolicy() { return _rpcClientThreadPolicy; }
	int32_t workerThreadPriority() { return _workerThreadPriority; }
	int32_t workerThreadPolicy() { return _workerThreadPolicy; }
	int32_t packetQueueThreadPriority() { return _packetQueueThreadPriority; }
	int32_t packetQueueThreadPolicy() { return _packetQueueThreadPolicy; }
	int32_t packetReceivedThreadPriority() { return _packetReceivedThreadPriority; }
	int32_t packetReceivedThreadPolicy() { return _packetReceivedThreadPolicy; }
	uint32_t eventThreadMax() { return _eventThreadMax; }
	int32_t eventTriggerThreadPriority() { return _eventTriggerThreadPriority; }
	int32_t eventTriggerThreadPolicy() { return _eventTriggerThreadPolicy; }
	uint32_t scriptThreadMax() { return _scriptThreadMax; }
	std::string deviceDescriptionPath() { return _deviceDescriptionPath; }
	std::string clientSettingsPath() { return _clientSettingsPath; }
	std::string serverSettingsPath() { return _serverSettingsPath; }
	std::string mqttSettingsPath() { return _mqttSettingsPath; }
	std::string physicalInterfaceSettingsPath() { return _physicalInterfaceSettingsPath; }
	std::string modulePath() { return _modulePath; }
	std::string scriptPath() { return _scriptPath; }
	std::string firmwarePath() { return _firmwarePath; }
	std::string tempPath() { return _tempPath; }
	std::map<std::string, bool>& tunnelClients() { return _tunnelClients; }
	std::map<std::string, std::string>& clientAddressesToReplace() { return _clientAddressesToReplace; }
	std::string gpioPath() { return _gpioPath; }
private:
	BaseLib::Obj* _bl = nullptr;
	std::string _path;
	int32_t _lastModified = -1;
	int32_t _clientSettingsLastModified = -1;
	int32_t _serverSettingsLastModified = -1;
	int32_t _mqttSettingsLastModified = -1;

	std::string _certPath;
	std::string _keyPath;
	bool _loadDHParamsFromFile = true;
	std::string _dhParamPath;
	int32_t _debugLevel = 3;
	bool _enableUPnP = true;
	std::string _uPnPIpAddress = "";
	std::string _ssdpIpAddress = "";
	int32_t _ssdpPort = 1900;
	bool _devLog = false;
	std::string _databasePath;
	bool _databaseSynchronous = true;
	bool _databaseMemoryJournal = false;
	bool _databaseWALJournal = true;
	uint32_t _databaseMaxBackups = 10;
	std::string _logfilePath;
	bool _prioritizeThreads = true;
	uint32_t _workerThreadWindow = 3000;
	uint32_t _cliServerMaxConnections = 50;
	uint32_t _rpcServerMaxConnections = 50;
	int32_t _rpcServerThreadPriority = 0;
	int32_t _rpcServerThreadPolicy = SCHED_OTHER;
	uint32_t _rpcClientMaxServers = 50;
	int32_t _rpcClientThreadPriority = 0;
	int32_t _rpcClientThreadPolicy = SCHED_OTHER;
	int32_t _workerThreadPriority = 0;
	int32_t _workerThreadPolicy = SCHED_OTHER;
	int32_t _packetQueueThreadPriority = 45;
	int32_t _packetQueueThreadPolicy = SCHED_FIFO;
	int32_t _packetReceivedThreadPriority = 0;
	int32_t _packetReceivedThreadPolicy = SCHED_OTHER;
	uint32_t _eventThreadMax = 20;
	int32_t _eventTriggerThreadPriority = 0;
	int32_t _eventTriggerThreadPolicy = SCHED_OTHER;
	uint32_t _scriptThreadMax = 10;
	std::string _deviceDescriptionPath;
	std::string _clientSettingsPath;
	std::string _serverSettingsPath;
	std::string _mqttSettingsPath;
	std::string _physicalInterfaceSettingsPath;
	std::string _modulePath;
	std::string _scriptPath;
	std::string _firmwarePath;
	std::string _tempPath;
	std::map<std::string, bool> _tunnelClients;
	std::map<std::string, std::string> _clientAddressesToReplace;
	std::string _gpioPath;

	void reset();
};

}
#endif /* SETTINGS_H_ */
